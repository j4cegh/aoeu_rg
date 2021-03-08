#include "health_bar.h"
#include "utils.h"

#include "song.h"
#include "popup.h"

void health_bar_reset(health_bar *ptr, struct song *song)
{
    ptr->val = HEALTH_BAR_VAL_MAX;
    ptr->rendval = HEALTH_BAR_VAL_MIN;
    ptr->song = song;
}

void health_bar_add(health_bar *ptr, float amount)
{
    ptr->val += amount;
    health_bar_clamp(ptr);
}

void health_bar_sub(health_bar *ptr, float amount)
{
    ptr->val -= amount;
    health_bar_clamp(ptr);
}

void health_bar_clamp(health_bar *ptr)
{
    clampr(&(ptr->val), HEALTH_BAR_VAL_MIN, HEALTH_BAR_VAL_MAX);
}

void health_bar_update(health_bar *ptr)
{
    song *song = ptr->song;
    
    if(song->game_mode_objects->count > 0)
    {
        object *first = song->game_mode_objects->start->val;
        if(song->current_time_ms < first->start_time_ms)
        {
            float nv = scale_value_to(song->current_time_ms, -SONG_BEFORE_START_TIME_MS, first->start_time_ms, HEALTH_BAR_VAL_MIN, HEALTH_BAR_VAL_MAX);
            ptr->val = nv;
            ptr->rendval = ptr->val;
        }
    }

    float diff = 0.1f * rg.frame_delta_ms;
    if(ptr->val > ptr->rendval)
    {
        ptr->rendval += diff;
        if(ptr->rendval > ptr->val) ptr->rendval = ptr->val;
    }
    else if(ptr->val < ptr->rendval)
    {
        ptr->rendval -= diff;
        if(ptr->rendval < ptr->val) ptr->rendval = ptr->val;
    }

    if(ptr->val <= 0 && !(rg.editor_testing_song))
    {
        if(rg.to_screen != screen_final_score && !rg.enable_slider_debug)
        {
            song->failed = 1;
            change_screen(screen_final_score);
            popup_open(rg.global_popup, popup_type_dismiss, "you failed! better luck next time.");
        }
    }
}

void health_bar_render(health_bar *ptr)
{
	float width = 300;
	float height = 10;
	float x = height;
	float y = height;
	float_rect behind_rect = FLOATRECT(x, y, width, height);
    float_rect bar_rect = FLOATRECT(x, y, scale_value_to(ptr->rendval, HEALTH_BAR_VAL_MIN, HEALTH_BAR_VAL_MAX, 0, width), height);
	GL_RECT2BOX(behind_rect, col_black);
    GL_RECT2BOX(bar_rect, col_green);
}
