#include "input_box.h"
#include "utils.h"
#include "rg.h"
#include "ezgui.h"
#include "popup.h"
#include "console.h"

input_box *current_input_box;

input_box *input_box_init(float x, float y, int box_width_pixels, char *initial_text, char *empty_text)
{
	input_box *ret = malloc(sizeof *ret);

	ret->pos = V2F(x, y);
	ret->size = V2F(box_width_pixels, 0);
	ret->box_width_pixels = box_width_pixels;

	if(empty_text != NULL) ret->empty_text = dupe_str(empty_text);
	else ret->empty_text = init_empty_string();

	if(initial_text != NULL) ret->text = dupe_str(initial_text);
	else ret->text = init_empty_string();

	ret->text_before_history_browse = NULL;

	ret->text_ptr = text_init(ret->text, ret->pos.x + PADDING, ret->pos.y + PADDING, col_white);
	ret->size.y = ret->text_ptr->size.y;
	ret->empty_text_ptr = text_init(ret->empty_text, ret->pos.x + PADDING, ret->pos.y + PADDING, color4_gen(50, 50, 50));
	ret->label_ptr = text_init(ret->empty_text, 0, 0, col_yellow);

	ret->blink_interval_ms = INPUT_BOX_BLINK_INTERVAL;
	ret->blink_clock = timer_init();
	ret->show_blink = 0;

	ret->history = list_init();

	ret->view_index = 0;

	ret->caret_posf = 0.0f;
	if(initial_text) ret->caret_index = strlen(initial_text);
	else ret->caret_index = 0;
	ret->caret_pos_at_start_of_click = 0;
	ret->char_select_index_start = ret->caret_index;
	ret->char_select_index_end = ret->caret_index;
	ret->xoff = 0.0f;

	ret->is_selecting_with_mouse = 0;
	ret->sel_state = input_box_sel_state_neutral;

	ret->before = NULL;
	ret->next = NULL;

	ret->menu = context_menu_init(0);
	context_menu_add_option(ret->menu, "copy", 0);
	context_menu_add_option(ret->menu, "delete", 0);
	context_menu_add_option(ret->menu, "paste", 0);
	context_menu_add_option(ret->menu, "select all", 0);
	context_menu_add_option(ret->menu, "clear", 0);
	context_menu_add_option(ret->menu, "cancel", 0);

	ret->show_label_above = 0;
	ret->force_numbers_only = 0;

	ret->is_inside_ezgui = 0;

	ret->is_password = 0;
	ret->is_browsing_history = 0;
	ret->history_index = 0;

	input_box_update_text(ret);
	return ret;
}

void input_box_free(input_box *ptr)
{
	context_menu_free(ptr->menu);
	list_free(ptr->history, free);
	timer_free(ptr->blink_clock);
	text_free(ptr->label_ptr);
	text_free(ptr->empty_text_ptr);
	text_free(ptr->text_ptr);
	free(ptr->empty_text);
	if(ptr->text_before_history_browse != NULL) free(ptr->text_before_history_browse);
	free(ptr->text);
	free(ptr);
}

void input_box_paste(input_box *ptr)
{
	ptr->is_browsing_history = 0;
	input_box_delete_selection(ptr);

	char *paste = dupe_str(SDL_GetClipboardText());
	str_remove_forbidden_chars(&paste, USEFUL_ASCII_CHARS, 1);
	size_t i;
	for(i = 0; i < strlen(paste); ++i)
	{
		char ch = paste[i];
		input_box_append_char(ptr, ch);
	}
	input_box_update_text(ptr);

	input_box_reset_selection(ptr);
}

void input_box_sel_all(input_box *ptr)
{
	ptr->char_select_index_start = 0;
	ptr->char_select_index_end = strlen(ptr->text);
	ptr->caret_index = ptr->char_select_index_end;
	ptr->sel_state = input_box_sel_state_right;
}

void input_box_events(input_box *ptr, SDL_Event e)
{
	int key = e.key.keysym.sym;
	if(key == SDLK_MENU) context_menu_activate(ptr->menu, mouse.x, mouse.y);
	if(key == SDLK_BACKSPACE) input_box_backspace(current_input_box, 0);
	if(key == SDLK_DELETE) input_box_backspace(current_input_box, 1);
	if(key == SDLK_ESCAPE && !cons_ptr->is_animating && !rg.global_popup->is_opened)
	{
		ptr->menu->activated = 0;
		current_input_box = NULL;
		return;
	}

	char shift = rg.kp[SDL_SCANCODE_LSHIFT];

	if(key == SDLK_TAB)
	{
		if(shift && current_input_box->before)
		{
			current_input_box = current_input_box->before;
		}
		else if(current_input_box->next)
		{
			current_input_box = current_input_box->next;
		}
	}

	if(ptr->char_select_index_start == ptr->char_select_index_end) ptr->sel_state = input_box_sel_state_neutral;

	if(key == SDLK_HOME)
	{
		ptr->caret_index = 0;

		if(!shift) input_box_reset_selection(ptr);
		else if(ptr->sel_state == input_box_sel_state_neutral || ptr->sel_state == input_box_sel_state_left)
		{
			ptr->sel_state = input_box_sel_state_left;
			ptr->char_select_index_start = ptr->caret_index;
		}
		else if(ptr->sel_state == input_box_sel_state_right)
		{
			ptr->char_select_index_end = ptr->char_select_index_start;
			ptr->char_select_index_start = 0;

			ptr->sel_state = input_box_sel_state_left;
		}
		ptr->is_selecting_with_mouse = 0;
	}
	else if(key == SDLK_END)
	{
		ptr->caret_index = strlen(ptr->text);

		if(!shift) input_box_reset_selection(ptr);
		else if(ptr->sel_state == input_box_sel_state_neutral || ptr->sel_state == input_box_sel_state_right)
		{
			ptr->sel_state = input_box_sel_state_right;
			ptr->char_select_index_end = ptr->caret_index;
		}
		else if(ptr->sel_state == input_box_sel_state_left)
		{

			ptr->char_select_index_start = ptr->char_select_index_end;
			ptr->char_select_index_end = strlen(ptr->text);

			ptr->sel_state = input_box_sel_state_right;
		}
		ptr->is_selecting_with_mouse = 0;
	}

	if(key == SDLK_LEFT)
	{
		ptr->caret_index--;

		if(!shift) input_box_reset_selection(ptr);
		else if(ptr->sel_state == input_box_sel_state_neutral || ptr->sel_state == input_box_sel_state_left)
		{
			ptr->sel_state = input_box_sel_state_left;
			ptr->char_select_index_start = ptr->caret_index;
		}
		else if(ptr->sel_state == input_box_sel_state_right) ptr->char_select_index_end--;
		ptr->is_selecting_with_mouse = 0;
	}
	else if(key == SDLK_RIGHT)
	{
		ptr->caret_index++;

		if(!shift) input_box_reset_selection(ptr);
		else if(ptr->sel_state == input_box_sel_state_neutral || ptr->sel_state == input_box_sel_state_right)
		{
			ptr->sel_state = input_box_sel_state_right;
			ptr->char_select_index_end = ptr->caret_index;
		}
		else if(ptr->sel_state == input_box_sel_state_left) ptr->char_select_index_start++;
		ptr->is_selecting_with_mouse = 0;
	}
	else if(ptr->history->count > 0 && (key == SDLK_UP || key == SDLK_DOWN))
	{
		char update = 0;
		if(key == SDLK_UP)
		{
			if(ptr->is_browsing_history == 0)
			{
				ptr->is_browsing_history = 1;
				ptr->history_index = ptr->history->count == 1 ? 0 : ptr->history->count - 1;
				if(ptr->text_before_history_browse != NULL) free(ptr->text_before_history_browse);
				ptr->text_before_history_browse = dupe_str(ptr->text);
				update = 1;
			}
			else
			{
				ptr->history_index = clamp(--ptr->history_index, 0, ptr->history->count);
				update = 1;
			}
		}
		else if(key == SDLK_DOWN && ptr->is_browsing_history == 1)
		{
			ptr->history_index = clamp(++ptr->history_index, 0, ptr->history->count);
			if(ptr->history_index == ptr->history->count)
			{
				ptr->is_browsing_history = 0;
				free(ptr->text);
				ptr->text = dupe_str(ptr->text_before_history_browse);
				input_box_update_text(ptr);
			}
			else update = 1;
		}
		if(update)
		{
			free(ptr->text);
			ptr->text = dupe_str((char*) list_get_val_at(ptr->history, ptr->history_index));
			ptr->caret_index = strlen(ptr->text);
			input_box_update_text(ptr);
		}
	}

	ptr->caret_index = clamp(ptr->caret_index, 0, strlen(ptr->text));
	ptr->char_select_index_start = clamp(ptr->char_select_index_start, 0, strlen(ptr->text));
	ptr->char_select_index_end = clamp(ptr->char_select_index_end, 0, strlen(ptr->text));

	if(rg.kp[SDL_SCANCODE_LCTRL])
	{
		if(key == SDLK_v)
		{
			input_box_paste(ptr);
		}
		else if(key == SDLK_c)
		{
			input_box_copy_selection(ptr);
		}
		else if(key == SDLK_a)
		{
			input_box_sel_all(ptr);
		}
		ptr->is_selecting_with_mouse = 0;
	}
}

int get_char_index_closest_to_mouse(input_box *ptr, char mouse_coords_at_start_of_click)
{
	int mouse_x_on_box = 0;
	int mouse_y_on_box = 0;

	if(mouse_coords_at_start_of_click)
	{
		mouse_x_on_box = mouse.x_on_click - ptr->pos.x + ptr->xoff;
		mouse_y_on_box = mouse.y_on_click - ptr->pos.y;
	}
	else
	{
		mouse_x_on_box = mouse.x - ptr->pos.x + ptr->xoff;
		mouse_y_on_box = mouse.y - ptr->pos.y;
	}


	float last_dist = INFINITY;
	int last_index = 0;

	int i;
	int len = strlen(ptr->text);
	for(i = 0; i <= len; i++)
	{
		float index_pos = input_box_find_char_x(ptr, i);
		float dist_calc = get_distf(index_pos, 0, mouse_x_on_box, mouse_y_on_box);
		if(dist_calc < last_dist)
		{
			last_dist = dist_calc;
			last_index = i;
		}
	}

	return last_index;
}

void input_box_update(input_box *ptr)
{
	if(ptr != current_input_box && ptr->menu->activated) ptr->menu->activated = 0;

	if(timer_milliseconds(ptr->blink_clock) > ptr->blink_interval_ms)
	{
		ptr->show_blink = !ptr->show_blink;
		timer_restart(ptr->blink_clock);
	}

	float_rect rect = FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y);
	char inside = mouse.x >= ptr->pos.x && mouse.y >= ptr->pos.y && mouse.x <= ptr->pos.x + rect.width && mouse.y <= ptr->pos.y + rect.height;
	char oc_inside = mouse.x_on_click >= ptr->pos.x && mouse.y_on_click >= ptr->pos.y && mouse.x_on_click <= ptr->pos.x + rect.width && mouse.y_on_click <= ptr->pos.y + rect.height;
	if(mouse.y < 0) inside = 0;
	if(inside && (left_click == 1) && !cons_ptr->activated && !cons_ptr->is_animating && (ptr == rg.gui_render_end_of_frame) && !current_context_menu)
	{
		current_input_box = ptr;
	}

	if(!(current_input_box == ptr) && left_click) input_box_reset_selection(ptr);

	if(left_click == 1 && current_input_box == ptr && inside && !(ptr->menu->activated))
	{
		ptr->caret_pos_at_start_of_click = get_char_index_closest_to_mouse(ptr, 1);
		ptr->caret_index = ptr->caret_pos_at_start_of_click;
		input_box_reset_selection(ptr);
		ptr->is_selecting_with_mouse = 1;
	}
	else if(oc_inside && left_click > 1 && current_input_box == ptr && !(ptr->menu->activated))
	{
		int cur_mouse_char_pos = get_char_index_closest_to_mouse(ptr, 0);
		if(cur_mouse_char_pos < ptr->caret_pos_at_start_of_click) ptr->sel_state = input_box_sel_state_left;
		else if(cur_mouse_char_pos > ptr->caret_pos_at_start_of_click) ptr->sel_state = input_box_sel_state_right;
		else ptr->sel_state = input_box_sel_state_neutral;

		if(ptr->sel_state == input_box_sel_state_left)
		{
			ptr->char_select_index_start = cur_mouse_char_pos;
			ptr->char_select_index_end = ptr->caret_pos_at_start_of_click;
			ptr->caret_index = ptr->char_select_index_start;
		}
		else if(ptr->sel_state == input_box_sel_state_right)
		{
			ptr->char_select_index_start = ptr->caret_pos_at_start_of_click;
			ptr->char_select_index_end = cur_mouse_char_pos;
			ptr->caret_index = ptr->char_select_index_end;
		}
		else
		{
			ptr->caret_index = cur_mouse_char_pos;
			input_box_reset_selection(ptr);
		}
		ptr->is_selecting_with_mouse = 1;
		if(ptr->text_ptr->size.x > ptr->size.x)
		{
			int mouse_x_on_box = mouse.x - ptr->pos.x;
			if(mouse_x_on_box <= 0) ptr->xoff -= 0.5;
			if(mouse_x_on_box >= ptr->size.x) ptr->xoff += 0.5;
			ptr->xoff = clamp(ptr->xoff, 0, ptr->text_ptr->size.x - ptr->size.x);
		}
	}
	else if(oc_inside && inside && right_click_released)
	{
		current_input_box = ptr;
		if((abs(ptr->char_select_index_end - ptr->char_select_index_start)) == 0)
		{
			ptr->caret_pos_at_start_of_click = get_char_index_closest_to_mouse(ptr, 1);
			ptr->caret_index = ptr->caret_pos_at_start_of_click;
			input_box_reset_selection(ptr);
			ptr->is_selecting_with_mouse = 1;
		}
		context_menu_activate(ptr->menu, mouse.x, mouse.y);
	}

	context_menu_update(ptr->menu);

	int clicked = context_menu_get_clicked_option(ptr->menu);
	
	if(clicked == 0) input_box_copy_selection(ptr);
	else if(clicked == 1) input_box_backspace(ptr, 0);
	else if(clicked == 2) input_box_paste(ptr);
	else if(clicked == 3) input_box_sel_all(ptr);
	else if(clicked == 4)
	{
		input_box_sel_all(ptr);
		input_box_backspace(ptr, 0);
	}
	else if(clicked == 5) ptr->menu->activated = 0;

	if(ptr != current_input_box) ptr->show_blink = 0;

	ptr->caret_posf = input_box_find_char_x(ptr, ptr->caret_index);
}

float input_box_find_char_x(input_box *ptr, int index)
{
	char *str = ptr->text_ptr->str;
	char cbi = str[index];
	str[index] = '\0';
	v2i char_size = V2IZERO;
	TTF_SizeText(rg.font, str, &(char_size.x), &(char_size.y));
	str[index] = cbi;
	return char_size.x;
}

void input_box_render(input_box *ptr)
{
	if(ptr->caret_posf < ptr->xoff) ptr->xoff = ptr->caret_posf;
	if(ptr->caret_posf > ptr->xoff + ptr->size.x) ptr->xoff = ptr->caret_posf - ptr->size.x;
	if(ptr->xoff > ptr->text_ptr->size.x - ptr->size.x) ptr->xoff = ptr->text_ptr->size.x - ptr->size.x;
	if(ptr->xoff < 0) ptr->xoff = 0;

	if(ptr->show_label_above) text_render(ptr->label_ptr);

	GL_OLBOX(FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y), (current_input_box == ptr ? col_white : color4_gen(50, 50 ,50)), col_black, 1);

	if(!ptr->is_inside_ezgui) glEnable(GL_SCISSOR_TEST);
	else glPushAttrib(GL_SCISSOR_BIT);
	GL_SCISSOR_COORDS(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y);

	text_set_position(ptr->empty_text_ptr, ptr->pos.x + PADDING, ptr->pos.y + PADDING);
	text_set_position(ptr->text_ptr, (ptr->pos.x - ptr->xoff), ptr->pos.y + 3);

	if(strlen(ptr->text) < 1 && !(ptr->show_label_above)) text_render(ptr->empty_text_ptr);
	else text_render(ptr->text_ptr);

	float selbox_posfs = input_box_find_char_x(ptr, ptr->char_select_index_start) - ptr->xoff;
	float selbox_posfe = input_box_find_char_x(ptr, ptr->char_select_index_end) - ptr->xoff;
	
	glColor4ub(255, 255, 255, clamp(scale_value_to(timer_milliseconds(ptr->blink_clock), 0, ptr->blink_interval_ms, 100, 75), 75, 100));
	glBegin(GL_QUADS);
	glVertex2f(ptr->pos.x + selbox_posfs, ptr->pos.y + 1);
	glVertex2f(ptr->pos.x + selbox_posfe, ptr->pos.y + 1);
	glVertex2f(ptr->pos.x + selbox_posfe, ptr->pos.y + ptr->size.y - 1);
	glVertex2f(ptr->pos.x + selbox_posfs, ptr->pos.y + ptr->size.y - 1);
	glEnd();

	if(ptr == current_input_box)
	{
		glColor4ub(255, 255, 255, clamp(scale_value_to(timer_milliseconds(ptr->blink_clock), 0, ptr->blink_interval_ms, 255, 50), 50, 255));
		glBegin(GL_QUADS);
		glVertex2f(ptr->pos.x + ptr->caret_posf - ptr->xoff, ptr->pos.y + 1);
		glVertex2f(ptr->pos.x + ptr->caret_posf - ptr->xoff + 1, ptr->pos.y + 1);
		glVertex2f(ptr->pos.x + ptr->caret_posf - ptr->xoff + 1, ptr->pos.y + ptr->size.y - 1);
		glVertex2f(ptr->pos.x + ptr->caret_posf - ptr->xoff, ptr->pos.y + ptr->size.y - 1);
		glEnd();
	}

	if(!ptr->is_inside_ezgui) glDisable(GL_SCISSOR_TEST);
	else glPopAttrib();
	
	char inside = mouse.x >= ptr->pos.x && mouse.y >= ptr->pos.y && mouse.x <= ptr->pos.x + ptr->size.x && mouse.y <= ptr->pos.y + ptr->size.y;
	if(inside) rg.gui_render_ptr = ptr;
	float_rect bounds = FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->pos.x + ptr->size.x, ptr->pos.y + ptr->size.y);
}

void input_box_append_char(input_box *ptr, char append)
{
	if(ptr != current_input_box) return;

	input_box_delete_selection(ptr);

	str_insert_char(&ptr->text, ptr->caret_index, append);
	
	input_box_update_text(ptr);

	ptr->caret_index++;
	input_box_reset_selection(ptr);
	ptr->is_browsing_history = 0;
}

void input_box_backspace(input_box *ptr, char del)
{
	if(ptr != current_input_box) return;

	if((abs(ptr->char_select_index_end - ptr->char_select_index_start)) > 0)
	{
		input_box_delete_selection(ptr);
		input_box_update_text(ptr);
		return;
	}

	int l = strlen(ptr->text);
	if(l < 1) return;
	if(del && ptr->caret_index == l) return;

	if(!del && !(ptr->caret_index == 0))
	{
		char *a = dupe_str(ptr->text);
		a[ptr->caret_index - 1] = '\0';
		char *b = dupe_str(a);
		free(a);
		char *aft = dupe_str(&ptr->text[ptr->caret_index]);
		append_to_str(&b, aft);
		free(aft);
		free(ptr->text);
		ptr->text = b;
		input_box_update_text(ptr);

		ptr->caret_index--;
	}
	else if(del)
	{
		char *text_before_caretb = dupe_str(ptr->text);
		text_before_caretb[ptr->caret_index] = '\0';
		char *text_before_caret = dupe_str(text_before_caretb);
		free(text_before_caretb);

		char *text_after_deleted_char = dupe_str(&ptr->text[ptr->caret_index + 1]);

		char *final = init_empty_string();
		append_to_str(&final, text_before_caret);
		append_to_str(&final, text_after_deleted_char);
		free(text_before_caret);
		free(text_after_deleted_char);
		free(ptr->text);
		ptr->text = final;

		input_box_update_text(ptr);
	}
	ptr->is_browsing_history = 0;

	input_box_reset_selection(ptr);
}

void input_box_clear_text(input_box *ptr)
{
	if(!(ptr->is_password)) input_box_add_to_history(ptr);
	free(ptr->text);
	ptr->text = init_empty_string();
	input_box_update_text(ptr);

	ptr->caret_index = 0;
	input_box_reset_selection(ptr);
	ptr->is_browsing_history = 0;

	ptr->view_index = 0;
}

void input_box_set_text(input_box *ptr, char *str)
{
	input_box_add_to_history(ptr);
	free(ptr->text);
	ptr->text = dupe_str(str);
	input_box_update_text(ptr);

	ptr->caret_index = 0;
	input_box_reset_selection(ptr);

	ptr->view_index = 0;
	ptr->is_browsing_history = 0;
}

void input_box_update_text(input_box *ptr)
{
	if(ptr->is_password)
	{
		size_t len = strlen(ptr->text);
		char *pwd_text = malloc((len + 1) * sizeof(char));
		memset(pwd_text, '*', len);
		pwd_text[len] = '\0';
		text_set_string(ptr->text_ptr, pwd_text);
		free(pwd_text);
	}
	else text_set_string(ptr->text_ptr, ptr->text);
	text_set_string(ptr->empty_text_ptr, ptr->empty_text);

	text_set_position(ptr->text_ptr, PADDING, PADDING);
	text_set_position(ptr->empty_text_ptr, PADDING, PADDING);
	text_set_position(ptr->label_ptr, ptr->pos.x, ptr->pos.y - PADDING - ptr->label_ptr->size.y);
}

void input_box_update_vars(input_box *ptr, float x, float y, int box_width)
{
	ptr->pos.x = x;
	ptr->pos.y = y;

	input_box_update_text(ptr);

	if(box_width != ptr->box_width_pixels)
	{
		ptr->box_width_pixels = box_width;
		ptr->size = V2F(box_width, ptr->text_ptr->size.y);
	}
}

void input_box_reset_selection(input_box *ptr)
{
	ptr->char_select_index_start = ptr->caret_index;
	ptr->char_select_index_end = ptr->caret_index;
	ptr->sel_state = input_box_sel_state_neutral;
}

void input_box_delete_selection(input_box *ptr)
{
	if((abs(ptr->char_select_index_end - ptr->char_select_index_start)) == 0) return;

	char *sel_end_to_text_end = dupe_str(&ptr->text[ptr->char_select_index_end]);
	char *nulled_at_text_start = dupe_str(ptr->text);
	nulled_at_text_start[ptr->char_select_index_start] = '\0';
	char *text_start_to_sel_start = dupe_str(nulled_at_text_start);
	free(nulled_at_text_start);

	char *final = init_empty_string();
	append_to_str(&final, text_start_to_sel_start);
	append_to_str(&final, sel_end_to_text_end);
	free(text_start_to_sel_start);
	free(sel_end_to_text_end);
	free(ptr->text);
	ptr->text = final;

	ptr->caret_index = ptr->char_select_index_start;
	ptr->char_select_index_end = ptr->caret_index;
}

void input_box_copy_selection(input_box *ptr)
{
	if((abs(ptr->char_select_index_end - ptr->char_select_index_start)) == 0) return;

	char *cpy = dupe_str(ptr->text);
	cpy[ptr->char_select_index_end] = '\0';

	char *final = dupe_str(&cpy[ptr->char_select_index_start]);

	SDL_SetClipboardText(final);

	free(cpy);
	free(final);
}

void input_box_add_to_history(input_box *ptr)
{
	if(ptr->history->count > 0 && (strs_are_equal(ptr->text, (char*) list_get_val_at(ptr->history, ptr->history->count - 1)))) return;
	list_push_back(ptr->history, dupe_str(ptr->text));
}
