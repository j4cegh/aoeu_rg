#pragma once

#include "vec.h"
#include "dir.h"

typedef struct checkbox
{
	char *label;
	v2f pos;
	v2f size;
	char is_checked;
	char clicked;
	char hovered;
	char enabled;
} checkbox;

checkbox *checkbox_init(char *label, float x, float y, char is_checked);
void checkbox_free(checkbox *ptr);
void checkbox_set_pos(checkbox *ptr, float x, float y);
void checkbox_set_size(checkbox *ptr, float width, float height);
void checkbox_update(checkbox *ptr);
v2f checkbox_render(checkbox *ptr);