#pragma once

#include "includes.h"

#define USE_BASS

#ifdef USE_BASS
#include <bass.h>
#include <bass_fx.h>
#endif

typedef enum ae_action
{
    ae_pause,
    ae_play,
    ae_stop,
    ae_restart
} ae_action;

typedef struct ae_music_handle
{
#ifdef USE_BASS
    HSTREAM nofx_handle;
    HSTREAM stream_handle;
#endif
    float length_ms;
} ae_music_handle;

void audio_engine_init();
void audio_engine_free();
ae_music_handle *ae_music_handle_init();
void ae_music_handle_free(ae_music_handle *ptr);
char ae_music_load_file(ae_music_handle *ptr, char *file_loc);
char ae_music_handle_is_null(ae_music_handle *ptr);
char ae_music_handle_is_active(ae_music_handle *ptr);
float ae_music_handle_get_length_ms(ae_music_handle *ptr);
float ae_music_handle_get_time_ms(ae_music_handle *ptr);
void ae_music_handle_set_time_ms(ae_music_handle *ptr, float time_ms);
void ae_music_handle_set_volume(ae_music_handle *ptr, float volume);
void ae_music_handle_set_speed(ae_music_handle *ptr, float speed);
void ae_music_handle_do(ae_music_handle *ptr, ae_action action);

typedef struct ae_sound_effect
{
    char placeholder_variable;
#ifdef USE_BASS
    HSAMPLE sample;
    HCHANNEL channel;
#endif
} ae_sound_effect;

ae_sound_effect *ae_sound_effect_init(char *file_loc);
void ae_sound_effect_free(ae_sound_effect *ptr);
void ae_sound_effect_set_volume(ae_sound_effect *ptr, float volume);
void ae_sound_effect_play(ae_sound_effect *ptr);
void ae_sound_effect_play_no_restart(ae_sound_effect *ptr);
