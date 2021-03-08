#pragma once

#include "timer.h"
#include <SDL2/SDL_mouse.h>

extern float mouse_state_x;
extern float mouse_state_y;
extern float mouse_x;
extern float mouse_y;
extern float gui_mouse_x;
extern float gui_mouse_y;
extern float mouse_x_on_click;
extern float mouse_x_on_click_real;
extern float mouse_y_on_click;
extern float mouse_y_on_click_real;
extern float mouse_delta_x;
extern float mouse_delta_y;
extern float mouse_drag_delta_x;
extern float mouse_drag_delta_y;
extern float mouse_release_velocity_x;
extern float mouse_release_velocity_y;
extern float mouse_vel_slow_ms_x;
extern float mouse_vel_slow_ms_y;
extern timer *mouse_release_velocity_timer;
extern char mouse_inside_window;
extern char mouse_outside_window_during_drag;
extern char mouse_moved;
extern char mouse_dragged;
extern char mouse_state_left;
extern char mouse_state_middle;
extern char mouse_state_right;
extern int left_click;
extern char left_click_released;
extern int middle_click;
extern char middle_click_released;
extern int right_click;
extern char right_click_released;
extern timer *left_click_held;
extern timer *middle_click_held;
extern timer *right_click_held;
extern char mouse_button_released;
extern SDL_SystemCursor cursor_type;
