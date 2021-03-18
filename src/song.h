#pragma once

#include "list.h"
#include "object.h"
#include "includes.h"
#include "text.h"
#include "text2.h"
#include "button.h"
#include "timing_section.h"
#include "song_select.h"
#include "audio_engine.h"
#include "timeline.h"
#include "health_bar.h"

#define SONG_PFW 640
#define SONG_PFH 480
#define SONG_UNKNOWN_NAME "unknown"
#define SONG_FILE_VERSION 1
#define SONG_SER_SEC_SEP "~"
#define SONG_SER_SEC_SEP_C '~'
#define SONG_SER_OBJ_SEP "&"
#define SONG_SER_OBJ_SEP_C '&'
#define SONG_SER_TSEC_SEP "#"
#define SONG_SER_TSEC_SEP_C '#'
#define SONG_DEFAULT_GRID_RESOLUTION 16
#define SONG_BG_DEFAULT_FILENAME "bg.png"
#define SONG_BEFORE_START_TIME_MS 2500
#define SONG_MANIA_JUDGEMENT_LINE_Y_BOTTOM 64
#define SONG_SKIP_PADDING_MS 1000
#define SONG_END_PADDING_MS 1000
#define SONG_MIN_SPEED 0.05f
#define SONG_MAX_SPEED 10.0f
#define SONG_MAX_BEAT_DIVIDE 256.0f
#define SONG_MIN_RAW_OBJECT_SIZE 0.0f
#define SONG_MAX_RAW_OBJECT_SIZE 10.0f
#define SONG_MIN_RAW_APPROACH_RATE 0.0f
#define SONG_MAX_RAW_APPROACH_RATE 10.0f

typedef enum song_state
{
    song_paused,
    song_playing
} song_state;

#include "globals.h"

typedef struct song
{
    int version;
    game_mode game_mode_in_editor;
	object_type editor_placement_type;
    
    char *name;
    char *bg_filename;
    char *folder_loc;
    char *data_file_loc;
    char *music_filename;
    char *music_file_loc;

    list *all_objects;
    list *game_mode_objects;
    list *clipboard_objects;
    list *selected_objects;
    list *timing_sections;
    list *explosions;
    list *replays;

    TIME_VAR cur_time_micro_sec;
    TIME_VAR current_time_ms;
    const TIME_VAR beat_length_ms;
    const TIME_VAR timing_section_start_ms;
    TIME_VAR length_ms;
    const TIME_VAR beat_divisor;
    TIME_VAR time_ms_on_editor_exit;
    TIME_VAR time_ms_on_editor_play;
    TIME_VAR preview_time_ms;
    TIME_VAR song_global_offset;
    TIME_VAR song_start_ms;
    TIME_VAR song_end_ms;
    TIME_VAR speed_multiplier;

    float raw_object_size;
    float object_size_pf;
    float object_size;
    float raw_approach_rate;
    TIME_VAR approach_time_ms;

    timer *song_timer;
    ae_music_handle *music;
    song_state music_state;

    float_rect selection_rect;
    struct object *selected_object;

    timer *beat_guess_clock;
    list *beat_guess_list;
    char beat_guess_build_up;
    
    int score;
    int passed_segs;
    int good_hits;
    int misses;
	int current_combo;
	int highest_combo;
    char failed;
    
    char align_on_scroll;
    char obj_hovered_on_click;
	char slider_control_point_box_hovered;
	char mouse_hovering_slider_body;
    int grid_resolution;
    char song_not_started;
    char paste_flip_mode;
    char slider_startend_hit;
    char dragging_selected_objects;
    int edit_count;
    struct se_loaded_texture *background_texture;
    struct replay *replay;

    list *undo_states;
    int undo_state;

    timeline tl;

    char metronome_on;
    int metronome_stanza;

    song_select_entry *ss_entry;

    health_bar hbar;

	char hash[SHA256_STRING_LEN];
} song;

song *song_init(char *name, char *folder_loc, char *data_file_loc);
void song_free(song *ptr);

float song_get_real_raw_object_size(song *ptr);
float song_get_real_raw_approach_rate(song *ptr);
void song_calc_difficulty(song *ptr);

void song_restart(song *ptr);
void song_play(song *ptr);
void song_preview_play(song *ptr);
void song_pause(song *ptr);
void song_toggle_play_pause(song *ptr);
void song_stop(song *ptr);

void song_calc_song_skip_and_end(song *ptr);
void song_set_music_vol(song *ptr, float vol);
void song_set_speed(song *ptr, float speed);
void song_apply_speed(song *ptr);
void song_stack_objects(song *ptr);

void song_copy(song *ptr);
void song_paste(song *ptr);
void song_select_all(song *ptr);
void song_clear_clipboard(song *ptr);
void song_clear_selected_objects(song *ptr);
void song_delete_selected_objects(song *ptr);
void song_unselect_object(song *ptr);

void song_tick_timing(song *ptr);
void song_draw_background(song *ptr);
void song_tick(song *ptr);

float song_calc_accuracy(song *ptr, char live);
char song_calc_rank(song *ptr);

void song_inc_combo(song *ptr);
void song_break_combo(song *ptr);

void song_reset_score(song *ptr);
void song_reset_object_state(song *ptr);

void song_capture_undo_state(song *ptr);
void song_chop_undo_states(song *ptr);
void song_undo(song *ptr);
void song_redo(song *ptr);

void song_clear_explosions(song *ptr);
void song_clear_replays(song *ptr);
void song_clear_objects(song *ptr, char cap_undo_state);
void song_clear_timing_sections(song *ptr);

timing_section *song_apply_timing_sec(song *ptr, TIME_VAR time_ms);
timing_section *song_get_timing_section(song *ptr, TIME_VAR start_ms);

void song_set_time(song *ptr, TIME_VAR time_ms);
void song_set_time_to_song_time(song *ptr);
void song_scroll(song *ptr, int dir);

TIME_VAR song_align_ms_unclamped(song *ptr, TIME_VAR unaligned_ms);
TIME_VAR song_align_ms(song *ptr, TIME_VAR unaligned_ms);
void song_align_ctms(song *ptr);

void song_all_sliders_del_textures(song *ptr);
void song_all_sliders_recalc_paths(song *ptr);

v2f song_map_coord_to_play_field(song *ptr, v2f object_coord);
float_rect song_map_float_rect_to_play_field(song *ptr, float_rect rect);
v2f song_get_playfield_dimensions(song *ptr);
float song_get_pf_margin(song *ptr);

float song_mania_get_judge_line_y(song *ptr);
float song_mania_map_time_to_y(song *ptr, TIME_VAR rel_time_ms);

void song_sort_objects(song *ptr);
void song_sort_selected_objects(song *ptr);
void song_sort_timing_sections(song *ptr);
object *song_get_obj_by_number(song *ptr, int num);

void song_add_circle(song *ptr, float x, float y, TIME_VAR time_ms);
void song_add_mania_note(song *ptr, int lane, TIME_VAR time_ms);

char song_reload_bg(song *ptr);
char song_load_music(song *ptr);
char *song_serialize(song *ptr);
void song_overwrite(song *ptr, char *serialized_data, char is_undo_redo);
char song_load(song *ptr);
char song_load_replay(song *ptr, char *data);
void song_new(song *ptr, char *music_file_name);
void song_save(song *ptr);
void song_save_score(song *ptr);
char *song_get_scores_dir(song *ptr);

/*song *song_convert_from_osu_beatmap(char *osu_file_loc);*/