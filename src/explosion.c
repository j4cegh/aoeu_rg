#include "explosion.h"
#include "circle.h"
#include "skin.h"
#include "rg.h"
#include "utils.h"

explosion *explosion_init_circ(v2f pos, song *song, TIME_VAR on_ms)
{
	explosion *ret = malloc(sizeof *ret);
	ret->pos = pos;
	ret->song = song;
	ret->on_ms = on_ms;
    ret->type = explosion_circle;
    ret->rot = 0.0f;
    ret->judge = judgement_unknown;
	return ret;
}

explosion *explosion_init_rev(v2f pos, song *song, TIME_VAR on_ms, float rot)
{
    explosion *ret = malloc(sizeof *ret);
	ret->pos = pos;
	ret->song = song;
	ret->on_ms = on_ms;
    ret->type = explosion_reverse;
    ret->rot = rot;
    ret->judge = judgement_unknown;
	return ret;
}

explosion *explosion_init_jdgmnt(v2f pos, song *song, TIME_VAR on_ms, judgement judge)
{
    explosion *ret = malloc(sizeof *ret);
	ret->pos = pos;
	ret->song = song;
	ret->on_ms = on_ms;
    ret->type = explosion_judgement;
    ret->rot = 0.0f;
    ret->judge = judge;
	return ret;
}

void explosion_tick(explosion *ptr)
{
    skin *skin = rg.skin;
    loaded_texture *sp = NULL;
    if(ptr->type == explosion_circle) sp = skin->circle;
    if(ptr->type == explosion_reverse) sp = skin->reversearrow;
    if(ptr->type == explosion_judgement) sp = skin->miss;
    if(sp)
    {
        TIME_VAR reltime = ptr->song->current_time_ms - ptr->on_ms;
        float obj_sc = ptr->song->object_size;
        float dist = scale_value_to(CIRCLE_FADE_OUT_TIME_MS - reltime, CIRCLE_FADE_OUT_TIME_MS, 0, 0.25, 1);
        float nscale = scale_value_to(reltime, 0, (CIRCLE_FADE_OUT_TIME_MS * dist), 1, CIRCLE_EXPLOSION_SCALE);
        char is_miss = ptr->type == explosion_judgement && ptr->judge == judgement_miss;
        if(nscale < 1.0f || is_miss) nscale = 1.0f;
        nscale *= obj_sc;
        color4 col = col_white;
        if(is_miss) col.a = 255 - clamp((int)scale_value_to(reltime - (MISS_TIME / 2), 0, 500, 0, 255), 0, 255);
        else col.a = clamp(scale_value_to(reltime, 0, CIRCLE_FADE_OUT_TIME_MS + 50, 255, 0), 0, 255);
        glPushMatrix();
        v2f op_pos = song_map_coord_to_play_field(ptr->song, ptr->pos);
        glTranslatef(op_pos.x, op_pos.y + (is_miss ? scale_value_to(col.a, 255, 0, 0, 50 * obj_sc) : 0), 0);
        if(is_miss) glRotatef(scale_value_to(col.a, 255, 0, 0, 10), 0.0, 0.0, 1.0);
        else glRotatef(ptr->rot, 0.0, 0.0, 1.0);
        textures_drawsc(sp, 0, 0, nscale, nscale, col);
        glPopMatrix();
    }
}
