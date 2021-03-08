#pragma once

#include "color.h"
#include "timer.h"
#include "text.h"

#define NOTIF_SHOW_TIME_MS 6000
#define NOTIF_RISE_MS 1000

typedef struct notification
{
	char *msg;
	color4 col;
	timer *timer;
	text *t;
	int height;
} notification;

extern list *notifications_list;

void show_notif_col(char *msg, color4 col);
void show_notif(char *msg);
void show_notif_free(char *msg);

#define show_notif_fmt(fmt, ...) \
{ \
	char *fmtd = dupfmt(fmt, __VA_ARGS__); \
	show_notif(fmtd); \
	free(fmtd); \
}

void notif_delete(notification *noti);

void render_notifs();
