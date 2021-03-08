#pragma once

#include "includes.h"
#include "input_box.h"
#include "button.h"
#include "rg.h"

#define TIMING_SECTION_HEIGHT 110
#define TIMING_SECTION_SERIALIZED_SEP "?"

typedef struct timing_section
{
	struct song *host_song;
	TIME_VAR beat_length_ms;
	TIME_VAR start_ms;
	TIME_VAR slider_time_pixel_ratio;
	TIME_VAR beat_divisor;
	input_box *bpmi;
	input_box *offseti;
	input_box *slider_pix_ratioi;
	input_box *beat_divi;
	button *deleteb, *addaboveb, *addbelowb, *jumpb;
} timing_section;

typedef enum timing_section_tick_action
{
	timing_section_ta_nothing, timing_section_ta_add_above, timing_section_ta_add_below, timing_section_ta_delete_me
} timing_section_tick_action;

timing_section *timing_section_init(struct song *host_song, TIME_VAR beat_length_ms, TIME_VAR start_ms, TIME_VAR slider_time_pixel_ratio, TIME_VAR beat_divisor);
timing_section *timing_section_deserialize(struct song *host_song, char *serialized);
void timing_section_free(timing_section *ptr);
timing_section_tick_action timing_section_tick(timing_section *ptr, float x, float y);
char *timing_section_serialize(timing_section *ptr);
