#include "audio_engine.h"
#include "utils.h"
#include "logger.h"
#include "notification.h"

void audio_engine_init()
{
#ifdef USE_BASS
    BASS_Init(-1, 44100, 0, NULL, NULL);
#endif
}

void audio_engine_free()
{
#ifdef USE_BASS
    BASS_Free();
#endif
}

ae_music_handle *ae_music_handle_init()
{
    ae_music_handle *ret = malloc(sizeof *ret);
#ifdef USE_BASS
    ret->nofx_handle = 0;
    ret->stream_handle = 0;
#endif
    ret->length_ms = 0.0f;
    return ret;
}

void ae_music_handle_free(ae_music_handle *ptr)
{
#ifdef USE_BASS
	if(!ae_music_handle_is_null(ptr)) BASS_StreamFree(ptr->stream_handle);
#endif
    free(ptr);
}

char ae_music_load_file(ae_music_handle *ptr, char *file_loc)
{
#ifdef USE_BASS
	if(!file_exists(file_loc)) return 0;
	if(!ae_music_handle_is_null(ptr)) BASS_StreamFree(ptr->stream_handle);
	ptr->nofx_handle = BASS_StreamCreateFile(FALSE, file_loc, 0, 0, BASS_STREAM_DECODE);
	ptr->stream_handle = BASS_FX_TempoCreate(ptr->nofx_handle, BASS_FX_FREESOURCE);
	long len = BASS_ChannelGetLength(ptr->stream_handle, BASS_POS_BYTE);
	ptr->length_ms = BASS_ChannelBytes2Seconds(ptr->stream_handle, len) * 1000;
	return 1;
#else
    return 0;
#endif
}

char ae_music_handle_is_null(ae_music_handle *ptr)
{
#ifdef USE_BASS
    return ptr->stream_handle == 0;
#else
    return 1;
#endif
}

char ae_music_handle_is_active(ae_music_handle *ptr)
{
#ifdef USE_BASS
    if(!ae_music_handle_is_null(ptr)) return BASS_ChannelIsActive(ptr->stream_handle);
#endif
    return 1;
}

float ae_music_handle_get_length_ms(ae_music_handle *ptr)
{
    return ptr->length_ms;
}

float ae_music_handle_get_time_ms(ae_music_handle *ptr)
{
#ifdef USE_BASS
    return BASS_ChannelBytes2Seconds(ptr->stream_handle, BASS_ChannelGetPosition(ptr->stream_handle, BASS_POS_BYTE)) * 1000;
#endif
    return 0.0f;
}

void ae_music_handle_set_time_ms(ae_music_handle *ptr, float time_ms)
{
#ifdef USE_BASS
    if(!ae_music_handle_is_null(ptr)) BASS_ChannelSetPosition(ptr->stream_handle, BASS_ChannelSeconds2Bytes(ptr->stream_handle, time_ms / 1000), BASS_POS_BYTE);
#endif
}

void ae_music_handle_set_volume(ae_music_handle *ptr, float volume)
{
#ifdef USE_BASS
    if(!ae_music_handle_is_null(ptr)) BASS_ChannelSetAttribute(ptr->stream_handle, BASS_ATTRIB_VOL, scale_value_to(volume, 0, 100, 0, 1));
#endif
}

void ae_music_handle_set_speed(ae_music_handle *ptr, float speed)
{
#ifdef USE_BASS
	if(!ae_music_handle_is_null(ptr)) BASS_ChannelSetAttribute(ptr->stream_handle, BASS_ATTRIB_TEMPO, scale_value_to(speed, 1, 2, 0, 100));
#endif
}

void ae_music_handle_do(ae_music_handle *ptr, ae_action action)
{
#ifdef USE_BASS
    if(!ae_music_handle_is_null(ptr))
    {
        switch(action)
        {
            case ae_pause:
            {
                BASS_ChannelPause(ptr->stream_handle);
                break;
            }
            case ae_play:
            {
                BASS_ChannelPlay(ptr->stream_handle, FALSE);
                break;
            }
            case ae_stop:
            {
                BASS_ChannelStop(ptr->stream_handle);
                break;
            }
            case ae_restart:
            {
                BASS_ChannelPlay(ptr->stream_handle, TRUE);
                break;
            }
        }
    }
#endif
}

ae_sound_effect *ae_sound_effect_init(char *file_loc)
{
    ae_sound_effect *ret = malloc(sizeof *ret);
    if(!file_exists(file_loc))
    {
        char *missing_eff_warning = dupfmt("missing effect! filename: %s", file_loc);
		show_notif(missing_eff_warning);
		log_warning(missing_eff_warning);
		printf(missing_eff_warning);
		free(missing_eff_warning);
    }
#ifdef USE_BASS
    ret->sample = BASS_SampleLoad(FALSE, file_loc, 0, 0, 128, BASS_SAMPLE_MONO);
    ret->channel = BASS_SampleGetChannel(ret->sample, FALSE);
#endif
    return ret;
}

void ae_sound_effect_free(ae_sound_effect *ptr)
{
#ifdef USE_BASS
    if(ptr->sample != 0) BASS_SampleFree(ptr->sample);
#endif
    free(ptr);
}

void ae_sound_effect_set_volume(ae_sound_effect *ptr, float volume)
{
#ifdef USE_BASS
    BASS_ChannelSetAttribute(ptr->channel, BASS_ATTRIB_VOL, scale_value_to(volume, 0, 100, 0, 1));
#endif
}

void ae_sound_effect_play(ae_sound_effect *ptr)
{
#ifdef USE_BASS
    BASS_ChannelPlay(ptr->channel, TRUE);
#endif
}

void ae_sound_effect_play_no_restart(ae_sound_effect *ptr)
{
#ifdef USE_BASS
    BASS_ChannelPlay(ptr->channel, FALSE);
#endif
}