#pragma once

#include "gui_win.h"
#include "input_box.h"
#include "button.h"

typedef struct login_win_vars
{
    input_box *username_ibox, *password_ibox;
    button *login_btn, *register_btn, *offline_btn;
} login_win_vars;

extern login_win_vars lw_vars;

void login_win_init(struct gui_win *ptr);
void login_win_free(struct gui_win *ptr);
void login_win_tick(struct gui_win *ptr);
void bef_rend_func(struct gui_win *ptr);