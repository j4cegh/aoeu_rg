#include "spinner.h"
#include "rg.h"
#include "skin.h"
#include "song.h"
#include "textures.h"
#include "circle.h"

spinner *spinner_init(object *host_obj)
{
    spinner *ret = malloc(sizeof *ret);
    ret->host_obj = host_obj;
    spinner_reset(ret);
    ret->deflate = timer_init();
    return ret;
}

void spinner_free(spinner *ptr)
{
    timer_free(ptr->deflate);
    free(ptr);
}

void spinner_reset(spinner *ptr)
{
    ptr->accumulative = 0;
    ptr->accum_rend = 0;
    ptr->accum_fullrot = 0;
    ptr->accum_abs = 0;
    ptr->rotation = 0;
    ptr->rotations = 0;
}

float spinner_get_mouse_rotation(spinner *ptr)
{
    return rotate_towards(ptr->host_obj->real_pos, V2F(rg.mop_x, rg.mop_y));
}

void spinner_update(spinner *ptr)
{
    object *obj = ptr->host_obj;
    song *song = obj->host_song;
    if(rg.screen == screen_player && song->music_state == song_playing)
    {
        TIME_VAR reltime = obj->relative_time_ms;
        char during = (reltime >= 0 && reltime <= obj->length_time_ms);
        if((rg.ouendan_click_1 || rg.ouendan_click_2) && during)
        {
            if(obj->start_judgement == judgement_unknown)
            {
                obj->start_judgement = judgement_perfect;
                obj->hit_time_ms = song->current_time_ms;
                obj->hit_time_ms_rel = obj->relative_time_ms;
            }
            float mrot = spinner_get_mouse_rotation(ptr);
            if((rg.ouendan_click_1 == 1 && rg.ouendan_click_2 == 0) || (rg.ouendan_click_2 == 1 && rg.ouendan_click_1 == 0)) ptr->rotation = mrot;
            float delta = (mrot - ptr->rotation);
            if(fabsf(delta) <= 360 / 20)
            {
                ptr->accumulative += delta * song->speed_multiplier;
                ptr->accum_rend += delta;
                ptr->accum_fullrot += delta;
            }
            if(fabsf(ptr->accum_fullrot) >= 360.0f)
            {
                ptr->rotations++;
                /* show_notif("full rot"); */
                ae_sound_effect_play(rg.skin->long_middle);
                ptr->accum_fullrot = 0.0f;
                song->score += 100 * ptr->rotations;
            }
            ptr->rotation = mrot;
            if(delta >= 10) timer_restart(ptr->deflate);
        }
        if(reltime > obj->length_time_ms && obj->end_judgement == judgement_unknown)
        {
            float lm = obj->length_time_ms / 500;
            if(ptr->rotations >= lm && obj->start_judgement == judgement_perfect)
            {
                obj->end_judgement = judgement_perfect;
                song->score += 312 * ptr->rotations;
                ae_sound_effect_play(rg.skin->long_end);
            }
            else
            {
                obj->end_judgement = judgement_miss;
                song_break_combo(song);
            }
        }
        ptr->accum_abs = clamp(fabsf(ptr->accumulative), 0, SPINNER_MAX_ACCUM);
        /* cf(ptr->accum_abs, "accum"); */
        if(timer_milliseconds(ptr->deflate) > 5 && during)
        {
            ptr->accumulative *= 0.96f;
            timer_restart(ptr->deflate);
        }
    }
}

void spinner_render(spinner *ptr)
{
    object *obj = ptr->host_obj;
    song *song = obj->host_song;
    v2f mpos = song_map_coord_to_play_field(song, obj->real_pos);
    float sc = song->object_size;
	TIME_VAR reltime = obj->relative_time_ms;
	TIME_VAR objlen = obj->length_time_ms;
	TIME_VAR apptime = song->approach_time_ms;
	color4 col = col_white;
	if(reltime < 0) col.a = clamp(scale_value_to(reltime, -apptime, -apptime + CIRCLE_FADE_IN_TIME_MS, 0, 255), 0, 255);
	else if(reltime <= objlen) col.a = 255;
	else col.a = clamp(scale_value_to(reltime, objlen, objlen + (rg.screen == screen_editor ? CIRCLE_EDITOR_FADE_OUT_MS : CIRCLE_FADE_OUT_TIME_MS), 255, 0), 0, 255);
    GL_ROT(mpos.x, mpos.y, ptr->accum_rend);
    textures_drawsc(rg.skin->m_up_inner, 0, 0, sc, sc, col);
    GL_ROT_END();
}
