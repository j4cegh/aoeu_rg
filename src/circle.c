#include "circle.h"
#include "song.h"
#include "skin.h"
#include "rg.h"

circle *circle_init(object *host_obj)
{
    circle *ret = malloc(sizeof *ret);
    ret->host_obj = host_obj;
    ret->number = 0;
    ret->num_digits = 1;
    ret->num_str = NULL;
    ret->num_width = 0.0f;
    return ret;
}

void circle_free(circle *ptr)
{
    if(ptr->num_str) free(ptr->num_str);
    free(ptr);
}

char circle_check_hovered(circle *ptr)
{
    object *obj = ptr->host_obj;
    song *song = obj->host_song;
    v2f obj_pos_mapped = song_map_coord_to_play_field(song, obj->real_pos);
    v2f mouse_mapped = song_map_coord_to_play_field(song, V2F(rg.mop_x, rg.mop_y));
    float mouse_to_obj_dist = get_distf(obj_pos_mapped.x, obj_pos_mapped.y, mouse_mapped.x, mouse_mapped.y);
    obj->hovered = (mouse_to_obj_dist <= (CIRCLE_RADIUS * song->object_size));
    return obj->hovered;
}

void circle_set_number(circle *ptr, int number)
{
    if(ptr == NULL || ptr->number == number) return;
    ptr->number = number;
    ptr->num_digits = ptr->number == 0 ? 1 : (floor(log10(abs(ptr->number))) + 1);
    if(ptr->num_str) free(ptr->num_str);
    ptr->num_str = int_to_str(ptr->number);
    ptr->num_width = 0.0f;
    int i;
    for(i = 0; i < ptr->num_digits; ++i)
    {
        char c = ptr->num_str[i];
        v2f size = skin_get_num_size(rg.skin, c);
        ptr->num_width += size.x;
    }
}

void circle_update(circle *ptr)
{

}

void circle_snap_to_grid(circle *ptr)
{
    object *obj = ptr->host_obj;
    if(obj->type != object_circle) return;
    song *song = obj->host_song;
    if(obj->do_grid_snap == 1 && song->grid_resolution != SONG_PFW)
    {
        float div = SONG_PFW / song->grid_resolution;
        float sox = round((obj->real_pos.x) / div) * div;
        float soy = round((obj->real_pos.y) / div) * div;
        ptr->host_obj->real_pos.x = sox;
        ptr->host_obj->real_pos.y = soy;
        obj->do_grid_snap = 0;
    }
}

void circle_render(circle *ptr)
{
    object *obj = ptr->host_obj;
    song *song = obj->host_song;
    TIME_VAR apptime = song->approach_time_ms;
    TIME_VAR reltime = obj->relative_time_ms;
    TIME_VAR hitreltime = obj->hit_time_ms_rel;
    judgement result = obj->start_judgement;
    
    v2f pos = song_map_coord_to_play_field(song, obj->real_pos);
    pos.x += obj->wiggle_xoff;

    float exp_sc = 1.0f;
    if(rg.screen == screen_player && result != judgement_unknown && result != judgement_miss) 
        exp_sc = clamped_scale_value_to
        (
            reltime,
            hitreltime,
            hitreltime
            +
            (
                CIRCLE_FADE_OUT_TIME_MS
                *
                scale_value_to
                (
                    CIRCLE_FADE_OUT_TIME_MS
                    -
                    (
                        reltime
                        -
                        hitreltime
                    ),
                    CIRCLE_FADE_OUT_TIME_MS,
                    0,
                    0.25,
                    1
                )
            ),
            1.0f,
            CIRCLE_EXPLOSION_SCALE
        );
    
    exp_sc = clamp(exp_sc, 1.0f, CIRCLE_EXPLOSION_SCALE);
    exp_sc *= song->object_size;

    color4 col = col_white;
    if(reltime <= 0 && result == judgement_unknown) col.a = clamped_scale_value_to(reltime, -apptime, -apptime + CIRCLE_FADE_IN_TIME_MS, 0, 255);
    else col.a = 255 - clamped_scale_value_to(reltime - hitreltime, 0, ((rg.screen == screen_editor) ? (obj->type == object_slider ? CIRCLE_FADE_OUT_TIME_MS : CIRCLE_EDITOR_FADE_OUT_MS) : CIRCLE_FADE_OUT_TIME_MS) - (result == judgement_miss ? MISS_TIME : 0), 0, 255);
    if((obj->type == object_slider ? result != judgement_unknown : 1) || rg.screen == screen_editor) textures_drawsc(rg.skin->circle, pos.x, pos.y, exp_sc, exp_sc, col);
    
    circle_render_number(ptr, pos.x, pos.y, CIRCLE_RADIUS, exp_sc, col);
}

void circle_render_number(circle *ptr, float x, float y, float radius, float sc, color4 col)
{
    float num_sc = radius / (ptr->num_width * (ptr->num_digits < 2 ? 2 : 1));
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(sc, sc, 0);
    float xr = 0.0f;
    int i;
    for(i = 0; i < ptr->num_digits; ++i)
    {
        char c = ptr->num_str[i];
        loaded_texture *tex = skin_get_num(rg.skin, c);
        v2f size = V2F(tex->surf->w, tex->surf->h);
        v2f pos = V2F(-(((ptr->num_width / 2) * num_sc)) + xr, -((size.y * num_sc) / 2));
        textures_drawsc(tex, pos.x, pos.y, num_sc, num_sc, col);
        xr += size.x * num_sc;
    }
    glPopMatrix();
}
