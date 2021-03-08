#pragma once

#include "includes.h"
#include "text.h"
#include "list.h"
#include "context_menu.h"

#define INPUT_BOX_BLINK_INTERVAL 500

typedef enum input_box_sel_state
{
	input_box_sel_state_left,
	input_box_sel_state_right,
	input_box_sel_state_neutral
} input_box_sel_state;

typedef struct input_box
{
	v2f pos;
	v2f size;
	float box_width_pixels;
	char *empty_text;
	char *text;
	char *text_before_history_browse;
	text *text_ptr;
	text *empty_text_ptr;
	text *label_ptr;
	int blink_interval_ms;
	timer *blink_clock;
	char show_blink;
	list *history;
	int view_index;
	float caret_posf;
	int caret_index;
	int caret_pos_at_start_of_click;
	int char_select_index_start;
	int char_select_index_end;
	float xoff;
	char is_selecting_with_mouse;
	input_box_sel_state sel_state;
	struct input_box *before, *next;
	context_menu *menu;
	char show_label_above;
	char force_numbers_only;
	char is_inside_ezgui;
	char is_password;
	char is_browsing_history;
	int history_index;
} input_box;

input_box *input_box_init(float x, float y, int box_width_pixels, char *initial_text, char *empty_text);
void input_box_free(input_box *ptr);
void input_box_events(input_box *ptr, SDL_Event e);
void input_box_update(input_box *ptr);
float input_box_find_char_x(input_box *ptr, int index);
void input_box_render(input_box *ptr);
void input_box_append_char(input_box *ptr, char append);
void input_box_backspace(input_box *ptr, char del);
void input_box_clear_text(input_box *ptr);
void input_box_set_text(input_box *ptr, char *str);
void input_box_update_text(input_box *ptr);
void input_box_update_vars(input_box *ptr, float x, float y, int box_width);
void input_box_reset_selection(input_box *ptr);
void input_box_delete_selection(input_box *ptr);
void input_box_copy_selection(input_box *ptr);
void input_box_add_to_history(input_box *ptr);

extern input_box *current_input_box;
