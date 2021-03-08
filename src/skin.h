#pragma once

#include "includes.h"
#include "textures.h"
#include "audio_engine.h"

#define SKIN_SKINS_DIR "./skins/"

#define SKIN_BG_FILE "bg.png"

#define SKIN_CURSOR_FILE "cursor.png"
#define SKIN_CIRCLE_FILE "circle.png"
#define SKIN_APPROACH_CIRCLE_FILE "approach.png"
#define SKIN_SLIDER_BALL_FILE "ball.png"
#define SKIN_SLIDER_FOLLOW_CIRCLE_FILE "follow.png"
#define SKIN_MISS_FILE "miss.png"
#define SKIN_MISS_SOUND_FILE "miss.wav"
#define SKIN_REVERSEARROW_FILE "reverse.png"

#define SKIN_HIT_SOUND_FILE "hit.wav"
#define SKIN_LONG_START_SOUND_FILE "long_start.wav"
#define SKIN_LONG_MIDDLE_SOUND_FILE "long_middle.wav"
#define SKIN_LONG_END_SOUND_FILE "long_end.wav"
#define SKIN_METRONOME_SOUND_FILE "metronome.wav"

#define SKIN_MANIA_UP_INNER_FILE "m_up_inner.png"
#define SKIN_MANIA_UP_OUTLINE_FILE "m_up_outline.png"
#define SKIN_MANIA_DOWN_INNER_FILE "m_down_inner.png"
#define SKIN_MANIA_DOWN_OUTLINE_FILE "m_down_outline.png"
#define SKIN_MANIA_LEFT_INNER_FILE "m_left_inner.png"
#define SKIN_MANIA_LEFT_OUTLINE_FILE "m_left_outline.png"
#define SKIN_MANIA_RIGHT_INNER_FILE "m_right_inner.png"
#define SKIN_MANIA_RIGHT_OUTLINE_FILE "m_right_outline.png"

#define SKIN_SONGSEL_ENTRY_FILE "songsel_entry.png"

#define SKIN_NUM_0_FILE "0.png"
#define SKIN_NUM_1_FILE "1.png"
#define SKIN_NUM_2_FILE "2.png"
#define SKIN_NUM_3_FILE "3.png"
#define SKIN_NUM_4_FILE "4.png"
#define SKIN_NUM_5_FILE "5.png"
#define SKIN_NUM_6_FILE "6.png"
#define SKIN_NUM_7_FILE "7.png"
#define SKIN_NUM_8_FILE "8.png"
#define SKIN_NUM_9_FILE "9.png"

#define SKIN_HIGH_RES_SC_DIV 5

typedef struct skin
{
    char *name;
    loaded_texture *n0, *n1, *n2, *n3, *n4, *n5, *n6, *n7, *n8, *n9;
    loaded_texture *bg;
    loaded_texture *title;
    loaded_texture *cursor, *circle, *approachcircle, *sliderball, *followcircle, *miss, *reversearrow;
    loaded_texture *m_up_inner, *m_up_outline, *m_down_inner, *m_down_outline, *m_left_inner, *m_left_outline, *m_right_inner, *m_right_outline;
    loaded_texture *songsel_entry;
    ae_sound_effect *hit, *miss_eff, *long_start, *long_middle, *long_end, *metronome;
} skin;

skin *skin_init(char *name);
void skin_set_effect_vol(skin *ptr, float vol);
loaded_texture *skin_get_num(skin *ptr, char c);
v2f skin_get_num_size(skin *ptr, char c);
void skin_free(skin *ptr);
