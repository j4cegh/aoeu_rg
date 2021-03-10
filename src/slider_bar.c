#include "slider_bar.h"
#include "utils.h"
#include "rg.h"
#include "gl_helper.h"
#include "text3.h"

slider_bar *slider_bar_init(char *label, float x, float y, float width, float initial_val)
{
    slider_bar *ret = malloc(sizeof *ret);
    ret->label = text_init(label, 0, 0, col_white);
    ret->pos = V2F(x, y);
    ret->size = V2F(ret->label->size.x, ret->label->size.y);
    ret->size.x = return_highest(ret->size.x, width);
    ret->size.y += SLIDER_BAR_HEIGHT;
    ret->bar_size = V2F(width, SLIDER_BAR_HEIGHT);
    ret->val = initial_val;
    ret->x_on_bar = scale_value_to(ret->val, 0, 100, 0, ret->bar_size.x);
    ret->hovered = 0;
    ret->hovered_oc = 0;
    return ret;
}

void slider_bar_update(slider_bar *ptr)
{
    ptr->hovered =
        mouse_x >= ptr->pos.x &&
        mouse_x <= ptr->pos.x + ptr->bar_size.x &&
        mouse_y >= ptr->pos.y &&
        mouse_y <= ptr->pos.y + ptr->bar_size.y;
    
    ptr->hovered_oc =
        mouse_x_on_click >= ptr->pos.x &&
        mouse_x_on_click <= ptr->pos.x + ptr->bar_size.x &&
        mouse_y_on_click >= ptr->pos.y &&
        mouse_y_on_click <= ptr->pos.y + ptr->bar_size.y;
    
    if(ptr->hovered_oc && left_click)
    {
        ptr->x_on_bar = clamp(mouse_x - ptr->pos.x, 0, ptr->bar_size.x);
        ptr->val = scale_value_to(ptr->x_on_bar, 0, ptr->bar_size.x, 0, 100);
    }
}

void slider_bar_render(slider_bar *ptr)
{
    float y = ptr->pos.y + (ptr->bar_size.y / 2);
    
	GL_DRAWLINE(V2F(ptr->pos.x, y), V2F(ptr->pos.x + ptr->bar_size.x, y), col_white);
	GL_DRAWLINE(V2F(ptr->pos.x + ptr->x_on_bar, ptr->pos.y), V2F(ptr->pos.x + ptr->x_on_bar, ptr->pos.y + SLIDER_BAR_HEIGHT), col_yellow);

    text_set_position(ptr->label, ptr->pos.x, ptr->pos.y - ptr->label->size.y);
    text_render(ptr->label);
}

void slider_bar_set_val(slider_bar *ptr, float val)
{
    ptr->val = val;
    ptr->x_on_bar = scale_value_to(ptr->val, 0, 100, 0, ptr->bar_size.x);
}

void slider_bar_set_pos(slider_bar *ptr, float x, float y)
{
    ptr->pos.x = x;
    ptr->pos.y = y;
}

void slider_bar_free(slider_bar *ptr)
{
    text_free(ptr->label);
    free(ptr);
}
