#pragma once

#include "includes.h"

#define SCROLL_BAR_WIDTH 10
#define SCROLL_BAR_HANDLE_PADDING 1

typedef struct scroll_bar
{
	float x;
	float y;
	float velocity;
	float height;
	float max_value;
	float cur_value;
	float vlim;
	char handle_clicked;
	int my_on_handle;
	int should_go_to_bottom;
	int work_if_console_is_open;
	v2f hr_size;
	v2f hr_pos;
	v2f b_pos;
	v2f b_size;
} scroll_bar;

#define SCROLL_BAR_ARGS float x, float y, float height, float max_value
scroll_bar *scroll_bar_init(SCROLL_BAR_ARGS);
void scroll_bar_update_vars(scroll_bar *ptr, SCROLL_BAR_ARGS);
void scroll_bar_update_rects(scroll_bar *ptr);
void scroll_bar_tick(scroll_bar *ptr);
void scroll_bar_render(scroll_bar *ptr);
void scroll_bar_go_to_bottom(scroll_bar *ptr);
float scroll_bar_get_real_value(scroll_bar *ptr);
void scroll_bar_set_me_as_cur(scroll_bar *ptr);
void scroll_bar_free(scroll_bar *ptr);

extern scroll_bar *current_scroll_bar;
