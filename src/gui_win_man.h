#pragma once

#include "list.h"
#include "gui_win.h"

void gui_win_man_init();
void gui_win_man_register(gui_win *ptr);
gui_win *gui_win_man_get_active();
void gui_win_man_render_windows();
void gui_win_man_set_active(gui_win *ptr);
void gui_win_man_free();

extern list *gui_windows_list;
extern char gui_win_is_grabbed;
