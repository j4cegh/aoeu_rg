#include "button.h"
#include "rg.h"
#include "context_menu.h"

button *button_init(float x, float y, char *text, int enabled, char inside_cmenu, context_menu *host_cmenu)
{
	button *ret = malloc(sizeof *ret);
	ret->pos.x = x;
	ret->pos.y = y;
	ret->button_text = text_init(text, ret->pos.x, ret->pos.y, col_white);
	ret->label_text = NULL;
	ret->size.x = ret->button_text->size.x + (PADDING * 2);
	ret->size.y = ret->button_text->size.y;
	ret->enabled = enabled;
	ret->host_cmenu = host_cmenu;
	ret->on_click_cmenu = NULL;
	ret->hovered = 0;
	ret->hover_frames = 0;
	ret->clicked = 0;
	ret->is_in_popup = 0;
	ret->inside_popup = 0;
	ret->flashes = 0;
	ret->flashing = 0;
	ret->flash_state = 0;
	ret->flash_clock = timer_init();
	ret->enable_in_console = 0;
	ret->blocked = 0;
	ret->col = col_white;
	ret->bg_col = col_black;
	ret->outline_col = col_white;
	ret->held_for_long = 0;
	ret->held_time_min = 250;
	return ret;
}

void button_free(button *ptr)
{
	if(ptr->on_click_cmenu) context_menu_free(ptr->on_click_cmenu);
	timer_free(ptr->flash_clock);
	if(ptr->label_text != NULL) text_free(ptr->label_text);
	text_free(ptr->button_text);
	free(ptr);
}

void button_set_to_oc_cmenu(button *ptr)
{
	if(ptr->on_click_cmenu == NULL)
		ptr->on_click_cmenu = context_menu_init(0);
}

void button_update(button *ptr)
{
	float_rect rect = FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->pos.x + ptr->size.x, ptr->pos.y + ptr->size.y);
	ptr->hovered = mouse.x >= ptr->pos.x && mouse.y >= ptr->pos.y && mouse.x <= (rect.width - PADDING) && mouse.y <= (rect.height - PADDING);
	ptr->hover_frames++;
	if(!(ptr->hovered))
	{
		if(ptr->host_cmenu && left_click > 0 && ptr->hover_frames > 0)
			ptr->host_cmenu->button_click_avoided = 1;
		ptr->hovered = 0;
		ptr->hover_frames = 0;
	}

	if(!(ptr->enabled))
	{
		ptr->bg_col = col_black;
		ptr->outline_col = color4_gen(50, 50, 50);
		text_set_color(ptr->button_text, color4_gen(50, 50, 50));
		ptr->clicked = 0;
		return;
	}

	char oc_in_bounds = mouse.x_on_click >= ptr->pos.x && mouse.y_on_click >= ptr->pos.y && mouse.x_on_click <= rect.width && mouse.y_on_click <= rect.height;
	if(ptr->hovered && (ptr == rg.gui_render_end_of_frame))
	{
		ptr->bg_col = ptr->col;
		ptr->outline_col = col_black;
		text_set_color(ptr->button_text, col_black);
	}
	else
	{
		ptr->bg_col = col_black;
		ptr->outline_col = col_white;
		text_set_color(ptr->button_text, ptr->col);
	}

	char clicked = (left_click_released) && ptr->hovered && (ptr == rg.gui_render_end_of_frame);

	if(clicked && oc_in_bounds)
	{
		ptr->flashing = 1;
		timer_restart(ptr->flash_clock);
	}
	else ptr->clicked = 0;

	if(!(ptr->flashing) && left_click && oc_in_bounds)
	{
		ptr->bg_col = col_black;
		ptr->outline_col = col_white;
		text_set_color(ptr->button_text, ptr->col);
	}

	int flash_time = timer_milliseconds(ptr->flash_clock);
	if(ptr->flashing && flash_time >= BUTTON_FLASH_TIME_MS)
	{
		ptr->flashes++;
		ptr->flash_state = !(ptr->flash_state);
		timer_restart(ptr->flash_clock);
	}

	if(ptr->flashes >= BUTTON_FLASH_COUNT && oc_in_bounds)
	{
		if(ptr->on_click_cmenu != NULL)
		{
			context_menu_activate(ptr->on_click_cmenu, ptr->pos.x + PADDING, ptr->pos.y + PADDING);
		}
		ptr->clicked = 1;
		ptr->flashing = 0;
		ptr->flashes = 0;
		ptr->flash_state = 0;
		if(ptr->host_cmenu) ptr->host_cmenu->button_click_avoided = 0;
	}

	if(ptr->flashing)
	{
		if(ptr->flash_state == 0)
		{
			ptr->bg_col = col_white;
			ptr->outline_col = col_black;
			text_set_color(ptr->button_text, col_black);
		}
		else if(ptr->flash_state == 1)
		{
			ptr->bg_col = col_black;
			ptr->outline_col = col_white;
			text_set_color(ptr->button_text, col_white);
		}
	}

	if(ptr->hovered && (timer_milliseconds(left_click_held) >= ptr->held_time_min) && oc_in_bounds && (ptr == rg.gui_render_end_of_frame))
	{
		ptr->bg_col = color4_gen_alpha(20, 20, 20, 255);
		text_set_color(ptr->button_text, color4_gen_alpha(50, 50, 50, 255));
		ptr->held_for_long = 1;
	}
	else ptr->held_for_long = 0;
}

void button_render(button *ptr)
{
	if(ptr->host_cmenu != NULL) { GL_RECT2BOX(FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y), ptr->bg_col); }
	else GL_OLBOX(FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y), ptr->outline_col, ptr->bg_col, 1);
	
	text_render(ptr->button_text);
	if(ptr->label_text != NULL) text_render(ptr->label_text);
	if(ptr->hovered) rg.gui_render_ptr = ptr;

	float_rect bounds = FLOATRECT
	(
		ptr->pos.x,
		ptr->pos.y,
		ptr->pos.x + ptr->size.x,
		ptr->pos.y + ptr->size.y
	);
}

void button_enable(button *ptr)
{
	ptr->enabled = 1;
}

void button_disable(button *ptr)
{
	ptr->enabled = 0;
}

void button_set_pos(button *ptr, float x, float y)
{
	ptr->pos.x = x;
	ptr->pos.y = y;
	text_set_position(ptr->button_text, x + PADDING, y + PADDING);
	if(ptr->label_text != NULL) text_set_position(ptr->label_text, x, y - ptr->button_text->size.y);
}

void button_set_text(button *ptr, char *btn_text)
{
	text_set_string(ptr->button_text, btn_text);
	button_set_size(ptr, ptr->button_text->size.x + (PADDING * 2), ptr->button_text->size.y);
}

void button_set_size(button *ptr, float width, float height)
{
	ptr->size.x = width;
	ptr->size.y = height;
}

void button_click(button *ptr)
{
	ptr->clicked = 1;
}
