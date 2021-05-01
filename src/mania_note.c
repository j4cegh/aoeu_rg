#include "includes.h"
#include "mania_note.h"
#include "song.h"
#include "textures.h"
#include "skin.h"
#include "utils.h"

mania_note *mania_note_init(object *host_obj)
{
    mania_note *ret = malloc(sizeof *ret);
    ret->host_obj = host_obj;
    ret->lane = 0;
    
    ret->host_obj->real_pos = V2F(SONG_PFW / 2, SONG_PFH / 2);
    return ret;
}

void mania_note_free(mania_note *ptr)
{
    free(ptr);
}

void mania_note_update(mania_note *ptr)
{

}

void mania_note_render(mania_note *ptr)
{
    object *obj = ptr->host_obj;
    if((obj->start_judgement != judgement_unknown || obj->end_judgement != judgement_unknown)
    && !(obj->start_judgement == judgement_miss)) return;
    song *song = obj->host_song;
    TIME_VAR reltime = obj->relative_time_ms;
    float ypos = song_mania_map_time_to_y(song, reltime);
    float pfw_fourth = SONG_PFW / 4;
	float pfw_eighth = SONG_PFW / 8;
    float sc = song->object_size;

    int pos = ptr->lane;

    float xpos = pfw_eighth + (
        pos == 0 ? 0 :
        pos == 1 ? (pfw_fourth) :
        pos == 2 ? (pfw_fourth * 2) :
        pos == 3 ? (pfw_fourth * 3) :
        0
    );

    v2f npos = song_map_coord_to_play_field(song, V2F(xpos, 0));
    npos.x += obj->wiggle_xoff;
    npos.y = ypos;

    textures_drawsc(mania_note_get_tex(ptr), npos.x, npos.y, sc, sc, col_white);
}

int *mania_note_get_key(mania_note *ptr)
{
    switch(ptr->lane)
    {
        case 0: return &rg.mania_click_1;
        case 1: return &rg.mania_click_2;
        case 2: return &rg.mania_click_3;
        case 3: return &rg.mania_click_4;
    }
    return NULL;
}

loaded_texture *mania_note_get_tex(mania_note *ptr)
{
    skin *sk = rg.skin;
    int pos = ptr->lane;
    return (
        pos == 0 ? sk->m_left_inner  :
        pos == 1 ? sk->m_down_inner  :
        pos == 2 ? sk->m_up_inner    :
        pos == 3 ? sk->m_right_inner :
        missing_texture_ptr
    );
}

char *mania_note_serialize(mania_note *ptr)
{
    return int_to_str(ptr->lane);
}

void mania_note_overwrite(mania_note *ptr, char *serialized_data)
{
    ptr->lane = atoi(serialized_data);
}

char mania_note_key_good(mania_note *ptr)
{
    if(rg.screen == screen_editor) return 0;
    char ret = 0;
	if(rg.mania_click_1 == 1 && ptr->lane == 0) ret = rg.mania_click_1++;
	if(rg.mania_click_2 == 1 && ptr->lane == 1) ret = rg.mania_click_2++;
	if(rg.mania_click_3 == 1 && ptr->lane == 2) ret = rg.mania_click_3++;
	if(rg.mania_click_4 == 1 && ptr->lane == 3) ret = rg.mania_click_4++;
    return ret;
}
