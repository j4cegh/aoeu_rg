#pragma once

#include "globals.h"
#include "includes.h"
#include "includes.h"
#include "mouse.h"
#include "notification.h"
#include "center_status.h"

#ifdef USING_WINDOWS
#include <discord_rpc.h>
#endif

#define CLICK_BLOCK_TIME_MS 50
#define FPS_LIMIT_NO_FOCUS 40
#define MOUSE_MOVED_DECAY_MS 50
#define MOUSE_VEL_CHECK_MS 3
#define FRAMERATE_LIMIT 1000
#define FADE_MS 175

#define SONGS_DIR "./songs/"
#define DATA_DIR "./data/"
#define SCORES_DIR DATA_DIR "scores/"

#define CURSOR_EXPAND_MS 150
#define CURSOR_EXPAND_MULTIPLIER 1.4f

#define CONFIG_NAME_FULLSCREEN "fullscreen"
#define CONFIG_NAME_OUENDAN_KEY_1 "ouendan_keycode_1"
#define CONFIG_NAME_OUENDAN_KEY_2 "ouendan_keycode_2"
#define CONFIG_NAME_MANIA_KEY_1 "mania_keycode_1"
#define CONFIG_NAME_MANIA_KEY_2 "mania_keycode_2"
#define CONFIG_NAME_MANIA_KEY_3 "mania_keycode_3"
#define CONFIG_NAME_MANIA_KEY_4 "mania_keycode_4"
#define CONFIG_NAME_BG_DIM_LEVEL "bg_dim_level"
#define CONFIG_NAME_MASTER_VOL "master_vol"
#define CONFIG_NAME_MUSIC_VOL "music_vol"
#define CONFIG_NAME_EFFECT_VOL "effect_vol"
#define CONFIG_NAME_EASY_MODE "easy_mode_enabled"
#define CONFIG_NAME_BEAT_FLASHES "beat_flashes_enabled"
#define CONFIG_NAME_POTATO_PC "potato_pc_enabled"
#define CONFIG_NAME_DISABLE_MOUSE_BUTTONS "disable_mouse_buttons"
#define CONFIG_NAME_GAME_MODE "game_mode"
#define CONFIG_NAME_SKIN "skin_name"
#define CONFIG_NAME_USERNAME "username"
#define CONFIG_NAME_PASS_HASH "pass_hash"

typedef struct vars
{
	game_mode mode;
	game_mode to_mode;

	/* online */
    struct online_client *client;
#ifndef RELEASE
    struct online_server *server;
#endif

	/* window */
	SDL_Window *win;
	SDL_GLContext context;

	/* window size */
	float win_width;
	float win_height;
	float win_width_mid;
	float win_height_mid;
    int win_width_fixed;
    int win_height_fixed;
	float win_width_prev;
	float win_height_prev;
	int win_width_real;
	int win_height_real;

	/* window misc */
	char is_open;
	unsigned int resized;
	char fullscreen_enabled;
	char do_fullscreen_switch;

	/* window focus */
	char focus;
	unsigned long long int focus_frames;

	/* gui view */
	float gui_view_scale;
	float gui_view_xoff;
	float gui_view_yoff;

	/* framerate */
	float fps_limit;
	float frame_delta_ms;
	float frames_per_second;
	char framerate_display_enabled;
	unsigned long long int frames;

	/* mouse */
    float mop_x;
    float mop_y;
    float mop_xoc;
    float mop_yoc;
    float mop_fdx;
    float mop_fdy;
    char is_mouse_on_playfield;
    char is_mouse_on_click_op;
    float mouse_x_bot;
    float mouse_y_bot;
    char hide_game_cursor;
    timer *cursor_expand_timer;
	timer *cursor_trail_timer;
    struct list *cursor_trail_points;
    char disable_mouse_buttons;
    cursor_state cursor_state;

	/* keyboard */
	int kp[KEY_COUNT];
	char kr[KEY_COUNT];
    int ouendan_click_1;
    int ouendan_click_2;
	TIME_VAR ouendan_click_1_frame_1_time_ms;
	TIME_VAR ouendan_click_2_frame_1_time_ms;
    int mania_click_1;
    int mania_click_2;
    int mania_click_3;
    int mania_click_4;
    int mania_key_click_count;

	/* keycodes */
    int ouendan_keycode_1;
    int ouendan_keycode_2;
    int mania_keycode_1;
    int mania_keycode_2;
    int mania_keycode_3;
    int mania_keycode_4;

	/* screen */
    screen_type screen;
    screen_type to_screen;
    screen_type last_screen;
	
	/* audio */
    float master_vol;
    float music_vol;
    float effect_vol;

	/* graphics */
    fade_state fade_state;
    float bg_dim_level;
    float fade_alpha;
    char enable_beat_flash;
    char potato_pc_enabled;

	/* misc */
    struct skin *skin;
    struct song *song;
    struct gui_win *login_win;
	struct replay *replay_to_watch;
	struct popup *global_popup;
	TTF_Font *font;
	timer *time_open;
    timer *fade_clock;

	/* options */
    char editor_testing_song;
    char bot_enabled;
    char replay_mode_enabled;
	char enable_slider_debug;
	char set_keys_state;

	/* used to only allow interaction with the topmost rendered gui elements */
	/* there is a 1 frame delay but this is a good comprimise */
	void *gui_render_ptr; /* set this to your gui element pointer */
	void *gui_render_end_of_frame; /* check if your gui element is topmost with this pointer */

	/* discord */
#ifdef USING_WINDOWS
    DiscordRichPresence rpc;
#endif

	struct button *song_sel_button;
	struct button *options_button;
	struct button *exit_button;
	
	struct button *back_button;
	struct scroll_bar *sbar;
	
	struct input_box *cs_input;
	struct input_box *ar_input;
	struct input_box *bg_fname_input;
	struct input_box *speed_input;
	struct input_box *song_offset_input;
	struct button *save_song_button;
	struct button *test_song_button;
	struct button *save_quit_button;
	struct button *just_quit_button;
	struct button *goto_timing_points_button;
	struct button *grid_res_menu_button;
	struct context_menu *grid_res_cmenu;
	
	struct slider_bar *opts_master_vol_bar;
	struct slider_bar *opts_music_vol_bar;
	struct slider_bar *opts_effect_vol_bar;
	struct slider_bar *opts_bg_dim_bar;
	struct checkbox *opts_beat_flash_cbox;
	struct checkbox *opts_fullscreen_cbox;
	struct checkbox *opts_bot_cbox;
	struct checkbox *opts_disable_mouse_btns_cbox;
	struct button *opts_goto_set_keys_ouendan;
	struct button *opts_goto_set_keys_mania;
	
	struct button *editor_menu_btn;
	struct context_menu *editor_menu;
	struct button *editor_menu_delete_all_btn;
	struct button *editor_menu_object_type_btn;
	
	struct gui_win *editor_settings_win;
} vars;

extern vars rg;

void set_resolution(unsigned int width, unsigned int height);
void apply_volume();
void escape();
TIME_VAR beat_length_ms_to_beats_per_minute(TIME_VAR beat_length_ms);
TIME_VAR beats_per_minute_to_beat_length_ms(TIME_VAR beats_per_minute);
void write_config();
char *ms_to_hr_time(TIME_VAR ms);
char change_skin(char *name);
void change_screen(screen_type to_screen);
void apply_cursor();