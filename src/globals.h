#pragma once

typedef enum game_mode
{
    game_mode_ouendan,
    game_mode_mania,
	game_mode_aoeu,
    game_mode_count
} game_mode;

typedef enum screen_type
{
    screen_main_menu,
    screen_song_select,
    screen_options,
    screen_player,
    screen_editor,
    screen_timing_sections,
    screen_set_keys_ouendan,
    screen_set_keys_mania,
    screen_fullscreen_switch,
    screen_mode_switch,
    screen_final_score,
    screen_import_editor,
    screen_tog_login_win,
    screen_empty,
    screen_exit
} screen_type;

typedef enum fade_state
{
    fade_done,
    fade_in,
    fade_out
} fade_state;

typedef enum cursor_state
{
    cursor_neutral,
    cursor_contract,
    cursor_expand
} cursor_state;

#define TIME_VAR float