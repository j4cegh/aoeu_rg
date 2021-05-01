#include "approach_circle.h"
#include "song.h"
#include "circle.h"
#include "slider.h"
#include "rg.h"
#include "skin.h"
#include "textures.h"
#include "utils.h"

void approach_circle_draw(object *ptr)
{
	if(ptr->type == object_slider && rg.enable_slider_debug) return;
    song *song = ptr->host_song;
    TIME_VAR reltime = ptr->relative_time_ms;
    TIME_VAR apptime = song->approach_time_ms;
    if
    (
        (
            ptr->slider != NULL
            &&
            ptr->slider->tex == 0
        )
        ||
        (
            ptr->start_judgement
            !=
            judgement_unknown
        )
        ||
        reltime >= 0
    )
        return;
    v2f pos = song_map_coord_to_play_field(song, ptr->real_pos);
    pos.x += ptr->wiggle_xoff;
    float scale =
        (
            scale_value_to
            (
                reltime,
                -apptime,
                0.0f,
                APPROACH_CIRCLE_STARTING_SCALE,
                APPROACH_CIRCLE_ENDING_SCALE
            )
            *
            song->object_size
        );
    color4 col = col_white;
    col.a =
        clamp
        (
            scale_value_to
            (
                reltime,
                -apptime,
                (
                    -apptime
                    +
                    CIRCLE_FADE_IN_TIME_MS
                ),
                0,
                255
            ),
            0,
            255
        );
    textures_drawsc(rg.skin->approachcircle, pos.x, pos.y, scale, scale, col);
}
