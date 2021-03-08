#include "popup.h"
#include "rg.h"
#include "utils.h"
#include "input_box.h"

popup *popup_init()
{
	popup *ret = malloc(sizeof *ret);
	ret->type = popup_type_dismiss;
	ret->msg_text = text_init("", 0, 0, col_white);
	ret->dismiss_btn = button_init(0, 0, "", 1, 0, NULL);
	ret->yes_btn = button_init(0, 0, "yes", 1, 0, NULL);
	ret->no_btn = button_init(0, 0, "no", 1, 0, NULL);
	ret->ibox = input_box_init(0, 0, 0, "", "...");
	text_calc_bounds(ret->msg_text);
	ret->pos = V2F(0, 0);
	popup_update_size(ret);
	ret->is_opened = 0;
	ret->not_used = 1;
	ret->anim_timer = timer_init();
	ret->yes_no_callback = NULL;
	ret->txtbox_callback = NULL;
	ret->ptr = NULL;
	return ret;
}

void popup_free(popup *ptr)
{
	timer_free(ptr->anim_timer);
	input_box_free(ptr->ibox);
	button_free(ptr->no_btn);
	button_free(ptr->yes_btn);
	button_free(ptr->dismiss_btn);
	text_free(ptr->msg_text);
	free(ptr);
}

void popup_update_size(popup *ptr)
{
	ptr->size = V2FZERO;
	ptr->opened_size = V2FZERO;

	if(ptr->type == popup_type_dismiss)
	{
	}
	else if(ptr->type == popup_type_yes_no)
	{
	}
	else if(ptr->type == popup_type_input)
	{
	}

	if(ptr->type == popup_type_dismiss)
	{
		ptr->opened_size = V2F
		(
			ptr->msg_text->size.x * 1.1,
			(ptr->msg_text->size.y * 1.5)
			+
			(ptr->dismiss_btn->size.y * 2)
		);
		ptr->opened_size.x = clamp(ptr->opened_size.x, ptr->dismiss_btn->size.x + 20, rg.win_width);
	}
	else if(ptr->type == popup_type_yes_no)
	{
		ptr->opened_size = V2F
		(
			ptr->msg_text->size.x * 1.1,
			(ptr->msg_text->size.y * 1.5)
			+
			(ptr->yes_btn->size.y * 2)
		);
		ptr->opened_size.x = clamp(ptr->opened_size.x, ptr->yes_btn->size.x + ptr->no_btn->size.x + 20, rg.win_width);
	}
	else if(ptr->type == popup_type_input)
	{
		ptr->opened_size = V2F
		(
			ptr->msg_text->size.x * 1.1,
			(ptr->msg_text->size.y * 1.5)
			+
			(ptr->ibox->size.y * 1.5)
			+
			(ptr->dismiss_btn->size.y * 2)
		);
		ptr->opened_size.x = clamp(ptr->opened_size.x, ptr->dismiss_btn->size.x + 20, rg.win_width);
	}

}

void popup_update(popup *ptr)
{
	int ms_passed = clamp(timer_milliseconds(ptr->anim_timer), 0, POPUP_ANIM_LEN_MS);

	if(ptr->is_opened)
	{
		ptr->size.x = scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, 0, ptr->opened_size.x);
		ptr->size.y = scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, 0, ptr->opened_size.y);
	}
	else if(!ptr->not_used)
	{
		ptr->size.x = scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, ptr->opened_size.x, 0);
		ptr->size.y = scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, ptr->opened_size.y, 0);
	}

	ptr->size.x = clamp(ptr->size.x, 0, ptr->opened_size.x);
	ptr->size.y = clamp(ptr->size.y, 0, ptr->opened_size.y);
	
	int wm = (ptr->size.x / 2);
	int hm = (ptr->size.y / 2);

	ptr->pos = V2F
	(
		rg.win_width_mid - wm,
		rg.win_height_mid - hm
	);

	if(!ptr->is_opened) return;

	float y = ptr->pos.y + (ptr->msg_text->size.y * 0.5);

	text_set_position(ptr->msg_text, ptr->pos.x + wm - (ptr->msg_text->size.x / 2), y);

	if(rg.kp[SDL_SCANCODE_ESCAPE] == 1)
	{
		ptr->is_opened = 0;
		timer_restart(ptr->anim_timer);
	}

	if(ptr->type == popup_type_dismiss)
	{
		float by = ptr->msg_text->pos.y + ptr->msg_text->size.y * 1.5;
		button_set_pos(ptr->dismiss_btn, ptr->pos.x + wm - (ptr->dismiss_btn->size.x / 2), by);

		button_update(ptr->dismiss_btn);

		if(rg.kp[SDL_SCANCODE_RETURN] == 1 && rg.gui_render_end_of_frame == ptr) button_click(ptr->dismiss_btn);

		if(ptr->dismiss_btn->clicked) popup_close(ptr);
	}
	else if(ptr->type == popup_type_yes_no)
	{
		float by = ptr->msg_text->pos.y + ptr->msg_text->size.y * 1.5;
		button_set_pos(ptr->yes_btn, ptr->pos.x + wm - ptr->yes_btn->size.x, by);
		button_set_pos(ptr->no_btn, ptr->pos.x + wm + 10, by);

		button_update(ptr->yes_btn);
		button_update(ptr->no_btn);

		if(ptr->yes_btn->clicked)
		{
			if(ptr->yes_no_callback != NULL) ptr->yes_no_callback(ptr->ptr, 1);
			popup_close(ptr);
		}
		else if(ptr->no_btn->clicked)
		{
			if(ptr->yes_no_callback != NULL) ptr->yes_no_callback(ptr->ptr, 0);
			popup_close(ptr);
		}
	}
	else if(ptr->type == popup_type_input)
	{
		input_box_update_vars(ptr->ibox, ptr->pos.x + wm - (ptr->ibox->size.x / 2), y  + (ptr->msg_text->size.y * 1.5), ptr->msg_text->size.x);
		input_box_update(ptr->ibox);

		button_set_pos(ptr->dismiss_btn, ptr->pos.x + wm - (ptr->dismiss_btn->size.x / 2), ptr->ibox->pos.y + ptr->ibox->size.y * 1.5);
		
		button_update(ptr->dismiss_btn);

		/*if(rg.kp[SDL_SCANCODE_RETURN] == 1) se_button_click(ptr->dismiss_btn);*/

		if(ptr->dismiss_btn->clicked)
		{
			if(ptr->txtbox_callback != NULL) ptr->txtbox_callback(ptr->ptr, ptr->ibox->text);
			popup_close(ptr);
		}
	}
}

void popup_render(popup *ptr)
{
	float_rect rect = FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y);

	if(ptr->size.x > 1 && ptr->size.y > 1)
	{
		rg.gui_render_ptr = ptr;
		int ms_passed = clamp(timer_milliseconds(ptr->anim_timer), 0, POPUP_ANIM_LEN_MS);
		if(ptr->is_opened) { GL_RECT2BOX(FLOATRECT(0, 0, rg.win_width, rg.win_height), color4_mod_alpha(col_black, scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, 0, POPUP_DIM_ALPHA))); }
		else { GL_RECT2BOX(FLOATRECT(0, 0, rg.win_width, rg.win_height), color4_mod_alpha(col_black, scale_value_to(ms_passed, 0, POPUP_ANIM_LEN_MS, POPUP_DIM_ALPHA, 0))) };
		GL_OLBOX(rect, col_white, col_black, 1);
	}

	if(ptr->size.x != ptr->opened_size.x || ptr->size.y != ptr->opened_size.y) return;
	if(!ptr->is_opened) return;

	text_render(ptr->msg_text);

	if(ptr->type == popup_type_dismiss) button_render(ptr->dismiss_btn);
	else if(ptr->type == popup_type_yes_no)
	{
		button_render(ptr->yes_btn);
		button_render(ptr->no_btn);
	}
	else if(ptr->type == popup_type_input)
	{
		input_box_render(ptr->ibox);
		button_render(ptr->dismiss_btn);
		context_menu_render(ptr->ibox->menu);
	}
}

void popup_setmsg(popup *ptr, char *msg)
{
	text_set_string(ptr->msg_text, msg);
	text_calc_bounds(ptr->msg_text);
	popup_update_size(ptr);
}

void popup_close(popup *ptr)
{
	if(!ptr->is_opened) return;
	ptr->is_opened = 0;
	timer_restart(ptr->anim_timer);
	ptr->yes_no_callback = NULL;
	ptr->txtbox_callback = NULL;
	ptr->ptr = NULL;
}

void popup_open(popup *ptr, popup_type type, char *msg)
{
	if(ptr->is_opened && strs_are_equal(msg, ptr->msg_text->str)) return;
	ptr->type = type;
	if(ptr->type == popup_type_input) button_set_text(ptr->dismiss_btn, "submit");
	else button_set_text(ptr->dismiss_btn, "dismiss");
	popup_setmsg(ptr, msg);
	ptr->not_used = 0;
	ptr->is_opened = 1;
	if(ptr->type == popup_type_input) current_input_box = ptr->ibox;
	timer_restart(ptr->anim_timer);
}
