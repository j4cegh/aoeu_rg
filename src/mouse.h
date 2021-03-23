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
	SDL_SystemCursor cursor_type;
	float gui_x;
	float gui_y;
	timer *left_click_held;
	timer *middle_click_held;
	timer *right_click_held;
	
	int left_click;
	char left_click_released;

	int middle_click;
	char middle_click_released;

	int right_click;
	char right_click_released;
} mouse_data;

extern mouse_data mouse;
