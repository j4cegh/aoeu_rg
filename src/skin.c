#include "skin.h"
#include "utils.h"
#include "textures.h"


char *skin_load_dir;

void slt(char *filename, loaded_texture **tex, char center_origin)
{
    char *r_file = dupfmt("%s%s", skin_load_dir, filename);
    *tex = textures_get(r_file, tex_load_loc_filesystem, 1);
    loaded_texture *texd = *tex;
    if(center_origin) texd->o = V2F(texd->surf->w / 2, texd->surf->h / 2);
    free(r_file);
}

void slse(char *filename, ae_sound_effect **se)
{
    char *sound_file = dupfmt("%s%s", skin_load_dir, filename);
    *se = ae_sound_effect_init(sound_file);
    free(sound_file);
}

skin *skin_init(char *name)
{
    skin *ret = malloc(sizeof *ret);
    ret->name = dupe_str(name);

    skin_load_dir = dupfmt(SKIN_SKINS_DIR "%s/", name);

    slt(SKIN_BG_FILE, &ret->bg, 0);
    slt("title.png", &ret->title, 0);
    ret->title->no_smoothing = 1;
    slt(SKIN_CURSOR_FILE, &ret->cursor, 1);
    slt(SKIN_CIRCLE_FILE, &ret->circle, 1);
    ret->circle->sc_div = SKIN_HIGH_RES_SC_DIV;
    slt(SKIN_APPROACH_CIRCLE_FILE, &ret->approachcircle, 1);
    ret->approachcircle->sc_div = SKIN_HIGH_RES_SC_DIV;
    slt(SKIN_SLIDER_BALL_FILE, &ret->sliderball, 1);
    slt(SKIN_SLIDER_FOLLOW_CIRCLE_FILE, &ret->followcircle, 1);
    slt(SKIN_MISS_FILE, &ret->miss, 1);
    slt(SKIN_REVERSEARROW_FILE, &ret->reversearrow, 1);
    
    slt(SKIN_MANIA_UP_INNER_FILE, &ret->m_up_inner, 1);
    slt(SKIN_MANIA_UP_OUTLINE_FILE, &ret->m_up_outline, 1);
    slt(SKIN_MANIA_DOWN_INNER_FILE, &ret->m_down_inner, 1);
    slt(SKIN_MANIA_DOWN_OUTLINE_FILE, &ret->m_down_outline, 1);
    slt(SKIN_MANIA_LEFT_INNER_FILE, &ret->m_left_inner, 1);
    slt(SKIN_MANIA_LEFT_OUTLINE_FILE, &ret->m_left_outline, 1);
    slt(SKIN_MANIA_RIGHT_INNER_FILE, &ret->m_right_inner, 1);
    slt(SKIN_MANIA_RIGHT_OUTLINE_FILE, &ret->m_right_outline, 1);

    slt(SKIN_SONGSEL_ENTRY_FILE, &ret->songsel_entry, 0);

    slt(SKIN_NUM_0_FILE, &ret->n0, 0);
    slt(SKIN_NUM_1_FILE, &ret->n1, 0);
    slt(SKIN_NUM_2_FILE, &ret->n2, 0);
    slt(SKIN_NUM_3_FILE, &ret->n3, 0);
    slt(SKIN_NUM_4_FILE, &ret->n4, 0);
    slt(SKIN_NUM_5_FILE, &ret->n5, 0);
    slt(SKIN_NUM_6_FILE, &ret->n6, 0);
    slt(SKIN_NUM_7_FILE, &ret->n7, 0);
    slt(SKIN_NUM_8_FILE, &ret->n8, 0);
    slt(SKIN_NUM_9_FILE, &ret->n9, 0);

    slse(SKIN_HIT_SOUND_FILE, &ret->hit);
    slse(SKIN_MISS_SOUND_FILE, &ret->miss_eff);
    slse(SKIN_LONG_START_SOUND_FILE, &ret->long_start);
    slse(SKIN_LONG_MIDDLE_SOUND_FILE, &ret->long_middle);
    slse(SKIN_LONG_END_SOUND_FILE, &ret->long_end);
    slse(SKIN_METRONOME_SOUND_FILE, &ret->metronome);

    free(skin_load_dir);

    return ret;
}

void skin_set_effect_vol(skin *ptr, float vol)
{
    ae_sound_effect_set_volume(ptr->hit, vol);
    ae_sound_effect_set_volume(ptr->miss_eff, vol);
    ae_sound_effect_set_volume(ptr->long_start, vol);
    ae_sound_effect_set_volume(ptr->long_middle, vol);
    ae_sound_effect_set_volume(ptr->long_end, vol);
    ae_sound_effect_set_volume(ptr->metronome, vol);
}

loaded_texture *skin_get_num(skin *ptr, char c)
{
    switch(c)
    {
        case '0': return ptr->n0;
        case '1': return ptr->n1;
        case '2': return ptr->n2;
        case '3': return ptr->n3;
        case '4': return ptr->n4;
        case '5': return ptr->n5;
        case '6': return ptr->n6;
        case '7': return ptr->n7;
        case '8': return ptr->n8;
        case '9': return ptr->n9;
    }
    return missing_texture_ptr;
}

v2f skin_get_num_size(skin *ptr, char c)
{
    loaded_texture *tex = skin_get_num(ptr, c);
    return V2F(tex->surf->w, tex->surf->h);
}

void skin_free(skin *ptr)
{
    ae_sound_effect_free(ptr->hit);
    ae_sound_effect_free(ptr->miss_eff);
    ae_sound_effect_free(ptr->long_start);
    ae_sound_effect_free(ptr->long_middle);
    ae_sound_effect_free(ptr->long_end);
    ae_sound_effect_free(ptr->metronome);
    free(ptr->name);
    free(ptr);
}
