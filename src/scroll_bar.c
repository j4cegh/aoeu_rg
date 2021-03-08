#include "scroll_bar.h"
#include "rg.h"
#include "utils.h"
#include "console.h"

scroll_bar *current_scroll_bar;

scroll_bar *scroll_bar_init(SCROLL_BAR_ARGS)
{
	scroll_bar *ret = (scroll_bar*) malloc(sizeof *ret);
	ret->handle_clicked = 0;
	ret->cur_value = 0;
	ret->work_if_console_is_open = 0;
	ret->vlim = 0;
	ret->should_go_to_bottom = 0;
	ret->velocity = 0.0f;

	scroll_bar_update_vars(ret, x, y, height, max_value);
	return ret;
}

void scroll_bar_update_vars(scroll_bar *ptr, SCROLL_BAR_ARGS)
{
	ptr->x = x;
	ptr->y = y;
	ptr->height = height;
	ptr->max_value = max_value;

	ptr->b_size = V2F(SCROLL_BAR_WIDTH, height);

	ptr->b_pos = V2F(x, y);
}

void scroll_bar_update_rects(scroll_bar *ptr)
{
	ptr->hr_size = V2F(SCROLL_BAR_WIDTH - (SCROLL_BAR_HANDLE_PADDING * 2), (ptr->height / (ptr->max_value / ptr->height)) - (SCROLL_BAR_HANDLE_PADDING * 2));
	
	ptr->hr_pos = V2F(ptr->x + SCROLL_BAR_HANDLE_PADDING, (ptr->y + SCROLL_BAR_HANDLE_PADDING) + ptr->cur_value);
}

void scroll_bar_tick(scroll_bar *ptr)
{
	scroll_bar_update_rects(ptr);

	if(ptr->work_if_console_is_open || !cons_ptr->activated)
	{
		int left_pressed = left_click;

		char on_bar = mouse_x >= ptr->hr_pos.x && mouse_y >= ptr->hr_pos.y && mouse_x_on_click <= ptr->hr_pos.x + ptr->hr_size.x && mouse_y_on_click <= ptr->hr_pos.y + ptr->hr_size.y;
		if(on_bar && left_click == 1)
		{
			ptr->handle_clicked = 1;
			ptr->my_on_handle = mouse_y - (ptr->hr_pos.y - SCROLL_BAR_HANDLE_PADDING);
		}

		if(ptr->handle_clicked && left_click_released)
		{
			ptr->velocity = -mouse_release_velocity_y;
		}

		ptr->vlim = (ptr->height - SCROLL_BAR_HANDLE_PADDING * 2) - ptr->hr_size.y;

		if(ptr->handle_clicked && left_pressed)
		{
			float my_on_bar = clamp(mouse_y - (ptr->y) - ptr->my_on_handle, 0, ptr->vlim);

			ptr->cur_value = my_on_bar;
		}
		else ptr->handle_clicked = 0;

		if(ptr->should_go_to_bottom)
		{
			ptr->cur_value = ptr->vlim;
			ptr->should_go_to_bottom = 0;
		}
	}
	else ptr->handle_clicked = 0;

	if(timer_name_bool("sbv", 3)) ptr->velocity *= 0.991f;
	float bef_cur_value = ptr->cur_value;
	ptr->cur_value += ptr->velocity;
	ptr->cur_value = clamp(ptr->cur_value, 0, ptr->vlim);
	if(bef_cur_value == ptr->cur_value) ptr->velocity = 0.0f;

	scroll_bar_update_rects(ptr);
}

void scroll_bar_render(scroll_bar *ptr)
{
	float_rect brect = FLOATRECT(ptr->b_pos.x, ptr->b_pos.y, ptr->b_size.x, ptr->b_size.y);
	GL_OLBOX(brect, col_white, col_black, 1);
	float_rect rect = FLOATRECT(ptr->hr_pos.x, ptr->hr_pos.y, ptr->hr_pos.x + ptr->hr_size.x, ptr->hr_pos.y + ptr->hr_size.y);
	GL_RECT2QUAD(rect, col_white);
}

void scroll_bar_go_to_bottom(scroll_bar *ptr)
{
	ptr->should_go_to_bottom = 1;
	ptr->velocity = 0.0f;
}

float scroll_bar_get_real_value(scroll_bar *ptr)
{
	float real = (ptr->height - SCROLL_BAR_HANDLE_PADDING * 2) - ptr->hr_size.y;
	if(real == 0) return 0;
	float x = clamp(scale_value_to(ptr->cur_value, 0, real, 0, ptr->max_value), 0, ptr->max_value);
	return x;
}

void scroll_bar_set_me_as_cur(scroll_bar *ptr)
{
	current_scroll_bar = ptr;
}

void scroll_bar_free(scroll_bar *ptr)
{
	free(ptr);
}
