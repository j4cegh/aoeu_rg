#pragma once

#include "includes.h"
#include "list.h"
#include "button.h"

#define CONTEXT_MENU_CB_FP void (*cmenu_cb_fp)(void*)

typedef struct context_menu_option
{
    char *name;
    char disabled;
    button *option_button;
    CONTEXT_MENU_CB_FP;
    void *pass;
} context_menu_option;

context_menu_option *context_menu_option_init(char *name, char disabled, struct context_menu *host);
void context_menu_option_free(context_menu_option *ptr);

typedef struct context_menu
{
    list *options;
    v2f pos;
    v2f size;
    char activated;
    char hovered;
    char button_click_avoided;
	char horizontal;
} context_menu;

extern context_menu *current_context_menu;

context_menu *context_menu_init(char horizontal);
void context_menu_free(context_menu *ptr);
void context_menu_activate(context_menu *ptr, float x, float y);
context_menu_option *context_menu_add_option(context_menu *ptr, char *name, char disabled);
void context_menu_add_option_with_callback(context_menu *ptr, char *name, char disabled, CONTEXT_MENU_CB_FP, void *pass);
int context_menu_get_clicked_option(context_menu *ptr);
void context_menu_update(context_menu *ptr);
void context_menu_render(context_menu *ptr);
