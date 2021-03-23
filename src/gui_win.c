#include "gui_win.h"
#include "rg.h"
#include "utils.h"
#include "gui_win_man.h"

char gui_win_hovered;

void gui_win_empty_func(gui_win *ptr)
{
}

gui_win *gui_win_init(char *win_name,
	char *scene_name,
	float start_x,
	float start_y,
	int width,
	int height,
	color4 border_color,
	color4 bg_color,
	int do_not_add_to_window_manager,
	void(*init_func)(struct gui_win *ptr),
	void(*render_update_func)(struct gui_win *ptr),
	void(*deinit_func)(struct gui_win *ptr)
)
{
	gui_win *ret = malloc(sizeof *ret);

	ret->win_name = dupe_str(win_name);
	ret->scene_name = dupe_str(scene_name);

	ret->yu = 0;
	ret->visible = 0;
	ret->not_in_win_man = do_not_add_to_window_manager;
	ret->allow_drag_off_screen = 0;
	ret->rel_handle_mx = 0;
	ret->rel_handle_my = 0;
	ret->is_handle_hovered = 0;
	ret->is_hovered = 0;
	ret->border_color = border_color;
	ret->bg_color = bg_color;
	
	ret->size = V2F(width, height);
	ret->pos = V2F(start_x < 0 ? rg.win_width_mid - (ret->size.x / 2) : start_x, start_y < 0 ? rg.win_height_mid - (ret->size.y / 2) : start_y);

	ret->line_size = V2F(ret->size.x, BORDER_THICKNESS);
	ret->line_pos = V2F(start_x, start_y);

	ret->handle_text = text_init(ret->win_name, 0, 0, ret->border_color);
	ret->size.y += ret->handle_text->size.y;
	ret->line_pos.y += ret->handle_text->size.y;

	{
		if(render_update_func != NULL) ret->render_update_func = render_update_func;
		else ret->render_update_func = &gui_win_empty_func;

		if(init_func != NULL) ret->init_func = init_func;
		else ret->init_func = &gui_win_empty_func;

		if(deinit_func != NULL) ret->deinit_func = deinit_func;
		else ret->deinit_func = &gui_win_empty_func;

		ret->before_rend_func = &gui_win_empty_func;
	}

	ret->init_func(ret);

	if(!do_not_add_to_window_manager) gui_win_man_register(ret);
	return ret;
}

void gui_win_center(gui_win *ptr)
{
	ptr->pos = V2F(rg.win_width_mid - (ptr->size.x / 2), rg.win_height_mid - (ptr->size.y / 2));
}

void gui_win_move(gui_win *ptr, float x, float y)
{
	ptr->pos = V2F(x, y);
}

void gui_win_set_title(gui_win *ptr, char *new_title)
{
	free(ptr->win_name);
	ptr->win_name = dupe_str(new_title);
	text_set_string(ptr->handle_text, ptr->win_name);
}

void gui_win_check_grab(gui_win *ptr)
{
	int mx = mouse.x;
	int my = mouse.y;

	float x = ptr->pos.x;
	float y = ptr->pos.y;

	int relmx = mouse.x - x;
	int relmy = mouse.y - y;
	
	if(relmx >= 0 && relmy >= 0 && relmx <= ptr->size.x && relmy <= ptr->size.y) gui_win_hovered = 1;

	int handle_right_side = x + ptr->size.x;
	int handle_bottom = y + ptr->handle_text->size.y;
	ptr->yu = handle_bottom;

	if(left_click <= 2)
	{
		ptr->rel_handle_mx = relmx;
		ptr->rel_handle_my = relmy;
	}

	int left_pressed = left_click;

	if(!left_pressed)
	{
		ptr->is_handle_hovered = (mx >= x && my >= y && mx <= handle_right_side && my <= handle_bottom);
		ptr->is_hovered = (mx >= x && my >= y && mx <= x + ptr->size.x && my <= y + ptr->size.y);
	}

	if(ptr->is_hovered) rg.gui_render_ptr = ptr;

	if(rg.gui_render_end_of_frame != ptr) return;

	if(left_click && (ptr->is_hovered))
		gui_win_man_set_active(ptr);

	if(ptr->is_handle_hovered && left_pressed && (ptr == gui_win_man_get_active() || ptr->not_in_win_man))
	{
		gui_win_is_grabbed = 1;
		gui_win_move(ptr, mouse.x - ptr->rel_handle_mx, mouse.y - ptr->rel_handle_my);
	}
}

void gui_win_render(gui_win *ptr)
{
	ptr->before_rend_func(ptr);
	
	if(!ptr->allow_drag_off_screen && rg.win_width > ptr->size.x && rg.win_height > ptr->size.y) gui_win_move(ptr, clamp(ptr->pos.x, BORDER_THICKNESS, rg.win_width - ptr->size.x - BORDER_THICKNESS), clamp(ptr->pos.y, BORDER_THICKNESS, rg.win_height - (ptr->size.y) - BORDER_THICKNESS));
	
	gui_win_check_grab(ptr);

	float_rect brect = FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y);
	GL_OLBOX(brect, ptr->border_color, ptr->bg_color, 1);

	ptr->render_update_func(ptr);

	ptr->line_size = V2F(ptr->size.x, BORDER_THICKNESS);
	ptr->line_pos = V2F(ptr->pos.x, ptr->pos.y + ptr->handle_text->size.y);
	float_rect lrect = FLOATRECT(ptr->line_pos.x, ptr->line_pos.y, ptr->line_pos.x + ptr->line_size.x, ptr->line_pos.y + ptr->line_size.y);
	GL_RECT2QUAD(lrect, ptr->border_color);

	text_set_position(ptr->handle_text, ptr->pos.x + PADDING, ptr->pos.y + PADDING);
	text_render(ptr->handle_text);
}

void gui_win_free(gui_win *ptr)
{
	ptr->deinit_func(ptr);
	text_free(ptr->handle_text);
	free(ptr->scene_name);
	free(ptr->win_name);
	free(ptr);
}
