#include "rg.h"
#include "utils.h"
#include "text.h"
#include "text2.h"
#include "text3.h"
#include "console.h"
#include "skin.h"
#include "song.h"
#include "curves.h"
#include "slider.h"
#include "song_select.h"
#include "scroll_bar.h"
#include "replay.h"
#include "ezgui.h"
#include "gui_win_man.h"
#include "checkbox.h"
#include "slider_bar.h"
#include "popup.h"
#include "logger.h"
#include "timer.h"
#include "audio_engine.h"
#include "login_win.h"
#include "discord.h"
#include "config.h"
#include "textures.h"
#include "context_menu.h"
#include "input_box.h"
#include "ezgui.h"
#include "glyphs.h"
#include "events.h"
#include "online_client.h"
#ifndef RELEASE
#include "online_server.h"
#endif

#ifndef RELEASE
void change_fps(CONSOLE_COMMAND_ON_CALL_ARGS)
{
	if(arguments->count >= 2)
	{
		char *fps_str = list_get_val_at(arguments, 1);
		float lfps = atof(fps_str);
		if(lfps < 1)
		{
			rg.fps_limit = 0;
			console_print(cons_ptr, "framerate is unlimited", col_green);
		}
		else
		{
			rg.fps_limit = clamp(lfps, 1, INFINITY);
			char *m = dupfmt("changed framerate limit to: %s", fps_str);
			console_print(cons_ptr, m, col_green);
			free(m);
		}
	}
	else console_print(cons_ptr, "you didn't give me a number.", col_red);
}

void set_resolution(unsigned int width, unsigned int height)
{
	width = clamp(width, 10, INFINITY);
	height = clamp(height, 10, INFINITY);
	SDL_SetWindowSize(rg.win, width, height);
	rg.win_width_prev = width;
	rg.win_height_prev = height;
	SDL_SetWindowPosition(rg.win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

#define RES_SEPERATOR "x"
void set_res_cmd(CONSOLE_COMMAND_ON_CALL_ARGS)
{
	if(arguments->count >= 2)
	{
		v2u size = V2U(800, 600);
		char good = 0;
		char *lower = str_to_lower(list_get_val_at(arguments, 1));
		if(strlen(lower) >= 5 && str_contains(lower, RES_SEPERATOR, 0))
		{
			list *split = split_str(lower, RES_SEPERATOR);
			if(split->count == 2)
			{
				int x = atoi(list_get_val_at(split, 0));
				int y = atoi(list_get_val_at(split, 1));
				size.x = x;
				size.y = y;
				good = 1;
			}
			list_free(split, free);
		}
		if(good)
		{
			set_resolution(size.x, size.y);
			rg.resized = 1;
			console_print(cons_ptr, "changed resolution.", col_green);
		}
		else console_print(cons_ptr, "invalid resolution!", col_red);

		free(lower);
	}
	else console_print(cons_ptr, "you need to provide a resolution!", col_red);
}
#endif

float last_time_fps = 0;
text *fps_text = NULL;
timer *fps_clock = NULL;
timer *fps_clock_interval = NULL;

void tick_framerate_display()
{
	float current_time = timer_seconds(fps_clock);
	rg.frames_per_second = 1.0f / (current_time - last_time_fps);
	if(rg.frames_per_second < 0) rg.frames_per_second = 0;
	last_time_fps = current_time;
	if(timer_milliseconds(fps_clock_interval) >= 1000)
	{
		timer_restart(fps_clock);
		timer_restart(fps_clock_interval);
		char *new_fps_text = float_to_str(rg.frames_per_second);
		text_set_string(fps_text, new_fps_text);
		free(new_fps_text);
	}
	if(rg.framerate_display_enabled || rg.kp[SDL_SCANCODE_RCTRL])
	{
		text_set_position(fps_text, rg.win_width_mid - (fps_text->size.x / 2), rg.win_height - fps_text->size.y);
		text_render(fps_text);
	}
}

vars rg;

#ifndef RELEASE
void rg_cmd(CONSOLE_COMMAND_ON_CALL_ARGS)
{
    int argc = arguments->count;
	char print_args = 0;
	if(argc > 1)
	{
		char *pt = list_get_val_at(arguments, 1);
		if(argc == 2)
		{
			if(strs_are_equal(pt, "prev")) rg.song->preview_time_ms = rg.song->current_time_ms;
			else print_args = 1;
		}
		else if(argc >= 3)
		{
			char *val = list_get_val_at(arguments, 2);
			if(strs_are_equal(pt, "skin")) change_skin(val);
			else if(strs_are_equal(pt, "offset")) rg.song->song_global_offset = atof(val);
			else if(strs_are_equal(pt, "bpm")) song_get_timing_section(rg.song, rg.song->current_time_ms)->beat_length_ms = beats_per_minute_to_beat_length_ms(atof(val));
			else if(strs_are_equal(pt, "ar")) rg.song->raw_approach_rate = atof(val);
			else if(strs_are_equal(pt, "cs"))
			{
				rg.song->raw_object_size = atof(val);
				song_calc_difficulty(rg.song);
				song_all_sliders_del_textures(rg.song);
			}
			else if(strs_are_equal(pt, "speed")) song_set_speed(rg.song, atof(val));
			else if(strs_are_equal(pt, "master")) rg.master_vol = clamp(atof(val), 0, 100);
			else if(strs_are_equal(pt, "mus")) rg.music_vol = clamp(atof(val), 0, 100);
			else if(strs_are_equal(pt, "eff")) rg.effect_vol = clamp(atof(val), 0, 100);
			else if(strs_are_equal(pt, "gm"))
			{
				if(strs_are_equal(val, "o")) rg.to_mode = game_mode_ouendan;
				if(strs_are_equal(val, "m")) rg.to_mode = game_mode_mania;
				if(rg.mode != rg.to_mode) change_screen(screen_mode_switch);
			}
			else if(strs_are_equal(pt, "pm"))
			{
				if(strs_are_equal(val, "c")) rg.song->editor_placement_type = object_circle;
				else if(strs_are_equal(val, "sl")) rg.song->editor_placement_type = object_slider;
				else if(strs_are_equal(val, "sp")) rg.song->editor_placement_type = object_spinner;
			}
			else if(strs_are_equal(pt, "sc"))
			{
				if(strs_are_equal(val, "menu")) change_screen(screen_main_menu);
				else if(strs_are_equal(val, "options")) change_screen(screen_options);
				else if(strs_are_equal(val, "songsel")) change_screen(screen_song_select);
				else if(strs_are_equal(val, "editor")) change_screen(screen_editor);
				else if(strs_are_equal(val, "timesecs")) change_screen(screen_timing_sections);
				else if(strs_are_equal(val, "setkeyso")) change_screen(screen_set_keys_ouendan);
				else if(strs_are_equal(val, "setkeysm")) change_screen(screen_set_keys_mania);
				else if(strs_are_equal(val, "play")) change_screen(screen_player);
				else if(strs_are_equal(val, "finalscore")) change_screen(screen_final_score);
				else 
				console_print
					(
						from, 
						"args: "
						CONSOLE_CMD_PREFIX_STR
						"rg sc [menu, options, songsel, editor, timesecs, setkeyso, setkeysm, play, finalscore]",
						col_red
					);
			}
			else if(strs_are_equal(pt, "cons")) online_server_run_cons_cmd(rg.server, after_arg0 + strlen(pt) + 1);
			else print_args = 1;
		} else print_args = 1;
	} else print_args = 1;
	
	if(print_args) console_print(from, "args: " CONSOLE_CMD_PREFIX_STR "rg [skin, prev, offset, bpm, ar, cs, speed, master, mus, eff, gm, pm, sc] [val]", col_red);
}
#endif

void render_back_button()
{
	button_set_pos(rg.back_button, (PADDING * 4), rg.win_height - (rg.back_button->size.y) - (PADDING * 4));
	button_update(rg.back_button);
	if(rg.back_button->clicked) escape();
	button_render(rg.back_button);
}

void render_change_mode_button(float x, float y)
{
	button *btn = ez_bp("switch_mode_btn", rg.mode == game_mode_ouendan ? "ouendan" : rg.mode == game_mode_mania ? "mania" : rg.mode == game_mode_aoeu ? "aoeu" : "???", x, y);
	if(btn->hovered)
	{
		rg.gui_render_ptr = btn;
		if(rg.gui_render_end_of_frame == btn) text3_fmt(mouse_x, mouse_y - btn->size.y, rg.win_width_mid, 255, origin_center, origin_bottom, text3_justification_left, "~u%s", "click to change game mode");
	}
	if(btn->clicked)
	{
		if(++rg.mode > game_mode_count - 1) rg.mode = 0;
		online_client_request_replay_list(rg.client);
		song_sort_objects(rg.song);
	}
}

void apply_volume()
{
	float m_master_vol = (rg.master_vol / 100) * clamp(1.0f - clamped_scale_value_to(rg.fade_alpha, 0.0f, 255.0f, 0.0f, 1.0f), 0.15f, 1.0f);
	float m_music_vol = rg.music_vol * m_master_vol;
	float m_effect_vol = rg.effect_vol * m_master_vol;
	song_set_music_vol(rg.song, m_music_vol);
	skin_set_effect_vol(rg.skin, m_effect_vol);
}

void just_quit_pop_cback(void *ptr, char yes)
{
	if(yes) change_screen(screen_song_select);
}

void volume_control_tick()
{
	float volper = scale_value_to(rg.music_vol, 0, 100, 0, 1);
	float volperscmod = mouse_y < rg.win_height_mid ? 1.5 : 1;
	if(volper > 0.0f) GL_CIRCLE(rg.win_width_mid, rg.win_height_mid - (rg.win_height / 4), (return_lowest(rg.win_width, rg.win_height) / 6) * volperscmod, 100, volper, col_red);

	float effper = scale_value_to(rg.effect_vol, 0, 100, 0, 1);
	float effperscmod = mouse_y >= rg.win_height_mid ? 1.5 : 1;
	if(effper > 0.0f) GL_CIRCLE(rg.win_width_mid, rg.win_height_mid + (rg.win_height / 4), (return_lowest(rg.win_width, rg.win_height) / 6) * effperscmod, 100, effper, col_cyan);
}

void write_config()
{
	config_setf(CONFIG_NAME_FULLSCREEN,            int_to_str(rg.fullscreen_enabled));
	config_setf(CONFIG_NAME_OUENDAN_KEY_1,         int_to_str(rg.ouendan_keycode_1));
	config_setf(CONFIG_NAME_OUENDAN_KEY_2,         int_to_str(rg.ouendan_keycode_2));
	config_setf(CONFIG_NAME_MANIA_KEY_1,           int_to_str(rg.mania_keycode_1));
	config_setf(CONFIG_NAME_MANIA_KEY_2,           int_to_str(rg.mania_keycode_2));
	config_setf(CONFIG_NAME_MANIA_KEY_3,           int_to_str(rg.mania_keycode_3));
	config_setf(CONFIG_NAME_MANIA_KEY_4,           int_to_str(rg.mania_keycode_4));
	config_setf(CONFIG_NAME_BG_DIM_LEVEL,          float_to_str(rg.bg_dim_level));
	config_setf(CONFIG_NAME_MASTER_VOL,            float_to_str(rg.master_vol));
	config_setf(CONFIG_NAME_MUSIC_VOL,             float_to_str(rg.music_vol));
	config_setf(CONFIG_NAME_EFFECT_VOL,            float_to_str(rg.effect_vol));
	config_setf(CONFIG_NAME_BEAT_FLASHES,          int_to_str(rg.enable_beat_flash));
	config_setf(CONFIG_NAME_POTATO_PC,             int_to_str(rg.potato_pc_enabled));
	config_setf(CONFIG_NAME_DISABLE_MOUSE_BUTTONS, int_to_str(rg.disable_mouse_buttons));
	config_setf(CONFIG_NAME_GAME_MODE,             int_to_str(rg.mode));
	config_set (CONFIG_NAME_SKIN, rg.skin->name);
	config_set (CONFIG_NAME_USERNAME, rg.client->me->name);
	if(rg.client->me->pass_hash != NULL) config_set(CONFIG_NAME_PASS_HASH, rg.client->me->pass_hash);
}

void tick_fade()
{
	float fade_ms = FADE_MS * (rg.screen == screen_song_select ? 2 : 1);
	if(rg.frames == 0) timer_restart(rg.fade_clock);
	float fade = timer_milliseconds(rg.fade_clock);
	float fadec = clamp(fade, 0, fade_ms );
	float ffade = apply_easing(easing_type_out, fadec, 0, 255, fade_ms);
	if(rg.fade_state == fade_out) rg.fade_alpha = ffade;
	else rg.fade_alpha = 255 - ffade;
	color4 col = rg.screen == screen_song_select ? color4_gen_alpha(0, 0, 0, rg.fade_alpha) : color4_gen_alpha(0, 0, 0, rg.fade_alpha);

	if(fade_ms > 1) GL_RECT2BOX(FLOATRECT(0, 0, rg.win_width, rg.win_height), col);
	if(rg.fade_state != fade_done)
	{
		if(fade > fade_ms)
		{
			current_input_box = NULL;
			if(rg.fade_state == fade_out)
			{
				screen_type old_screen = rg.screen;
				if(rg.to_screen == screen_exit) rg.is_open = 0;
				else if(rg.to_screen == screen_fullscreen_switch)
				{
					rg.fullscreen_enabled = !rg.fullscreen_enabled;
					rg.do_fullscreen_switch = 1;
					rg.screen = old_screen;
					rg.to_screen = old_screen;
				}
				else if(rg.to_screen == screen_mode_switch)
				{
					rg.mode = rg.to_mode;
					rg.screen = old_screen;
					rg.to_screen = old_screen;
					song_clear_explosions(rg.song);
					song_sort_objects(rg.song);
				}
				else if(rg.to_screen == screen_tog_login_win)
				{
					rg.screen = old_screen;
					rg.to_screen = old_screen;
					rg.login_win->visible = !(rg.login_win->visible);
				}
				else
				{
					rg.last_screen = rg.screen;
					rg.screen = rg.to_screen;
					
					if(old_screen == screen_player)
					{
						if(!rg.replay_mode_enabled && !rg.editor_testing_song) song_save_score(rg.song);
						rg.replay_mode_enabled = 0;
						rg.bot_enabled = 0;
						rg.editor_testing_song = 0;

						if(rg.to_screen == screen_editor) song_all_sliders_del_textures(rg.song);
					}
					
					if(rg.to_screen == screen_player)
					{
						if(old_screen == screen_editor)
						{
							rg.editor_testing_song = 1;
							if(old_screen == screen_editor)
							{
								rg.bot_enabled = 0;
								rg.replay_mode_enabled = 0;
							}
							song_all_sliders_del_textures(rg.song);
							song_reset_object_state(rg.song);
							if(rg.song->music_state == song_paused) song_play(rg.song);
						}

						if(!rg.editor_testing_song) song_restart(rg.song);
						else
						{
							song_reset_object_state(rg.song);
							song_reset_score(rg.song);
						}
					}
					
					if(rg.to_screen == screen_song_select)
					{
						if(old_screen == screen_editor) song_load(rg.song);
						if(rg.song->music_state == song_paused) song_play(rg.song);
						song_select_refresh(1);
						timer_restart(ss->scroll_clock);
					}

					if(old_screen == screen_song_select)
					{
						if(rg.to_screen == screen_editor) song_stop(rg.song);
					}
					
					if(rg.to_screen == screen_import_editor)
					{
						if(ss->load_next != NULL)
						{
							if(!song_select_load(ss->load_next)) song_new(rg.song, ss->load_next->name);
							old_screen = screen_song_select;
							rg.screen = screen_editor;
							rg.to_screen = screen_editor;
							apply_volume();
							song_play(rg.song);
							ss->load_next = NULL;
						}
						else
						{
							rg.screen = old_screen;
							rg.to_screen = old_screen;
						}
					}

					if(old_screen == screen_song_select && (rg.to_screen == screen_player || rg.to_screen == screen_editor))
					{
						if(ss->load_next != NULL)
						{
							song_free(rg.song);
							rg.song = song_init(ss->load_next->name, ss->load_next->folder_loc, ss->load_next->data_file_loc);
							if(!song_load(rg.song))
							{
								song_new(rg.song, ss->load_next->name);
								rg.screen = screen_editor;
								rg.to_screen = screen_editor;
							}
							else song_preview_play(rg.song);
							ss->load_next = NULL;
						}
					}
					
					if(old_screen == screen_timing_sections)
					{
						song_sort_timing_sections(rg.song);
						song_apply_timing_sec(rg.song, rg.song->current_time_ms);
						song_all_sliders_recalc_paths(rg.song);
						song_all_sliders_del_textures(rg.song);
					}

					if(rg.to_screen == screen_editor)
					{
#ifndef RELEASE
						if(old_screen == screen_main_menu)
						{
							song_free(rg.song);
							rg.song = song_init("adhoc edit", "", "adhoc_edit.song");
							song_load(rg.song);
						}
#endif

						if(old_screen != screen_timing_sections && old_screen != screen_player)
						{
							song_sort_timing_sections(rg.song);
							song_all_sliders_recalc_paths(rg.song);
							song_all_sliders_del_textures(rg.song);
							song_set_time(rg.song, rg.song->time_ms_on_editor_exit);
						}

						song_align_ctms(rg.song);
						song_reset_object_state(rg.song);

						if(rg.song->game_mode_objects->count < 1)
						{
							apply_volume();
							song_play(rg.song);
						}
					}
					
					if(rg.to_screen == screen_editor || rg.to_screen == screen_player) song_sort_objects(rg.song);
					
					if
					(
						(
							old_screen == screen_options
							&&
							rg.to_screen == screen_main_menu
						)
						||
						(
							old_screen == screen_set_keys_ouendan
							||
							old_screen == screen_set_keys_mania
						)
					)
					{
						write_config();
					}

					if
					(
						rg.to_screen == screen_set_keys_ouendan
						||
						rg.to_screen == screen_set_keys_mania
					) rg.set_keys_state = 0;
				}
				apply_cursor();
				timer_restart(rg.fade_clock);
				rg.fade_state = fade_in;
			}
			else if(rg.fade_state == fade_in) rg.fade_state = fade_done;
		}
	}
}

void escape()
{
	if(rg.login_win->visible)
	{
		change_screen(screen_tog_login_win);
		return;
	}
	if(rg.editor_settings_win->visible)
	{
		rg.editor_settings_win->visible = 0;
		return;
	}
	if(rg.global_popup->is_opened) return;
	current_input_box = NULL;
#ifndef RELEASE
	if(rg.screen == screen_main_menu) change_screen(screen_editor);
	else
#endif
	if(rg.screen == screen_options) change_screen(screen_main_menu);
	else if
	(
		rg.screen == screen_set_keys_ouendan
		||
		rg.screen == screen_set_keys_mania
	) change_screen(screen_options);
	else if(rg.screen == screen_song_select) change_screen(screen_main_menu);
	else if(rg.screen == screen_player)
	{
		if(rg.editor_testing_song) change_screen(screen_editor);
		else if(rg.replay_mode_enabled) change_screen(screen_song_select);
		else change_screen(screen_final_score);
	}
	else if(rg.screen == screen_final_score) change_screen(screen_song_select);
	else if(rg.screen == screen_editor)
	{
		if(rg.song->selected_object != NULL || rg.song->selected_objects->count > 0)
		{
			song_unselect_object(rg.song);
			song_clear_selected_objects(rg.song);
			rg.song->tl.selected_obj = NULL;
			rg.song->tl.moved_object = 0;
		}
		else if(rg.editor_menu->activated) rg.editor_menu->activated = 0;
		else if(current_context_menu != NULL) current_context_menu->activated = 0;
		else
		{
			rg.editor_settings_win->visible = !rg.editor_settings_win->visible;
			gui_win_center(rg.editor_settings_win);
		}
	}
	else if(rg.screen == screen_timing_sections) change_screen(rg.last_screen);
}

TIME_VAR beat_length_ms_to_beats_per_minute(TIME_VAR beat_length_ms)
{
	return 60000 / beat_length_ms;
}

TIME_VAR beats_per_minute_to_beat_length_ms(TIME_VAR beats_per_minute)
{
    return (60 / beats_per_minute) * 1000;
}

char *ms_to_hr_time(TIME_VAR ms)
{
	int sec = ms / 1000;
	return dupfmt("%02d:%02d", sec / 60, sec % 60);
}

char change_skin(char *name)
{
	char ret = 0;
	char *dir = dupfmt("%s/%s", SKIN_SKINS_DIR, name);
	if(make_dir(dir) == -1)
	{
		skin_free(rg.skin);
		rg.skin = skin_init(name);
		ret = 1;
	}
	free(dir);
	song_all_sliders_del_textures(rg.song);
	return ret;
}

void change_screen(screen_type to_screen)
{
	if(rg.screen == rg.to_screen) timer_restart(rg.fade_clock);
	rg.fade_state = fade_out;
	rg.to_screen = to_screen;
}

void apply_cursor()
{
	char cbrm = (rg.bot_enabled || rg.replay_mode_enabled) && rg.screen == screen_player;
	if(cbrm)
	{
		SDL_ShowCursor(1);
		rg.hide_game_cursor = 0;
	}
	else
	{
		char show_os_cursor = 0;
		if(rg.screen != screen_player) show_os_cursor = 1;
		SDL_ShowCursor(show_os_cursor);
		rg.hide_game_cursor = show_os_cursor;
	}
}

void editor_settings_tick(struct gui_win *ptr)
{
	if(rg.resized) gui_win_center(ptr);
#define ADD_PADDING yv += 20
	int yv = ptr->pos.y + 50;
	int xo = ptr->pos.x + 5;

	if(current_input_box != rg.cs_input)
	{
		char *cs_str = float_to_str(rg.song->raw_object_size);
		input_box_set_text(rg.cs_input, cs_str);
		free(cs_str);
	}
	else rg.song->raw_object_size = clamp(atof(rg.cs_input->text), SONG_MIN_RAW_OBJECT_SIZE, SONG_MAX_RAW_OBJECT_SIZE);
	rg.cs_input->blink_interval_ms = rg.song->beat_length_ms;
	input_box_update_vars(rg.cs_input, xo, yv, rg.cs_input->box_width_pixels);
	input_box_update(rg.cs_input);
	input_box_render(rg.cs_input);
	yv += 30;
	ADD_PADDING;

	if(current_input_box != rg.bg_fname_input) input_box_set_text(rg.bg_fname_input, rg.song->bg_filename);
	else
	{
		char diff = !strs_are_equal(rg.bg_fname_input->text, rg.song->bg_filename);
		if(rg.song->bg_filename) free(rg.song->bg_filename);
		rg.song->bg_filename = dupe_str(rg.bg_fname_input->text);
		if(diff) song_reload_bg(rg.song);
	}
	rg.bg_fname_input->blink_interval_ms = rg.song->beat_length_ms;
	input_box_update_vars(rg.bg_fname_input, xo, yv, rg.bg_fname_input->box_width_pixels);
	input_box_update(rg.bg_fname_input);
	input_box_render(rg.bg_fname_input);
	yv += 30;
	ADD_PADDING;

	button_set_pos(rg.grid_res_menu_button, xo, yv);
	button_update(rg.grid_res_menu_button);
	button_render(rg.grid_res_menu_button);

	if(rg.grid_res_menu_button->clicked) context_menu_activate(rg.grid_res_cmenu, mouse_x, mouse_y);
	context_menu_update(rg.grid_res_cmenu);
	int cmgc = context_menu_get_clicked_option(rg.grid_res_cmenu);
	if(cmgc != -1)
	{
		if(cmgc == 0) rg.song->grid_resolution = 16;
		if(cmgc == 1) rg.song->grid_resolution = 32;
		if(cmgc == 2) rg.song->grid_resolution = 64;
		if(cmgc == 3) rg.song->grid_resolution = 128;
		if(cmgc == 4) rg.song->grid_resolution = 256;
		if(cmgc == 5) rg.song->grid_resolution = SONG_PFW;
	}
	yv += 30;
	ADD_PADDING;

	render_change_mode_button(xo, yv);

	yv = ptr->pos.y + 50;
	xo = ptr->pos.x + 200;

	button_set_pos(rg.goto_timing_points_button, xo, yv);
	button_update(rg.goto_timing_points_button);
	if(rg.goto_timing_points_button->clicked) change_screen(screen_timing_sections);
	button_render(rg.goto_timing_points_button);
	yv += 30;
	ADD_PADDING;

	button_set_pos(rg.save_song_button, xo, yv);
	button_update(rg.save_song_button);
	if(rg.save_song_button->clicked) song_save(rg.song);
	button_render(rg.save_song_button);
	yv += 30;
	ADD_PADDING;

	/*test_song_button->enabled = (rg.song->current_time_ms < rg.song->song_end_ms);*/
	button_set_pos(rg.test_song_button, xo, yv);
	button_update(rg.test_song_button);
	if(rg.test_song_button->clicked) change_screen(screen_player);
	button_render(rg.test_song_button);
	yv += 30;
	ADD_PADDING;

	button_set_pos(rg.save_quit_button, xo, yv);
	button_update(rg.save_quit_button);
	if(rg.save_quit_button->clicked)
	{
		song_save(rg.song);
		change_screen(screen_song_select);
	}
	button_render(rg.save_quit_button);
	yv += 30;
	ADD_PADDING;

	button_set_pos(rg.just_quit_button, xo, yv);
	button_update(rg.just_quit_button);
	if(rg.just_quit_button->clicked)
	{
		if(rg.song->edit_count > 0)
		{
			popup_open(rg.global_popup, popup_type_yes_no, "you forgot to save... exit anyway?");
			rg.global_popup->yes_no_callback = &just_quit_pop_cback;
		}
		else change_screen(screen_song_select);
	}
	button_render(rg.just_quit_button);

	yv = ptr->pos.y + 50;
	xo = ptr->pos.x + 120;

	if(current_input_box != rg.ar_input)
	{
		char *ar_str = float_to_str(rg.song->raw_approach_rate);
		input_box_set_text(rg.ar_input, ar_str);
		free(ar_str);
	}
	else rg.song->raw_approach_rate = clamp(atof(rg.ar_input->text), SONG_MIN_RAW_APPROACH_RATE, SONG_MAX_RAW_APPROACH_RATE);
	rg.ar_input->blink_interval_ms = rg.song->beat_length_ms;
	input_box_update_vars(rg.ar_input, xo, yv, rg.ar_input->box_width_pixels);
	input_box_update(rg.ar_input);
	input_box_render(rg.ar_input);
	yv += 30;
	ADD_PADDING;

	if(current_input_box != rg.speed_input)
	{
		char *si_str = float_to_str(rg.song->speed_multiplier);
		input_box_set_text(rg.speed_input, si_str);
		free(si_str);
	}
	else song_set_speed(rg.song, atof(rg.speed_input->text));
	rg.speed_input->blink_interval_ms = rg.song->beat_length_ms;
	input_box_update_vars(rg.speed_input, xo, yv, rg.speed_input->box_width_pixels);
	input_box_update(rg.speed_input);
	input_box_render(rg.speed_input);
	yv += 30;
	ADD_PADDING;

	if(current_input_box != rg.song_offset_input)
	{
		char *sgo_str = float_to_str(rg.song->song_global_offset);
		input_box_set_text(rg.song_offset_input, sgo_str);
		free(sgo_str);
	}
	else rg.song->song_global_offset = clamp(atof(rg.song_offset_input->text), -(rg.song->length_ms), rg.song->length_ms);
	rg.song_offset_input->blink_interval_ms = rg.song->beat_length_ms;
	input_box_update_vars(rg.song_offset_input, xo, yv, rg.song_offset_input->box_width_pixels);
	input_box_update(rg.song_offset_input);
	input_box_render(rg.song_offset_input);
	yv += 30;
	ADD_PADDING;
		
	context_menu_render(rg.cs_input->menu);
	context_menu_render(rg.bg_fname_input->menu);
	context_menu_render(rg.grid_res_cmenu);
	context_menu_render(rg.ar_input->menu);
	context_menu_render(rg.speed_input->menu);
	context_menu_render(rg.song_offset_input->menu);

	render_back_button();
#undef ADD_PADDING
}

int main(int argc, char *argv[])
{
	rg.time_open = timer_init();

	current_input_box = NULL;
	current_scroll_bar = NULL;
	current_context_menu = NULL;
	gui_win_is_grabbed = 0;
	gui_win_hovered = 0;

	rg.is_open = 1;

	rg.fps_limit = 500;
	rg.frame_delta_ms = 1;

	fps_clock = timer_init();
	fps_clock_interval = timer_init();
	rg.frames_per_second = 0.0f;
	rg.framerate_display_enabled = 0;

	rg.focus = 1;
	rg.focus_frames = 0;
	rg.frames = 0;

	rg.resized = 1;

	memset(&rg.kp, 0, KEY_COUNT * sizeof(int));
	memset(&rg.kr, 0, KEY_COUNT * sizeof(char));

	mouse_state_x = 0;
	mouse_state_y = 0;
	mouse_x = 0;
	mouse_y = 0;
	gui_mouse_x = 0;
	gui_mouse_y = 0;
	mouse_x_on_click = 0;
	mouse_x_on_click_real = 0;
	mouse_y_on_click = 0;
	mouse_y_on_click_real = 0;
	mouse_delta_x = 0;
	mouse_delta_y = 0;
	mouse_drag_delta_x = 0;
	mouse_drag_delta_y = 0;
	mouse_release_velocity_x = 0;
	mouse_release_velocity_y = 0;
	mouse_vel_slow_ms_x = 0;
	mouse_vel_slow_ms_y = 0;
	mouse_release_velocity_timer = timer_init();
	mouse_inside_window = 0;
	mouse_outside_window_during_drag = 0;
	mouse_moved = 0;
	mouse_dragged = 0;

	mouse_state_left = 0;
	mouse_state_middle = 0;
	mouse_state_right = 0;
	left_click = 0;
	left_click_released = 0;
	middle_click = 0;
	middle_click_released = 0;
	right_click = 0;
	right_click_released = 0;
	left_click_held = timer_init();
	middle_click_held = timer_init();
	right_click_held = timer_init();
	mouse_moved_clock = timer_init();
	mouse_button_released = 0;
	cursor_type = SDL_SYSTEM_CURSOR_ARROW;

	rg.gui_view_scale = 1.0f;
	rg.gui_view_xoff = 0;
	rg.gui_view_yoff = 0;
	rg.gui_render_ptr = NULL;
	rg.gui_render_end_of_frame = NULL;

	rg.replay_to_watch = NULL;

	notifications_list = list_init();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

	if(TTF_Init() < 0)
	{
		rg.font = NULL;
		log_error("font subsystem failed to initialize.");
	}

	rg.font = TTF_OpenFont(RES_DIR "font.otf", 16);
	if(rg.font == NULL) log_error("failed to load font!");

	center_status_text = text_init("", 0, 0, col_cyan);
	center_status_text->draw_bg = 1;
	center_status_timer = timer_init();

	textures_init();

	cons_ptr = console_init();
#ifndef RELEASE
	console_command_register(cons_ptr, "fps", "change fps limit", &change_fps);
	console_command_register(cons_ptr, "res", "change window resolution", &set_res_cmd);
#endif

	gui_win_man_init();
	rg.global_popup = popup_init();

	fps_text = text_init("", 0, 0, col_yellow);
	fps_text->draw_bg = 1;
	timer *delta_clock = timer_init();

	timer *click_block_timer = timer_init();

	rg.win = NULL;
	rg.context = NULL;

	make_dir(SONGS_DIR);
	make_dir(DATA_DIR);
	make_dir(SCORES_DIR);

	audio_engine_init();

	config_init();

	config_set_if_missing(CONFIG_NAME_FULLSCREEN,            "0"       );
	config_set_if_missing(CONFIG_NAME_OUENDAN_KEY_1,         "29"      );
	config_set_if_missing(CONFIG_NAME_OUENDAN_KEY_2,         "27"      );
	config_set_if_missing(CONFIG_NAME_MANIA_KEY_1,           "7"       );
	config_set_if_missing(CONFIG_NAME_MANIA_KEY_2,           "9"       );
	config_set_if_missing(CONFIG_NAME_MANIA_KEY_3,           "13"      );
	config_set_if_missing(CONFIG_NAME_MANIA_KEY_4,           "14"      );
	config_set_if_missing(CONFIG_NAME_BG_DIM_LEVEL,          "60"      );
	config_set_if_missing(CONFIG_NAME_MASTER_VOL,            "100"     );
	config_set_if_missing(CONFIG_NAME_MUSIC_VOL,             "25"      );
	config_set_if_missing(CONFIG_NAME_EFFECT_VOL,            "50"      );
	config_set_if_missing(CONFIG_NAME_BEAT_FLASHES,          "0"       );
	config_set_if_missing(CONFIG_NAME_POTATO_PC,             "0"       );
	config_set_if_missing(CONFIG_NAME_DISABLE_MOUSE_BUTTONS, "0"       );
	config_set_if_missing(CONFIG_NAME_GAME_MODE,             "0"       );
	config_set_if_missing(CONFIG_NAME_SKIN,                  "default" );
	config_set_if_missing(CONFIG_NAME_USERNAME,              ""        );
	config_set_if_missing(CONFIG_NAME_PASS_HASH,             ""        );

	rg.fullscreen_enabled = atoi(config_get(CONFIG_NAME_FULLSCREEN));
	if(rg.fullscreen_enabled) rg.do_fullscreen_switch = 1;
	rg.ouendan_keycode_1     =     clamp(atoi(config_get(CONFIG_NAME_OUENDAN_KEY_1         )), 4, KEY_COUNT);
	rg.ouendan_keycode_2     =     clamp(atoi(config_get(CONFIG_NAME_OUENDAN_KEY_2         )), 4, KEY_COUNT);
	rg.mania_keycode_1       =     clamp(atoi(config_get(CONFIG_NAME_MANIA_KEY_1           )), 4, KEY_COUNT);
	rg.mania_keycode_2       =     clamp(atoi(config_get(CONFIG_NAME_MANIA_KEY_2           )), 4, KEY_COUNT);
	rg.mania_keycode_3       =     clamp(atoi(config_get(CONFIG_NAME_MANIA_KEY_3           )), 4, KEY_COUNT);
	rg.mania_keycode_4       =     clamp(atoi(config_get(CONFIG_NAME_MANIA_KEY_4           )), 4, KEY_COUNT);
	rg.bg_dim_level          =     clamp(atof(config_get(CONFIG_NAME_BG_DIM_LEVEL          )), 0, 100);
	rg.master_vol            =     clamp(atof(config_get(CONFIG_NAME_MASTER_VOL            )), 0, 100);
	rg.music_vol             =     clamp(atof(config_get(CONFIG_NAME_MUSIC_VOL             )), 0, 100);
	rg.effect_vol            =     clamp(atof(config_get(CONFIG_NAME_EFFECT_VOL            )), 0, 100);
	rg.enable_beat_flash     =     atoi(config_get(CONFIG_NAME_BEAT_FLASHES                )) > 0;
	rg.potato_pc_enabled     =     atoi(config_get(CONFIG_NAME_POTATO_PC                   )) > 0;
	rg.disable_mouse_buttons =     atoi(config_get(CONFIG_NAME_DISABLE_MOUSE_BUTTONS       )) > 0;
	rg.mode                  =     clamp(atoi(config_get(CONFIG_NAME_GAME_MODE             )), 0, game_mode_count - 1);
	rg.to_mode = rg.mode;
	rg.fade_clock = timer_init();
	rg.cursor_expand_timer = timer_init();
	rg.cursor_trail_timer = timer_init();
	timer_set_milliseconds(rg.cursor_expand_timer, CURSOR_EXPAND_MS);
	rg.fade_alpha = 255;
	rg.enable_slider_debug = 0;

	v2i numsize;
	TTF_SizeText(rg.font, "100.00", &(numsize.x), &(numsize.y));
	numsize.x += 2;

	rg.song_sel_button = button_init(0, 0, "song selection", 1, 0, NULL);
	rg.options_button = button_init(0, 0, "game options", 1, 0, NULL);
	rg.exit_button = button_init(0, 0, "exit game", 1, 0, NULL);
	
	rg.back_button = button_init(0, 0, "go back", 1, 0, NULL);
	rg.sbar = scroll_bar_init(0, 0, 0, 0);

	rg.cs_input = input_box_init(0, 0, numsize.x, "", "object size");
	rg.cs_input->force_numbers_only = 1;
	rg.cs_input->show_label_above = 1;
	rg.ar_input = input_box_init(0, 0, numsize.x, "", "approach speed");
	rg.ar_input->force_numbers_only = 1;
	rg.ar_input->show_label_above = 1;
	rg.bg_fname_input = input_box_init(0, 0, numsize.x, "", "bg filename");
	rg.bg_fname_input->show_label_above = 1;
	rg.speed_input = input_box_init(0, 0, numsize.x, "", "speed");
	rg.speed_input->force_numbers_only = 1;
	rg.speed_input->show_label_above = 1;
	rg.song_offset_input = input_box_init(0, 0, numsize.x, "", "global offset");
	rg.song_offset_input->force_numbers_only = 1;
	rg.song_offset_input->show_label_above = 1;
	rg.save_song_button = button_init(0, 0, "save your song", 1, 0, NULL);
	rg.test_song_button = button_init(0, 0, "test play song", 1, 0, NULL);
	rg.save_quit_button = button_init(0, 0, "save & quit", 1, 0, NULL);
	rg.just_quit_button = button_init(0, 0, "quit without saving", 1, 0, NULL);
	rg.goto_timing_points_button = button_init(0, 0, "edit timing points", 1, 0, NULL);
	rg.grid_res_menu_button = button_init(0, 0, "grid res", 1, 0, NULL);
	rg.grid_res_cmenu = context_menu_init(0);
	context_menu_add_option(rg.grid_res_cmenu, "16", 0);
	context_menu_add_option(rg.grid_res_cmenu, "32", 0);
	context_menu_add_option(rg.grid_res_cmenu, "64", 0);
	context_menu_add_option(rg.grid_res_cmenu, "128", 0);
	context_menu_add_option(rg.grid_res_cmenu, "256", 0);
	context_menu_add_option(rg.grid_res_cmenu, "no grid", 0);
	rg.cs_input->before = rg.song_offset_input;
	rg.cs_input->next = rg.bg_fname_input;
	rg.bg_fname_input->before = rg.cs_input;
	rg.bg_fname_input->next = rg.ar_input;
	rg.ar_input->before = rg.bg_fname_input;
	rg.ar_input->next = rg.speed_input;
	rg.speed_input->before = rg.ar_input;
	rg.speed_input->next = rg.song_offset_input;
	rg.song_offset_input->before = rg.speed_input;
	rg.song_offset_input->next = rg.cs_input;
	
	float opts_bar_width = 100;
	rg.opts_master_vol_bar = slider_bar_init("master vol", 0, 0, opts_bar_width, rg.master_vol);
	rg.opts_music_vol_bar = slider_bar_init("music vol", 0, 0, opts_bar_width, rg.music_vol);
	rg.opts_effect_vol_bar = slider_bar_init("effect vol", 0, 0, opts_bar_width, rg.effect_vol);
	rg.opts_bg_dim_bar = slider_bar_init("bg dim", 0, 0, opts_bar_width, rg.bg_dim_level);

	rg.opts_beat_flash_cbox =         checkbox_init("beat flashes", 0, 0, rg.enable_beat_flash);
	rg.opts_fullscreen_cbox =         checkbox_init("fullscreen", 0, 0, rg.fullscreen_enabled);
	rg.opts_bot_cbox =                checkbox_init("bot enabled", 0, 0, rg.bot_enabled);
	rg.opts_disable_mouse_btns_cbox = checkbox_init("disable mouse buttons", 0, 0, rg.disable_mouse_buttons);

	rg.opts_goto_set_keys_ouendan = button_init(0, 0, "set ouendan keys", 1, 0, NULL);
	rg.opts_goto_set_keys_mania = button_init(0, 0, "set mania keys", 1, 0, NULL);

	rg.editor_menu_btn = button_init(0, 0, "menu", 1, 0, NULL);
	rg.editor_menu = context_menu_init(0);
	context_menu_add_option(rg.editor_menu, "settings", 0);
	context_menu_add_option(rg.editor_menu, "play", 0);
	context_menu_add_option(rg.editor_menu, "pause", 0);
	context_menu_add_option(rg.editor_menu, "restart", 0);
	rg.editor_menu_delete_all_btn = context_menu_add_option(rg.editor_menu, "delete all (hold 5s)", 0)->option_button;
	context_menu_add_option(rg.editor_menu, "-------", 1);
	rg.editor_menu_object_type_btn = context_menu_add_option(rg.editor_menu, "sel obj type", 0)->option_button;
	rg.editor_menu_delete_all_btn->col = col_red;
	rg.editor_menu_delete_all_btn->held_time_min = 5000;
	button_set_to_oc_cmenu(rg.editor_menu_object_type_btn);
	context_menu_add_option(rg.editor_menu_object_type_btn->on_click_cmenu, "single", 0);
	context_menu_add_option(rg.editor_menu_object_type_btn->on_click_cmenu, "long", 0);
	context_menu_add_option(rg.editor_menu_object_type_btn->on_click_cmenu, "spin", 0);

	rg.editor_settings_win = gui_win_init("song settings", "rg", 0, 0, 400, 300, col_white, col_black, 1, &gui_win_empty_func, &editor_settings_tick, &gui_win_empty_func);
	rg.editor_settings_win->allow_drag_off_screen = 1;

	rg.set_keys_state = 0;

#ifndef RELEASE
    console_command_register(cons_ptr, "rg", "change rg song props", &rg_cmd);
#endif

	rg.fade_state = fade_in;
	rg.cursor_state = cursor_neutral;

#ifndef RELEASE
	rg.screen = screen_main_menu;
#else
    rg.screen = screen_main_menu;
#endif

	rg.to_screen = rg.screen;
	rg.last_screen = rg.screen;
	rg.hide_game_cursor = 1;

	rg.replay_mode_enabled = 0;
    rg.ouendan_click_1 = 0;
    rg.ouendan_click_2 = 0;
	rg.ouendan_click_1_frame_1_time_ms = 0.0f;
	rg.ouendan_click_2_frame_1_time_ms = 0.0f;
	rg.mania_click_1 = 0;
	rg.mania_click_2 = 0;
	rg.mania_click_3 = 0;
	rg.mania_click_4 = 0;
	rg.mania_key_click_count = 0;
    rg.skin = skin_init(config_get(CONFIG_NAME_SKIN));
    rg.mop_x = SONG_PFW / 2;
    rg.mop_y = SONG_PFH / 2;
	rg.cursor_trail_points = list_init();
	rg.mop_xoc = rg.mop_x;
	rg.mop_yoc = rg.mop_y;
	rg.mop_fdx = 0;
	rg.mop_fdy = 0;
    rg.is_mouse_on_playfield = 0;
	rg.is_mouse_on_click_op = 0;
	rg.mouse_x_bot = rg.mop_x;
	rg.mouse_y_bot = rg.mop_y;
    rg.song = song_init(SONG_UNKNOWN_NAME, "", "");

	song_select_init();

	rg.login_win = gui_win_init("online login", "rg", -1, -1, 200, 150, col_white, col_black, 1, login_win_init, login_win_tick, login_win_free);
#ifdef RELEASE
	if(config_get(CONFIG_NAME_PASS_HASH)[0] == '\0') rg.login_win->visible = 1;
#endif
	rg.login_win->before_rend_func = &bef_rend_func;

#ifndef RELEASE
	rg.server = online_server_init();
	online_server_start(rg.server);
#endif

	rg.client = online_client_init();

	if(config_get(CONFIG_NAME_PASS_HASH)[0] != '\0')
	{
		online_client_set_name_and_pass_hash(rg.client, config_get(CONFIG_NAME_USERNAME), config_get(CONFIG_NAME_PASS_HASH));
		rg.client->after_conn_act = online_client_after_connect_action_login;
		online_client_connect(rg.client);
	}
	
	song_select_refresh(1);

#ifdef USING_WINDOWS
	discord_init();
#endif

	rg.win_width_prev = 1280;
	rg.win_height_prev = 720;
	float new_width;
	float new_height;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);
#ifdef RELEASE
	char *title = "aoeu's rhythm game";
#else
	char *title = "aoeu's rhythm game DEBUG BUILD " COMPILE_MSG;
#endif
	if(rg.do_fullscreen_switch)
	{
		rg.win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dm.w, dm.h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIDDEN);
		new_width = dm.w;
		new_height = dm.h;
	}
	else
	{
		rg.win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, rg.win_width_prev, rg.win_height_prev, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
#ifndef RELEASE
		| SDL_WINDOW_RESIZABLE
#endif
		);
		new_width = rg.win_width_prev;
		new_height = rg.win_height_prev;
	}
	rg.do_fullscreen_switch = 0;
	rg.win_width_real = new_width;
	rg.win_width = rg.win_width_real;
	rg.win_width_mid = rg.win_width / 2;
	rg.win_height_real = new_height;
	rg.win_height = rg.win_height_real;
	rg.win_height_mid = rg.win_height / 2;
	cons_ptr->vertical_drop = -rg.win_height;
	rg.resized = 1;
	rg.context = SDL_GL_CreateContext(rg.win);
	glewInit();

	glyphs_init();

	IMG_Init(IMG_INIT_PNG);

	widgets_init();

	SDL_Cursor *arrow_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	SDL_Cursor *horiz_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);

	while(rg.is_open)
	{
		if(rg.do_fullscreen_switch)
		{
			if(rg.fullscreen_enabled) SDL_SetWindowFullscreen(rg.win, SDL_WINDOW_FULLSCREEN_DESKTOP);
			else if(rg.frames > 0)
			{
				SDL_SetWindowFullscreen(rg.win, 0);
				SDL_SetWindowSize(rg.win, rg.win_width_prev, rg.win_height_prev);
				SDL_SetWindowPosition(rg.win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#ifndef RELEASE
				SDL_SetWindowResizable(rg.win, SDL_TRUE);
#endif
			}
			rg.do_fullscreen_switch = 0;
			rg.resized = 1;
		}

		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0, 0, 0, 0);

		if(current_context_menu && !(current_context_menu->activated)) current_context_menu = NULL;

		if(rg.focus)
		{
			size_t key_index;
			const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
			for(key_index = 0; key_index < KEY_COUNT; key_index++)
			{
				int *pressed_ptr = &rg.kp[key_index];
				char *released_ptr = &rg.kr[key_index];

				if(*released_ptr) *released_ptr = 0;
				char pressed_currently = keyboard_state[key_index];

				if(!pressed_currently && (*pressed_ptr) > 0)
				{
					*pressed_ptr = 0;
					*released_ptr = 1;
				}
				else if(pressed_currently) *pressed_ptr = (*pressed_ptr) + 1;
			}
		}

		mouse_button_released = 0;

		events();

		if(rg.kr[SDL_SCANCODE_LCTRL] || rg.kr[SDL_SCANCODE_LALT])
		{
			float d2z = get_distf(rg.gui_view_xoff, rg.gui_view_yoff, 0.0f, 0.0f);
			if(d2z <= 10.0f && d2z != 0.0f)
			{
				rg.gui_view_xoff = 0.0f;
				rg.gui_view_yoff = 0.0f;
			}
		}

		if(rg.resized) SDL_GetWindowSize(rg.win, &rg.win_width_real, &rg.win_height_real);

		left_click_released = 0;
		middle_click_released = 0;
		right_click_released = 0;

		if(rg.focus)
		{
			int cbt = clamp(timer_milliseconds(click_block_timer), 0, CLICK_BLOCK_TIME_MS);
			char ccc = mouse_state_left || mouse_state_middle || mouse_state_right;
			char good = (!current_context_menu || (current_context_menu->activated && current_context_menu->hovered));

			if(ccc && cbt < CLICK_BLOCK_TIME_MS) timer_restart(click_block_timer);

			if(current_context_menu && current_context_menu->button_click_avoided == 0 && !good && ccc)
			{
				current_context_menu->activated = 0;
				current_context_menu = NULL;
				timer_restart(click_block_timer);
			}
			else if(cbt >= CLICK_BLOCK_TIME_MS)
			{
				if(good && mouse_state_left) left_click++;
				else
				{
					if(good && left_click > 0) left_click_released = 1;
					left_click = 0;
					timer_restart(left_click_held);
				}

				if(good && mouse_state_middle) middle_click++;
				else
				{
					if(good && middle_click > 0) middle_click_released = 1;
					middle_click = 0;
					timer_restart(middle_click_held);
				}

				if(good && mouse_state_right) right_click++;
				else
				{
					if(good && right_click > 0) right_click_released = 1;
					right_click = 0;
					timer_restart(right_click_held);
				}
			}

			if(SDL_GetMouseFocus() != NULL) mouse_inside_window = 1;
			else mouse_inside_window = 0;
		}
		else
		{
			left_click = 0;
			left_click_released = 0;
			timer_restart(left_click_held);

			middle_click = 0;
			middle_click_released = 0;
			timer_restart(middle_click_held);

			right_click = 0;
			right_click_released = 0;
			timer_restart(right_click_held);
		}

		if(current_context_menu && mouse_button_released && !(current_context_menu->hovered)) current_context_menu->button_click_avoided = 0;

		if((left_click > 0 || middle_click > 0 || right_click > 0) && !mouse_inside_window) mouse_outside_window_during_drag = 1;
		else if(left_click + middle_click + right_click == 0) mouse_outside_window_during_drag = 0;

		if(left_click == 1 || middle_click == 1 || right_click == 1)
		{
			mouse_x_on_click_real = mouse_state_x;
			mouse_y_on_click_real = mouse_state_y;
		}

		float_rect view_rect = FLOATRECT(rg.gui_view_xoff, rg.gui_view_yoff, rg.win_width_real * rg.gui_view_scale, rg.win_height_real * rg.gui_view_scale);

		rg.win_width = view_rect.width;
		rg.win_width_mid = rg.win_width / 2;
		rg.win_height = view_rect.height;
		rg.win_height_mid = rg.win_height / 2;

		glViewport(0, 0, rg.win_width_real, rg.win_height_real);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(view_rect.left, view_rect.left + view_rect.width, view_rect.top + view_rect.height, view_rect.top);

		v2f mpf = V2F(scale_value_to(mouse_state_x, 0, rg.win_width_real, view_rect.left, view_rect.left + view_rect.width), scale_value_to(mouse_state_y, 0, rg.win_height_real, view_rect.top, view_rect.top + view_rect.height));
		mouse_delta_x = mpf.x - gui_mouse_x;
		mouse_delta_y = mpf.y - gui_mouse_y;
		if(mouse_delta_x != 0 || mouse_delta_y != 0) mouse_moved = 1;
		mouse_x = mpf.x;
		mouse_y = mpf.y;
		gui_mouse_x = mpf.x;
		gui_mouse_y = mpf.y;

		mouse_x_on_click = scale_value_to(mouse_x_on_click_real, 0, rg.win_width_real, view_rect.left, view_rect.left + view_rect.width);
		mouse_y_on_click = scale_value_to(mouse_y_on_click_real, 0, rg.win_height_real, view_rect.top, view_rect.top + view_rect.height);

		if(left_click || middle_click || right_click)
		{
			mouse_drag_delta_x = mouse_x - mouse_x_on_click;
			mouse_drag_delta_y = mouse_y - mouse_y_on_click;
		}
		else
		{
			mouse_drag_delta_x = 0;
			mouse_drag_delta_y = 0;
		}

		mouse_dragged = (mouse_drag_delta_x != 0 || mouse_drag_delta_y != 0);

		if(left_click || middle_click || right_click)
		{
			mouse_release_velocity_x -= (mouse_delta_x / 2) * 0.05;
			mouse_release_velocity_y -= (mouse_delta_y / 2) * 0.05;
		}
		else
		{
			mouse_vel_slow_ms_x = 0;
			mouse_vel_slow_ms_y = 0;
		}

		if(mouse_delta_x == 0.0f) mouse_vel_slow_ms_x += rg.frame_delta_ms;
		if(mouse_delta_y == 0.0f) mouse_vel_slow_ms_y += rg.frame_delta_ms;

		online_client_tick(rg.client);

		rg.gui_render_ptr = rg.song;

		char load_random =
		(
			rg.to_screen != screen_editor
			&&
			(rg.screen == screen_main_menu || rg.screen == screen_options)
			&&
			(
				rg.song->music_state != song_playing
				||
				!ae_music_handle_is_active(rg.song->music)
			)
		);

#ifndef RELEASE
		if(rg.kp[SDL_SCANCODE_LCTRL] && rg.kr[SDL_SCANCODE_M]) load_random = 1;
#endif

		if(load_random) song_select_load_random();

		song_tick_timing(rg.song);

		apply_volume();
	
		if(load_random || (rg.screen == screen_song_select && !ae_music_handle_is_active(rg.song->music))) song_preview_play(rg.song);

		if(!rg.replay_mode_enabled && !rg.bot_enabled)
		{
			if(rg.kp[rg.ouendan_keycode_1] || (!rg.disable_mouse_buttons && left_click)) rg.ouendan_click_1++;
			else rg.ouendan_click_1 = 0;
			if(rg.kp[rg.ouendan_keycode_2] || (!rg.disable_mouse_buttons && right_click)) rg.ouendan_click_2++;
			else rg.ouendan_click_2 = 0;
			if(rg.kp[rg.mania_keycode_1]) rg.mania_click_1++;
			else rg.mania_click_1 = 0;
			if(rg.kp[rg.mania_keycode_2]) rg.mania_click_2++;
			else rg.mania_click_2 = 0;
			if(rg.kp[rg.mania_keycode_3]) rg.mania_click_3++;
			else rg.mania_click_3 = 0;
			if(rg.kp[rg.mania_keycode_4]) rg.mania_click_4++;
			else rg.mania_click_4 = 0;

			rg.mania_key_click_count = ((rg.mania_click_1 > 0) + (rg.mania_click_2 > 0) + (rg.mania_click_3 > 0) + (rg.mania_click_4 > 0));
		}

		rg.win_width_fixed = rg.win_width;
		rg.win_height_fixed = rg.win_height;
		if(rg.win_width_fixed * 3 > rg.win_height_fixed * 4) rg.win_width_fixed = rg.win_height_fixed * 4 / 3;
		else rg.win_height_fixed = rg.win_width_fixed * 3 / 4;

		song_draw_background(rg.song);

		if(rg.screen != screen_editor) rg.editor_settings_win->visible = 0;

		if(rg.screen == screen_main_menu)
		{
			loaded_texture *ttex = rg.skin->title;
			GL_SCALE
			(
				rg.win_width_mid,
				(rg.win_height_mid / 4) + (ttex->surf->h / 2),
				scale_value_to
				(
					apply_easing
					(
						easing_type_out,
						fmodf
						(
							(
								ae_music_handle_get_time_ms(rg.song->music)
								-
								rg.song->timing_section_start_ms
								+
								rg.song->song_global_offset
							),
							rg.song->beat_length_ms
						),
						rg.song->beat_length_ms,
						rg.song->beat_length_ms,
						rg.song->beat_length_ms
					),
					0.0f,
					rg.song->beat_length_ms,
					1.75f,
					1.5f
				) * (rg.win_width_mid / ttex->surf->w)
			);
			textures_drawsc(ttex, -(ttex->surf->w / 2.0f), -(ttex->surf->h / 2.0f), 1.0f, 1.0f, col_white);
			GL_SCALE_END();

			float vert_padding = 20;
			float xo = vert_padding;
			float yv = rg.win_height - xo - rg.exit_button->size.y;

			button_set_pos(rg.exit_button, xo, yv);
			button_update(rg.exit_button);
			if(rg.exit_button->clicked) change_screen(screen_exit);
			button_render(rg.exit_button);
			yv -= rg.exit_button->size.y + vert_padding;

			button_set_pos(rg.options_button, xo, yv);
			yv -= rg.options_button->size.y + vert_padding;
			button_update(rg.options_button);
			if(rg.options_button->clicked) change_screen(screen_options);
			button_render(rg.options_button);

			button_set_pos(rg.song_sel_button, xo, yv);
			button_update(rg.song_sel_button);
			if(rg.song_sel_button->clicked)
			{
				change_screen(screen_song_select);
				timer_restart(ss->scroll_clock);
			}
			button_render(rg.song_sel_button);

			char logged_in = rg.client->connected && rg.client->authed;
			button *online_btn = ez_bp("rglbtn", logged_in ? "logout" : "online login", xo, vert_padding);
			if(online_btn->clicked)
			{
				if(logged_in) online_client_disconnect(rg.client, 0);
				else change_screen(screen_tog_login_win);
			}
		
			if(logged_in)
			{
				online_player *me = rg.client->me;
				char role_col = me->is_admin ? 'u' : me->is_mod ? 'z' : 'c';
				char rank_col = me->rank == 1 ? 'z' : me->rank > 10 ? 'w' : 'u';
				text3_fmt(vert_padding, online_btn->pos.y + online_btn->size.y + vert_padding, rg.win_width, 255/2, origin_left, origin_top, text3_justification_right,
					"name: ~%c%s~w\n"
					"score: %lu\n"
					"play count: %lu\n"
					"rank: ~%c#%lu~w\n",
					role_col,
					me->name,
					me->score,
					me->play_count,
					rank_col,
					me->rank
				);
			}

			float_rect ltr;
			char *dev_url = "https://github.com/aoeu1/aoeu_rg/";
			text3_fmt_rect(ltr, rg.win_width, rg.win_height, rg.win_width, 255, origin_right, origin_bottom, text3_justification_right, "~u%s", dev_url);
			char ltri = FR_CONTAINS2(ltr, mouse_x, mouse_y);
			if(ltri && left_click == 1) open_url("https://github.com/aoeu1/aoeu_rg/");
		}
		else if(rg.screen == screen_song_select)
		{
			song_select_tick();

			render_back_button();

			button *ref = ez_bp("ss_ref_btn", "refresh", rg.back_button->pos.x + (PADDING * 4) + rg.back_button->size.x, rg.back_button->pos.y);
			if(ref->clicked) song_select_refresh(0);
			render_change_mode_button(ref->pos.x + (PADDING * 4) + ref->size.x, ref->pos.y);
		}
		else if(rg.screen == screen_options)
		{
			int yv = 25;
			int xo = 25;

			int opts_text_height = ez_tp("opts_text", "game options", xo, yv, col_white, 0, 0)->size.y;
			yv += opts_text_height + 50;

			int ypad = 25;

			slider_bar_update(rg.opts_master_vol_bar);
			slider_bar_set_pos(rg.opts_master_vol_bar, xo, yv);
			if(left_click && rg.opts_master_vol_bar->hovered_oc) rg.master_vol = rg.opts_master_vol_bar->val;
			else slider_bar_set_val(rg.opts_master_vol_bar, rg.master_vol);
			slider_bar_render(rg.opts_master_vol_bar);
			yv += rg.opts_master_vol_bar->size.y + ypad;

			slider_bar_update(rg.opts_music_vol_bar);
			slider_bar_set_pos(rg.opts_music_vol_bar, xo, yv);
			if(left_click && rg.opts_music_vol_bar->hovered_oc) rg.music_vol = rg.opts_music_vol_bar->val;
			else slider_bar_set_val(rg.opts_music_vol_bar, rg.music_vol);
			slider_bar_render(rg.opts_music_vol_bar);
			yv += rg.opts_music_vol_bar->size.y + ypad;

			slider_bar_update(rg.opts_effect_vol_bar);
			slider_bar_set_pos(rg.opts_effect_vol_bar, xo, yv);
			if(left_click && rg.opts_effect_vol_bar->hovered_oc)
			{
				rg.effect_vol = rg.opts_effect_vol_bar->val;
				apply_volume();
				ae_sound_effect_play_no_restart(rg.skin->metronome);
			}
			else slider_bar_set_val(rg.opts_effect_vol_bar, rg.effect_vol);
			slider_bar_render(rg.opts_effect_vol_bar);
			yv += rg.opts_effect_vol_bar->size.y + ypad;

			slider_bar_update(rg.opts_bg_dim_bar);
			slider_bar_set_pos(rg.opts_bg_dim_bar, xo, yv);
			if(left_click && rg.opts_bg_dim_bar->hovered_oc) rg.bg_dim_level = rg.opts_bg_dim_bar->val;
			else slider_bar_set_val(rg.opts_bg_dim_bar, rg.bg_dim_level);
			slider_bar_render(rg.opts_bg_dim_bar);
			yv += rg.opts_bg_dim_bar->size.y + ypad;

			xo = rg.opts_bg_dim_bar->pos.x + rg.opts_bg_dim_bar->size.x + 50;
			yv = rg.opts_master_vol_bar->pos.y;

			checkbox_set_pos(rg.opts_beat_flash_cbox, xo, yv);
			checkbox_update(rg.opts_beat_flash_cbox);
			v2f opts_beat_flash_cbox_size = checkbox_render(rg.opts_beat_flash_cbox);
			if(rg.opts_beat_flash_cbox->clicked)
			{
				rg.enable_beat_flash = rg.opts_beat_flash_cbox->is_checked;
				if(rg.enable_beat_flash) popup_open(rg.global_popup, popup_type_dismiss, "WARNING: Beat flash may cause seizures.");
			}
			yv += opts_beat_flash_cbox_size.y + ypad;

			if(rg.opts_fullscreen_cbox->is_checked != rg.fullscreen_enabled) rg.opts_fullscreen_cbox->is_checked = rg.fullscreen_enabled;
			checkbox_set_pos(rg.opts_fullscreen_cbox, xo, yv);
			checkbox_update (rg.opts_fullscreen_cbox);
			v2f opts_fullscreen_cbox_size = checkbox_render(rg.opts_fullscreen_cbox);
			if(rg.opts_fullscreen_cbox->clicked) change_screen(screen_fullscreen_switch);
			yv += opts_fullscreen_cbox_size.y + ypad;

			if(rg.opts_bot_cbox->is_checked != rg.bot_enabled) rg.opts_bot_cbox->is_checked = rg.bot_enabled;
			checkbox_set_pos(rg.opts_bot_cbox, xo, yv);
			checkbox_update (rg.opts_bot_cbox);
			v2f opts_bot_cbox_size = checkbox_render(rg.opts_bot_cbox);
			if(rg.opts_bot_cbox->clicked) rg.bot_enabled = rg.opts_bot_cbox->is_checked;
			yv += opts_bot_cbox_size.y + ypad;

			checkbox_set_pos(rg.opts_disable_mouse_btns_cbox, xo, yv);
			checkbox_update(rg.opts_disable_mouse_btns_cbox);
			v2f opts_disable_mouse_btns_cbox_size = checkbox_render(rg.opts_disable_mouse_btns_cbox);
			if(rg.opts_disable_mouse_btns_cbox->clicked) rg.disable_mouse_buttons = rg.opts_disable_mouse_btns_cbox->is_checked;
			yv += opts_disable_mouse_btns_cbox_size.y + ypad;

			xo = rg.opts_disable_mouse_btns_cbox->pos.x + opts_disable_mouse_btns_cbox_size.x + 100;
			yv = opts_text_height + 50 + PADDING;

			button_set_pos(rg.opts_goto_set_keys_ouendan, xo, yv);
			button_update(rg.opts_goto_set_keys_ouendan);
			button_render(rg.opts_goto_set_keys_ouendan);
			if(rg.opts_goto_set_keys_ouendan->clicked) change_screen(screen_set_keys_ouendan);
			yv += rg.opts_goto_set_keys_ouendan->size.y + ypad;

			button_set_pos(rg.opts_goto_set_keys_mania, xo, yv);
			button_update(rg.opts_goto_set_keys_mania);
			button_render(rg.opts_goto_set_keys_mania);
			if(rg.opts_goto_set_keys_mania->clicked) change_screen(screen_set_keys_mania);
			yv += rg.opts_goto_set_keys_mania->size.y + ypad;

			render_back_button();

			text3_fmt(rg.win_width, rg.win_height, rg.win_width, 255, origin_right, origin_bottom, text3_justification_left, "%s", COMPILE_MSG);
		}
		else if(rg.screen == screen_timing_sections)
		{
			current_scroll_bar = rg.sbar;
			list *timing_sections = rg.song->timing_sections;
			list_node *tn;
			float x = 0.0f;
			float total_height = ((timing_sections->count * (TIMING_SECTION_HEIGHT + 25)));
			scroll_bar_update_vars(rg.sbar, rg.win_width - SCROLL_BAR_WIDTH - PADDING, PADDING, rg.win_height - (PADDING * 2), clamp(total_height, rg.win_height - (PADDING * 2), INFINITY));
			scroll_bar_tick(rg.sbar);
			scroll_bar_render(rg.sbar);
			float y = 25.0f - (int)scale_value_to(scroll_bar_get_real_value(rg.sbar), 0, total_height, 0, clamp(total_height - (rg.win_height - 25), 0, INFINITY));
			char tab = 0;
			timing_section *tats = NULL;
			list_node *tan = NULL;
			timing_section_tick_action tata = timing_section_ta_nothing;
			for(tn = timing_sections->start; tn != NULL; tn = tn->next)
			{
				timing_section *ts = tn->val;
				timing_section_tick_action ta = timing_section_tick(ts, x, y);
				if(!tab && ta != timing_section_ta_nothing)
				{
					tats = ts;
					tata = ta;
					tan = tn;
					tab = 1;
				}
				y += TIMING_SECTION_HEIGHT + 25;
			}

			for(tn = timing_sections->start; tn != NULL; tn = tn->next)
			{
				timing_section *ts = tn->val;
				context_menu_render(ts->bpmi->menu);
				context_menu_render(ts->offseti->menu);
				context_menu_render(ts->slider_pix_ratioi->menu);
				context_menu_render(ts->beat_divi->menu);
			}

			if(tab)
			{
				switch (tata)
				{
					case timing_section_ta_nothing: break;
					case timing_section_ta_add_above:
					{
						timing_section *newtsec = timing_section_init(rg.song, tats->beat_length_ms, rg.song->current_time_ms, tats->slider_time_pixel_ratio, tats->beat_divisor);
						list_insert(timing_sections, tan, newtsec, 1);
						break;
					}
					case timing_section_ta_add_below:
					{
						timing_section *newtsec = timing_section_init(rg.song, tats->beat_length_ms, rg.song->current_time_ms, tats->slider_time_pixel_ratio, tats->beat_divisor);
						list_insert(timing_sections, tan, newtsec, 0);
						break;
					}
					case timing_section_ta_delete_me:
					{
						if(timing_sections->count >= 1)
						{
							timing_section_free(tats);
							list_erase(timing_sections, tan);
						}
						break;
					}
				}
				song_sort_timing_sections(rg.song);
			}

			render_back_button();
		}
		else if(rg.screen == screen_set_keys_ouendan)
		{
			if(rg.set_keys_state < 3)
			{
				if(rg.set_keys_state == 2)
				{
					change_screen(screen_options);
					++rg.set_keys_state;
				}

				text3_fmt(rg.win_width_mid, rg.win_height_mid, rg.win_width, 255, origin_center, origin_center, text3_justification_center, "press what you want key %s to be\n\ncurrent: %s", rg.set_keys_state == 0 ? "1" : "2", SDL_GetScancodeName(rg.set_keys_state == 0 ? rg.ouendan_keycode_1 : rg.ouendan_keycode_2));
			}
		}
		else if(rg.screen == screen_set_keys_mania)
		{
			if(rg.set_keys_state < 5)
			{
				if(rg.set_keys_state == 4)
				{
					change_screen(screen_options);
					++rg.set_keys_state;
				}

				text3_fmt(rg.win_width_mid, rg.win_height_mid, rg.win_width, 255, origin_center, origin_center, text3_justification_center, "press what you want mania key %s to be\n\ncurrent: %s", rg.set_keys_state == 0 ? "left (1)" : rg.set_keys_state == 1 ? "down (2)" : rg.set_keys_state == 2 ? "up (3)" : "right (4)", SDL_GetScancodeName(rg.set_keys_state == 0 ? rg.mania_keycode_1 : rg.set_keys_state == 1 ? rg.mania_keycode_2 : rg.set_keys_state == 2 ? rg.mania_keycode_3 : rg.mania_keycode_4));
			}
		}
		else if(rg.screen == screen_final_score)
		{
			char rank = song_calc_rank(rg.song);
			float acc = song_calc_accuracy(rg.song, 0);

			text3_fmt(rg.win_width_mid, rg.win_height_mid, rg.win_width, 255, origin_center, origin_center, text3_justification_center,
				"Rank: %c\nAccuracy: %.2f%%\nScore: %d\nCombo: %dx\nHits: %d\nMisses: %d",
					rank, acc, rg.song->score, rg.song->highest_combo, rg.song->good_hits, rg.song->misses
				);
		
			render_back_button();

			button *retry_btn = ez_bp("rgretrybtn", "retry?", rg.back_button->pos.x + rg.back_button->size.x + (PADDING * 4), rg.back_button->pos.y);
			if(retry_btn->clicked) change_screen(screen_player);
			button *watch_replay_btn = ez_bp("rgwatchrepbtn", "watch replay?", retry_btn->pos.x + retry_btn->size.x + (PADDING * 4), retry_btn->pos.y);
			if(watch_replay_btn->clicked)
			{
				rg.replay_mode_enabled = 1;
				rg.bot_enabled = 0;
				change_screen(screen_player);
			}
		}
		else if(rg.screen == screen_player || rg.screen == screen_editor)
		{
			song_calc_difficulty(rg.song);

			if(!rg.bot_enabled || rg.screen == screen_editor)
			{
				if(rg.replay_mode_enabled)
				{
					replay_frame *crf = NULL;
					replay_frame *crfn = NULL;

					replay *replay = rg.song->replay;
					crf = replay_get_frame(replay, rg.song->current_time_ms, 0);
					if(crf) crfn = crf->next;
					if(crf && crfn == NULL) crfn = crf;

					if(crf)
					{
						TIME_VAR ctms = clamp(rg.song->current_time_ms, crf->time_ms, crfn->time_ms);
						float mouse_x_on_playfield_new = scale_value_to(ctms, crf->time_ms, crfn->time_ms, crf->cursor_x, crfn->cursor_x);
						float mouse_y_on_playfield_new = scale_value_to(ctms, crf->time_ms, crfn->time_ms, crf->cursor_y, crfn->cursor_y);
						rg.mop_fdx = mouse_x_on_playfield_new - rg.mop_x;
						rg.mop_fdy = mouse_y_on_playfield_new - rg.mop_y;
						rg.mop_x = mouse_x_on_playfield_new;
						rg.mop_y = mouse_y_on_playfield_new;
					}
				}
				else
				{
					v2f zero_pos = song_map_coord_to_play_field(rg.song, V2FZERO);
					v2f pf_dim = song_get_playfield_dimensions(rg.song);
					float mouse_x_on_playfield_new = scale_value_to(mouse_x - zero_pos.x, 0, pf_dim.x, 0, SONG_PFW);
					float mouse_y_on_playfield_new = scale_value_to(mouse_y - zero_pos.y, 0, pf_dim.y, 0, SONG_PFH);
					rg.mop_fdx = mouse_x_on_playfield_new - rg.mop_x;
					rg.mop_fdy = mouse_y_on_playfield_new - rg.mop_y;
					rg.mop_x = mouse_x_on_playfield_new;
					rg.mop_y = mouse_y_on_playfield_new;
				}
			}

			if(left_click == 1 || right_click == 1)
			{
				rg.mop_xoc = rg.mop_x;
				rg.mop_yoc = rg.mop_y;
			}

			rg.is_mouse_on_playfield =
			(
				rg.mop_x >= 0
				&&
				rg.mop_y >= 0
				&&
				rg.mop_x <= SONG_PFW
				&&
				rg.mop_y <= SONG_PFH
			);

			rg.is_mouse_on_click_op =
			(
				rg.mop_xoc >= 0
				&&
				rg.mop_yoc >= 0
				&&
				rg.mop_xoc <= SONG_PFW
				&&
				rg.mop_yoc <= SONG_PFH
			);

			if
			(
				rg.screen == screen_editor
				&&
				rg.focus
				&&
				rg.gui_render_end_of_frame == rg.song
			)
			{
				if
				(
					rg.kp[SDL_SCANCODE_LSHIFT]
					&&
					!rg.kp[SDL_SCANCODE_LCTRL]
					&&
					(
						rg.kp[SDL_SCANCODE_LSHIFT] == 1
						||
						mouse_moved
					)
				)
				{
					song_set_time(rg.song, scale_value_to(rg.mop_x, 0, SONG_PFW, 0, rg.song->length_ms));
					song_reset_object_state(rg.song);
					rg.song->align_on_scroll = 1;
					char *ts = str_glue_fmt("11", "%s/%s", ms_to_hr_time(rg.song->current_time_ms), ms_to_hr_time(rg.song->length_ms));
					show_center_status(ts);
					free(ts);
				}

				if
				(
					rg.kp[SDL_SCANCODE_TAB]
					&&
					mouse_moved
				)
				{
					rg.song->raw_object_size += mouse_delta_y * 0.025;
					rg.song->raw_approach_rate += mouse_delta_x * 0.025;
					song_all_sliders_del_textures(rg.song);
					rg.song->raw_object_size = clamp(rg.song->raw_object_size, SONG_MIN_RAW_OBJECT_SIZE, SONG_MAX_RAW_OBJECT_SIZE);
					rg.song->raw_approach_rate = clamp(rg.song->raw_approach_rate, SONG_MIN_RAW_APPROACH_RATE, SONG_MAX_RAW_APPROACH_RATE);
					char *ts = dupfmt("object size: %.2f approach speed: %.2f", rg.song->raw_object_size, rg.song->raw_approach_rate);
					show_center_status(ts);
					free(ts);
				}
			}

			song_tick(rg.song);

 			if(rg.screen == screen_editor)
			{
				button_set_pos(rg.editor_menu_btn, rg.win_width - PADDING - rg.editor_menu_btn->size.x, PADDING);
				button_update(rg.editor_menu_btn);
				button_render(rg.editor_menu_btn);
				if(rg.editor_menu_btn->clicked) context_menu_activate(rg.editor_menu, rg.editor_menu_btn->pos.x, rg.editor_menu_btn->pos.y + rg.editor_menu_btn->size.y + PADDING);
				context_menu_update(rg.editor_menu);
				context_menu_render(rg.editor_menu);

				int ecm_opt = context_menu_get_clicked_option(rg.editor_menu);
				if(ecm_opt == 0)
				{
					rg.editor_settings_win->visible = 1;
					gui_win_center(rg.editor_settings_win);
				}
				if(ecm_opt == 1) song_play(rg.song);
				if(ecm_opt == 2) song_pause(rg.song);
				if(ecm_opt == 3) song_restart(rg.song);
				if(rg.editor_menu_delete_all_btn->held_for_long) song_clear_objects(rg.song, 1);
			
				context_menu_update(rg.editor_menu_object_type_btn->on_click_cmenu);
				context_menu_render(rg.editor_menu_object_type_btn->on_click_cmenu);
				int objtype_opt = context_menu_get_clicked_option(rg.editor_menu_object_type_btn->on_click_cmenu);
				if(objtype_opt == 0) rg.song->editor_placement_type = object_circle;
				if(objtype_opt == 1) rg.song->editor_placement_type = object_slider;
				if(objtype_opt == 2) rg.song->editor_placement_type = object_spinner;

				if(rg.editor_settings_win->visible) gui_win_render(rg.editor_settings_win);
			}
		}
		if(rg.mode == game_mode_ouendan)
		{
 			if(rg.cursor_trail_points->count > 500 || (timer_bool(rg.cursor_expand_timer, 5) && rg.cursor_trail_points->count > 0))
			{
				free(rg.cursor_trail_points->start->val);
				list_pop_front(rg.cursor_trail_points);
			}

			list_push_back(rg.cursor_trail_points, v2f_init(rg.mop_x, rg.mop_y));

			if(rg.screen == screen_player)
			{
				glBegin(GL_LINE_STRIP);
				glColor3ub(color4_2func(col_white));
				list_node *ctn;
				for(ctn = rg.cursor_trail_points->start; ctn != NULL; ctn = ctn->next)
				{
					v2f ctp = song_map_coord_to_play_field(rg.song, *((v2f*) ctn->val));
					glVertex2f(ctp.x, ctp.y);
				}
				glEnd();
			}

			if(!rg.hide_game_cursor)
			{
				if(rg.ouendan_click_1 == 1 || rg.ouendan_click_2 == 1)
				{
					rg.cursor_state = cursor_expand;
					timer_restart(rg.cursor_expand_timer);
				}
				float cems = timer_milliseconds(rg.cursor_expand_timer);
				float csc = 1.0f;
				if(rg.cursor_state == cursor_expand)
				{
					csc = clamped_scale_value_to(cems, 0, CURSOR_EXPAND_MS, 1.0f, CURSOR_EXPAND_MULTIPLIER);
					if(rg.ouendan_click_1 == 0 && rg.ouendan_click_2 == 0)
					{
						rg.cursor_state = cursor_contract;
						timer_restart(rg.cursor_expand_timer);
					}
				}
				else if(rg.cursor_state == cursor_contract)
				{
					csc = clamp(scale_value_to(cems, 0, CURSOR_EXPAND_MS, CURSOR_EXPAND_MULTIPLIER, 1.0f), 1.0f, CURSOR_EXPAND_MULTIPLIER);
				}
				float cursor_sc = rg.song->object_size * csc;
				v2f cpos = song_map_coord_to_play_field(rg.song, V2F(rg.mop_x, rg.mop_y));
				textures_drawsc(rg.skin->cursor, cpos.x, cpos.y, cursor_sc, cursor_sc, col_white);
			}

			/*text3_fmt(0, 0, rg.win_width, 255, origin_left, origin_top, text3_justification_left, "ouendan click 1 frame 1 time ms: %.2f\nouendan ouendan click 2 frame 1 time ms: %.2f", rg.ouendan_click_1_frame_1_time_ms, rg.ouendan_click_2_frame_1_time_ms);*/
		}

		if(rg.login_win->visible) gui_win_render(rg.login_win);

		tick_fade();

		gui_win_man_render_windows();

		render_notifs();

		render_center_status();

		popup_update(rg.global_popup);
		popup_render(rg.global_popup);

		tick_framerate_display();

		console_tick(cons_ptr);

#ifndef RELEASE
		if(rg.gui_view_xoff != 0.0f || rg.gui_view_yoff != 0.0f) GL_LINEBOX(FLOATRECT(-1, -1, rg.win_width + 1, rg.win_height + 1), col_yellow);
		
		if(rg.kp[SDL_SCANCODE_LCTRL])
		{
			GL_DRAWLINE(V2F(0, rg.win_height_mid), V2F(rg.win_width, rg.win_height_mid), col_yellow);
			GL_DRAWLINE(V2F(rg.win_width_mid, 0), V2F(rg.win_width_mid, rg.win_height), col_yellow);

			GL_DRAWLINE(V2F(0, rg.win_height_mid * 0.5), V2F(rg.win_width, rg.win_height_mid * 0.5), col_yellow);
			GL_DRAWLINE(V2F(rg.win_width_mid * 0.5, 0), V2F(rg.win_width_mid * 0.5, rg.win_height), col_yellow);

			GL_DRAWLINE(V2F(0, rg.win_height_mid * 1.5), V2F(rg.win_width, rg.win_height_mid * 1.5), col_yellow);
			GL_DRAWLINE(V2F(rg.win_width_mid * 1.5, 0), V2F(rg.win_width_mid * 1.5, rg.win_height), col_yellow);
		}
#endif

		SDL_GL_SwapWindow(rg.win);

		if(mouse_button_released)
		{
			mouse_release_velocity_x = 0.0f;
			mouse_release_velocity_y = 0.0f;
		}

		float slowthres = 150;
		if(mouse_vel_slow_ms_x >= slowthres)
		{
			mouse_release_velocity_x = 0.0f;
			mouse_vel_slow_ms_x = 0;
		}
		if(mouse_vel_slow_ms_y >= slowthres)
		{
			mouse_release_velocity_y = 0.0f;
			mouse_vel_slow_ms_y = 0;
		}
		
		float sst = 0.04 * rg.frame_delta_ms;

		if(mouse_release_velocity_y > 0)
		{
			mouse_release_velocity_y -= sst;
			if(mouse_release_velocity_y < 0) mouse_release_velocity_y = 0;
		}
		else if(mouse_release_velocity_y < 0)
		{
			mouse_release_velocity_y += sst;
			if(mouse_release_velocity_y > 0) mouse_release_velocity_y = 0;
		}

		++rg.frames;

		if(rg.focus) ++rg.focus_frames;
		else rg.focus_frames = 0;

		if(rg.resized > 0) --rg.resized;

		if(timer_milliseconds(mouse_release_velocity_timer) >= MOUSE_VEL_CHECK_MS) timer_restart(mouse_release_velocity_timer);

		if(mouse_moved && timer_bool(mouse_moved_clock, MOUSE_MOVED_DECAY_MS)) mouse_moved = 0;

		if(cursor_type == SDL_SYSTEM_CURSOR_ARROW) SDL_SetCursor(arrow_cursor);
		if(cursor_type == SDL_SYSTEM_CURSOR_SIZEWE) SDL_SetCursor(horiz_cursor);
		cursor_type = SDL_SYSTEM_CURSOR_ARROW;

		rg.gui_render_end_of_frame = rg.gui_render_ptr;

		if(rg.fps_limit != 0)
		{
			if(rg.focus) SDL_Delay(1000.0f / rg.fps_limit);
			else SDL_Delay(1000.0f / FPS_LIMIT_NO_FOCUS);
		}

		rg.frame_delta_ms = timer_milliseconds(delta_clock);
		timer_restart(delta_clock);

		if(rg.frames == 1) SDL_ShowWindow(rg.win);
	}

	SDL_FreeCursor(horiz_cursor);
	SDL_FreeCursor(arrow_cursor);

	widgets_free();

	IMG_Quit();
	
	glyphs_free();

	SDL_GL_DeleteContext(rg.context);
	SDL_DestroyWindow(rg.win);
	SDL_Quit();

	write_config();

#ifdef USING_WINDOWS
	discord_done();
#endif

	online_client_free(rg.client);

#ifndef RELEASE
	online_server_free(rg.server);
#endif

	gui_win_free(rg.editor_settings_win);
	gui_win_free(rg.login_win);

	context_menu_free(rg.editor_menu);
	button_free(rg.editor_menu_btn);
	
	scroll_bar_free(rg.sbar);
	
	song_select_free(0);
	
	button_free(rg.opts_goto_set_keys_mania);
	button_free(rg.opts_goto_set_keys_ouendan);
	checkbox_free(rg.opts_disable_mouse_btns_cbox);
	checkbox_free(rg.opts_bot_cbox);
	checkbox_free(rg.opts_fullscreen_cbox);
	checkbox_free(rg.opts_beat_flash_cbox);
	slider_bar_free(rg.opts_bg_dim_bar);
	slider_bar_free(rg.opts_effect_vol_bar);
	slider_bar_free(rg.opts_music_vol_bar);
	slider_bar_free(rg.opts_master_vol_bar);
	
	context_menu_free(rg.grid_res_cmenu);
	button_free(rg.grid_res_menu_button);
	button_free(rg.goto_timing_points_button);
	button_free(rg.just_quit_button);
	button_free(rg.save_quit_button);
	button_free(rg.test_song_button);
	button_free(rg.save_song_button);
	input_box_free(rg.song_offset_input);
	input_box_free(rg.speed_input);
	input_box_free(rg.bg_fname_input);
	input_box_free(rg.ar_input);
	input_box_free(rg.cs_input);
	
	button_free(rg.back_button);
	button_free(rg.exit_button);
	button_free(rg.options_button);
	button_free(rg.song_sel_button);
    
	song_free(rg.song);
	list_free(rg.cursor_trail_points, free);
    skin_free(rg.skin);

	timer_free(rg.cursor_trail_timer);
	timer_free(rg.cursor_expand_timer);
	timer_free(rg.fade_clock);

	audio_engine_free();

	timer_free(click_block_timer);

	timer_free(delta_clock);
	text_free(fps_text);

	popup_free(rg.global_popup);
	gui_win_man_free();

	console_free(cons_ptr);

	textures_free();

	TTF_CloseFont(rg.font);
	TTF_Quit();

	SDL_Quit();

	list_free(notifications_list, notif_delete);

	timer_free(center_status_timer);
	text_free(center_status_text);

	config_deinit();

	timer_free(mouse_moved_clock);
	timer_free(right_click_held);
	timer_free(middle_click_held);
	timer_free(left_click_held);

	timer_free(mouse_release_velocity_timer);

	timer_free(fps_clock_interval);
	timer_free(fps_clock);

	timer_free(rg.time_open);
}
