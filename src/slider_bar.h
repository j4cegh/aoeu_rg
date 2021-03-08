#pragma once

#include "vec.h"
#include "text.h"

#define SLIDER_BAR_HEIGHT 16

typedef struct slider_bar
{
    text *label;
    v2f pos;
    v2f size;
    v2f bar_size;
    float x_on_bar;
    char hovered;
    char hovered_oc;
    float val;
} slider_bar;

slider_bar *slider_bar_init(char *label, float x, float y, float width, float initial_val);
void slider_bar_update(slider_bar *ptr);
void slider_bar_render(slider_bar *ptr);
void slider_bar_set_val(slider_bar *ptr, float val);
void slider_bar_set_pos(slider_bar *ptr, float x, float y);
void slider_bar_free(slider_bar *ptr);