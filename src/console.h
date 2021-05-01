#pragma once

#include "includes.h"
#include "text.h"
#include "textures.h"
#include "list.h"

#define CONSOLE_CMD_PREFIX '.'
#define CONSOLE_CMD_PREFIX_STR "."
#define CHAR_LIMIT_HORIZONTAL 150
#define CONSOLE_MSG_LIMIT INFINITY
#define CONSOLE_EMPTY_BOX_MSG ""

#define CONSOLE_QUEUE_TRANSFER_INTERVAL 50

#define CONSOLE_DROP_ANIM_TIME_MS 300
#define CONSOLE_LEV_ANIM_TIME_MS 300
#define CONSOLE_MSG_SLIDE_ANIM_MS 400

#define CONS_UNKNOWN_CMD_ARGS struct console *from, list *arguments, char *text, char *after_arg0
#define CONS_UNKNOWN_CMD void (*unknown_command)(CONS_UNKNOWN_CMD_ARGS)

typedef struct console
{
	list *message_queue;
	list *message_history;
	list *commands;
	struct input_box *command_box;
	struct scroll_bar *bar;
	char activated;
	char not_toggled_yet;
	float vertical_drop;
	char is_animating;
	CONS_UNKNOWN_CMD;
	timer *anim_clock, *msg_queue_timer;
	char drag_scroll;
} console;

extern console *cons_ptr;

#define CONSOLE_COMMAND_ON_CALL_ARGS console *from, list *arguments, char *txt, char *after_arg0
#define CONSOLE_COMMAND_ON_CALL void (*on_call)(CONSOLE_COMMAND_ON_CALL_ARGS)

typedef struct console_command
{
	char *name;
	char *desc;
	CONSOLE_COMMAND_ON_CALL;
} console_command;


void console_command_register(console *ptr, char *name, char *desc, CONSOLE_COMMAND_ON_CALL);
void console_command_free(console_command *ptr);

typedef struct console_msg
{
	text *text;
	timer *anim_timer;
} console_msg;

console_msg *console_msg_init(char *msg, color4 col);
void console_msg_free(console_msg *ptr);

console *console_init();
void console_free(console *ptr);
void console_toggle(console *ptr);
void console_print(console *ptr, char *msg, color4 color);
void console_print_free(console *ptr, char *msg, color4 color);
void console_parse(console *ptr, char *text);
void console_tick(console *ptr);
void console_enter(console *ptr);
void console_reset_callbacks(console *ptr);
