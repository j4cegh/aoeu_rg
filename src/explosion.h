#pragma once

#include "includes.h"
#include "song.h"
#include "object.h"

typedef enum explosion_type
{
	explosion_circle, explosion_reverse, explosion_judgement
} explosion_type;

typedef struct explosion
{
	v2f pos;
	song *song;
	TIME_VAR on_ms;
	explosion_type type;
	float rot;
	judgement judge;
} explosion;

explosion *explosion_init_circ(v2f pos, song *song, TIME_VAR on_ms);
explosion *explosion_init_rev(v2f pos, song *song, TIME_VAR on_ms, float rot);
explosion *explosion_init_jdgmnt(v2f pos, song *song, TIME_VAR on_ms, judgement judge);
void explosion_tick(explosion *ptr);
