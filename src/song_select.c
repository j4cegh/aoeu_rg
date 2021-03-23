#include "song_select.h"
#include "utils.h"
#include "rg.h"
#include "skin.h"
#include "logger.h"
#include "song.h"
#include "text.h"
#include "text3.h"
#include "ezgui.h"
#include "online_client.h"
#include "textures.h"
#include "discord.h"
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <dirent.h>

song_select *ss;

int song_select_count_songs_in_folder()
{
	int ic = 0;
	DIR *dr = opendir(SONGS_DIR);
	struct dirent *entry;
	while((entry = readdir(dr)) != NULL) { if(entry->d_type == DT_REG || entry->d_type == DT_DIR) ++ic; }
	closedir(dr);
	return ic;
}

song_select_entry *song_select_entry_init(char *name, char *folder_loc, char *data_file_loc, char no_data_files)
{
	song_select_entry *ret = malloc(sizeof *ret);
	ret->name = dupe_str(name);
	ret->folder_loc = dupe_str(folder_loc);
	ret->data_file_loc = dupe_str(data_file_loc);
	ret->no_data_files = no_data_files;
	ret->scale = 1.0f;
	ret->rect = FLOATRECTZERO;
	ret->ry = 0.0f;
	ret->hovered = 0;
	ret->hovered_oc = 0;
	ret->should_render = 0;
	ret->x = 0;
	ret->traveled_to = 0;
	return ret;
}

void song_select_entry_click(song_select_entry* ptr, char enter)
{
	if (!ptr) return;
	if (!strs_are_equal(ptr->data_file_loc, rg.song->data_file_loc))
	{
		song_select_load(ptr);
		song_preview_play(rg.song);
		online_replay_list_reload(ss->orl);
		timer_restart(rg.fade_clock);
	}
	else
	{
		if (ptr->no_data_files || rg.song->game_mode_objects->count < 1) change_screen(screen_editor);
		else if (mouse.left_click_released || enter) change_screen(screen_player);
		else if (mouse.right_click_released) change_screen(screen_editor);
		if(rg.to_screen != rg.screen)
		{
			ss->bef_travel_y = ss->y_scroll;
			rg.song->ss_entry->traveled_to = 0;
			ss->show_ents = 0;
		}
	}
}

void song_select_entry_free(song_select_entry *ptr)
{
	free(ptr->data_file_loc);
	free(ptr->folder_loc);
	free(ptr->name);
	free(ptr);
}

void song_select_init()
{
	ss = malloc(sizeof *ss);

	ss->entries = list_init();

	ss->y_scroll = 0;
	ss->y_scroll_speed = 0.0f;
	ss->y_max_height = 0;

	ss->no_songs_text = text_init("drag an mp3 or ogg file into the window to import.", 0, 0, col_red);

	ss->scroll_clock = timer_init();
	ss->scan_clock = timer_init();
	ss->load_next = NULL;

	ss->last_song_count = song_select_count_songs_in_folder();
	ss->list_global_x = SONG_SELECT_ENTRY_WIDTH;
	ss->show_ents = 0;

	ss->bef_travel_y = 0;

	ss->orl = online_replay_list_init();
}

void song_select_free(char clear_only)
{
	rg.song->ss_entry = NULL;
	if(!clear_only)
	{
		online_replay_list_free(ss->orl);
		timer_free (ss->scan_clock);
		timer_free (ss->scroll_clock);
		text_free  (ss->no_songs_text);
	}
	if(clear_only) { list_clear(ss->entries, song_select_entry_free); }
	else list_free(ss->entries, song_select_entry_free);
	ss->load_next = NULL;
	if(!clear_only) free(ss);
}

song_select_entry *song_select_refresh(char no_status)
{
	timer_restart(rg.fade_clock);

	ss->orl->scroll_y = 0;

	ss->show_ents = 1;

	song_select_entry *ret = NULL;

	song_select_free(1);

	online_replay_list_reload(ss->orl);

	/* int i = 1000;
	while(i-- > 0) se_list_push_back(ss->entries, song_select_entry_init("entry", "", "", 0));
	goto rgssreloadend; */
	DIR *dr = opendir(SONGS_DIR);
	
	if(dr != NULL)
	{
		struct dirent *de;
		struct dirent *song_de;
		while((de = readdir(dr)) != NULL)
		{
			if(strs_are_equal(de->d_name, ".") || strs_are_equal(de->d_name, "..")) continue;
			if(de->d_type == DT_DIR)
			{
				char *folder = dupfmt(SONGS_DIR "%s/", de->d_name);
				DIR *song_dir;
/*
#ifndef RELEASE
				song_dir = opendir(folder);
				list *osu_files = list_init();
				if(song_dir != NULL)
				{
					while((song_de = readdir(song_dir)) != NULL)
					{
						list *file_split = split_str(song_de->d_name, ".");
						if(file_split != NULL)
						{
							if(file_split->count >= 2)
							{
								char *lower = str_to_lower((char*) file_split->end->val);
								if(strs_are_equal(lower, "osu")) list_push_back(osu_files, dupe_str(song_de->d_name));
								free(lower);
							}
							list_free(file_split, free);
						}
					}
					closedir(song_dir);
				}
				
				if(osu_files->count > 0)
				{
					list_node *on;
					for(on = osu_files->start; on != NULL; on = on->next)
					{
						char *osu_file = on->val;
						char *osu_file_loc = dupfmt("%s%s", folder, osu_file);
						song *converted = song_convert_from_osu_beatmap(osu_file_loc);
						free(osu_file_loc);
						if(converted)
						{
							char *new_song_file_loc = dupfmt("%s%s.song", folder, osu_file);
							char *converted_serialized = song_serialize(converted);
							str_to_file(new_song_file_loc, converted_serialized);
							free(converted_serialized);
							free(new_song_file_loc);
							song_free(converted);
						} else show_notif("error while converting .osu file");
					}
				}
				list_free(osu_files, free);
#endif
*/
				song_dir = opendir(folder);
				if(song_dir != NULL)
				{
					char no_song_files_found = 1;
					list *music_files_found = list_init();
					while((song_de = readdir(song_dir)) != NULL)
					{
						if(strlen(song_de->d_name) > 5)
						{
							list *file_split = split_str(song_de->d_name, ".");
							if(file_split != NULL)
							{
								if(file_split->count >= 2)
								{
									char *endstr = str_to_lower((char*) file_split->end->val);
									
									if(strs_are_equal(endstr, "song"))
									{
										no_song_files_found = 0;
										char *fixed = dupe_str(song_de->d_name);
										size_t fixed_len = strlen(fixed);
										fixed[fixed_len - 5] = '\0';
										char *song_file_str = dupfmt("%s%s", folder, song_de->d_name);
										list_push_back(ss->entries, song_select_entry_init(fixed, folder, song_file_str, 0));
										free(song_file_str);
										free(fixed);
									}
									else if(strs_are_equal(endstr, "ogg") || strs_are_equal(endstr, "mp3"))
										list_push_back(music_files_found, dupe_str(song_de->d_name));
									
									free(endstr);
								}
								list_free(file_split, free);
							}
						}
					}
					if(no_song_files_found)
					{
						list_node *mn;
						for(mn = music_files_found->start; mn != NULL; mn = mn->next)
						{
							char *mf = (char*) mn->val;
							char *song_file_str = dupfmt("%s%s  ", folder, mf);
							strcpy(song_file_str + strlen(song_file_str) - 5, "song");
							list_push_back(ss->entries, song_select_entry_init(mf, folder, song_file_str, 1));
							free(song_file_str);
						}
					}
					list_free(music_files_found, free);
					closedir(song_dir);
				}
				free(folder);
			}
			else if(de->d_type == DT_REG)
			{
				if(strlen(de->d_name) >= 4)
				{
					list *split = split_str(de->d_name, ".");
					if(split != NULL)
					{
						if(split->count >= 2)
						{
							char *last_str = str_to_lower((char*) (split->end->val));
							char is_ogg = str_contains(last_str, "ogg", 1);
							char is_mp3 = str_contains(last_str, "mp3", 1);
							if(strlen(last_str) == 3 && (is_ogg || is_mp3))
							{
								char *name = dupe_str(de->d_name);
								name[strlen(name) - 4] = '\0';
								
								char *new_folder_for_song = dupfmt(SONGS_DIR "%s/", name);
								make_dir(new_folder_for_song);

								char *mus_file_loc = dupfmt(SONGS_DIR "%s", de->d_name);
								char *new_loc = dupfmt("%s%s", new_folder_for_song, de->d_name);
								if(!file_exists(new_loc))
								{
									rename(mus_file_loc, new_loc);

									char *song_file_str = dupfmt("%s%s  ", new_folder_for_song, de->d_name);
									strcpy(song_file_str + strlen(song_file_str) - 5, "song");

									song_select_entry *sse = song_select_entry_init(de->d_name, new_folder_for_song, song_file_str, 0);
									list_push_back(ss->entries, sse);
									ret = sse;

									free(song_file_str);
								}
								free(new_loc);
								free(mus_file_loc);
								free(new_folder_for_song);
								free(name);
							}
							free(last_str);
							list_free(split, free);
						}
					}
				}
			}
		}
		closedir(dr);
	}
	/*rgssreloadend:*/
	ss->y_max_height = 0;
	if(ss->entries->count > 0)
	{
		ss->y_max_height = ss->entries->count * ((SONG_SELECT_ENTRY_HEIGHT) + (SONG_SELECT_ENTRY_PADDING));
		ss->y_max_height -= SONG_SELECT_ENTRY_PADDING;
		ss->y_max_height = clamp(ss->y_max_height, 0, INFINITY);
	}

	list_node *n;
	for(n = ss->entries->start; n != NULL; n = n->next)
	{
		song_select_entry *e = n->val;
		
		e->x = 50; SONG_SELECT_ENTRY_WIDTH;
	}
	ss->list_global_x = SONG_SELECT_ENTRY_WIDTH;
	/* if(rg.song->ss_entry) rg.song->ss_entry->traveled_to = 0; */
	
	//if(frames > 0 && !no_status) show_center_status("refreshed song list...");

	ss->y_scroll = 0;
	ss->y_scroll_speed = 0;

	return ret;
}

char song_select_load(song_select_entry *e)
{
	song_free(rg.song);
	rg.song = song_init(e->name, e->folder_loc, e->data_file_loc);
	char ret = song_load(rg.song);
#ifdef USING_WINDOWS
	char *status_text = dupfmt("listening to: %s", rg.song->name);
#ifdef RELEASE
	discord_update_presence(status_text, "", 0);
#else
	discord_update_presence(status_text, "DEBUG BUILD", 0);
#endif
	free(status_text);
#endif
	return ret;
}

void song_select_load_random()
{
	if(ss->entries->count < 1) return;
	srand(rg.time_open->microseconds * mouse.x * mouse.y * ss->bef_travel_y * ss->y_scroll_speed + ss->y_scroll * ss->y_max_height);
	
	int rn = rand_num_ranged(0, ss->entries->count - 1);
	song_select_entry *e = list_get_val_at(ss->entries, rn);
	song_select_load(e);

}

char one_hovered = 0;
char allow_hover_scroll = 1;
void song_select_tick()
{
	if(timer_bool(ss->scan_clock, 1000) && (rg.to_screen == screen_song_select && rg.screen == screen_song_select))
	{
		int new_count = song_select_count_songs_in_folder();
		if(ss->last_song_count != new_count)
		{
			song_select_refresh(1);
			show_center_status("song list updated.");
			ss->last_song_count = new_count;
		}
	}

	if(ss->entries->count < 1)
	{
		text_set_position(ss->no_songs_text, rg.win_width_mid - (ss->no_songs_text->size.x / 2), rg.win_height_mid);
		text_render(ss->no_songs_text);
	}
	else
	{
		rg.gui_render_ptr = rg.screen != rg.to_screen ? NULL : ss;
		
		//if(ss->list_global_x > 0)
		{
			if(ss->show_ents) ss->list_global_x -= rg.frame_delta_ms * 1.23;
			else /*if(ss->list_global_x < SONG_SELECT_ENTRY_WIDTH * 1.5)*/ ss->list_global_x += rg.frame_delta_ms * 1.23;
			if(ss->list_global_x < 0) ss->list_global_x = 0;
		}

		/*
		float mym = mouse.y - rg.whmid;
		if(allow_hover_scroll && mouse.inside_window && left_click == 0 && !one_hovered && abs(mym) >= rg.win_height / 2.2 && rg.gui_render_end_of_frame == ss && mouse.x >= rg.win_width / 3) ss->y_scroll_speed = clamp(mym, -3, 3);
		*/

		if(rg.song->ss_entry)
		{
			if(rg.song->ss_entry->traveled_to == 0)
			{
				float enty = rg.song->ss_entry->ry;
				float ny = ss->y_scroll;
				float to = enty - ((SONG_SELECT_ENTRY_HEIGHT + (SONG_SELECT_ENTRY_PADDING * 2)) / 2);
				float bef = ss->bef_travel_y;
				float spd = abs((ny - to) / 100);
				if(spd < 1) spd = 1;
				ss->y_scroll_speed = (ny < to ? spd : -spd);
				if((bef < enty && ny > to) || (bef > enty && ny < to))
				{
					ss->y_scroll = to;
					ss->y_scroll_speed = 0;
					rg.song->ss_entry->traveled_to = 1;
				}
			}
		}

		ss->y_scroll += (ss->y_scroll_speed * rg.frame_delta_ms);
		
		if(rg.gui_render_end_of_frame == ss)
		{
			if(mouse.left_click == 1) 
			{
				ss->y_scroll_speed = 0;
				allow_hover_scroll = 0;
			}
			if(mouse.left_click) ss->y_scroll -= mouse.delta_y;
			if(mouse.left_click_released) ss->y_scroll_speed = clamp(mouse.release_velocity_y, -7.0f, 7.0f);
		}

		if(abs(ss->y_scroll_speed) < 0.05) allow_hover_scroll = 1;
		float befclamp = ss->y_scroll;
		ss->y_scroll = clamp(ss->y_scroll, 0, ss->y_max_height);
		
		float middle = rg.win_height_mid;
		int cy = -(ss->y_scroll) + middle;

		list_node *n;
		
		float y = cy;
		one_hovered = 0;

		int v = ss->entries->count;
		
		for(n = ss->entries->start; n != NULL; n = n->next)
		{
			song_select_entry *e = n->val;
			e->ry = v * (SONG_SELECT_ENTRY_HEIGHT + SONG_SELECT_ENTRY_PADDING);
			v--;
		}
		
		if(rg.gui_render_end_of_frame == ss && rg.kr[SDL_SCANCODE_RETURN]) song_select_entry_click(rg.song->ss_entry, 1);

		for(n = ss->entries->end; n != NULL; n = n->before)
		{
			song_select_entry *e = n->val;
			if(rg.song->ss_entry == NULL && strs_are_equal(rg.song->data_file_loc, e->data_file_loc))
			{
				e->traveled_to = 0;
				ss->bef_travel_y = ss->y_scroll;
				rg.song->ss_entry = e;
			}
			float myheight = SONG_SELECT_ENTRY_HEIGHT * e->scale;
			float wantedx = e->hovered ? -50 : (abs(y -middle) / 10);
			float diff = (rg.frame_delta_ms / 4) * (abs(ss->y_scroll_speed) > 1 || mouse.left_click > 0 ? 0.1 : 1);
			if(e->x > wantedx)
			{
				e->x -= diff;
				if(e->x < wantedx) e->x = wantedx;
			}
			else if(e->x < wantedx)
			{
				e->x += diff;
				if(e->x > wantedx) e->x = wantedx;
			}
			float myw = SONG_SELECT_ENTRY_WIDTH * e->scale;
			e->rect = FLOATRECT(rg.win_width - myw + e->x + 50 + ss->list_global_x, y, myw, myheight);
			if(e->rect.left < 0) e->rect.left = 0;
			if(n != ss->entries->start) y += myheight + SONG_SELECT_ENTRY_PADDING;
			e->should_render =
			(
				e->rect.top + e->rect.height >= 0
				&&
				e->rect.top <= rg.win_height
			);

			e->hovered =
			(
				(
					mouse.x >= e->rect.left
					&&
					mouse.y >= e->rect.top
					&&
					mouse.x <= (e->rect.left + e->rect.width)
					&&
					mouse.y <= (e->rect.top + e->rect.height)

				)
				&&
				rg.gui_render_end_of_frame == ss
			);
			if(one_hovered) e->hovered = 0;
			if(e->should_render && e->hovered && e->hovered_oc && mouse.button_released) song_select_entry_click(e, 0);
			float sa = rg.frame_delta_ms / 12 / SONG_SELECT_ENTRY_HEIGHT;
			if(e->hovered)
			{
				e->scale += sa;
				one_hovered = 1;
			}
			else e->scale -= sa;
			e->scale = clamp(e->scale, 1.0f, 1.3f);
			e->hovered_oc =
			(
				(
					mouse.x_on_click >= e->rect.left
					&&
					mouse.y_on_click >= e->rect.top
					&&
					mouse.x_on_click <= (e->rect.left + e->rect.width)
					&&
					mouse.y_on_click <= (e->rect.top + e->rect.height)

				)
				&&
				get_distf(mouse.x_on_click, mouse.y_on_click, mouse.x, mouse.y) <= 20.0f
				&&
				rg.gui_render_end_of_frame == ss
			);
		}

		for(n = ss->entries->start; n != NULL; n = n->next)
		{
			song_select_entry *e = n->val;
			
			if(e->should_render)
			{
				color4 col = e->no_data_files ? col_yellow : e->hovered ? col_pink : rg.song->ss_entry == e ? col_green : col_white;
				GL_RECT2BOX(e->rect, color4_mod_alpha(col, e->hovered ? 100 : 50));
				textures_drawt(rg.skin->songsel_entry, e->rect.left, e->rect.top, e->rect.width, e->rect.height, col);
				GL_SCALE(e->rect.left + PADDING, e->rect.top + (e->rect.height / 2), e->scale);
				text3_fmt(0, 0, rg.win_width, 255, origin_left, origin_center, text3_justification_left, "~0%s", e->name);
				GL_SCALE_END();
			}
		}
		
		float sst = 0.005 * rg.frame_delta_ms;

		if(ss->y_scroll_speed > 0)
		{
			ss->y_scroll_speed -= sst;
			if(ss->y_scroll_speed < 0) ss->y_scroll_speed = 0;
		}
		else if(ss->y_scroll_speed < 0)
		{
			ss->y_scroll_speed += sst;
			if(ss->y_scroll_speed > 0) ss->y_scroll_speed = 0;
		}

		online_replay_list_tick(ss->orl, 300, 0, rg.win_height * 0.25 * 0.5);
	}
}
