#pragma once
#include "vec.h"
#include "rect.h"
#include "utils.h"
#include "dir.h"

#define TEXT3_SPECIAL_CHAR '~'
#define TEXT3_MAX_LINES 1024

typedef enum text3_justification
{
	text3_justification_left,
	text3_justification_center,
	text3_justification_right
} text3_justification;

float_rect text3_draw(char *str, float x, float y, float width_lim, float opacity, origin ox, origin oy, text3_justification just);
#define text3_fmt(x, y, width_lim, opacity, ox, oy, just, fmt, ...) \
{ \
	char *fmtd = dupfmt(fmt, __VA_ARGS__); \
	text3_draw(fmtd, x, y, width_lim, opacity, ox, oy, just); \
	free(fmtd); \
}

#define text3_fmt_rect(ret_rect, x, y, width_lim, opacity, ox, oy, just, fmt, ...) \
{ \
	char *fmtd = dupfmt(fmt, __VA_ARGS__); \
	float_rect rect = text3_draw(fmtd, x, y, width_lim, opacity, ox, oy, just); \
	ret_rect = rect; \
	free(fmtd); \
}
