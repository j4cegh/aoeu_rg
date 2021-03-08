#pragma once

#include "object.h"
#include "text.h"

#define CIRCLE_RADIUS 60.0f
#define CIRCLE_FADE_IN_TIME_MS 500
#define CIRCLE_FADE_OUT_TIME_MS 300
#define CIRCLE_EDITOR_FADE_OUT_MS 1000
#define CIRCLE_EXPLOSION_SCALE 2.0f

typedef struct circle
{
    object *host_obj;
    int number;
    int num_digits;
    char *num_str;
    float num_width;
} circle;

circle *circle_init(object *host_obj);
void circle_free(circle *ptr);
char circle_check_hovered(circle *ptr);
void circle_set_number(circle *ptr, int number);
void circle_update(circle *ptr);
void circle_snap_to_grid(circle *ptr);
void circle_render(circle *ptr);
void circle_render_number(circle *ptr, float x, float y, float radius, float sc, color4 col);
