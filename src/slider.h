#pragma once

#include "object.h"
#include "rg.h"
#include "list.h"

#define SLIDER_DATA_SEP "/"
#define SLIDER_DATA_SEP_C '/'
#define SLIDER_DATA_SEP_COUNT 3
#define SLIDER_CONTROL_POINT_COORD_SEP ":"
#define SLIDER_CONTROL_POINT_COORD_SEP_C ':'
#define SLIDER_CONTROL_POINT_DATA_SEP ";"
#define SLIDER_CONTROL_POINT_DATA_SEP_C ';'
#define SLIDER_POTATO_DIV 4
#define SLIDER_MAX_OPACITY 230
#define SLIDER_FOLLOW_OPEN_MS 200
#define SLIDER_FOLLOW_CLOSE_MS 300
#define SLIDER_LETGO_ANIM_TIME 75
#define SLIDER_HELD_NEAR_END_MS 110
#define SLIDER_FOLLOW_BEAT_TIME_MS 100
#define SLIDER_CONTROL_POINT_SIZE 8
#define SLIDER_CONTROL_POINT_SIZE_NO_TEX 32

typedef enum slider_curve_type
{
	slider_curve_linear,
	slider_curve_catmull,
	slider_curve_bezier,
	slider_curve_circle
} slider_curve_type;

typedef struct slider_control_point
{
	v2f pos;
} slider_control_point;

slider_control_point *slider_control_point_init(float x, float y);
void slider_control_point_free(slider_control_point *ptr);

typedef struct slider
{
    object *host_obj;

	slider_curve_type curve_type;

    list *control_points;
    list *curve_points;

 	float_rect bounds;

	GLuint tex;
	GLuint fbuf;
	v2f tex_size;

	v2f cur_point;
	v2f next_point;
	v2f cur_point_interpolated;

	v2f slider_start_pos_op;
	v2f slider_start_pos;
	v2f slider_end_pos_op;
	v2f slider_end_pos;

	int reverses;
	int segments_passed;
	int segment_count;
	int segment_index;
	TIME_VAR segment_len_ms;
	
	char rev_on_start;
	char slider_failed;
	char letgo;
	char click_try;
	char held_near_end;
	
	char did_rev_expl;
	char potato_state_on_regen;

	slider_control_point *selected_control_point;

	timer *slider_hold_time_timer;
	timer *not_good_time_timer;
	timer *letgo_clock;
	timer *follow_beat_timer;
	TIME_VAR slider_hold_time;
	TIME_VAR not_good_time;
	TIME_VAR follow_beat_time;
	char follow_do_beat;
	float slider_hold_scale_at_letgo;
	char slider_hold_scale_at_letgo_set;
	char last_is_good;
	char is_good;
} slider;

slider *slider_init(object *host_obj);
void slider_delete_tex(slider *ptr);
void slider_delete_control_points(slider *ptr);
void slider_free(slider *ptr);
char slider_check_hovered(slider *ptr);
void slider_tick_scoring(slider *ptr);
void slider_update_cur_point(slider *ptr);
void slider_update(slider *ptr);
void slider_render(slider *ptr);
void slider_snap_to_grid(slider *ptr, char regen_path_tex);
void slider_add_control_point(slider *ptr, v2f point);
void slider_calculate_path(slider *ptr, char dont_regen_texture);
void slider_calc_bounds(slider *ptr);
void slider_regenerate_texture(slider *ptr);
char *slider_control_points_serialize(slider *ptr);
void slider_overwrite_control_points(slider *ptr, char *serialized_data);
