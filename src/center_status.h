#pragma once

#include "text.h"
#include "timer.h"

#define CENTER_STATUS_SHOW_TIME_MS 5000

extern text *center_status_text;
extern timer *center_status_timer;

void show_center_status(char *message);
void center_status_free(char *message);
#define show_center_status_fmt(fmt, ...) \
{ \
	char *fmtd = dupfmt(fmt, __VA_ARGS__); \
	show_center_status(fmtd); \
	free(fmtd); \
}
extern char center_status_first_msg;

void render_center_status();
