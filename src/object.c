#include "object.h"
#include "circle.h"
#include "approach_circle.h"
#include "slider.h"
#include "spinner.h"

#include "song.h"
#include "utils.h"
#include "rg.h"
#include "skin.h"
#include "audio_engine.h"
#include "explosion.h"
#include "mania_note.h"

object *object_init(OBJECT_INIT_ARGS)
{
    object *ret = malloc(sizeof *ret);
    ret->pos = V2F(x, y);
    ret->type = type;
    ret->relative_time_ms = 0;
    ret->start_time_ms = start_time_ms;
    ret->tl_drag_rel_time_ms = 0;
    ret->length_time_ms = length_time_ms;
    ret->start_judgement = judgement_unknown;
    ret->end_judgement = judgement_unknown;
    ret->hit_time_ms_rel = MISS_TIME;
    ret->hit_time_ms = start_time_ms;
    ret->new_combo = 0;
    ret->hovered = 0;
    ret->delete_me = 0;
    ret->dragging = 0;
    ret->do_grid_snap = 0;
    ret->is_new = 0;
	ret->wiggle_clock = timer_init();
	ret->should_wiggle = 0;
    ret->wiggle_xoff = 0;
    ret->host_song = host_song;
    ret->circle = NULL;
    ret->slider = NULL;
    ret->spinner = NULL;
    ret->mania_note = NULL;
    ret->number = 0;
    switch(ret->type)
    {
        case object_circle:
        {
            ret->circle = circle_init(ret);
            break;
        }
        case object_slider:
        {
			ret->circle = circle_init(ret);
            ret->slider = slider_init(ret);
            break;
        }
        case object_spinner:
        {
            ret->spinner = spinner_init(ret);
            break;
        }
        case object_mania_note:
        {
            ret->mania_note = mania_note_init(ret);
            break;
        }
    }
    return ret;
}

void object_free(object *ptr)
{
    switch(ptr->type)
    {
        case object_circle:
        {
            circle_free(ptr->circle);
            break;
        }
        case object_slider:
        {
        	circle_free(ptr->circle);
            slider_free(ptr->slider);
            break;
        }
        case object_spinner:
        {
            spinner_free(ptr->spinner);
            break;
        }
        case object_mania_note:
        {
            mania_note_free(ptr->mania_note);
            break;
        }
    }
    timer_free(ptr->wiggle_clock);
    free(ptr);
}

char object_compatable_with_current_gm(object *ptr)
{
    if(rg.mode == game_mode_ouendan && (ptr->type == object_circle || ptr->type == object_slider || ptr->type == object_spinner)) return 1;
    if(rg.mode == game_mode_mania && (ptr->type == object_mania_note)) return 1;
    return 0;
}

char object_should_tick(object *ptr)
{
    song *song = ptr->host_song;
    float current_time_ms = song->current_time_ms;
    float approach_time_ms = song->approach_time_ms;
    float lead_out_time_ms = rg.screen == screen_editor ? CIRCLE_EDITOR_FADE_OUT_MS : CIRCLE_FADE_OUT_TIME_MS;
    char can_tick = object_compatable_with_current_gm(ptr);
    if(!can_tick) ptr->dragging = 0;
    if(rg.mode == game_mode_ouendan)
    {
        if(ptr->type == object_slider)
        {
            slider *sl = ptr->slider;
            if
            (
                sl->tex == 0
                &&
                current_time_ms >= ptr->start_time_ms - approach_time_ms
                &&
                current_time_ms <= ptr->start_time_ms + ptr->length_time_ms + lead_out_time_ms
            )
            {
                if(sl->curve_points != NULL && sl->curve_points->count < 1) slider_calculate_path(sl, 1);
                slider_regenerate_texture(sl);
            }
            else if
            (
                sl->tex != 0
                &&
                (
                    current_time_ms < ptr->start_time_ms - approach_time_ms
                    ||
                    current_time_ms > ptr->start_time_ms + ptr->length_time_ms + lead_out_time_ms
                )
            )
                slider_delete_tex(sl);
        }
        if
        (
            current_time_ms >= ptr->start_time_ms - approach_time_ms
            &&
            current_time_ms <= (ptr->start_time_ms + ptr->length_time_ms) + lead_out_time_ms
        )
        {
            return can_tick;
        }
    }
    if(rg.mode == game_mode_mania)
    {
        if
        (
            current_time_ms >= ptr->start_time_ms - (approach_time_ms * 1.86)
            &&
            current_time_ms <= (ptr->start_time_ms + ptr->length_time_ms) + approach_time_ms
        ) 
            return can_tick;
    }
    return 0;
}

void object_snap_to_grid(object *ptr)
{
    switch(ptr->type)
    {
        case object_circle:
        {
            circle_snap_to_grid(ptr->circle);
            break;
        }
        case object_slider:
        {
            circle_snap_to_grid(ptr->circle);
            slider_snap_to_grid(ptr->slider, 1);
            break;
        }
    }
}

char object_check_hovered(object *ptr)
{
    switch(ptr->type)
    {
        case object_circle: return circle_check_hovered(ptr->circle);
        case object_slider:
        {
		    if(rg.screen == screen_editor) return slider_check_hovered(ptr->slider);
            else return circle_check_hovered(ptr->circle);
        }
    }
    return 0;
}

void object_update(object *ptr)
{
    song *song = ptr->host_song;
    TIME_VAR curtime = song->current_time_ms;
    TIME_VAR reltime = ptr->relative_time_ms;
    judgement result = ptr->start_judgement;

    if(rg.screen == screen_editor && rg.gui_render_end_of_frame == ptr->host_song)
    {
        if(ptr->hovered && mouse.left_click == 1) ptr->dragging = 1;
        if(ptr->dragging && mouse.left_click == 0)
        {
            ptr->dragging = 0;

            object_snap_to_grid(ptr);
        }
    }
    
    if(ptr->type != object_spinner && reltime >= MISS_TIME && result == judgement_unknown)
    {
        ptr->start_judgement = judgement_miss;
        ptr->hit_time_ms_rel = reltime;
        if(ptr->type == object_circle || ptr->type == object_mania_note) ptr->end_judgement = ptr->start_judgement;
        song_break_combo(song);
        if(rg.screen != screen_editor && ptr->type != object_slider) list_push_back(song->explosions, explosion_init_jdgmnt(ptr->pos, song, curtime, judgement_miss));
        song->passed_segs++;
    }

    float rrpx = 0.0f;
	if(ptr->should_wiggle)
	{
		int wiggle_time = timer_milliseconds(ptr->wiggle_clock);
		if(wiggle_time <= OBJECT_WIGGLE_TIME_MS)
		{
			float wiggle_offset = 0.0f;
			int s1 = OBJECT_WIGGLE_TIME_MS / 4;
			int s2 = s1 * 2;
			int s3 = s1 * 3;
			int room = OBJECT_WIGGLE_ROOM * ptr->host_song->object_size;
			if(wiggle_time < s1) wiggle_offset = scale_value_to(wiggle_time, 0, s1, 0, room);
			else if(wiggle_time < s2) wiggle_offset = scale_value_to(wiggle_time, s1, s2, room, 0);
			else if(wiggle_time < s3) wiggle_offset = scale_value_to(wiggle_time, s2, s3, 0, -room);
			else if(wiggle_time > s3) wiggle_offset = scale_value_to(wiggle_time, s3, OBJECT_WIGGLE_TIME_MS, -room, 0);
			
			rrpx = rrpx + (wiggle_offset);
		}
		else ptr->should_wiggle = 0;
	}
    ptr->wiggle_xoff = rrpx;
    
    switch(ptr->type)
    {
        case object_circle:
        {
            circle_update(ptr->circle);
            break;
        }
        case object_slider:
        {
			circle_update(ptr->circle);
            slider_update(ptr->slider);
            break;
        }
        case object_spinner:
        {
            spinner_update(ptr->spinner);
            break;
        }
        case object_mania_note:
        {
            mania_note_update(ptr->mania_note);
            break;
        }
    }
}

void object_render(object *ptr)
{
    switch(ptr->type)
    {
        case object_circle:
        {
            circle_render(ptr->circle);
            break;
        }
        case object_slider:
        {
            slider_render(ptr->slider);
            break;
        }
        case object_spinner:
        {
            spinner_render(ptr->spinner);
            break;
        }
        case object_mania_note:
        {
            mania_note_render(ptr->mania_note);
            break;
        }
    }
}

void object_click(object *ptr, TIME_VAR time_ms)
{
    song *song = ptr->host_song;
    ptr->hit_time_ms = time_ms;
    ptr->hit_time_ms_rel = ptr->relative_time_ms;
    ptr->start_judgement = object_judge(ptr);
    song->passed_segs++;
    char single = (ptr->type == object_circle || ptr->type == object_mania_note);
    char hold = ptr->type == object_slider;
    if(single) ptr->end_judgement = ptr->start_judgement;
    if(rg.screen == screen_player)
    {
        song_inc_combo(song);
        song->score += 13 + song->current_combo;
    }
    if(ptr->start_judgement != judgement_miss)
    {
        song->good_hits++;
        if(single)
        {
            ae_sound_effect_play(rg.skin->hit);
        }
        else if(hold)
        {
            ae_sound_effect_play(rg.skin->long_start);
        }
    }
    else song_break_combo(song);
}

void object_reset(object *ptr)
{
    ptr->should_wiggle = 0;
    ptr->wiggle_xoff = 0;
    if((rg.screen == screen_editor || rg.editor_testing_song) && ptr->start_time_ms + ptr->length_time_ms < ptr->host_song->current_time_ms)
    {
        ptr->start_judgement = judgement_perfect;
        ptr->end_judgement = judgement_perfect;
        ptr->hit_time_ms = ptr->start_time_ms;
        ptr->hit_time_ms_rel = 0;
        if(ptr->circle != NULL) { }
        if(ptr->slider != NULL)
        {
            ptr->slider->segments_passed = 1 + ptr->slider->reverses;
            ptr->slider->slider_failed = 0;
            ptr->slider->slider_hold_time = SLIDER_FOLLOW_OPEN_MS;
            ptr->slider->letgo = 0;
            ptr->slider->click_try = 1;
            ptr->slider->held_near_end = 1;
            ptr->slider->last_is_good = 0;
            ptr->slider->is_good = 0;
        }
        if(ptr->spinner != NULL) { }
    }
    else
    {
        ptr->start_judgement = judgement_unknown;
        ptr->end_judgement = judgement_unknown;
        ptr->hit_time_ms = ptr->start_time_ms;
        ptr->hit_time_ms_rel = MISS_TIME;
        if(ptr->circle != NULL) { }
        if(ptr->slider != NULL)
        {
            ptr->slider->segments_passed = 0;
            ptr->slider->slider_failed = 0;
            ptr->slider->slider_hold_time = 0;
            ptr->slider->letgo = 0;
            ptr->slider->click_try = 0;
            ptr->slider->held_near_end = 0;
            ptr->slider->last_is_good = 0;
            ptr->slider->is_good = 0;
        }
        if(ptr->spinner != NULL) { spinner_reset(ptr->spinner); }
    }
}

char *object_serialize(object *ptr)
{
    char *ret = NULL;

    if(ptr->type == object_slider)
    {
        char *ctrl_pts_ser = slider_control_points_serialize(ptr->slider);
        ret = dupfmt
        (
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%d"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%s"
            OBJECT_SERIALIZED_SEP
            "%d",
            ptr->pos.x,
            ptr->pos.y,
            ptr->type,
            ptr->start_time_ms,
            ptr->length_time_ms,
            ctrl_pts_ser,
            ptr->new_combo
        );
        free(ctrl_pts_ser);
    }
    else if(ptr->type == object_mania_note)
    {
        char *m_data_ser = mania_note_serialize(ptr->mania_note);
        ret = dupfmt
        (
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%d"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%s"
            OBJECT_SERIALIZED_SEP
            "%d",
            ptr->pos.x,
            ptr->pos.y,
            ptr->type,
            ptr->start_time_ms,
            ptr->length_time_ms,
            m_data_ser,
            ptr->new_combo
        );
        free(m_data_ser);
    }
    else
    {
        ret = dupfmt
        (
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%d"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%.2f"
            OBJECT_SERIALIZED_SEP
            "%d",
            ptr->pos.x,
            ptr->pos.y,
            ptr->type,
            ptr->start_time_ms,
            ptr->length_time_ms,
            ptr->new_combo
        );
    }
    
    return ret;
}

void object_overwrite(object *ptr, char *serialized_data)
{
    int i;
    int seg_start = 0;
    int found = 0;
    for(i = 0;; ++i)
    {
        char cc = serialized_data[i];
        char end = cc == '\0';
        if(cc == OBJECT_SERIALIZED_SEP_C || end)
        {
            if(!end) serialized_data[i] = '\0';
            char *r = &serialized_data[seg_start];

            switch(found)
            {
                case 0:
                {
                    ptr->pos.x = atof(r);
                    break;
                }
                case 1:
                {
                    ptr->pos.y = atof(r);
                    break;
                }
                case 2:
                {
                    ptr->type = atoi(r);

                    if(ptr->circle == NULL && ptr->type == object_circle)
                    {
                        ptr->circle = circle_init(ptr);
                    }
                    if(ptr->slider == NULL && ptr->type == object_slider)
                    {
                        ptr->circle = circle_init(ptr);
                        ptr->slider = slider_init(ptr);
                    }
                    if(ptr->spinner == NULL && ptr->type == object_spinner)
                    {
                        ptr->spinner = spinner_init(ptr);
                    }
                    if(ptr->mania_note == NULL && ptr->type == object_mania_note)
                    {
                        ptr->mania_note = mania_note_init(ptr);
                    }
                    break;
                }
                case 3:
                {
                    ptr->start_time_ms = atof(r);
                    break;
                }
                case 4:
                {
                    ptr->length_time_ms = atof(r);
                    break;
                }
                case 5:
                {
                    if(ptr->type == object_slider) slider_overwrite_control_points(ptr->slider, r);
                    else if(ptr->type == object_mania_note) mania_note_overwrite(ptr->mania_note, r);
                    else if(ptr->type == object_circle) ptr->new_combo = atoi(r);
                    break;
                }
                case 6:
                {
                    if(ptr->type != object_circle) ptr->new_combo = atoi(r);
                    break;
                }
                default: continue;
            }

            if(end) break;

            serialized_data[i] = OBJECT_SERIALIZED_SEP_C;
            found++;
            seg_start = i + 1;
        }
    }
}

object *object_from_serialized(char *serialized_data, struct song *host_song)
{
    object *ret = object_init(0, 0, -1, 0, 0, host_song);
    object_overwrite(ret, serialized_data);
    return ret;
}

judgement object_judge(object *ptr)
{
	judgement ret = judgement_miss;
    TIME_VAR miss_time = MISS_TIME;
    if(ptr->hit_time_ms_rel >= -miss_time && ptr->hit_time_ms_rel <= miss_time) ret = judgement_perfect;
    if(ptr->circle != NULL && rg.screen != screen_editor && ret == judgement_miss)
        list_push_back(ptr->host_song->explosions, explosion_init_jdgmnt(ptr->pos, ptr->host_song, ptr->host_song->current_time_ms, judgement_miss));
    return ret;
}

void object_wiggle(object *ptr)
{
    if(timer_milliseconds(ptr->wiggle_clock) < OBJECT_WIGGLE_TIME_MS) return;
	ptr->should_wiggle = 1;
	timer_restart(ptr->wiggle_clock);
}
