#include "context_menu.h"
#include "rg.h"
#include "utils.h"

context_menu_option *context_menu_option_init(char *name, char disabled, struct context_menu *host)
{
    context_menu_option *ret = malloc(sizeof *ret);
    ret->name = dupe_str(name);
    ret->disabled = disabled;
    ret->option_button = button_init(0, 0, name, !disabled, 1, host);
    ret->cmenu_cb_fp = NULL;
    ret->pass = NULL;
    return ret;
}

void context_menu_option_free(context_menu_option *ptr)
{
    button_free(ptr->option_button);
    free(ptr->name);
    free(ptr);
}

context_menu *current_context_menu;

context_menu *context_menu_init(char horizontal)
{
    context_menu *ret = malloc(sizeof *ret);
    ret->pos = V2F(0, 0);
    ret->size = V2F(4, 4);
    ret->options = list_init();
    ret->activated = 0;
    ret->hovered = 1;
	ret->button_click_avoided = 0;
	ret->horizontal = horizontal;
    return ret;
}

void context_menu_free(context_menu *ptr)
{
    list_free(ptr->options, context_menu_option_free);
    free(ptr);
}

void context_menu_activate(context_menu *ptr, float x, float y)
{
	if(current_context_menu && current_context_menu != ptr) current_context_menu->activated = 0;
	current_context_menu = ptr;
	ptr->activated = 1;
	x += PADDING;
	y += PADDING;
	x = clamp(x, PADDING, rg.win_width - ptr->size.x - PADDING);
	y = clamp(y, PADDING, rg.win_height - ptr->size.y - PADDING);
	ptr->pos.x = x;
	ptr->pos.y = y;
	int by = y;
	int bynoy = 0;
	int bw = 0;
	list_node *n;
	for(n = ptr->options->start; n != NULL; n = n->next)
	{
		context_menu_option *opt = (context_menu_option*) n->val;
		button_set_pos(opt->option_button, x, by);
		int hhh = (opt->option_button->size.y);
		by += hhh;
		bynoy += hhh;
		int bwp = opt->option_button->size.x;
		if(bwp > bw) bw = bwp;
	}
    ptr->size.x = bw;
    ptr->size.y = bynoy;

	for(n = ptr->options->start; n != NULL; n = n->next)
	{
		context_menu_option *opt = (context_menu_option*) n->val;
		button *b = opt->option_button;
        b->size.x = ptr->size.x;
	}
}

context_menu_option *context_menu_add_option(context_menu *ptr, char *name, char disabled)
{
    context_menu_option *opt = context_menu_option_init(name, disabled, ptr);
    opt->option_button->host_cmenu = ptr;
    int wp = opt->option_button->size.x + (PADDING * 2);
    if(wp > ptr->size.x) ptr->size.x = wp;
    ptr->size.y += ((opt->option_button->size.y));
    list_push_back(ptr->options, opt);
    return opt;
}

void context_menu_add_option_with_callback(context_menu *ptr, char *name, char disabled, CONTEXT_MENU_CB_FP, void *pass)
{
    context_menu_option *opt = context_menu_add_option(ptr, name, disabled);
    opt->cmenu_cb_fp = cmenu_cb_fp;
    opt->pass = pass;
}

int context_menu_get_clicked_option(context_menu *ptr)
{
    int ret_opt = -1;
    list_node *n;
    int i = 0;
    for(n = ptr->options->start; n != NULL; n = n->next)
    {
        context_menu_option *opt = (context_menu_option*) n->val;
        if(opt->option_button->clicked)
        {
            ret_opt = i;
            ptr->activated = 0;
            if(current_context_menu == ptr) current_context_menu = NULL;
            opt->option_button->clicked = 0;
            if(opt->cmenu_cb_fp != NULL) opt->cmenu_cb_fp(opt->pass);
            return ret_opt;
        }
        i++;
    }
    return ret_opt;
}

void context_menu_update(context_menu *ptr)
{
    if(ptr->activated)
    {
        ptr->hovered = mouse_x >= ptr->pos.x && mouse_y >= ptr->pos.y && mouse_x <= ptr->pos.x + ptr->size.x && mouse_y <= ptr->pos.y + ptr->size.y;
        list_node *n;
        for(n = ptr->options->start; n != NULL; n = n->next)
        {
            context_menu_option *opt = (context_menu_option*) n->val;
            button_update(opt->option_button);
        }
    }
}

void context_menu_render(context_menu *ptr)
{
    if(ptr->activated)
    {
		rg.gui_render_ptr = ptr;
        GL_OLBOX(FLOATRECT(ptr->pos.x, ptr->pos.y, ptr->size.x, ptr->size.y), col_white, col_black, 1);
        list_node *n;
        for(n = ptr->options->start; n != NULL; n = n->next)
        {
            context_menu_option *opt = n->val;
            button_render(opt->option_button);
        }
    }
}
