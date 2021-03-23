#pragma once

#include "timer.h"
#include <SDL2/SDL_mouse.h>

typedef struct
{
	float state_x;
	float state_y;
	float x;
	float y;
	float x_on_click;
	float x_on_click_real;
	float y_on_click;
	float y_on_click_real;
	float delta_x;
	float delta_y;
	float drag_delta_x;
	float drag_delta_y;
	float release_velocity_x;
	float release_velocity_y;
	float vel_slow_ms_x;
	float vel_slow_ms_y;
	timer *release_velocity_timer;
	char inside_window;
	char outside_window_during_drag;
	char moved;
	char dragged;
	char state_left;
	char state_middle;
	char state_right;
	timer *moved_clock;
	char button_released;
} mouse_data;

extern mouse_data mouse;
extern float gui_mouse_x;
extern float gui_mouse_y;
extern int left_click;
extern char left_click_released;
extern int middle_click;
extern char middle_click_released;
extern int right_click;
extern char right_click_released;
extern timer *left_click_held;
extern timer *middle_click_held;
extern timer *right_click_held;
extern SDL_SystemCursor cursor_type;
