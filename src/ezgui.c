#include "ezgui.h"
#include "rg.h"
#include "utils.h"

list *widgets_list;
v2f ezgui_container_pos;
v2f ezgui_container_size;
v2f ezgui_container_padding;
ez_placement_state ezgui_placement_state;

v2f rcp;
char done_rendering = 1;
widget *lrw = NULL;
list *just_positioned_for_rendering;
list *widgets_of_last_row;
float last_split_row_height;
char not_inside_container;

widget *widget_init(char *name, widget_type type)
{
    widget *ret = malloc(sizeof *ret);
    ret->name = dupe_str(name);
    ret->ib = NULL;
    ret->b = NULL;
    ret->sb = NULL;
    ret->t = NULL;
    ret->type = type;
    ret->wanted_width = 0;
    return ret;
}

void widget_free(widget *ptr)
{
    if(ptr->ib != NULL) input_box_free(ptr->ib);
    if(ptr->b != NULL) button_free(ptr->b);
    if(ptr->sb != NULL) scroll_bar_free(ptr->sb);
    if(ptr->t != NULL) text_free(ptr->t);
    free(ptr->name);
    free(ptr);
}

void widget_tick(widget *w)
{
    if(w->ib == NULL && w->b == NULL && w->sb == NULL && w->t == NULL) return;
    if(w->type == widget_type_input_box)
    {
        input_box_update(w->ib);
        input_box_render(w->ib);
    }
    else if(w->type == widget_type_button)
    {
        button_update(w->b);
        button_render(w->b);
    }
    else if(w->type == widget_type_scroll_bar)
    {
    }
    else if(w->type == widget_type_text)
    {
		text_render(w->t);
    }
}

void widgets_init()
{
    rcp = V2F(0, 0);
    widgets_list = list_init();
    ezgui_container_pos = V2F(0, 0);
    ezgui_container_size = V2F(0, 0);
    ezgui_container_padding = V2F(0, 0);
    ezgui_placement_state = ez_placement_state_whole;

    just_positioned_for_rendering = list_init();
    widgets_of_last_row = list_init();
    not_inside_container = 1;
}

void widgets_free()
{
    list_free(widgets_of_last_row, list_dummy);
    list_free(just_positioned_for_rendering, list_dummy);
    list_free(widgets_list, widget_free);
}

char dont_scissor = 0;
void ez_cs(v2f pos, v2f size, v2f padding, char do_not_scissor)
{
    dont_scissor = do_not_scissor;
    not_inside_container = 0;
    done_rendering = 1;
    last_split_row_height = 0;
    list_clear(just_positioned_for_rendering, list_dummy);
    list_clear(widgets_of_last_row, list_dummy);
    ezgui_container_size = size;
    ezgui_container_padding = padding;
    ezgui_container_pos = pos;
    rcp = ezgui_container_pos;
    rcp.x += ezgui_container_padding.x;
    rcp.y += ezgui_container_padding.y;
    ezgui_placement_state = ez_placement_state_whole;

    if(!dont_scissor)
    {
        glEnable(GL_SCISSOR_TEST);
        GL_SCISSOR_COORDS(pos.x, pos.y, size.x, size.y);
    }
}

void ez_ce()
{
	/*
	float_rect wrect = FLOATRECT(0, 0, rg.win_width, rg.win_height);
	GL_RECT2QUAD(wrect, color4_mod_alpha(col_red, 50));
    float_rect wwrect = FLOATRECT(ezgui_container_pos.x, ezgui_container_pos.y, ezgui_container_pos.x + ezgui_container_size.x, ezgui_container_pos.y + ezgui_container_size.y);
	GL_RECT2QUAD(wwrect, color4_mod_alpha(col_green, 50));
	*/

    list_node *n;
    for(n = just_positioned_for_rendering->start; n != NULL; n = n->next)
    {
        widget *w = (widget*) n->val;
        widget_tick(w);
    }
    if(!dont_scissor) glDisable(GL_SCISSOR_TEST);
    lrw = NULL;
    not_inside_container = 1;
}

void ez_w()
{
    ezgui_placement_state = ez_placement_state_whole;
}

void ez_s()
{
    list_clear(widgets_of_last_row, list_dummy);
    ezgui_placement_state = ez_placement_state_split_row;
}

float_rect ez_gbs(widget *w)
{
    float_rect ret = FLOATRECT(0, 0, 0, 0);

    if(w)
    {
        if(w->type == widget_type_input_box)
        {
            ret.left = w->ib->pos.x;
            ret.top = w->ib->pos.y;
            ret.width = w->ib->size.x;
            ret.height = w->ib->size.y;
        }
        else if(w->type == widget_type_button)
        {
            ret.left = w->b->pos.x;
            ret.top = w->b->pos.y;
            ret.width = w->b->size.x;
            ret.height = w->b->size.y;
        }
        else if(w->type == widget_type_scroll_bar)
        {
            ret.left = w->sb->b_pos.x;
            ret.top = w->sb->b_pos.y;
            ret.width = w->sb->b_size.x;
            ret.height = w->sb->b_size.y;
        }
        else if(w->type == widget_type_text)
        {
            ret.left = w->t->pos.x;
            ret.top = w->t->pos.y;
            ret.width = w->t->size.x;
            ret.height = w->t->size.y;
        }
    }

    return ret;
}

widget *ez_g(char *name)
{
    widget *found = NULL;

    list_node *n;
    for(n = widgets_list->start; n != NULL; n = n->next)
    {
        widget *w = (widget*) n->val;
        if(strs_are_equal(w->name, name))
        {
            found = w;
            break;
        }
    }

    return found;
}

widget *ez_ga(char *name, widget_type type)
{
    widget *found = ez_g(name);
    if(!found)
    {
        found = widget_init(name, type);
        list_push_back(widgets_list, found);
    }
    return found;
}

void widget_set_pos_and_size(widget *w, v2f pos, v2f size)
{
    if(w)
    {
        if(w->type == widget_type_input_box)
        {
            input_box_update_vars(w->ib, pos.x, pos.y, size.x);
        }
        else if(w->type == widget_type_button)
        {
            button_set_pos(w->b, pos.x, pos.y);
            w->b->size.x = size.x;
        }
        else if(w->type == widget_type_text)
        {
            text_set_position(w->t, pos.x, pos.y);
        }
    }
}

widget *ez_pos(char *name, widget_type type)
{
    widget *already = ez_g(name);

    widget *w = already == NULL ? widget_init(name, type) : already;

    char just_fill = 0;
    int FONT_CHAR_HEIGHT = 30;
    if(ezgui_placement_state == ez_placement_state_split_row)
    {
        if(widgets_of_last_row->count > 0)
        {
            int new_widget_split_count = widgets_of_last_row->count + 1;
            int new_split_width_pixels = ((ezgui_container_size.x - (ezgui_container_padding.x * (2 + new_widget_split_count))) / new_widget_split_count);
            
            rcp.x = ezgui_container_pos.x + (ezgui_container_padding.x);

            list_node *n;
            for(n = widgets_of_last_row->start; n != NULL; n = n->next)
            {
                widget *lw = (widget*) n->val;
                float cwh = ez_gbs(lw).height;
                widget_set_pos_and_size(lw, rcp, (v2f) { new_split_width_pixels, cwh });
                rcp.x += new_split_width_pixels + ((ezgui_container_padding.x * new_widget_split_count));
                if(cwh > last_split_row_height) last_split_row_height = cwh;
            }
            
            w->wanted_width = new_split_width_pixels;
            if(already != NULL) widget_set_pos_and_size(already, rcp, V2F(new_split_width_pixels, FONT_CHAR_HEIGHT));
        }
        else just_fill = 1;
    }
    else if(ezgui_placement_state == ez_placement_state_whole)
    {
        rcp.x = ezgui_container_pos.x + (ezgui_container_padding.x);
        rcp.y += last_split_row_height + ezgui_container_padding.y;
        last_split_row_height = FONT_CHAR_HEIGHT;
        just_fill = 1;
    }
    

    if(already != NULL && just_fill)
    {
        rcp.x = ezgui_container_pos.x + (ezgui_container_padding.x);
        widget_set_pos_and_size(already, rcp, (v2f) { ezgui_container_size.x - (ezgui_container_padding.x * 2), FONT_CHAR_HEIGHT });
    }
    else if(just_fill) w->wanted_width = (ezgui_container_size.x - (ezgui_container_padding.x * 2));
    
    if(!already) list_push_back(widgets_list, w);
    list_push_back(just_positioned_for_rendering, w);
    list_push_back(widgets_of_last_row, w);

    done_rendering = 0;
    return w;
}

input_box *ez_ib(char *name, char *initial_text)
{
    widget *w = ez_pos(name, widget_type_input_box);
    if(w != NULL)
    {
        if(w->ib == NULL)
        {
            w->ib = input_box_init(rcp.x, rcp.y, w->wanted_width, initial_text, name);
            /* w->ib->is_inside_ezgui = 1; */
        }
        return w->ib;
    }
    return NULL;
}

button *ez_b(char *name, char *t)
{
    widget *w = ez_pos(name, widget_type_button);
    if(w != NULL)
    {
        if(w->b == NULL) w->b = button_init(rcp.x, rcp.y, t, 1, 0, NULL);
        return w->b;
    }
    return NULL;
}

button *ez_bp(char *name, char *str, float x, float y)
{
    widget *w = ez_ga(name, widget_type_button);
    if(w != NULL)
    {
        if(w->b == NULL) w->b = button_init(x, y, str, 1, 0, NULL);
        else
        {
            button_set_pos(w->b, x, y);
            button_set_text(w->b, str);
        }
        button_update(w->b);
        button_render(w->b);
        return w->b;
    }
    return NULL;
}

text *ez_t(char *name, char *str)
{
    widget *w = ez_pos(name, widget_type_text);
    if(w != NULL)
    {
        if(w->t == NULL) w->t = text_init(str, rcp.x, rcp.y, col_white);
        return w->t;
    }
    return NULL;
}

text *ez_tp(char *name, char *str, float x, float y, color4 col, char center_x, char center_y)
{
    widget *w = ez_ga(name, widget_type_text);
    if(w->t == NULL) w->t = text_init(str, x, y, col);
    else
    {
        text_set_string(w->t, str);
        text_set_position(w->t, x - (center_x ? w->t->size.x / 2 : 0), y - (center_y ? w->t->size.y / 2 : 0));
        text_set_color(w->t, col);
    }
    text_render(w->t);
    return w->t;
}

text *ez_tg(char *name, char *str, color4 col)
{
    widget *w = ez_ga(name, widget_type_text);
    if(w != NULL)
    {
        if(w->t == NULL) w->t = text_init(str, 0, 0, col);
        text_set_string(w->t, str);
        return w->t;
    }
    return NULL;
}

button *ez_bg(char *name, char *text)
{
    widget *w = ez_ga(name, widget_type_button);
    if(w != NULL)
    {
        if(w->b == NULL) w->b = button_init(0, 0, text, 1, 0, NULL);
        return w->b;
    }
    return NULL;
}