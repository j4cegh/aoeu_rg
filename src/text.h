#pragma once

#include "includes.h"

typedef struct text
{
	char *str;
	v2f pos;
	v2i size;
	char draw_bg;
	SDL_Surface *surf;
	GLuint fon_tex;
	color4 col, bg_col;
	char enable_smoothing;
	float vertical_offset;
} text;

text *text_init(char *str, float x, float y, color4 color);
void text_render(text *ptr);
void text_gen_surface(text *ptr);
void text_calc_bounds(text *ptr);
void text_set_position(text *ptr, float x, float y);
void text_set_string(text *ptr, char *new_str);
void text_set_color(text *ptr, color4 color);
void text_set_opacity(text *ptr, float val);
void text_free(text *ptr);
