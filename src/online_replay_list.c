#include "online_replay_list.h"
#include "online_client.h"
#include "mouse.h"
#include "text3.h"
#include "popup.h"
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <dirent.h>

online_replay_list_ent *online_replay_list_ent_init(char *name_on_server, char *name, int score, int combo, float accuracy)
{
	online_replay_list_ent *ret = malloc(sizeof *ret);
	ret->name_on_server = dupe_str(name_on_server);
	ret->name = dupe_str(name);
	ret->score = score;
	ret->combo = combo;
	ret->accuracy = accuracy;
	ret->is_local = 0;
	return ret;
}

online_replay_list_ent *online_replay_list_ent_deserialize(char *data)
{
	online_replay_list_ent *ret = NULL;
	list *split = split_str(data, "$");
	if(split->count == 5)
	{
		ret = malloc(sizeof *ret);
		int i = 0;
		ret->name_on_server = dupe_str(list_get_val_at(split, i++));
		ret->name = dupe_str(list_get_val_at(split, i++));
		ret->score = atoi(list_get_val_at(split, i++));
		ret->combo = atoi(list_get_val_at(split, i++));
		ret->accuracy = atof(list_get_val_at(split, i++));
		ret->is_local = 0;
	}
	list_free(split, free);
	return ret;
}

char *online_replay_list_ent_serialize(online_replay_list_ent *ptr)
{
	return dupfmt("%s$%s$%d$%d$%.2f", ptr->name_on_server, ptr->name, ptr->score, ptr->combo, ptr->accuracy);
}

void online_replay_list_ent_free(online_replay_list_ent *ptr)
{
	free(ptr->name);
	free(ptr->name_on_server);
	free(ptr);
}

online_replay_list *online_replay_list_init()
{
	online_replay_list *ret = malloc(sizeof *ret);
	ret->ents = list_init();
	ret->scroll_y = 0;
	ret->is_local_cbox = checkbox_init("local scores", 0, 0, 0);
	return ret;
}

void online_replay_list_overwrite(online_replay_list *ptr, char *data)
{
	ptr->scroll_y = 0;
	online_replay_list_clear(ptr);
	if(data == NULL || data[0] == '\0') return;
	list *split = split_str(data, "|");
	list_node *n;
	for (n = split->start; n != NULL; n = n->next)
	{
		online_replay_list_ent *nent = online_replay_list_ent_deserialize(n->val);
		if(nent) online_replay_list_add(ptr, nent);
	}
	list_free(split, free);
}

void online_replay_list_sort(online_replay_list *ptr)
{
	list_node *sn;

	char swapped;

	do
	{
		swapped = 0;
		sn = ptr->ents->start;
		if (sn == NULL) break;
		list_node *ln = NULL;
		while (sn->next != ln)
		{
			online_replay_list_ent *o = sn->val;
			online_replay_list_ent *on = sn->next->val;
			if (o->score < on->score)
			{
				list_swap(sn, sn->next);
				swapped = 1;
			}
			sn = sn->next;
		}
		ln = sn;
	} while (swapped);
}

void online_replay_list_reload(online_replay_list *ptr)
{
	if(ptr->is_local_cbox->is_checked) online_replay_list_load_local(ptr);
	else if(rg.client->authed) online_client_request_replay_list(rg.client);
}

char *online_replay_list_serialize(online_replay_list *ptr)
{
	char *ret = init_empty_string();
	list_node *n;
	for (n = ptr->ents->start; n != NULL; n = n->next)
	{
		online_replay_list_ent *ent = n->val;
		append_to_str_free_append(&ret, online_replay_list_ent_serialize(ent));
		if (n != ptr->ents->end) append_to_str(&ret, "|");
	}
	return ret;
}

void online_replay_list_add(online_replay_list *ptr, online_replay_list_ent *ent)
{
	list_push_back(ptr->ents, ent);
}

void online_replay_list_clear(online_replay_list *ptr)
{
	list_clear(ptr->ents, online_replay_list_ent_free);
}

void online_replay_list_load_local(online_replay_list *ptr)
{
	online_replay_list_clear(ptr);
	char *scores_dir = song_get_scores_dir(rg.song);
	DIR *dr = opendir(scores_dir);

	if(dr != NULL)
	{
		struct dirent *de;
		while((de = readdir(dr)) != NULL)
		{
			if(strs_are_equal(de->d_name, ".") || strs_are_equal(de->d_name, "..")) continue;
			if(de->d_type == DT_REG)
			{
				if(strlen(de->d_name) > 5)
				{
					list *split = split_str(de->d_name, ".");
					if(split != NULL && split->count == 3)
					{
						char *last_str = (split->end->val);
						if(strlen(last_str) == 5 && str_contains(last_str, "score", 1))
						{
							char *loc = dupfmt("%s%s", scores_dir, de->d_name);

							list *sp = split_str(de->d_name, " ");
							list *sp2 = tokenize_str(de->d_name, " - ");

							if(sp != NULL && sp->count >= 15 && sp2 != NULL && sp2->count >= 7)
							{
								online_replay_list_ent *ent = 
									online_replay_list_ent_init(loc, 
										list_get_val_at(sp2, 0),
										atoi(list_get_val_at(sp, sp->count - 8)),
										atoi(list_get_val_at(sp, sp->count - 5)),
										atof(list_get_val_at(sp, sp->count - 2)));
								ent->is_local = 1;
								online_replay_list_add(ptr, ent);
							}

							list_free(sp2, free);
							list_free(sp, free);
							free(loc);
						}
					}
					list_free(split, free);
				}
			}
		}
		closedir(dr);
	}
	free(scores_dir);
}

#ifndef SERV
void online_replay_list_tick(online_replay_list *ptr, float width_lim, float x, float y)
{
	if(!(rg.song->game_mode_objects->count > 0)) return;
	if(!(rg.client->authed)) ptr->is_local_cbox->is_checked = 1;

	int enth = ONLINE_RL_ENTH;
	int maxh = rg.win_height * 0.75;
	int allh = enth * ptr->ents->count;
	if (allh < maxh)
	{
		ptr->scroll_y = 0;
		maxh = allh;
	}
	char in_bounds_oc = mouse.x_on_click >= x && mouse.x_on_click <= x + width_lim && mouse.y_on_click >= y && mouse.y_on_click <= y + maxh;
	char in_bounds = mouse.x >= x && mouse.x <= x + width_lim && mouse.y >= y && mouse.y <= y + maxh;
	if (mouse.left_click && in_bounds_oc && rg.gui_render_end_of_frame == ptr) ptr->scroll_y -= mouse.delta_y;
	list_node *n;
	ptr->scroll_y = clamp(ptr->scroll_y, 0, allh - maxh);
	float xo = x;
	float yo = y - ptr->scroll_y;
	glEnable(GL_SCISSOR_TEST);
	GL_SCISSOR_COORDS(x, y, width_lim, maxh);
	if(in_bounds_oc) rg.gui_render_ptr = ptr;
	int i = 1;
	for (n = ptr->ents->start; n != NULL; n = n->next)
	{
		online_replay_list_ent *e = n->val;
		e->rect = FLOATRECT(xo, yo, width_lim, enth);
		e->hovered = mouse.x >= e->rect.left && mouse.y >= e->rect.top && mouse.x <= e->rect.left + e->rect.width && mouse.y <= e->rect.top + e->rect.height && in_bounds;
		char click_good = get_distf(mouse.x, mouse.y, mouse.x_on_click, mouse.y_on_click) == 0.0f;
		float a = 100;
		color4 col = col_black;
		if (e->hovered) col = col_white;
		if(e->hovered && mouse.left_click_released && rg.gui_render_end_of_frame == ptr && click_good)
		{
			if(e->is_local)
			{
				char *data = file_to_str(e->name_on_server);
				song_load_replay(rg.song, data);
				free(data);
				rg.replay_mode_enabled = 1;
				rg.mode = rg.song->replay->mode;
				change_screen(screen_player);
			}
			else online_client_download_replay(rg.client, e->name_on_server);
		}
		GL_RECT2BOX(e->rect, color4_mod_alpha(col, a));
		text3_fmt(xo + 2, yo + (enth / 2), width_lim, 255, origin_left, origin_center, text3_justification_left, "%d. %s\nscore: %d acc: %.2f%% %dx", i++, e->name, e->score, e->accuracy, e->combo);
		yo += 30 * 1.5;
	}
	glDisable(GL_SCISSOR_TEST);

	//if(rg.client->authed)
	{
		checkbox_set_pos(ptr->is_local_cbox, PADDING, y - (ptr->is_local_cbox->size.y * 1.5));
		if(ptr->is_local_cbox->clicked)
		{
			if(!rg.client->authed)
			{
				popup_open(rg.global_popup, popup_type_dismiss, "you're not logged in!");
				ptr->is_local_cbox->is_checked = 1;
			}
			else
			{
				if(ptr->is_local_cbox->is_checked) online_replay_list_load_local(ptr);
				else online_client_request_replay_list(rg.client);
			}
		}
		checkbox_update(ptr->is_local_cbox);
		checkbox_render(ptr->is_local_cbox);
	}
}
#endif

void online_replay_list_free(online_replay_list *ptr)
{
	checkbox_free(ptr->is_local_cbox);
	online_replay_list_clear(ptr);
	list_free(ptr->ents, list_dummy);
	free(ptr);
}
