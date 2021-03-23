#include "console.h"
#include "rg.h"
#include "utils.h"
#include "input_box.h"
#include "scroll_bar.h"
#include "online_client.h"
#include "ezgui.h"

console *cons_ptr;

void console_command_register(console *ptr, char *name, char *desc, CONSOLE_COMMAND_ON_CALL)
{
	console_command *add = malloc(sizeof *add);
	add->name = dupe_str(name);
	add->desc = dupe_str(desc);
	add->on_call = on_call;
	list_push_back(ptr-> commands, add);
}

void console_command_free(console_command *ptr)
{
	free(ptr->desc);
	free(ptr->name);
	free(ptr);
}

console_msg *console_msg_init(char *msg, color4 col)
{
	console_msg *ret = malloc(sizeof *ret);
	ret->text = text_init(msg, 0, 0, col);
	ret->text->col.a = 25;
	ret->anim_timer = timer_init();
	return ret;
}

void console_msg_free(console_msg *ptr)
{
	timer_free(ptr->anim_timer);
	text_free(ptr->text);
	free(ptr);
}

void console_cmd_list_cmds(CONSOLE_COMMAND_ON_CALL_ARGS)
{
	console_print(from, "available commands:", col_green);
	list_node *n;
	for(n = from->commands->start; n != NULL; n = n->next)
	{
		console_command *cmd = n->val;
		char *cmd_st = dupfmt(" - " CONSOLE_CMD_PREFIX_STR "%s: %s", cmd->name, cmd->desc);
		console_print(from, cmd_st, col_red);
		free(cmd_st);
	}
}

void console_cmd_unknown_command(CONS_UNKNOWN_CMD_ARGS)
{
	console_print(from, "unknown command. type .cmds to list available commands.", col_green);
}

void console_cmd_clear(CONSOLE_COMMAND_ON_CALL_ARGS)
{
	list_clear(from->message_history, free);
	console_print(cons_ptr, "cleared console history!", col_green);
}

console *console_init()
{
	console *ret = malloc(sizeof *ret);
	ret->message_queue = list_init();
	ret->message_history = list_init();
	ret->commands = list_init();
	ret->command_box = input_box_init(0, 0, 1, "", CONSOLE_EMPTY_BOX_MSG);
	ret->bar = scroll_bar_init(0, 0, 0, 0);
	ret->bar->work_if_console_is_open = 1;
	ret->activated = 0;
	ret->not_toggled_yet = 1;
	ret->vertical_drop = 0;
	ret->is_animating = 0;
	ret->anim_clock = timer_init();
	ret->msg_queue_timer = timer_init();
	ret->drag_scroll = 0;

	console_command_register(ret, "cmds", "list all commands", &console_cmd_list_cmds);
	console_command_register(ret, "clear", "clear all message history", &console_cmd_clear);
	console_reset_callbacks(ret);

	list_node *n;
	for(n = ret->command_box->menu->options->start; n != NULL; n = n->next)
	{
		context_menu_option *opt = n->val;
		opt->option_button->enable_in_console = 1;
	}

	return ret;
}

void console_free(console *ptr)
{
 	timer_free(ptr->msg_queue_timer);
	timer_free(ptr->anim_clock);
	scroll_bar_free(ptr->bar);
	input_box_free(ptr->command_box);

	list_free(ptr->commands, console_command_free);
	list_free(ptr->message_history, console_msg_free);
	list_free(ptr->message_queue, console_msg_free);

	free(ptr);
}

void console_toggle(console *ptr)
{
	if(!ptr->not_toggled_yet && timer_milliseconds(ptr->anim_clock) < (ptr->activated ? CONSOLE_DROP_ANIM_TIME_MS : CONSOLE_LEV_ANIM_TIME_MS)) return;
	ptr->not_toggled_yet = 0;
	ptr->is_animating = 1;
	ptr->activated = !ptr->activated;
	if(!ptr->activated) current_input_box = NULL;
	else current_input_box = ptr->command_box;

	if(ptr->activated) current_scroll_bar = ptr->bar;
	else current_scroll_bar = NULL;

	timer_restart(ptr->anim_clock);
}

void console_print(console *ptr, char *msg, color4 color)
{
	while(ptr->message_history->count > CONSOLE_MSG_LIMIT - 1)
	{
		console_msg_free(ptr->message_history->end->val);
		list_pop_back(ptr->message_history);
	}

	if(strlen(msg) > 0)
	{
		list *limited = split_str_char_lim(msg, CHAR_LIMIT_HORIZONTAL);
		list_node *n;
		for(n = limited->start; n != NULL; n = n->next) list_push_back(ptr->message_queue, console_msg_init(n->val, color));
		list_free(limited, free);
	}
}

void console_print_free(console *ptr, char *msg, color4 color)
{
	console_print(ptr, msg, color);
	free(msg);
}

void console_parse(console *ptr, char *text)
{
	int len = strlen(text);
	if(len >= 2 && text[0] == CONSOLE_CMD_PREFIX) text = &text[1];
	else
	{
		online_client_send_chat(rg.client, text);
		return;
	}
	console_print(ptr, text, col_yellow);
	len = strlen(text);
	list *split = split_str(text, " ");
	char *first = (char*) split->start->val;
	list_node *n;
	int command_exists = 0;
	char *after_arg0 = NULL;
	if(split->count > 1 && text[len - 1] != ' ') after_arg0 = dupe_str(&text[strlen(first) + 1]);
	for(n = ptr->commands->start; n != NULL; n = n->next)
	{
		console_command *cmd = (console_command*)n->val;
		if(strs_are_equal(first, cmd->name))
		{
			command_exists = 1;
			cmd->on_call(ptr, split, text, after_arg0);
		}
	}
	if(!command_exists) ptr->unknown_command(ptr, split, text, after_arg0);
	list_free(split, free);
	if(after_arg0) free(after_arg0);
	scroll_bar_go_to_bottom(ptr->bar);
}

void console_tick(console *ptr)
{
	float cons_height = rg.win_height;

	if(current_input_box != ptr->command_box && ptr->activated) console_toggle(ptr);

	float anim_time_ms = (ptr->activated ? CONSOLE_DROP_ANIM_TIME_MS : CONSOLE_LEV_ANIM_TIME_MS);

	if(ptr->activated || ptr->vertical_drop > -cons_height)
	{
		float drop_time_ms =
			clamp
			(
				timer_milliseconds(ptr->anim_clock),
				0,
				anim_time_ms
			);
		if(ptr->activated) ptr->vertical_drop = apply_easing(easing_type_out_bounce, drop_time_ms, -cons_height, cons_height, CONSOLE_DROP_ANIM_TIME_MS);
		else ptr->vertical_drop = apply_easing(easing_type_in, drop_time_ms, 0, -cons_height, CONSOLE_LEV_ANIM_TIME_MS);

		if(ptr->vertical_drop != 0) ptr->is_animating = 1;
		else ptr->is_animating = 0;
	}

	if(ptr->vertical_drop <= -cons_height)
	{
		ptr->is_animating = 0;
		return;
	}

	char still = timer_milliseconds(ptr->anim_clock) < (anim_time_ms * 2);

	if(timer_milliseconds(ptr->msg_queue_timer) > CONSOLE_QUEUE_TRANSFER_INTERVAL)
	{
		if(ptr->message_queue->count)
		{
			console_msg *cm = list_get_val_at(ptr->message_queue, 0);
			timer_restart(cm->anim_timer);
			list_push_front(ptr->message_history, cm);
			list_pop_front(ptr->message_queue);
			scroll_bar_go_to_bottom(ptr->bar);
		}
		timer_restart(ptr->msg_queue_timer);
	}

	v2f bd_pos = V2F(0, ptr->vertical_drop);
	v2f bd_size = V2F(rg.win_width, cons_height);

	if(ptr->activated) rg.gui_render_ptr = ptr;

	GL_RECT2BOX(FLOATRECT(bd_pos.x, bd_pos.y, bd_size.x, bd_size.y), color4_mod_alpha(col_black, 150));

	int command_box_y = ptr->vertical_drop + cons_height - ptr->command_box->size.y - 2;
	if(rg.resized || ptr->is_animating || still) input_box_update_vars(ptr->command_box, 2, command_box_y, rg.win_width - 4);

	float sbar_height = (cons_height - ptr->command_box->size.y) - 7;
	float total_lines_height = 0.0f;
 	list_node *n;
	for(n = ptr->message_history->start; n != NULL; n = n->next)
	{
		console_msg *m = n->val;
		total_lines_height += m->text->size.y;
	}
	scroll_bar_update_vars
	(
		ptr->bar,
		rg.win_width - SCROLL_BAR_WIDTH - PADDING,
		ptr->vertical_drop + 2,
		sbar_height,
		clamp
		(
			total_lines_height,
			sbar_height,
			INFINITY
		)
	);

	scroll_bar_tick(ptr->bar);

	float sbdh = (ptr->bar->hr_size.y / ptr->bar->height);
	if(mouse.left_click && !ptr->bar->handle_clicked)
	{
		if(mouse.left_click == 1) ptr->bar->velocity = 0.0f;
		ptr->drag_scroll = 1;
		ptr->bar->cur_value -= mouse.delta_y * sbdh;
	}
	else if(mouse.left_click_released && ptr->drag_scroll)
	{
		ptr->drag_scroll = 0;
		ptr->bar->velocity = mouse.release_velocity_y / 2;
	}

	float rsv = scroll_bar_get_real_value(ptr->bar);
	float sat = scale_value_to(rsv, 0, ptr->bar->max_value, 0, total_lines_height - sbar_height);

	glEnable(GL_SCISSOR_TEST);
	GL_SCISSOR_COORDS
	(
		PADDING,
		ptr->vertical_drop
		+
		PADDING,
		rg.win_width
		-
		SCROLL_BAR_WIDTH
		-
		(PADDING * 3),
		sbar_height
	);

	int rl_lim = 0;
	float yval = ptr->vertical_drop;
	for(n = ptr->message_history->end; n != NULL; n = n->before)
	{
		float ypos = (yval + 2) - sat;

		console_msg *m = n->val;

		float thei = m->text->size.y;
		yval += thei;
		if(ypos < -thei || ypos > rg.win_height)
		{
			++rl_lim;
			continue;
		}
		
		m->text->col.a =
			clamp
			(
				scale_value_to
				(
					timer_milliseconds(m->anim_timer),
					0,
					CONSOLE_MSG_SLIDE_ANIM_MS,
					0,
					255
				),
				0,
				255
			);
		
		float tx =
			(
				PADDING
				+
				100
				-
				apply_easing
				(
					easing_type_out_bounce,
					clamp
					(
						timer_milliseconds(m->anim_timer),
						0,
						CONSOLE_MSG_SLIDE_ANIM_MS
					),
					0,
					100,
					CONSOLE_MSG_SLIDE_ANIM_MS
				)
			);
		text_set_position(m->text, tx, ypos);
		text_render(m->text);
		++rl_lim;
	}

	glDisable(GL_SCISSOR_TEST);

	input_box_update(ptr->command_box);
	input_box_render(ptr->command_box);
	
	char *it = ptr->command_box->text;
	size_t itlen = strlen(it);
	if(itlen > 1)
	{
		if(it[0] == CONSOLE_CMD_PREFIX)
		{
			++it;
			list *split = split_str(it, " ");
			
			struct candidate
			{
				int good;
				console_command *cmd;
			};

			if(split->count == 1)
			{
				char *first = split->start->val;
				size_t fl = strlen(first);

				list *candidates = list_init();

				list_node *n;
				for(n = ptr->commands->start; n != NULL; n = n->next)
				{
					console_command *cmd = n->val;
					char *cnm = cmd->name;
					size_t cmnl = strlen(cnm);
					int good = 0;
					size_t i1;
					for(i1 = 0; i1 < cmnl; ++i1)
					{
						char i1c = cnm[i1];
						size_t i2;
						for(i2 = 0; i2 < fl; ++i2)
						{
							char i2c = first[i2];
							if(i1c == i2c) good++;
						}
					}
					if(good > 0)
					{
						struct candidate *cand = malloc(sizeof *cand);
						cand->good = good;
						cand->cmd = cmd;
						list_push_back(candidates, cand);
					}
				}

				char swapped;

				do
				{
					swapped = 0;
					n = candidates->start;
					if(n == NULL) break;
					list_node *ln = NULL;
					while(n->next != ln)
					{
						struct candidate *o =  (struct candidate*) n->val;
						struct candidate *on = (struct candidate*) n->next->val;
						if(o->good < on->good)
						{
							list_swap(n, n->next);
							swapped = 1;
						}
						n = n->next;
					}
					ln = n;
				} while(swapped);

				char *cands_str = init_empty_string();

				for(n = candidates->start; n != NULL; n = n->next)
				{
					struct candidate *cand = n->val;
					append_to_str_free_append(&cands_str, dupfmt("%s (%d) ", cand->cmd->name, cand->good));
				}

				text *cands = ez_tg("cands", cands_str, col_yellow);
				text_set_position(cands, ptr->command_box->caret_posf - ptr->command_box->xoff, ptr->command_box->pos.y - cands->size.y);
				text_render(cands);

				free(cands_str);

				list_free(candidates, free);
			}

			list_free(split, free);
		}
	}

	scroll_bar_render(ptr->bar);

	context_menu_render(ptr->command_box->menu);
}

void console_enter(console *ptr)
{
	if(strlen(ptr->command_box->text) < 1)  return;
	console_parse(ptr, ptr->command_box->text);
	input_box_clear_text(ptr->command_box);
}

void console_reset_callbacks(console *ptr)
{
	ptr->unknown_command = &console_cmd_unknown_command;
}
