#pragma once

#include "object.h"

#define SPINNER_MAX_ACCUM 360

typedef struct spinner
{
    object *host_obj;
    float accumulative;
    float accum_rend;
    float accum_fullrot;
    float accum_abs;
    float rotation;
    float rotations;
    timer *deflate;
} spinner;

spinner *spinner_init(object *host_obj);
void spinner_free(spinner *ptr);
void spinner_reset(spinner *ptr);
void spinner_update(spinner *ptr);
void spinner_render(spinner *ptr);
