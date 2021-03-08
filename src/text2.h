#pragma once

#include "includes.h"

typedef struct text2
{
    char *str;
	v2f pos;
	v2i size;
	char draw_bg;
	list *surfs;
	GLuint fon_tex;
	color4 col, bg_col;
	char enable_smoothing;
} text2;

text2 *text2_init(char *str, float x, float y, color4 color);
void text2_render(text2 *ptr);
void text2_gen_surfaces(text2 *ptr);
void text2_set_position(text2 *ptr, float x, float y);
void text2_set_string(text2 *ptr, char *new_str);
void text2_set_color(text2 *ptr, color4 color);
void text2_set_opacity(text2 *ptr, float val);
void text2_add_surf(text2 *ptr, char *text, color4 col);
void text2_clear_surfs(text2 *ptr, char free_list);
void text2_free(text2 *ptr);
