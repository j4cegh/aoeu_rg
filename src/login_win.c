#include "login_win.h"
#include "rg.h"
#include "online_client.h"
#include "online_globals.h"
#include "config.h"
#include "popup.h"

login_win_vars lw_vars;

void login_win_init(struct gui_win *ptr)
{
    lw_vars.username_ibox = input_box_init(0, 0, 0, config_get(CONFIG_NAME_USERNAME), "username");
    lw_vars.password_ibox = input_box_init(0, 0, 0, "", "password");
    lw_vars.password_ibox->is_password = 1;
    lw_vars.username_ibox->next = lw_vars.password_ibox;
    lw_vars.username_ibox->before = lw_vars.password_ibox;
    lw_vars.password_ibox->next = lw_vars.username_ibox;
    lw_vars.password_ibox->before = lw_vars.username_ibox;

    lw_vars.login_btn = button_init(0, 0, "login!", 1, 0, NULL);
    lw_vars.register_btn = button_init(0, 0, "register!", 1, 0, NULL);
    lw_vars.offline_btn = button_init(0, 0, "play offline.", 1, 0, NULL);

   current_input_box = lw_vars.username_ibox;
}

void login_win_free(struct gui_win *ptr)
{
    button_free(lw_vars.offline_btn);
    button_free(lw_vars.register_btn);
    button_free(lw_vars.login_btn);
    input_box_free(lw_vars.password_ibox);
    input_box_free(lw_vars.username_ibox);
}

void login_win_tick(struct gui_win *ptr)
{
    if(current_input_box == NULL) current_input_box = lw_vars.username_ibox;
    
    rg.gui_render_ptr = ptr;

    if(rg.client->authed) change_screen(screen_tog_login_win);

    if(rg.resized) gui_win_center(ptr);

    float pad = 15;
    float x = ptr->pos.x + pad;
    float y = ptr->line_pos.y + pad;
    float w = ptr->size.x;
    float h = ptr->size.y - ptr->line_size.y;
    float ww = w - (pad * 2);

    input_box_update_vars(lw_vars.username_ibox, x, y, ww);
    input_box_update(lw_vars.username_ibox);

    y += lw_vars.username_ibox->size.y + pad;

    input_box_update_vars(lw_vars.password_ibox, x, y, ww);
    input_box_update(lw_vars.password_ibox);

    y += lw_vars.password_ibox->size.y + pad;
    y += lw_vars.password_ibox->size.y + pad;

    button_set_pos(lw_vars.login_btn, x, y);
    lw_vars.login_btn->enabled = lw_vars.username_ibox->text[0] != '\0' && lw_vars.password_ibox->text[0] != '\0' && is_str_legal(lw_vars.username_ibox->text, ONLINE_SERVER_ALLOWED_NAME_CHARS);
    button_update(lw_vars.login_btn);
    if((lw_vars.login_btn->clicked || rg.kp[SDL_SCANCODE_RETURN] == 1) && lw_vars.login_btn->enabled && rg.global_popup->is_opened == 0)
    {
		char *hash = sha256(lw_vars.password_ibox->text);
        online_client_set_name_and_pass_hash(rg.client, lw_vars.username_ibox->text, hash);
		free(hash);
#ifdef RELEASE
		input_box_clear_text(lw_vars.password_ibox);
#endif
		rg.client->after_conn_act = online_client_after_connect_action_login;
        online_client_connect(rg.client);
    }

    y += lw_vars.login_btn->size.y + pad;

    button_set_pos(lw_vars.register_btn, x, y);
    lw_vars.register_btn->enabled = lw_vars.login_btn->enabled;
    button_update(lw_vars.register_btn);
    if(lw_vars.register_btn->clicked)
    {
		char *hash = sha256(lw_vars.password_ibox->text);
        online_client_set_name_and_pass_hash(rg.client, lw_vars.username_ibox->text, hash);
		free(hash);
		rg.client->after_conn_act = online_client_after_connect_action_register;
		online_client_connect(rg.client);
    }

    y += lw_vars.register_btn->size.y + pad;

    button_set_pos(lw_vars.offline_btn, x, y);
    button_update(lw_vars.offline_btn);
    if(lw_vars.offline_btn->clicked) change_screen(screen_tog_login_win);

    y += lw_vars.offline_btn->size.y + pad;

    float height = (y - ptr->line_pos.y) + (pad * 1.5);

    ptr->size.x = w;
    ptr->size.y = height;

    input_box_render(lw_vars.username_ibox);
    input_box_render(lw_vars.password_ibox);
    button_render(lw_vars.login_btn);
    button_render(lw_vars.register_btn);
    button_render(lw_vars.offline_btn);

    context_menu_render(lw_vars.username_ibox->menu);
    context_menu_render(lw_vars.password_ibox->menu);
}

void bef_rend_func(struct gui_win *ptr)
{
    float_rect scr = FLOATRECT(0, 0, rg.win_width, rg.win_height);
    GL_RECT2BOX(scr, color4_mod_alpha(col_black, 200));
}
