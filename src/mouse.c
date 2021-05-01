#include "mouse.h"
#include "rg.h"
#include "context_menu.h"

mouse_data mouse;

timer *click_block_timer;

void mouse_init()
{
	mouse.state_x = 0;
	mouse.state_y = 0;
	mouse.x = 0;
	mouse.y = 0;
	mouse.x_on_click = 0;
	mouse.y_on_click = 0;
	mouse.delta_x = 0;
	mouse.delta_y = 0;
	mouse.drag_delta_x = 0;
	mouse.drag_delta_y = 0;
	mouse.release_velocity_x = 0;
	mouse.release_velocity_y = 0;
	mouse.vel_slow_ms_x = 0;
	mouse.vel_slow_ms_y = 0;
	mouse.release_velocity_timer = timer_init();
	mouse.inside_window = 0;
	mouse.outside_window_during_drag = 0;
	mouse.moved = 0;
	mouse.dragged = 0;
	mouse.state_left = 0;
	mouse.state_middle = 0;
	mouse.state_right = 0;
	mouse.left_click = 0;
	mouse.left_click_released = 0;
	mouse.middle_click = 0;
	mouse.middle_click_released = 0;
	mouse.right_click = 0;
	mouse.right_click_released = 0;
	mouse.left_click_held = timer_init();
	mouse.middle_click_held = timer_init();
	mouse.right_click_held = timer_init();
	mouse.moved_clock = timer_init();
	mouse.button_released = 0;
	mouse.cursor_type = SDL_SYSTEM_CURSOR_ARROW;

	click_block_timer = timer_init();
}

void mouse_new_frame()
{
	mouse.button_released = 0;
	mouse.left_click_released = 0;
	mouse.middle_click_released = 0;
	mouse.right_click_released = 0;
}

void mouse_events(SDL_Event event)
{
	if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		if(event.button.button == SDL_BUTTON_LEFT) mouse.state_left = 1;
		if(event.button.button == SDL_BUTTON_MIDDLE) mouse.state_middle = 1;
		if(event.button.button == SDL_BUTTON_RIGHT) mouse.state_right = 1;
	}
	else if(event.type == SDL_MOUSEBUTTONUP)
	{
		if(event.button.button == SDL_BUTTON_LEFT) mouse.state_left = 0;
		if(event.button.button == SDL_BUTTON_MIDDLE) mouse.state_middle = 0;
		if(event.button.button == SDL_BUTTON_RIGHT) mouse.state_right = 0;
	}

	/*
	if(event.type == SDL_MOUSEMOTION)
	{
		mouse.state_x = event.motion.x - 1;
		mouse.state_y = event.motion.y - 1;
	}
	*/

	SDL_GetMouseState(&mouse.state_x, &mouse.state_y);

	if(event.type == SDL_MOUSEBUTTONUP) mouse.button_released = 1;
}

void mouse_update()
{
	if(rg.focus)
	{
		int cbt = clamp(timer_milliseconds(click_block_timer), 0, CLICK_BLOCK_TIME_MS);
		char ccc = mouse.state_left || mouse.state_middle || mouse.state_right;
		char good = (!current_context_menu || (current_context_menu->activated && current_context_menu->hovered));

		if(ccc && cbt < CLICK_BLOCK_TIME_MS) timer_restart(click_block_timer);

		if(current_context_menu && current_context_menu->button_click_avoided == 0 && !good && ccc)
		{
			current_context_menu->activated = 0;
			current_context_menu = NULL;
			timer_restart(click_block_timer);
		}
		else if(cbt >= CLICK_BLOCK_TIME_MS)
		{
			if(good && mouse.state_left) mouse.left_click++;
			else
			{
				if(good && mouse.left_click > 0) mouse.left_click_released = 1;
				mouse.left_click = 0;
				timer_restart(mouse.left_click_held);
			}

			if(good && mouse.state_middle) mouse.middle_click++;
			else
			{
				if(good && mouse.middle_click > 0) mouse.middle_click_released = 1;
				mouse.middle_click = 0;
				timer_restart(mouse.middle_click_held);
			}

			if(good && mouse.state_right) mouse.right_click++;
			else
			{
				if(good && mouse.right_click > 0) mouse.right_click_released = 1;
				mouse.right_click = 0;
				timer_restart(mouse.right_click_held);
			}
		}

		if(SDL_GetMouseFocus() != NULL) mouse.inside_window = 1;
		else mouse.inside_window = 0;
	}
	else
	{
		mouse.left_click = 0;
		mouse.left_click_released = 0;
		timer_restart(mouse.left_click_held);

		mouse.middle_click = 0;
		mouse.middle_click_released = 0;
		timer_restart(mouse.middle_click_held);

		mouse.right_click = 0;
		mouse.right_click_released = 0;
		timer_restart(mouse.right_click_held);
	}

	if((mouse.left_click > 0 || mouse.middle_click > 0 || mouse.right_click > 0) && !mouse.inside_window) mouse.outside_window_during_drag = 1;
	else if(mouse.left_click + mouse.middle_click + mouse.right_click == 0) mouse.outside_window_during_drag = 0;

	if(mouse.left_click == 1 || mouse.middle_click == 1 || mouse.right_click == 1)
	{
		mouse.x_on_click = mouse.state_x;
		mouse.y_on_click = mouse.state_y;
	}

	mouse.delta_x = mouse.state_x - mouse.x;
	mouse.delta_y = mouse.state_y - mouse.y;
	if(mouse.delta_x != 0 || mouse.delta_y != 0) mouse.moved = 1;
	
	mouse.x = mouse.state_x;
	mouse.y = mouse.state_y;

	if(mouse.left_click || mouse.middle_click || mouse.right_click)
	{
		mouse.drag_delta_x = mouse.x - mouse.x_on_click;
		mouse.drag_delta_y = mouse.y - mouse.y_on_click;
	}
	else
	{
		mouse.drag_delta_x = 0;
		mouse.drag_delta_y = 0;
	}

	mouse.dragged = (mouse.drag_delta_x != 0 || mouse.drag_delta_y != 0);

	if(mouse.left_click || mouse.middle_click || mouse.right_click)
	{
		mouse.release_velocity_x -= (mouse.delta_x / 2) * 0.05;
		mouse.release_velocity_y -= (mouse.delta_y / 2) * 0.05;
	}
	else
	{
		mouse.vel_slow_ms_x = 0;
		mouse.vel_slow_ms_y = 0;
	}

	if(mouse.delta_x == 0.0f) mouse.vel_slow_ms_x += rg.frame_delta_ms;
	if(mouse.delta_y == 0.0f) mouse.vel_slow_ms_y += rg.frame_delta_ms;
}

void mouse_free()
{
	timer_free(click_block_timer);
}
