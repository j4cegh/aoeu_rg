#pragma once

#include "includes.h"
#include "text.h"

typedef struct gui_win
{
	char *win_name;
	char *scene_name;
	float yu;
	v2f size;
	v2f pos;
	v2f line_size;
	v2f line_pos;
	char visible;
	char not_in_win_man;
	char allow_drag_off_screen;
	color4 border_color;
	color4 bg_color;
	text *handle_text;
	int rel_handle_mx;
	int rel_handle_my;
	int is_handle_hovered;
	int is_hovered;
	void (*init_func)(struct gui_win *ptr);
	void (*render_update_func)(struct gui_win *ptr);
	void (*deinit_func)(struct gui_win *ptr);
	void (*before_rend_func)(struct gui_win *ptr);
} gui_win;

extern char gui_win_hovered;

void gui_win_empty_func(gui_win *ptr);

gui_win *gui_win_init(char *win_name,
	char *scene_name,
	float start_x,
	float start_y,
	int width,
	int height,
	color4 border_color,
	color4 bg_color,
	int do_not_add_to_window_manager,
	void (*init_func)(struct gui_win *ptr),
	void (*render_update_func)(struct gui_win *ptr),
	void (*deinit_func)(struct gui_win *ptr)
);

void gui_win_center(gui_win *ptr);
void gui_win_render(gui_win *ptr);
void gui_win_move(gui_win *ptr, float x, float y);
void gui_win_set_title(gui_win *ptr, char *new_title);
void gui_win_free(gui_win *ptr);
