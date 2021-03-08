#pragma once

#include "rg.h"

typedef struct timeline
{
    struct object *selected_obj;
    TIME_VAR sel_ms;
    TIME_VAR mouse_ms_oc;
    char selecting;
    char long_drag;
    int drag_revs;
    float long_drag_ms;
    char moved_object;
    char moved_object_once;
    float timeline_scale;
} timeline;

void timeline_init(timeline *tl);
void timeline_tick(timeline *tl, struct song *song);
