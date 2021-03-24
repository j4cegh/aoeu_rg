#include "events.h"
#include "rg.h"
#include "popup.h"
#include "console.h"
#include "scroll_bar.h"
#include "input_box.h"
#include "song_select.h"
#include "song.h"
#include "replay.h"

char exit_anyway;

void exit_pop_cback(void *ptr, char yes)
{
	if(yes)
	{
		exit_anyway = 1;
		change_screen(screen_exit);
	}
	apply_cursor();
}

void setkeys_events(SDL_Event event)
{
	if(event.type == SDL_KEYDOWN)
	{
		int key = event.key.keysym.scancode;
		if(key != SDL_SCANCODE_ESCAPE)
		{
			if(rg.screen == screen_set_keys_ouendan)
			{
				if(rg.set_keys_state == 0)
				{
					rg.ouendan_keycode_1 = key;
					++rg.set_keys_state;
				}
				else if(rg.set_keys_state == 1)
				{
					rg.ouendan_keycode_2 = key;
					++rg.set_keys_state;
				}
			}
			if(rg.screen == screen_set_keys_mania)
			{
				if(rg.set_keys_state == 0)
				{
					rg.mania_keycode_1 = key;
					++rg.set_keys_state;
				}
				else if(rg.set_keys_state == 1)
				{
					rg.mania_keycode_2 = key;
					++rg.set_keys_state;
				}
				else if(rg.set_keys_state == 2)
				{
					rg.mania_keycode_3 = key;
					++rg.set_keys_state;
				}
				else if(rg.set_keys_state == 3)
				{
					rg.mania_keycode_4 = key;
					++rg.set_keys_state;
				}
			}
		}
		else escape();
	}
}

void normal_events(SDL_Event event)
{
	if(event.type == SDL_KEYDOWN)
	{
		const int key = event.key.keysym.sym;
		if(key == SDLK_ESCAPE && event.key.repeat == 0) escape();
		if(key == SDLK_F12 && rg.kp[SDL_SCANCODE_LALT] && event.key.repeat == 0) rg.enable_slider_debug = !rg.enable_slider_debug;
		else
		{
			if(rg.screen == screen_editor || (rg.screen == screen_player && rg.editor_testing_song))
			{
				if(key == SDLK_SPACE && !(rg.screen == screen_player && rg.song->current_time_ms < rg.song->song_start_ms)) song_toggle_play_pause(rg.song);
				if(key == SDLK_UP) song_scroll(rg.song, -10);
				if(key == SDLK_DOWN) song_scroll(rg.song, 10);
				if(key == SDLK_LEFT) song_scroll(rg.song, -1);
				if(key == SDLK_RIGHT) song_scroll(rg.song, 1);
				if(key == SDLK_COMMA) song_set_time(rg.song, rg.song->current_time_ms - 1);
				if(key == SDLK_PERIOD) song_set_time(rg.song, rg.song->current_time_ms + 1);
				if(key == SDLK_HOME) song_set_time(rg.song, 0);
				if(key == SDLK_END) song_set_time(rg.song, rg.song->length_ms);
				if(key == SDLK_F5) song_restart(rg.song);
				if(key == SDLK_RETURN && rg.song->time_ms_on_editor_play != -1)
				{
					song_set_time(rg.song, rg.song->time_ms_on_editor_play);
					song_reset_object_state(rg.song);
					if(rg.song->music_state == song_paused) song_play(rg.song);
				}
			}

			if(rg.screen == screen_player)
			{
				if(key == SDLK_SPACE)
				{
					if(rg.song->current_time_ms < rg.song->song_start_ms && rg.song->song_start_ms > 0) 
					{
						song_set_time(rg.song, rg.song->song_start_ms);
						timer_restart(rg.fade_clock);
					}
				}
#ifndef RELEASE
				if(key == SDLK_F6 && rg.song->replay->frames->count > 0)
				{
					rg.replay_mode_enabled = 1;
					rg.bot_enabled = 0;
					rg.hide_game_cursor = 0;
					rg.song->replay->raw_approach_rate = rg.song->raw_approach_rate;
					rg.song->replay->raw_object_size = rg.song->raw_object_size;
					song_calc_song_skip_and_end(rg.song);
					apply_cursor();
					song_restart(rg.song);
				}

				if(key == SDLK_F7) song_set_time(rg.song, rg.song->song_end_ms - (SONG_END_PADDING_MS));
#endif
				if
				(
					key == SDLK_F8
#ifdef RELEASE
					&& rg.editor_testing_song
#endif
				)
				{
					rg.replay_mode_enabled = 0;
					rg.bot_enabled = !rg.bot_enabled;
					apply_cursor();
				}
			}

			if(rg.screen == screen_editor && rg.gui_render_end_of_frame == rg.song)
			{
				if(key == SDLK_BACKQUOTE)
				{
					if(rg.song->paste_flip_mode == 0)
					{
						rg.song->paste_flip_mode = 1;
						show_notif("paste flip mode: horizontal flip");
					}
					else if(rg.song->paste_flip_mode == 1)
					{
						rg.song->paste_flip_mode = 2;
						show_notif("paste flip mode: vertical flip");
					}
					else if(rg.song->paste_flip_mode == 2)
					{
						rg.song->paste_flip_mode = 3;
						show_notif("paste flip mode: horizontal & vertical flip");
					}
					else if(rg.song->paste_flip_mode == 3)
					{
						rg.song->paste_flip_mode = 0;
						show_notif("paste flip mode: no flip");
					}
				}
				if(key == SDLK_1) rg.song->editor_placement_type = object_circle;
				if(key == SDLK_2) rg.song->editor_placement_type = object_slider;
				if(key == SDLK_3) rg.song->editor_placement_type = object_spinner;
				if(key == SDLK_F8) change_screen(screen_player);
				if(key == SDLK_F6)
				{
					rg.song->preview_time_ms = rg.song->current_time_ms;
					show_notif_fmt("set preview time to %.2f ms.", rg.song->preview_time_ms);
				}
				if(rg.kp[SDL_SCANCODE_LCTRL])
				{
					if(key == SDLK_z) song_undo(rg.song);
					if(key == SDLK_y) song_redo(rg.song);
					if(key == SDLK_s) song_save(rg.song);
					if(key == SDLK_c) song_copy(rg.song);
					if(key == SDLK_v) song_paste(rg.song);
					if(key == SDLK_a) song_select_all(rg.song);
					if(key == SDLK_t) change_screen(screen_timing_sections);
				}
				else
				{
					if(key == SDLK_DELETE) song_delete_selected_objects(rg.song);
					if(key == SDLK_m)
					{
						rg.song->metronome_on = !rg.song->metronome_on;
						show_notif(rg.song->metronome_on ? "metronome: on" : "metronome: off");
					}
				}
			}

			if(rg.screen == screen_song_select && key == SDLK_F5 && event.key.repeat == 0) song_select_refresh(0);

			if(key == SDLK_F10)
			{
				rg.potato_pc_enabled = !rg.potato_pc_enabled;
				song_all_sliders_del_textures(rg.song);
				show_notif(rg.potato_pc_enabled ? "potato pc mode: on" : "potato pc mode: off");
			}
		}

	}
	if(event.type == SDL_MOUSEWHEEL)
	{
		int dist = event.wheel.y;
		if(rg.kp[SDL_SCANCODE_LALT] || rg.screen == screen_main_menu || rg.screen == screen_player)
		{
			rg.master_vol += dist < 0 ? -5 : dist > 0 ? 5 : 0;
			rg.master_vol = clamp(rg.master_vol, 0, 100);
			if(rg.screen != screen_options) show_center_status_fmt("master vol: %.0f", rg.master_vol);
		}
		else if(rg.screen == screen_song_select)
		{
			if (rg.gui_render_end_of_frame == ss)
			{
				ss->y_scroll_speed += -dist;
				ss->y_scroll_speed = clamp(ss->y_scroll_speed, -3, 3);
			}
			else if (rg.gui_render_end_of_frame == ss->orl)
			{
				ss->orl->scroll_y += -dist * ONLINE_RL_ENTH;
			}
		}
		else if(rg.screen == screen_editor)
		{
			if(rg.kp[SDL_SCANCODE_LCTRL] && !rg.kp[SDL_SCANCODE_LSHIFT])
			{
				if(dist < 0)
					rg.song->tl.timeline_scale -= 0.025f;
				if(dist > 0)
					rg.song->tl.timeline_scale += 0.025f;
				rg.song->tl.timeline_scale = clamp(rg.song->tl.timeline_scale, 0.05f, 6.0f);
			}
			else if(!rg.kp[SDL_SCANCODE_LCTRL] && !rg.kp[SDL_SCANCODE_LSHIFT]) song_scroll(rg.song, -dist);
		}
	}
}

void events()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		if(event.type == SDL_QUIT)
		{
			if
			(
				!exit_anyway
				&&
				(
					(
						rg.screen == screen_editor
						||
						rg.screen == screen_timing_sections
						||
						(
							rg.screen == screen_player
							&&
							rg.editor_testing_song
						)
					)
					&&
					rg.song->edit_count > 0
				)
			)
			{
				popup_open(rg.global_popup, popup_type_yes_no, "you have unsaved changes, exit anyway?");
				rg.global_popup->yes_no_callback = &exit_pop_cback;
				SDL_ShowCursor(1);
				exit_anyway = 0;
			}
			else change_screen(screen_exit);
		}

		if(event.type == SDL_WINDOWEVENT)
		{
			if(event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) rg.focus = 1;
			if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) rg.focus = 0;
			if(rg.frames >= 1 && event.window.event == SDL_WINDOWEVENT_RESIZED) rg.resized = 1;
		}

		if(event.type == SDL_DROPFILE)
		{
			char *ext = get_file_ext(event.drop.file);
			char bad_file = 1;
			if(ext != NULL)
			{
				if(rg.screen == screen_editor || rg.screen == screen_timing_sections)
				{
					if(strs_are_equal(ext, "png"))
					{
						bad_file = 0;

					}
				}
				else if(rg.screen == screen_main_menu || rg.screen == screen_song_select || rg.screen == screen_options)
				{
					if(strs_are_equal(ext, "mp3") || strs_are_equal(ext, "ogg"))
					{
						bad_file = 0;
						char *dest = dupfmt("%s%s", SONGS_DIR, get_file_from_path(event.drop.file));
						int i = copy_file_to(event.drop.file, dest);
						free(dest);
						song_select_entry *sse = song_select_refresh(1);
						if(sse)
						{
							ss->load_next = sse;
							change_screen(screen_import_editor);
						}
						else show_notif("failed to import music for editing...");
					}
				}
			}
			if(bad_file) show_notif("can't handle this type of file. sorry!");
			SDL_free(event.drop.file);
		}
		
		if(rg.focus)
		{
			mouse_events(event);

#ifndef RELEASE
			if(event.type == SDL_MOUSEMOTION && rg.kp[SDL_SCANCODE_LCTRL] && rg.kp[SDL_SCANCODE_LALT])
			{
				rg.gui_view_xoff -= event.motion.xrel;
				rg.gui_view_yoff -= event.motion.yrel;
			}
#endif

			if(event.type == SDL_MOUSEWHEEL)
			{
				int dist = event.wheel.y;
				if(current_scroll_bar)
				{
					scroll_bar *sbc = current_scroll_bar;
					sbc->velocity += -(dist * (sbc->hr_size.y / 100));
				}
			}

			if(event.type == SDL_KEYDOWN)
			{
				int key = event.key.keysym.sym;
				int mod = event.key.keysym.mod;

				if(key == SDLK_F1) console_toggle(cons_ptr);
				if(key == SDLK_RETURN && cons_ptr->activated) console_enter(cons_ptr);
				if((key == SDLK_F11 || (mod == KMOD_LALT && key == SDLK_RETURN)) && event.key.repeat == 0)
				{
#ifdef RELEASE
					if(rg.screen != screen_player)
#endif
						change_screen(screen_fullscreen_switch);
				}
				if(current_input_box) input_box_events(current_input_box, event);
			}

			if(event.type == SDL_TEXTINPUT)
			{
				int key = event.text.text[0];
				char key_good = (key >= ASCII_START && key <= ASCII_END);
				if(current_input_box)
				{
					char key_good_only_nums = (key == 46 || key == 45 || (key >= 48 && key <= 57));
					char period_count = count_char_occurances(current_input_box->text, '.');
					char dash_count = count_char_occurances(current_input_box->text, '-');
					char selall = (current_input_box->char_select_index_start == 0 && current_input_box->char_select_index_end == strlen(current_input_box->text));
					char good = current_input_box->force_numbers_only ? key_good_only_nums && !(key == 46 && period_count == 1 && !selall) && !(key == 45 && (dash_count == 1 || current_input_box->caret_index > 0) && !selall) : key_good;
					if(good) input_box_append_char(current_input_box, key);
				}
			}

			if(rg.screen == screen_set_keys_ouendan || rg.screen == screen_set_keys_mania) setkeys_events(event);
			else normal_events(event);
		}
	}
}