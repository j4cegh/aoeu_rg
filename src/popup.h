#pragma once

#include "includes.h"
#include "button.h"
#include "text.h"

#define POPUP_ANIM_LEN_MS 150
#define POPUP_DIM_ALPHA 200

typedef enum popup_type
{
	popup_type_dismiss,
	popup_type_yes_no,
	popup_type_input
} popup_type;

typedef struct popup
{
	popup_type type;
	text *msg_text;
	button *dismiss_btn;
	button *yes_btn;
	button *no_btn;
	struct input_box *ibox;
	v2f pos;
	v2f size;
	v2f opened_size;
	char is_opened;
	char not_used;
	timer *anim_timer;
	void (*yes_no_callback)(void *ptr, char yes);
	void (*txtbox_callback)(void *ptr, char *text);
	void *ptr;
} popup;

popup *popup_init();
void popup_free(popup *ptr);
void popup_update_size(popup *ptr);
void popup_update(popup *ptr);
void popup_render(popup *ptr);
void popup_setmsg(popup *ptr, char *msg);
void popup_close(popup *ptr);
void popup_open(popup *ptr, popup_type type, char *msg);
