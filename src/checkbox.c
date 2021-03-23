#include "checkbox.h"
#include "rg.h"
#include "mouse.h"
#include "text3.h"
#include "gl_helper.h"

checkbox *checkbox_init(char *label, float x, float y, char is_checked)
{
	checkbox *ret = malloc(sizeof *ret);
	if(label) ret->label = dupe_str(label);
	else ret->label = NULL;
	ret->clicked = 0;
	ret->hovered = 0;
	ret->enabled = 1;
	ret->is_checked = is_checked;
	checkbox_set_pos(ret, x, y);
	checkbox_set_size(ret, 16, 16);
	return ret;
}

void checkbox_free(checkbox *ptr)
{
	if(ptr->label) free(ptr->label);
	free(ptr);
}

void checkbox_set_pos(checkbox *ptr, float x, float y)
{
	ptr->pos.x = x;
	ptr->pos.y = y;
}

void checkbox_set_size(checkbox *ptr, float width, float height)
{
	ptr->size.x = width;
	ptr->size.y = height;
}

void checkbox_update(checkbox *ptr)
{
	ptr->clicked = 0;

	ptr->hovered = 
		(
			mouse.x >= ptr->pos.x
			&&
			mouse.x <= ptr->pos.x + ptr->size.x
			&&
			mouse.y >= ptr->pos.y
			&&
			mouse.y <= ptr->pos.y + ptr->size.y
			&&
			mouse.x_on_click >= ptr->pos.x
			&&
			mouse.x_on_click <= ptr->pos.x + ptr->size.x
			&&
			mouse.y_on_click >= ptr->pos.y
			&&
			mouse.y_on_click <= ptr->pos.y + ptr->size.y
		);

	if(ptr->hovered) rg.gui_render_ptr = ptr;
	
	if(ptr->enabled && ptr->hovered && mouse.left_click_released && (rg.gui_render_end_of_frame == ptr))
	{
		ptr->is_checked = !(ptr->is_checked);
		ptr->clicked = 1;
	}
}

v2f checkbox_render(checkbox *ptr)
{
	float inner_padding = 1;
	float_rect fmt_ret_rect = FLOATRECTZERO;
	if(ptr->label) text3_fmt_rect(fmt_ret_rect, ptr->pos.x, ptr->pos.y, rg.win_width, 255, origin_left, origin_bottom, text3_justification_left, "%s", ptr->label);
	color4 fcol = (ptr->enabled ? col_white : col_gray);
	GL_OLBOX(FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y), fcol, col_black, inner_padding);
	if(ptr->is_checked) GL_RECT2BOX(FLOATRECT(ptr->pos.x + inner_padding, ptr->pos.y + inner_padding, ptr->size.x - (inner_padding * 2), ptr->size.y - (inner_padding * 2)), col_yellow);
	return V2F(fmt_ret_rect.width + ptr->size.x, fmt_ret_rect.height + ptr->size.y);
}
