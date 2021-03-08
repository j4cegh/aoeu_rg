#pragma once

#include "includes.h"
#include "text.h"

#define BUTTON_FLASH_COUNT 1
#define BUTTON_FLASH_TIME_MS 1

typedef struct button
{
	v2f pos;
	v2f size;
	text *button_text, *label_text;
	char enabled;
	struct context_menu *host_cmenu;
	struct context_menu *on_click_cmenu;
	char hovered;
	int hover_frames;
	char clicked;
	char is_in_popup;
	char inside_popup;
	int flashes;
	char flashing;
	char flash_state;
	timer *flash_clock;
	char enable_in_console;
	char blocked;
	color4 col;
	color4 bg_col;
	color4 outline_col;
	char held_for_long;
	int held_time_min;
} button;

button *button_init(float x, float y, char *text, int enabled, char inside_cmenu, struct context_menu *host_cmenu);
void button_free(button *ptr);
void button_set_to_oc_cmenu(button *ptr);
void button_update(button *ptr);
void button_render(button *ptr);
void button_enable(button *ptr);
void button_disable(button *ptr);
void button_set_pos(button *ptr, float x, float y);
void button_set_text(button *ptr, char *btn_text);
void button_set_size(button *ptr, float width, float height);
void button_click(button *ptr);
