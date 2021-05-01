#pragma once

#include "includes.h"
#include "rg.h"

#define MISS_TIME 90
#define OBJECT_SERIALIZED_SEP "_"
#define OBJECT_SERIALIZED_SEP_C '_'
#define OBJECT_WIGGLE_TIME_MS 120
#define OBJECT_WIGGLE_ROOM 10

typedef enum object_type
{
    object_circle,
    object_slider,
    object_spinner,
    object_mania_note,
    object_count
} object_type;

typedef enum judgement
{
    judgement_unknown,
    judgement_miss,
    judgement_perfect
} judgement;

typedef struct object
{
    v2f real_pos;
	v2f render_pos;
    object_type type;
    TIME_VAR relative_time_ms;
    TIME_VAR start_time_ms;
    TIME_VAR tl_drag_rel_time_ms;
    TIME_VAR length_time_ms;
    judgement start_judgement;
    judgement end_judgement;
    TIME_VAR hit_time_ms_rel;
    TIME_VAR hit_time_ms;
    char new_combo;
    char hovered;
    char delete_me;
    char dragging;
	char do_grid_snap;
    char is_new;
	timer *wiggle_clock;
	char should_wiggle;
    float wiggle_xoff;
    struct song *host_song;
    struct circle *circle;
    struct slider *slider;
    struct spinner *spinner;
    struct mania_note *mania_note;
    int number;
} object;

#define OBJECT_INIT_ARGS float x, float y, object_type type, TIME_VAR start_time_ms, TIME_VAR length_time_ms, struct song *host_song
object *object_init(OBJECT_INIT_ARGS);
void object_free(object *ptr);
char object_compatable_with_current_gm(object *ptr);
char object_should_tick(object *ptr);
void object_snap_to_grid(object *ptr);
char object_check_hovered(object *ptr);
void object_update(object *ptr);
void object_render(object *ptr);
void object_click(object *ptr, TIME_VAR time_ms);
void object_reset(object *ptr);
char *object_serialize(object *ptr);
void object_overwrite(object *ptr, char *serialized_data);
object *object_from_serialized(char *serialized_data, struct song *host_song);
judgement object_judge(object *ptr);
void object_wiggle(object *ptr);
