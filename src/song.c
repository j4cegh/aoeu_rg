#include "song.h"
#include "rg.h"
#include "utils.h"
#include "text3.h"
#include "circle.h"
#include "slider.h"
#include "mania_note.h"
#include "skin.h"
#include "curves.h"
#include "explosion.h"
#include "replay.h"
#include "timeline.h"
#include "approach_circle.h"
#include "ezgui.h"
#include "online_client.h"
#include "logger.h"

song *song_init(char *name, char *folder_loc, char *data_file_loc)
{
    song *ret = malloc(sizeof *ret);

	ret->version = SONG_FILE_VERSION;
	ret->game_mode_in_editor = rg.mode;
	ret->editor_placement_type = 0;

	ret->name = dupe_str(name);
	ret->bg_filename = dupe_str(SONG_BG_DEFAULT_FILENAME);
	ret->folder_loc = dupe_str(folder_loc);
	ret->data_file_loc = dupe_str(data_file_loc);
	ret->music_filename = dupe_str(SONG_UNKNOWN_NAME);
	ret->music_file_loc = dupe_str(SONG_UNKNOWN_NAME);

	ret->all_objects = list_init();
	ret->game_mode_objects = list_init();
	ret->clipboard_objects = list_init();
	ret->selected_objects = list_init();
	ret->timing_sections = list_init();
	ret->explosions = list_init();
	ret->replays = list_init();

	ret->cur_time_micro_sec = 0;
	ret->current_time_ms = 0;
	const_write_float(&ret->beat_length_ms, 500);
	const_write_float(&ret->timing_section_start_ms, 0);
	ret->length_ms = 6000;
	const_write_float(&ret->beat_divisor, 4);
	ret->time_ms_on_editor_exit = 0;
	ret->time_ms_on_editor_play = 0;
	ret->preview_time_ms = 0;
	ret->song_global_offset = 0;
	ret->song_start_ms = 0;
	ret->song_end_ms = ret->length_ms;
	ret->speed_multiplier = 1.0f;

	ret->raw_object_size = 4;
	ret->object_size_pf = 1.0f;
	ret->object_size = 1.0f;
	ret->raw_approach_rate = 4;
	ret->approach_time_ms = 1000;

	ret->song_timer = timer_init();
	ret->music = ae_music_handle_init();
	ret->music_state = song_paused;

	ret->selection_rect = FLOATRECTZERO;
	ret->selected_object = NULL;

	ret->beat_guess_clock = timer_init();
	ret->beat_guess_list = list_init();
	ret->beat_guess_build_up = 0;

	ret->score = 0;
	ret->passed_segs = 0;
	ret->good_hits = 0;
	ret->misses = 0;
	ret->current_combo = 0;
	ret->highest_combo = 0;
	ret->failed = 0;

	ret->align_on_scroll = 0;
	ret->obj_hovered_on_click = 0;
	ret->slider_control_point_box_hovered = 0;
	ret->mouse_hovering_slider_body = 0;
	ret->grid_resolution = SONG_DEFAULT_GRID_RESOLUTION;
	ret->song_not_started = 1;
	ret->paste_flip_mode = 0;
	ret->slider_startend_hit = 0;
	ret->dragging_selected_objects = 0;
	ret->edit_count = -1;
	ret->background_texture = NULL;
	ret->replay = replay_init();

	list_push_back(ret->timing_sections, timing_section_init(ret, 500, 0, 1, 4));

	ret->undo_states = list_init();
	ret->undo_state = 0;

	timeline_init(&ret->tl);

	ret->metronome_on = 0;
	ret->metronome_stanza = -1;

	ret->ss_entry = NULL;

	health_bar_reset(&(ret->hbar), ret);

	ret->hash[0] = '\0';

	return ret;
}

void song_free(song *ptr)
{
	list_free(ptr->undo_states, free);
	replay_free(ptr->replay);
    list_free(ptr->beat_guess_list, free);
    timer_free(ptr->beat_guess_clock);
	ae_music_handle_free(ptr->music);
    timer_free(ptr->song_timer);
	list_node *n;
	list_free(ptr->replays, replay_free);
	list_free(ptr->explosions, free);
	list_free(ptr->timing_sections, timing_section_free);
	list_free(ptr->selected_objects, list_dummy);
	list_free(ptr->clipboard_objects, free);
	list_free(ptr->game_mode_objects, list_dummy);
    list_free(ptr->all_objects, object_free);
	free(ptr->music_file_loc);
    free(ptr->music_filename);
    free(ptr->data_file_loc);
	free(ptr->folder_loc);
	free(ptr->bg_filename);
    free(ptr->name);
    free(ptr);
}

float song_get_real_raw_object_size(song *ptr)
{
	if(rg.mode == game_mode_mania) return 5.0f;
	if(rg.screen == screen_editor) return ptr->raw_object_size;
	return ptr->raw_object_size * 1.0f;
}

float song_get_real_raw_approach_rate(song *ptr)
{
	if(rg.screen == screen_editor) return ptr->raw_approach_rate;
	else return ptr->raw_approach_rate * 1.0f;
}

void song_calc_difficulty(song *ptr)
{
    ptr->approach_time_ms = scale_value_to(12.5f - (rg.replay_mode_enabled ? ptr->replay->raw_approach_rate : song_get_real_raw_approach_rate(ptr)), 0.0f, 10.0f, 0.0f, 1800.0f);
	v2f dim = song_get_playfield_dimensions(ptr);
	float smallest = (dim.x) < dim.y ? dim.x : (dim.y < dim.x ? dim.y : dim.x);
	float pfa = (smallest == dim.x ? SONG_PFW : SONG_PFH);
	ptr->object_size_pf = scale_value_to(11.0f - (rg.replay_mode_enabled ? ptr->replay->raw_object_size : song_get_real_raw_object_size(ptr)), 0.0f, 10.0f, 0.0f, 1.0f);
    ptr->object_size = scale_value_to(smallest, 0.0f, pfa, 0.0f, ptr->object_size_pf);
}

void song_restart(song *ptr)
{
	ptr->song_not_started = 1;
	song_apply_speed(ptr);
	ae_music_handle_set_time_ms(ptr->music, 0);
	ae_music_handle_do(ptr->music, ae_stop);
	ptr->cur_time_micro_sec = (rg.screen == screen_player) ? -(SONG_BEFORE_START_TIME_MS * 1000) : 0;
	ptr->current_time_ms = ptr->cur_time_micro_sec / 1000;
	song_reset_score(ptr);
	song_reset_object_state(ptr);
	replay_reset(ptr->replay);
    ptr->music_state = song_playing;
	song_calc_song_skip_and_end(ptr);
    timer_restart(ptr->song_timer);
}

void song_play(song *ptr)
{
    ptr->music_state = song_playing;
	song_apply_speed(ptr);
	ae_music_handle_set_time_ms(ptr->music, ptr->current_time_ms);
	ae_music_handle_do(ptr->music, ae_play);
	if(rg.screen == screen_editor) ptr->time_ms_on_editor_play = ptr->current_time_ms;
    timer_restart(ptr->song_timer);
}

void song_preview_play(song *ptr)
{
	apply_volume();
	ptr->song_not_started = 0;
	ae_music_handle_set_time_ms(ptr->music, ptr->preview_time_ms);
	ae_music_handle_do(ptr->music, ae_play);
	ptr->music_state = song_playing;
}

void song_pause(song *ptr)
{
    ptr->music_state = song_paused;
	ae_music_handle_do(ptr->music, ae_pause);
	ptr->align_on_scroll = 1;
}

void song_toggle_play_pause(song *ptr)
{
	if(ptr->music_state == song_playing) song_pause(ptr);
	else if(ptr->music_state == song_paused) song_play(ptr);
}

void song_stop(song *ptr)
{
	ptr->music_state = song_paused;
	ae_music_handle_do(ptr->music, ae_stop);
}

void song_calc_song_skip_and_end(song *ptr)
{
	float latest_obj_ms = INFINITY;
	list_node *flo;
	for(flo = ptr->all_objects->end; flo != NULL; flo = flo->before)
	{
		object *o = flo->val;
		if(!object_compatable_with_current_gm(o)) continue;
		if(o->start_time_ms < latest_obj_ms) latest_obj_ms = o->start_time_ms;
	}
	ptr->song_start_ms = latest_obj_ms - SONG_SKIP_PADDING_MS;
	latest_obj_ms = 0.0f;
	if(rg.replay_mode_enabled)
	{
		for(flo = ptr->replay->frames->start; flo != NULL; flo = flo->next)
		{
			replay_frame *o = flo->val;
			float len = o->time_ms;
			if(len > latest_obj_ms) latest_obj_ms = len;
		}
		ptr->song_end_ms = latest_obj_ms;
	}
	else
	{
		for(flo = ptr->all_objects->start; flo != NULL; flo = flo->next)
		{
			object *o = flo->val;
			if(!object_compatable_with_current_gm(o)) continue;
			float len = o->start_time_ms + o->length_time_ms;
			if(len > latest_obj_ms) latest_obj_ms = len;
		}
		ptr->song_end_ms = latest_obj_ms + SONG_END_PADDING_MS;
	}

	if(ae_music_handle_is_null(ptr->music) && ptr->length_ms < ptr->song_end_ms) ptr->length_ms = ptr->song_end_ms;
}

void song_set_music_vol(song *ptr, float vol)
{
	ae_music_handle_set_volume(ptr->music, vol);
}

void song_set_speed(song *ptr, float speed)
{
	float rspeed = clamp(speed, SONG_MIN_SPEED, SONG_MAX_SPEED);
	if(ptr->speed_multiplier != rspeed)
	{
		ptr->speed_multiplier = rspeed;
		song_apply_speed(ptr);
	}
}

void song_apply_speed(song *ptr)
{
	ae_music_handle_set_speed(ptr->music, clamp(ptr->speed_multiplier, SONG_MIN_SPEED, SONG_MAX_SPEED));
}

void song_stack_objects(song *ptr)
{
	if(rg.screen == screen_editor)
	{
		song_sort_objects(ptr);
		if(rg.mode == game_mode_ouendan)
		{
			list_node *n;
			for(n = ptr->game_mode_objects->end; n != NULL; n = n->before)
			{
				if(n->before != NULL)
				{
					object *obj = n->val;
					object *bef_obj = n->before->val;
					float dist = get_distf(obj->pos.x, obj->pos.y, bef_obj->pos.x, bef_obj->pos.y);
					if(dist <= CIRCLE_RADIUS / 4)
					{
						
					}
				}
			}
		}
	}
}

void song_copy(song *ptr)
{
	if(ptr->clipboard_objects->count > 0) song_clear_clipboard(ptr);
	if(ptr->selected_objects->count > 0)
	{
		list_node *so;
		TIME_VAR earliest_ms = INFINITY;
		for(so = ptr->selected_objects->start; so != NULL; so = so->next)
		{
			object *obj = so->val;
			if(obj->start_time_ms < earliest_ms) earliest_ms = obj->start_time_ms;
		}
		for(so = ptr->selected_objects->start; so != NULL; so = so->next)
		{
			object *obj = so->val;
			float start_time_bef = obj->start_time_ms;
			obj->start_time_ms -= earliest_ms;
			char *serialized = object_serialize(obj);
			obj->start_time_ms = start_time_bef;
			if(serialized) list_push_back(ptr->clipboard_objects, serialized);
			else show_notif("object serialize error...");
		}
		if(ptr->clipboard_objects->count > 1) show_notif("objects copied!");
		if(ptr->clipboard_objects->count == 1) show_notif("object copied!");
	}
}

void song_paste(song *ptr)
{
	if(ptr->clipboard_objects->count > 0)
	{
		song_capture_undo_state(ptr);
		song_clear_selected_objects(ptr);
		list_node *co;
		for(co = ptr->clipboard_objects->start; co != NULL; co = co->next)
		{
			char *obj_serialized = co->val;
			object *ofs = object_from_serialized(obj_serialized, ptr);
			ofs->start_time_ms += song_align_ms(ptr,  ptr->current_time_ms);
			if(ptr->paste_flip_mode != 0)
			{
				if(ofs->type == object_circle)
				{
					if(ptr->paste_flip_mode == 3 || ptr->paste_flip_mode == 1) ofs->pos.x = SONG_PFW - ofs->pos.x;
					if(ptr->paste_flip_mode == 3 || ptr->paste_flip_mode == 2) ofs->pos.y = SONG_PFH - ofs->pos.y;
				}
				else if(ofs->type == object_slider)
				{
					list_node *cn;
					for(cn = ofs->slider->control_points->start; cn != NULL; cn = cn->next)
					{
						slider_control_point *pnt = cn->val;
						if(ptr->paste_flip_mode == 3 || ptr->paste_flip_mode == 1) pnt->pos.x = SONG_PFW - pnt->pos.x;
						if(ptr->paste_flip_mode == 3 || ptr->paste_flip_mode == 2) pnt->pos.y = SONG_PFH - pnt->pos.y;
					}
				}
			}
			if(ofs->type == object_slider) slider_calculate_path(ofs->slider, 0);
			list_push_back(ptr->all_objects, ofs);
			list_push_back(ptr->selected_objects, ofs);
		}
		song_sort_objects(ptr);
		show_notif(ptr->clipboard_objects->count == 1 ? "object pasted!" : "objects pasted!");
	}
}

void song_select_all(song *ptr)
{
	song_clear_selected_objects(ptr);
	list_node *n;
	for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		list_push_back(ptr->selected_objects, obj);
	}
}

void song_clear_clipboard(song *ptr)
{
	list_clear(ptr->clipboard_objects, free);
}

void song_clear_selected_objects(song *ptr)
{
	list_clear(ptr->selected_objects, list_dummy);
}

void song_delete_selected_objects(song *ptr)
{
	if(ptr->selected_objects->count < 1) return;
	list_node *n;
	for(n = ptr->all_objects->start; n != NULL;)
	{
		object *obj = n->val;
		list_node *sn;
		char cont = 0;
		for(sn = ptr->selected_objects->start; sn != NULL; sn = sn->next)
		{
			object *sobj = sn->val;
			if(sobj == obj)
			{
				if(obj == ptr->selected_object) ptr->selected_object = NULL;
				object_free(obj);
				n = list_erase(ptr->all_objects, n);
				cont = 1;
				break;
			}
		}
		if(cont) continue;
		n = n->next;
	}
	song_capture_undo_state(ptr);
	song_clear_selected_objects(ptr);
	song_sort_objects(ptr);
}

void song_unselect_object(song *ptr)
{
	if(ptr->selected_object != NULL) ptr->selected_object->is_new = 0;
	ptr->selected_object = NULL;
}

TIME_VAR msft = 0.0f;
timing_section *lcts = NULL;
void song_tick_timing(song *ptr)
{
	char stream_null = ae_music_handle_is_null(ptr->music);
	char internal_timing = ptr->song_not_started || stream_null;
	char active = ae_music_handle_is_active(ptr->music);

	if(internal_timing)
	{
		msft = timer_microseconds(ptr->song_timer);
		timer_restart(ptr->song_timer);
	}

	if(ptr->music_state == song_playing)
	{
		if(rg.screen == screen_editor && ptr->music_state == song_playing && (ptr->current_time_ms >= ptr->length_ms || (!active && ptr->current_time_ms >= 0 && !ptr->song_not_started))) song_pause(ptr);
		
		if((ptr->current_time_ms >= ptr->song_end_ms || !active) && !ptr->song_not_started)
		{
			if(rg.screen == screen_player && rg.to_screen == screen_player)
			{
				if(rg.editor_testing_song) song_restart(ptr);
				else change_screen(screen_final_score);
			}
		}
	
		if(internal_timing) ptr->cur_time_micro_sec += msft * ptr->speed_multiplier;
		else if(!stream_null && !ptr->song_not_started)
		{
			float ms = ae_music_handle_get_time_ms(ptr->music);
			ptr->cur_time_micro_sec = ms * 1000;
		}
		
		ptr->current_time_ms = clamp((ptr->cur_time_micro_sec / 1000.0f) + ptr->song_global_offset, -SONG_BEFORE_START_TIME_MS, ptr->length_ms);
#ifdef USING_WINDOWS
		ptr->current_time_ms += 10;
#endif

		if(ptr->song_not_started && ptr->current_time_ms >= 0)
		{
			if(!stream_null)
			{
				if(ptr->song_start_ms > 0) ae_music_handle_set_time_ms(ptr->music, ptr->current_time_ms);
				ae_music_handle_do(ptr->music, ae_play);
			}
			ptr->song_not_started = 0;
		}
	}

	timing_section *cts = song_apply_timing_sec(ptr, ptr->current_time_ms);
	if(ptr->music_state == song_paused && rg.screen == screen_editor && cts != lcts) ptr->align_on_scroll = 1;
	lcts = cts;
}

void song_draw_background(song *ptr)
{
	loaded_texture *tex = ptr->background_texture;
	if(tex == NULL) tex = rg.skin->bg;
	if(rg.bg_dim_level < 100)
	{
		tex->o = V2F(tex->surf->w / 2, tex->surf->h / 2);
		color4 bg_color = col_white;
		bg_color.a = scale_value_to(100 - ((rg.screen == screen_editor ? 65 : rg.screen != screen_player ? clamp(rg.bg_dim_level, 50, 100) : rg.bg_dim_level)), 0, 100, 0, 255);
		float bg_scale = rg.win_height / tex->surf->h;
		textures_drawsc(tex, rg.win_width_mid, rg.win_height_mid, bg_scale, bg_scale, bg_color);
	}
}

void song_draw_playfield_border(song *ptr)
{
	if(rg.screen == screen_editor || ((rg.enable_beat_flash && (rg.screen == screen_player || rg.screen == screen_main_menu)) && ptr->music_state == song_playing && ptr->current_time_ms >= ptr->timing_section_start_ms))
	{
		color4 pfr_col;
		if(rg.screen == screen_player || rg.screen == screen_main_menu) pfr_col = color4_gen_alpha(255, 255, 255, scale_value_to((int)(ptr->current_time_ms - ptr->timing_section_start_ms) % (int)(ptr->beat_length_ms), ptr->beat_length_ms, 0, 0, 100));
		else if(rg.screen == screen_editor) pfr_col = color4_gen_alpha(50, 50, 50, 50);
		v2f zeropos = song_map_coord_to_play_field(ptr, V2FZERO);
		v2f dim = song_get_playfield_dimensions(ptr);
		if(rg.mode == game_mode_ouendan) { GL_RECT2BOX(FLOATRECT(zeropos.x, zeropos.y, dim.x, dim.y), pfr_col); }
		if(rg.mode == game_mode_mania)
		{
			GL_LINEBOX(FLOATRECT(zeropos.x, zeropos.y, dim.x, dim.y), col_white);
			if(rg.screen == screen_editor)
			{
				glEnable(GL_SCISSOR_TEST);
				GL_SCISSOR_COORDS(zeropos.x, zeropos.y, dim.x - 2, dim.y - 2);
			}
		}
	}
}

void song_draw_grid(song *ptr)
{
	if(rg.mode == game_mode_ouendan && rg.screen == screen_editor && ptr->grid_resolution > 0 && ptr->grid_resolution != SONG_PFW)
	{
		float xdiv = SONG_PFW / ptr->grid_resolution;
		float ydiv = SONG_PFW / ptr->grid_resolution;

		glBegin(GL_LINES);

		int center = 150;
		int grid = 100;

		color4 gcol = col_white;
		int xi;
		for(xi = 0; xi <= SONG_PFH / ydiv; xi++)
		{
			if(xi == (SONG_PFH / ydiv) / 2) gcol.a = center;
			else gcol.a = grid;
			glColor4ub(color4_2func_alpha(gcol));
			v2f x1 = song_map_coord_to_play_field(ptr, V2F(0, xi * ydiv));
			glVertex2f(x1.x, x1.y);
			v2f x2 = song_map_coord_to_play_field(ptr, V2F(SONG_PFW, xi * ydiv));
			glVertex2f(x2.x, x2.y);
		}

		int yi;
		for(yi = 0; yi <= SONG_PFW / xdiv; yi++)
		{
			if(yi == (SONG_PFW / xdiv) / 2) gcol.a = center;
			else gcol.a = grid;
			glColor4ub(color4_2func_alpha(gcol));
			v2f y1 = song_map_coord_to_play_field(ptr, V2F(yi * xdiv, 0));
			glVertex2f(y1.x, y1.y);
			v2f y2 = song_map_coord_to_play_field(ptr, V2F(yi * xdiv, SONG_PFH));
			glVertex2f(y2.x, y2.y);
		}

		glEnd();
	}
	if(rg.mode == game_mode_mania)
	{
		float jline = song_mania_get_judge_line_y(ptr);
		v2f left = song_map_coord_to_play_field(ptr, V2F(0, jline));
		v2f right = song_map_coord_to_play_field(ptr, V2F(SONG_PFW, jline));

		if(rg.screen == screen_editor)
		{
			glBegin(GL_LINES);
			glColor4ub(255, 255, 255, 255);
			glVertex2f(left.x, left.y);
			glVertex2f(right.x, right.y);
			glEnd();
		}

		skin *sk = rg.skin;
		float pfw_fourth = SONG_PFW / 4;
		float pfw_eighth = SONG_PFW / 8;
		float sc = ptr->object_size;

		v2f m_left = song_map_coord_to_play_field(ptr, V2F(pfw_eighth, jline));
		textures_drawsc(sk->m_left_outline, m_left.x, m_left.y, sc, sc, col_white);

		v2f m_down = song_map_coord_to_play_field(ptr, V2F(pfw_eighth + (pfw_fourth), jline));
		textures_drawsc(sk->m_down_outline, m_down.x, m_down.y, sc, sc, col_white);

		v2f m_up = song_map_coord_to_play_field(ptr, V2F(pfw_eighth + (pfw_fourth * 2), jline));
		textures_drawsc(sk->m_up_outline, m_up.x, m_up.y, sc, sc, col_white);

		v2f m_right = song_map_coord_to_play_field(ptr, V2F(pfw_eighth + (pfw_fourth * 3), jline));
		textures_drawsc(sk->m_right_outline, m_right.x, m_right.y, sc, sc, col_white);
	}
}

void song_tick_beat_guess(song *ptr)
{
	if(rg.screen == screen_editor)
	{
		TIME_VAR bgc_ms = timer_milliseconds(ptr->beat_guess_clock);

		if(bgc_ms >= 5000 && ptr->beat_guess_build_up != 0)
		{
			ptr->beat_guess_build_up = 0;
			list_clear(ptr->beat_guess_list, free);
			show_center_status("reset bpm guess state.");
		}

		if(rg.kp[SDL_SCANCODE_B] == 1 && rg.focus)
		{
			if(ptr->music_state == song_paused)
			{
				song_play(ptr);
				ptr->beat_guess_build_up = 0;
			}
			if(ptr->beat_guess_build_up <= 3)
			{
				ptr->beat_guess_build_up++;
				char *msg = dupfmt("tap %d more times...", 5 - ptr->beat_guess_build_up);
				show_notif(msg);
				free(msg);
				timer_restart(ptr->beat_guess_clock);
			}
			else
			{
				int list_limit = 150;
				while(ptr->beat_guess_list->count >= list_limit)
				{
					free(ptr->beat_guess_list->start->val);
					list_pop_front(ptr->beat_guess_list);
				}
				list_push_back(ptr->beat_guess_list, new_float(bgc_ms));

				TIME_VAR sum = 0;
				list_node *gn;
				for(gn = ptr->beat_guess_list->start; gn != NULL; gn = gn->next)
				{
					sum += *((TIME_VAR*) gn->val);
				}
				TIME_VAR guess = sum / ptr->beat_guess_list->count;
				timing_section *cts = song_get_timing_section(ptr, ptr->current_time_ms);
				cts->beat_length_ms = guess;
				timer_restart(ptr->beat_guess_clock);
				char *msg = dupfmt("BPM Guess: %.2f (%d/%d)", beat_length_ms_to_beats_per_minute(cts->beat_length_ms), ptr->beat_guess_list->count, list_limit);
				show_center_status(msg);
				free(msg);
			}
		}
	}
}

void song_tick_slider_controls(song *ptr)
{
	if(rg.screen == screen_editor && ptr->selected_objects->count == 0)
	{
		slider *slider = ptr->selected_object ? ptr->selected_object->slider : NULL;
		if(rg.mode == game_mode_ouendan && slider != NULL)
		{
			slider_curve_type curve_type = slider->curve_type;
			if
			(
				ez_bp
				(
					"slider_switch_curve_type", 
					curve_type == slider_curve_linear  ? "linear"  : 
					curve_type == slider_curve_catmull ? "catmull" : 
					curve_type == slider_curve_bezier  ? "bezier"  : 
					curve_type == slider_curve_circle ? "circle" : 
					"????", 
					PADDING,
					PADDING
				)->clicked
			)
			{
				if(curve_type == slider_curve_linear)  slider->curve_type = slider_curve_catmull;
				else if(curve_type == slider_curve_catmull) slider->curve_type = slider_curve_bezier;
				else if(curve_type == slider_curve_bezier)  slider->curve_type = slider_curve_circle;
				else if(curve_type == slider_curve_circle) slider->curve_type = slider_curve_linear;
				slider_calculate_path(slider, 0);
			}
		}
	}
}

void song_tick_remove(song *ptr)
{
	if(rg.screen == screen_editor)
	{
		char obj_removed = 0;
		list_node *n;
		for(n = ptr->all_objects->start; n != NULL; n = n->next)
		{
			object *obj = n->val;
			if(obj->delete_me)
			{
				if(ptr->selected_objects->count > 0)
				{
					list_node *csn;
					for(csn = ptr->selected_objects->start; csn != NULL; csn = csn->next)
					{
						object *csno = csn->val;
						if(csno == obj)
						{
							list_erase(ptr->selected_objects, csn);
							break;
						}
					}
				}
				song_capture_undo_state(ptr);
				ptr->selected_object = NULL;
				object_free(obj);
				list_erase(ptr->all_objects, n);
				obj_removed = 1;
				break;
			}
		}
		if(obj_removed) song_sort_objects(ptr);
	}
}

char song_check_hovered(song *ptr)
{
	char ret = 0;
	list_node *n;
	for(n = ptr->all_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		if(object_should_tick(obj) && object_check_hovered(obj)) ret = 1;
	}
	return ret;
}

void song_reset_game_mode_objects_list(song *ptr)
{
	list_clear(ptr->game_mode_objects, list_dummy);
	list_node *n;
	for(n = ptr->all_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		if(object_compatable_with_current_gm(obj)) list_push_back(ptr->game_mode_objects, obj);
	}
}

object *before_next_to_be_clicked;
object *next_to_be_clicked;

void song_tick_bef_next_obj_clicked(song *ptr)
{
	before_next_to_be_clicked = NULL;
	next_to_be_clicked = NULL;

	list_node *n;
	for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		if(obj->start_time_ms + obj->length_time_ms <= ptr->current_time_ms - ptr->approach_time_ms) continue;
		if(obj->start_judgement == judgement_unknown)
		{
			next_to_be_clicked = obj;
			if(n->before == NULL) before_next_to_be_clicked = NULL;
			else before_next_to_be_clicked = n->before->val;
			break;
		}
	}
}

object *song_update_objects(song *ptr)
{
	object *ret = NULL;
	list_node *n;
	for(n = ptr->game_mode_objects->end; n != NULL; n = n->before)
	{
		object *obj = n->val;
		obj->relative_time_ms = ptr->current_time_ms - obj->start_time_ms;
		if(object_should_tick(obj))
		{
			if(rg.screen == screen_editor && obj->relative_time_ms < 0 && obj->start_judgement != judgement_unknown) object_reset(obj);
			object_update(obj);
		}
	}
	for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		if(object_should_tick(obj) && ret == NULL && obj->end_judgement == judgement_unknown) ret = obj;
	}
	return ret;
}

void song_render_objects(song *ptr)
{
	list_node *n;
	for(n = ptr->game_mode_objects->end; n != NULL; n = n->before)
	{
		object *obj = n->val;
		if(object_should_tick(obj)) object_render(obj);
	}
	if(rg.screen == screen_editor && rg.mode == game_mode_mania) glDisable(GL_SCISSOR_TEST);
}

void song_draw_approach_circles(song *ptr)
{
	if(rg.mode == game_mode_ouendan)
	{
		list_node *n;
		for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
		{
			object *obj = n->val;
			if(obj->circle != NULL && object_should_tick(obj)) approach_circle_draw(obj);
		}
	}
}

void song_draw_explosions(song *ptr)
{
	if(rg.screen == screen_player && !rg.potato_pc_enabled)
	{
		list_node *ce;
		for(ce = ptr->explosions->start; ce != NULL; ce = ce->next)
		{
			explosion *ceo = ce->val;
			explosion_tick(ceo);
		}
	}
	
	if(ptr->explosions->count > 0)
	{
		explosion *first_expl = ptr->explosions->start->val;
		if(ptr->current_time_ms - first_expl->on_ms >= 2000)
		{
			free(first_expl);
			list_pop_front(ptr->explosions);
		}
	}
}

void song_tick_bot(song *ptr)
{
	if(rg.mode == game_mode_ouendan)
	{
		if(!rg.replay_mode_enabled)
		{
			if(before_next_to_be_clicked == NULL && next_to_be_clicked == NULL && ptr->game_mode_objects->end != NULL) before_next_to_be_clicked = ptr->game_mode_objects->end->val;

			char on_long = 0;
			float bntbcx = SONG_PFW / 2;
			float bntbcy = SONG_PFH / 2;
			TIME_VAR bntbcsms = rg.screen == screen_editor ? 0 : -SONG_BEFORE_START_TIME_MS;
			if(before_next_to_be_clicked != NULL)
			{
				if(before_next_to_be_clicked->type == object_slider)
				{
					slider *bnsl = before_next_to_be_clicked->slider;
					bntbcx = bnsl->cur_point.x;
					bntbcy = bnsl->cur_point.y;
					rg.mouse_x_bot = bnsl->cur_point.x;
					rg.mouse_y_bot = bnsl->cur_point.y;
					bntbcsms = before_next_to_be_clicked->start_time_ms + before_next_to_be_clicked->length_time_ms;
					on_long = before_next_to_be_clicked->start_judgement != judgement_unknown && ptr->current_time_ms <= bntbcsms;
					if(rg.screen == screen_player && rg.bot_enabled)
					{
						if(on_long) rg.ouendan_click_1++;
						else rg.ouendan_click_1 = 0;
					}
				}
				else if(before_next_to_be_clicked->type == object_spinner)
				{
					int time = (int) before_next_to_be_clicked->relative_time_ms * 4;
					double val = (time * ((2.0f * PI_NUM) / (360))) - (PI_NUM / 2);
					int radius = CIRCLE_RADIUS / 2;
					int area = radius / 2;
					rg.mouse_x_bot = before_next_to_be_clicked->pos.x + (area * cos(val));
					rg.mouse_y_bot = before_next_to_be_clicked->pos.y + (area * sin(val));
					bntbcsms = before_next_to_be_clicked->start_time_ms + before_next_to_be_clicked->length_time_ms;
					on_long = before_next_to_be_clicked->start_judgement != judgement_unknown && ptr->current_time_ms <= bntbcsms;
					if(rg.screen == screen_player && rg.bot_enabled)
					{
						if(on_long) rg.ouendan_click_1++;
						else rg.ouendan_click_1 = 0;
					}
				}
				else
				{
					bntbcx = before_next_to_be_clicked->pos.x;
					bntbcy = before_next_to_be_clicked->pos.y;
					bntbcsms = before_next_to_be_clicked->start_time_ms;
				}
			}
			float ntbcx = SONG_PFW / 2;
			float ntbcy = SONG_PFH / 2;
			TIME_VAR ntbcsms = 0.0f;
			if(ptr->game_mode_objects->end != NULL)
			{
				object *eo = ptr->game_mode_objects->end->val;
				ntbcsms = eo->start_time_ms + eo->length_time_ms +  SONG_END_PADDING_MS;
			}
			if(next_to_be_clicked != NULL)
			{
				ntbcx = next_to_be_clicked->pos.x;
				ntbcy = next_to_be_clicked->pos.y;
				ntbcsms = next_to_be_clicked->start_time_ms;
			}

			if(!on_long)
			{
				TIME_VAR ctms = clamp(ptr->current_time_ms, bntbcsms, ntbcsms);
				/* TIME_VAR fctms = apply_easing(easing_type_out, ctms - bntbcsms, bntbcsms, ntbcsms - bntbcsms, ntbcsms - bntbcsms); */
				rg.mouse_x_bot = scale_value_to(/* f */ctms, bntbcsms, ntbcsms, bntbcx, ntbcx);
				rg.mouse_y_bot = scale_value_to(/* f */ctms, bntbcsms, ntbcsms, bntbcy, ntbcy);
			}

			if(rg.screen == screen_player && rg.bot_enabled)
			{
				if
				(
					next_to_be_clicked
					&&
					ptr->current_time_ms >= next_to_be_clicked->start_time_ms
					&&
					next_to_be_clicked->start_judgement == judgement_unknown
					&&
					(
						before_next_to_be_clicked == NULL
						||
						(
							before_next_to_be_clicked->start_judgement != judgement_unknown
							&&
							before_next_to_be_clicked->end_judgement != judgement_unknown
						)
					)
				)
					rg.ouendan_click_1++;
				else if(!on_long) rg.ouendan_click_1 = 0;

				/*
				int time = (int) (ptr->current_time_ms / 6);
				double val = time * ((2.0f * PI_NUM) / 360);
				int radius = (CIRCLE_RADIUS * ptr->object_size_pf);
				int area = radius / 2;
				rg.mop_x = rg.mouse_x_bot + (area * cos(val));
				rg.mop_y = rg.mouse_y_bot + (area * sin(val));
				*/

				rg.mop_x = rg.mouse_x_bot;
				rg.mop_y = rg.mouse_y_bot;
			}
		}
	}
	if(rg.mode == game_mode_mania && (rg.bot_enabled && !rg.replay_mode_enabled && rg.screen == screen_player))
	{
		rg.mania_click_1 = 0;
		rg.mania_click_2 = 0;
		rg.mania_click_3 = 0;
		rg.mania_click_4 = 0;
		list_node *n;
		for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
		{
			object *obj = n->val;
			if
			(
				obj->start_time_ms >= ptr->current_time_ms - MISS_TIME
				&&
				ptr->current_time_ms >= obj->start_time_ms
				&&
				obj->start_judgement == judgement_unknown
			) 
				*(mania_note_get_key(obj->mania_note)) += 1;
		}
	}
}

void song_render_editor_cursor(song *ptr)
{
	if(rg.mode == game_mode_ouendan && rg.screen == screen_editor && ptr->music_state == song_playing)
	{
		v2f cpos = song_map_coord_to_play_field(ptr, V2F(rg.mouse_x_bot, rg.mouse_y_bot));
		textures_drawsc(rg.skin->cursor, cpos.x, cpos.y, ptr->object_size, ptr->object_size, color4_mod_alpha(col_white, 150));
	}
}

void song_tick_object_move(song *ptr, char any_obj_hovered, char any_selected_obj_hovered)
{
	if(rg.screen == screen_editor)
	{
		if(ptr->selected_objects->count > 0)
		{
			if(ptr->dragging_selected_objects && rg.gui_render_end_of_frame == ptr)
			{
				list_node *seln;
				for(seln = ptr->selected_objects->start; seln != NULL; seln = seln->next)
				{
					object *selno = seln->val;
					if(selno->type == object_slider)
					{
						if(selno->slider->control_points->count > 0)
						{
							float sdx = rg.mop_fdx;
							float sdy = rg.mop_fdy;

							list_node *cn;
							for(cn = selno->slider->control_points->start; cn != NULL; cn = cn->next)
							{
								v2f *p = cn->val;
								p->x += sdx;
								p->y += sdy;
							}
							for(cn = selno->slider->curve_points->start; cn != NULL; cn = cn->next)
							{
								v2f *p = cn->val;
								p->x += sdx;
								p->y += sdy;
							}

							selno->slider->host_obj->pos.x += sdx;
							selno->slider->host_obj->pos.y += sdy;
							
							slider_regenerate_texture(selno->slider);

							selno->do_grid_snap = 1;
						}
					}
					else if(selno->type == object_circle)
					{
						selno->pos.x += rg.mop_fdx;
						selno->pos.y += rg.mop_fdy;
						selno->do_grid_snap = 1;
					}
				}	
			}
			else if(!any_selected_obj_hovered && any_obj_hovered && left_click > 0)
			{
				song_unselect_object(ptr);
				song_clear_selected_objects(ptr);
			}
		}
		else
		{
			object *obj = ptr->selected_object;
			if(obj != NULL && obj->dragging && left_click > 0)
			{
				object_type type = ptr->selected_object->type;
				if(type == object_circle)
				{
					obj->pos.x += rg.mop_fdx;
					obj->pos.y += rg.mop_fdy;
					obj->do_grid_snap = 1;
				}
				else if(type == object_slider)
				{
					slider *sl = obj->slider;
					if(!obj->is_new && sl->selected_control_point == NULL)
					{
						float sdx = rg.mop_fdx;
						float sdy = rg.mop_fdy;

						list_node *cn;
						for(cn = sl->control_points->start; cn != NULL; cn = cn->next)
						{
							slider_control_point *p = cn->val;
							p->pos.x += sdx;
							p->pos.y += sdy;
						}
						for(cn = sl->curve_points->start; cn != NULL; cn = cn->next)
						{
							v2f *p = cn->val;
							p->x += sdx;
							p->y += sdy;
						}

						obj->pos.x += sdx;
						obj->pos.y += sdy;

						if(mouse.moved || left_click == 1) slider_regenerate_texture(sl);
						obj->do_grid_snap = 1;
					}
				}
			}
		}
	}
}

void song_render_sels(song *ptr)
{
	if(rg.screen == screen_editor)
	{
		list_node *seln;
		for(seln = ptr->selected_objects->start; seln != NULL; seln = seln->next)
		{
			object *selno = seln->val;
			float_rect bounds = FLOATRECTZERO;
			if(selno->type == object_slider) bounds = song_map_float_rect_to_play_field(ptr, selno->slider->bounds);
			else if(selno->type == object_circle)
			{
				float r = CIRCLE_RADIUS * ptr->object_size;
				v2f pop = song_map_coord_to_play_field(ptr, selno->pos);
				bounds.left = pop.x - r;
				bounds.width = pop.x + r;
				bounds.top = pop.y - r;
				bounds.height = pop.y + r;
			}
			GL_LINESTRIP(bounds, col_cyan);
		}
	}
}

char song_tick_selection(song *ptr, char any_obj_hovered)
{
	if(rg.screen == screen_editor)
	{
		char selecting = 
			rg.gui_render_end_of_frame == ptr
			&&
			rg.screen == screen_editor
			&&
			!ptr->obj_hovered_on_click
			&&
			(
				ptr->selected_object == NULL
				&&
				!
				(
					left_click == 1
					&&
					any_obj_hovered
				)
				&&
				!ptr->slider_control_point_box_hovered
				&&
				left_click > 0
				&&
				mouse.dragged
				&&
				rg.is_mouse_on_click_op
			);
		if(selecting)
		{
			song_clear_selected_objects(ptr);
			if(left_click == 1)
			{
				ptr->selection_rect.left = mouse.x;
				ptr->selection_rect.top = mouse.y;
				ptr->selection_rect.width = mouse.x;
				ptr->selection_rect.height = mouse.y;
			}

			ptr->selection_rect.width += mouse.delta_x;
			ptr->selection_rect.height += mouse.delta_y;
		}
		else
		{
			ptr->selection_rect.left = mouse.x;
			ptr->selection_rect.top = mouse.y;
			ptr->selection_rect.width = mouse.x;
			ptr->selection_rect.height = mouse.y;
		}
		
		float_rect sel_rect = FLOATRECT(ptr->selection_rect.left, ptr->selection_rect.top, ptr->selection_rect.width - ptr->selection_rect.left, ptr->selection_rect.height - ptr->selection_rect.top);
		GL_RECT2BOX(sel_rect, color4_mod_alpha(col_white, 50));
		GL_LINEBOX(sel_rect, col_white);
		return selecting;
	}
	return 0;
}

char song_tick_any_obj_hovered(song *ptr)
{
	if(rg.screen == screen_editor)
	{
		list_node *n;
		for(n = ptr->selected_objects->start; n != NULL; n = n->next)
		{
			object *obj = n->val;
			if(obj->hovered) return 1;
		}
	}
	return 0;
}

void song_tick_obj_editor_input(song *ptr, char selecting, char any_selected_object_hovered)
{
	if(rg.screen == screen_editor && rg.focus && rg.gui_render_end_of_frame == ptr)
	{
		char del = 0;
		char recalc_nums_after = 0;

		float nearest_beat = song_align_ms(ptr, ptr->current_time_ms);
		if
		(
			rg.mode == game_mode_mania
		)
		{
			if
			(
			left_click_released
			&&
			rg.is_mouse_on_playfield
			&&
			rg.is_mouse_on_click_op
			&&
			get_distf(mouse.x, mouse.y, mouse.x_on_click, mouse.y_on_click) <= 1
			)
			{
				float pfw_fourth = SONG_PFW / 4;
				song_add_mania_note(ptr, clamp(rg.mop_x / pfw_fourth, 0, 3), nearest_beat);
			}
			else
			{
				if(rg.mania_click_1 == 1) song_add_mania_note(ptr, 0, nearest_beat);
				if(rg.mania_click_2 == 1) song_add_mania_note(ptr, 1, nearest_beat);
				if(rg.mania_click_3 == 1) song_add_mania_note(ptr, 2, nearest_beat);
				if(rg.mania_click_4 == 1) song_add_mania_note(ptr, 3, nearest_beat);
			}
		}

		if
		(
			rg.mode == game_mode_ouendan
			&&
			rg.is_mouse_on_playfield
			&&
			rg.is_mouse_on_click_op
			&&
			!ptr->mouse_hovering_slider_body
			&&
			!ptr->slider_control_point_box_hovered
			&&
			!(ptr->selected_object != NULL && ptr->selected_object->slider != NULL && ptr->selected_object->is_new)
		)
		{
			char slider_new_cp = (ptr->selected_object != NULL && ptr->selected_object->slider != NULL && rg.kp[SDL_SCANCODE_LCTRL]);
			if(left_click == 1 && (ptr->selected_objects->count > 0 || ptr->selected_object != NULL) && !slider_new_cp)
			{
				song_clear_selected_objects(ptr);
				song_unselect_object(ptr);
			}
			else if(left_click_released && get_distf(mouse.x, mouse.y, mouse.x_on_click, mouse.y_on_click) <= 1)
			{
				float div = SONG_PFW / ptr->grid_resolution;
				float snap_x = clamp(round(rg.mop_x / div) * div, 0, SONG_PFW);
				float snap_y = clamp(round(rg.mop_y / div) * div, 0, SONG_PFH);

				char space_free = 1;
				list_node *n;
				for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
				{
					object *obj = n->val;
					if(obj->start_time_ms == nearest_beat)
					{
						space_free = 0;
						break;
					}
				}

				if(space_free)
				{
					object_type mode = ptr->editor_placement_type;
					if(mode == object_circle) song_add_circle(ptr, snap_x, snap_y, nearest_beat);
					else if(mode == object_slider)
					{
						object *obj = object_init(snap_x, snap_y, object_slider, nearest_beat, 0, ptr);
						list_push_front(ptr->all_objects, obj);
						slider *sl = obj->slider;
						v2f po = V2F(snap_x, snap_y);
						slider_add_control_point(sl, po);
						slider_add_control_point(sl, po);
						obj->is_new = 1;
						ptr->selected_object = obj;
					}
					else if(mode == object_spinner)
					{
						object *obj = object_init(SONG_PFW / 2, SONG_PFH / 2, object_spinner, nearest_beat, 5000, ptr);
						list_push_front(ptr->all_objects, obj);
						obj->is_new = 1;
						ptr->selected_object = obj;
					}
					song_sort_objects(ptr);
				}
				else show_center_status("there's already an object at this time...");
			}
		}

		list_node *n;
		for(n = ptr->game_mode_objects->end; n != NULL; n = n->before)
		{
			object *obj = n->val;

			if(object_should_tick(obj))
			{
				if(rg.mode == game_mode_ouendan)
				{
					if(selecting)
					{
						char add = 0;
						v2f rcp = song_map_coord_to_play_field(ptr, obj->pos);
						if(obj->type == object_slider)
						{
							list_node *sn;
							for(sn = obj->slider->curve_points->start; sn != NULL; sn = sn->next)
							{
								v2f *pt = sn->val;
								v2f rpt = song_map_coord_to_play_field(ptr, *pt);
								if(FR_CONTAINS(FR_XYLOWEST(ptr->selection_rect), rpt.x, rpt.y))
								{
									add = 1;
									break;
								}
							}
						}
						else if(obj->type == object_circle && FR_CONTAINS(FR_XYLOWEST(ptr->selection_rect), rcp.x, rcp.y)) add = 1;

						if(add)
						{
							char already_in = 0;
							list_node *seln;
							for(seln = ptr->selected_objects->start; seln != NULL; seln = seln->next)
							{
								object *selno = seln->val;
								if(selno == obj)
								{
									already_in = 1;
									break;
								}
							}
							if(!already_in) list_push_back(ptr->selected_objects, obj);
						}
					}
					if(obj->hovered && !(ptr->selected_object != NULL && ptr->selected_object->slider != NULL && ptr->selected_object->is_new))
					{
						if(left_click == 1) ptr->selected_object = obj;
					}
				}
			}
		}
		for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
		{
			object *obj = n->val;

			if(object_should_tick(obj))
			{
				if(rg.mode == game_mode_ouendan)
				{
					if(obj->hovered && !(ptr->selected_object != NULL && ptr->selected_object->slider != NULL && ptr->selected_object->is_new) && rg.is_mouse_on_click_op)
					{
						if(middle_click == 1 && !recalc_nums_after)
						{
							obj->new_combo = !obj->new_combo;
							recalc_nums_after = 1;
						}
						else if(right_click == 1 && !del && !(ptr->slider_control_point_box_hovered))
						{
							if(ptr->selected_objects->count > 0 && any_selected_object_hovered)
							{
								song_delete_selected_objects(ptr);
								break;
							}
							else obj->delete_me = 1;
							del = 1;
						}
					}
				}
			}
		}

		if(recalc_nums_after) song_sort_objects(ptr);
	}
}

void song_tick_obj_input(song *ptr, char selecting)
{
	char first_obj_wiggled = 0;

	list_node *n;

	for(n = ptr->game_mode_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;

		if(object_should_tick(obj))
		{
			if
			(
				ptr->music_state == song_playing
				&&
				obj->type != object_spinner
				&&
				obj->start_judgement == judgement_unknown
			)
			{
				char is_next_obj = 
				rg.mode == game_mode_ouendan ? next_to_be_clicked == obj : 
				obj->relative_time_ms >= -MISS_TIME;
				char click_try = 0;
				if(rg.mode == game_mode_ouendan) click_try = obj->hovered && (rg.ouendan_click_1 == 1 || rg.ouendan_click_2 == 1);
				if(rg.mode == game_mode_mania) click_try = mania_note_key_good(obj->mania_note);
				if(rg.screen == screen_player && !is_next_obj && click_try && !obj->should_wiggle && !first_obj_wiggled)
				{
					object_wiggle(obj);
					first_obj_wiggled = 1;
				}
				if((is_next_obj && click_try) || (rg.screen == screen_editor && obj->relative_time_ms >= 0))
				{
					replay_add_frame(ptr->replay, ptr, ptr->current_time_ms, rg.mop_x, rg.mop_y, rg.ouendan_click_1, rg.ouendan_click_2, 0, 0, 0, replay_note_click, obj->number);
					object_click(obj, ptr->current_time_ms);
					if(rg.mode == game_mode_ouendan) break;
				}
			}
		}
	}
}

void song_tick_play_ui(song *ptr)
{
	if(rg.screen == screen_player)
	{
		health_bar_update(&(ptr->hbar));
		health_bar_render(&(ptr->hbar));
	}
}

void song_tick(song *ptr)
{
	if(rg.screen == screen_editor && ptr->music_state == song_playing && ptr->current_time_ms >= ptr->timing_section_start_ms)
	{
		int cur_stanza = (ptr->current_time_ms - ptr->timing_section_start_ms) / ptr->beat_length_ms;
		if(ptr->metronome_stanza > cur_stanza) ptr->metronome_stanza = -1;
		if(ptr->metronome_on && cur_stanza > ptr->metronome_stanza)
		{
			ptr->metronome_stanza = cur_stanza;
			ae_sound_effect_play(rg.skin->metronome);
		}
	}

	if(rg.screen != screen_player && ptr->undo_states->count == 0) song_capture_undo_state(ptr);

	if(rg.resized && rg.mode == game_mode_ouendan) song_all_sliders_del_textures(ptr);

	char any_obj_hovered = song_check_hovered(ptr);

	if(left_click == 1) ptr->obj_hovered_on_click = any_obj_hovered;

	song_tick_remove(ptr);

	song_tick_beat_guess(ptr);

	timeline_tick(&ptr->tl, ptr);

	song_draw_grid(ptr);

	song_draw_playfield_border(ptr);

	song_tick_bef_next_obj_clicked(ptr);

	char selecting = song_tick_selection(ptr, any_obj_hovered);

	char any_selected_obj_hovered = song_tick_any_obj_hovered(ptr);

	if
	(
		left_click == 1
		&&
		any_selected_obj_hovered
		&&
		rg.is_mouse_on_playfield
		&&
		!ptr->slider_control_point_box_hovered
	) ptr->dragging_selected_objects = 1;

	if(ptr->dragging_selected_objects && (left_click == 0 || ptr->selected_objects->count < 1))
	{
		ptr->dragging_selected_objects = 0;
		list_node *n;
		for(n = ptr->selected_objects->start; n != NULL; n = n->next) object_snap_to_grid(n->val);
	}

	if(!selecting) song_tick_object_move(ptr, any_obj_hovered, any_selected_obj_hovered);

	if(!ptr->dragging_selected_objects) song_tick_obj_editor_input(ptr, selecting, any_selected_obj_hovered);

	if(rg.screen == screen_editor && ptr->selected_object != NULL)
	{
		if(!object_should_tick(ptr->selected_object))
		{
			ptr->selected_object->is_new = 0;
			song_unselect_object(ptr);
		}
	}

	ptr->slider_control_point_box_hovered = 0;
	ptr->mouse_hovering_slider_body = 0;
	ptr->slider_startend_hit = 0;

	song_tick_slider_controls(ptr);

	if(rg.ouendan_click_1 == 1) rg.ouendan_click_1_frame_1_time_ms = ptr->current_time_ms;
	if(rg.ouendan_click_2 == 1) rg.ouendan_click_2_frame_1_time_ms = ptr->current_time_ms;
	
	song_update_objects(ptr);
	if(rg.replay_mode_enabled)
	{
		replay_frame *fr = replay_get_frame(ptr->replay, ptr->current_time_ms, 1);
		if(fr && !fr->done)
		{
			object *gn = song_get_obj_by_number(ptr, fr->objnum);
			if(gn)
			{
				if(fr->event == replay_note_click)
				{
					object_click(gn, fr->time_ms);
				}
				if(gn->type == object_slider && fr->event == replay_hold_toggle)
				{
					slider *sl = gn->slider;
					sl->is_good = !sl->is_good;
				}
				fr->done = 1;
			}
		}
	}

	song_tick_bot(ptr);

	if(rg.mode == game_mode_ouendan) replay_add_frame(ptr->replay, ptr, ptr->current_time_ms, rg.mop_x, rg.mop_y, rg.ouendan_click_1, rg.ouendan_click_2, 0, 0, 0, 0, 0);
	if(rg.mode == game_mode_mania) replay_add_frame(ptr->replay, ptr, ptr->current_time_ms, rg.mop_x, rg.mop_y, rg.mania_click_1, rg.mania_click_2, rg.mania_click_3, rg.mania_click_4, 0, 0, 0);

	song_tick_obj_input(ptr, selecting);

	song_tick_play_ui(ptr);

	song_render_objects(ptr);

	song_draw_approach_circles(ptr);

	song_draw_explosions(ptr);

	song_render_sels(ptr);

	song_render_editor_cursor(ptr);

	if(rg.screen == screen_player && rg.replay_mode_enabled) text3_fmt(rg.win_width_mid, rg.win_height_mid / 2, rg.win_width, 255, origin_center, origin_center, text3_justification_center, "~uYou are watching a replay by ~c%s~u.", rg.song->replay->player_name);
}

float song_calc_accuracy(song *ptr, char live)
{
	if(ptr->good_hits == 0) return 0.0f;
	if(ptr->passed_segs == 0) return 100.0f;
	int segs = live ? ptr->passed_segs : ptr->game_mode_objects->count;
	return ((return_lowest(ptr->good_hits, segs) / return_highest(ptr->good_hits, segs))) * 100;
}

char song_calc_rank(song *ptr)
{
	float acc = song_calc_accuracy(ptr, 0);
	return acc == 100.0f ? 'P' : acc >= 99.0f ? 'G' : acc >= 95.0f ? 'A' : acc >= 88.0f ? 'B' : acc >= 70.0f ? 'C' : 'F';
}

void song_inc_combo(song *ptr)
{
	health_bar_add(&ptr->hbar, 10);
	++ptr->current_combo;
	if(ptr->current_combo > ptr->highest_combo) ptr->highest_combo = ptr->current_combo;
}

void song_break_combo(song *ptr)
{
	if(rg.screen == screen_editor) return;
	health_bar_sub(&(ptr->hbar), 10);
	if(ptr->current_combo >= 10) ae_sound_effect_play(rg.skin->miss_eff);
	if(ptr->current_combo > ptr->highest_combo) ptr->highest_combo = ptr->current_combo;
	ptr->current_combo = 0;
	++ptr->misses;
}

void song_reset_score(song *ptr)
{
	ptr->score = 0;
	ptr->passed_segs = 0;
	ptr->good_hits = 0;
	ptr->misses = 0;
	ptr->current_combo = 0;
	ptr->highest_combo = 0;
	ptr->failed = 0;
	if(!rg.replay_mode_enabled) replay_clear_frames(ptr->replay);
	health_bar_reset(&ptr->hbar, ptr);
}

void song_reset_object_state(song *ptr)
{
	song_clear_explosions(ptr);
    list_node *n;
    for(n = ptr->all_objects->start; n != NULL; n = n->next)
    {
        object *obj = n->val;
		object_reset(obj);
    }
}

void song_capture_undo_state(song *ptr)
{
	if(ptr->undo_states->count > 100)
	{
		free(ptr->undo_states->start->val);
		list_pop_front(ptr->undo_states);
	}
	song_chop_undo_states(ptr);
	list_push_back(ptr->undo_states, song_serialize(ptr));
	if(ptr->undo_states->count > 1) ptr->undo_state++;
	ptr->edit_count++;
}

void song_chop_undo_states(song *ptr)
{
	if(ptr->undo_states->count < 1) return;
	while(ptr->undo_states->count - 1 > ptr->undo_state)
	{
		free(ptr->undo_states->end->val);
		list_pop_back(ptr->undo_states);
	}
}

void song_undo(song *ptr)
{
	if(ptr->undo_states->count <= 1)
	{
		show_notif("nothing to undo...");
		return;
	}
	if(ptr->undo_state == 0)
	{
		show_notif("can't undo any further");
		return;
	}

	song_overwrite(ptr, list_get_val_at(ptr->undo_states, clamp(--ptr->undo_state, 0, ptr->undo_states->count - 1)), 1);

	show_notif("undo!");

	song_clear_selected_objects(ptr);
	song_reset_object_state(ptr);
}

void song_redo(song *ptr)
{
	if(ptr->undo_states->count <= 1)
	{
		show_notif("nothing to redo...");
		return;
	}
	if(ptr->undo_state == ptr->undo_states->count - 1)
	{
		show_notif("can't redo any further");
		return;
	}

	song_overwrite(ptr, list_get_val_at(ptr->undo_states, clamp(++ptr->undo_state, 0, ptr->undo_states->count - 1)), 1);

	show_notif("redo!");

	song_clear_selected_objects(ptr);
	song_reset_object_state(ptr);
}

void song_clear_explosions(song *ptr)
{
	list_clear(ptr->explosions, free);
}

void song_clear_replays(song *ptr)
{
	list_clear(ptr->replays, replay_free);
}

void song_clear_objects(song *ptr, char cap_undo_state)
{
    if(ptr->all_objects->count > 0)
    {
		song_unselect_object(ptr);
		if(cap_undo_state) song_capture_undo_state(ptr);
        list_clear(ptr->all_objects, object_free);
		song_sort_objects(ptr);
    }
}

void song_clear_timing_sections(song *ptr)
{
	list_clear(ptr->timing_sections, timing_section_free);
}

timing_section *song_apply_timing_sec(song *ptr, TIME_VAR time_ms)
{
	timing_section *cts = song_get_timing_section(ptr, time_ms);
	if(cts != NULL)
	{
		const_write_float(&ptr->beat_length_ms, cts->beat_length_ms);
		const_write_float(&ptr->timing_section_start_ms, cts->start_ms);
		const_write_float(&ptr->beat_divisor, clamp(cts->beat_divisor, 1.0f, SONG_MAX_BEAT_DIVIDE));
	}
	return cts;
}

timing_section *song_get_timing_section(song *ptr, TIME_VAR start_ms)
{
	timing_section *ret = NULL;
	list_node *n;
	for(n = ptr->timing_sections->start; n != NULL; n = n->next)
	{
		timing_section *ts = n->val;
		if(start_ms >= ts->start_ms - 1) ret = ts;
	}
	return ret;
}

void song_set_time(song *ptr, TIME_VAR time_ms)
{
    time_ms = clamp(time_ms, 0, ptr->length_ms);
	if(time_ms == ptr->length_ms)
	{
		ptr->music_state = song_paused;
		ae_music_handle_do(ptr->music, ae_stop);
	}
	if(ptr->music_state == song_playing) ae_music_handle_set_time_ms(ptr->music, time_ms);
	ptr->cur_time_micro_sec = time_ms * 1000.0f;
	ptr->current_time_ms = ptr->cur_time_micro_sec / 1000;
}

void song_set_time_to_song_time(song *ptr)
{
	if(ae_music_handle_is_null(ptr->music)) return;
	ptr->cur_time_micro_sec = ae_music_handle_get_time_ms(ptr->music) * 1000;
	ptr->current_time_ms = ptr->cur_time_micro_sec / 1000;
}

void song_scroll(song *ptr, int dir)
{
	song_apply_timing_sec(ptr, ptr->current_time_ms);
	float div = (ptr->beat_length_ms / ptr->beat_divisor) * abs(dir);
	if(ptr->align_on_scroll)
	{
		if(dir > 0) song_set_time(ptr, song_align_ms(ptr, ptr->current_time_ms + (div / 2)));
		if(dir < 0) song_set_time(ptr, song_align_ms(ptr, ptr->current_time_ms - (div / 2)));
		ptr->align_on_scroll = 0;
		return;
	}
	if(ptr->music_state == song_playing) song_set_time(ptr, clamp(ptr->current_time_ms + ((((dir > 0 ? ptr->beat_length_ms : -ptr->beat_length_ms) * ptr->speed_multiplier) * abs(dir)) + -ptr->song_global_offset), 0, ptr->length_ms));
	else if(ptr->music_state == song_paused)
	{
		if(dir > 0) song_set_time(ptr, ptr->current_time_ms + div);
		if(dir < 0) song_set_time(ptr, ptr->current_time_ms - div);
	}
	song_reset_object_state(ptr);
}

TIME_VAR song_align_ms_unclamped(song *ptr, TIME_VAR unaligned_ms)
{
	TIME_VAR div = ptr->beat_length_ms / ptr->beat_divisor;
	TIME_VAR off = fmodf(ptr->timing_section_start_ms, div);
	TIME_VAR nearest_beat = round((unaligned_ms - off) / div) * div;
	return nearest_beat + off;
}

TIME_VAR song_align_ms(song *ptr, TIME_VAR unaligned_ms)
{
	song_apply_timing_sec(ptr, unaligned_ms);
	return clamp(song_align_ms_unclamped(ptr, unaligned_ms), 0, ptr->length_ms);
}

void song_align_ctms(song *ptr)
{
	song_set_time(ptr, song_align_ms(ptr, ptr->current_time_ms));
}

void song_all_sliders_del_textures(song *ptr)
{
	list_node *n;
	for(n = ptr->all_objects->start; n != NULL; n = n->next)
	{
		object *obj = n->val;
		if(obj->type == object_slider) slider_delete_tex(obj->slider);
	}
}

void song_all_sliders_recalc_paths(song *ptr)
{
	song_calc_difficulty(ptr);
	list_node *obrn;
	for(obrn = ptr->all_objects->start; obrn != NULL; obrn = obrn->next)
	{
		object *oobj = obrn->val;
		if(oobj->type == object_slider)
			slider_calculate_path(oobj->slider, 1);
	}
}

v2f song_map_coord_to_play_field(song *ptr, v2f object_coord)
{
    v2f ret;

	float margin = song_get_pf_margin(ptr);

	float margin_x_pix = SONG_PFW * margin;
	ret.x = scale_value_to(object_coord.x, 0, SONG_PFW, margin_x_pix, rg.win_width_fixed - margin_x_pix)
		+ (rg.win_width > rg.win_width_fixed ? (rg.win_width - rg.win_width_fixed) / 2 : 0);

	float margin_y_pix = SONG_PFH * margin;
	ret.y = scale_value_to(object_coord.y, 0, SONG_PFH, margin_y_pix, rg.win_height_fixed - margin_y_pix)
		+ (rg.win_height > rg.win_height_fixed ? (rg.win_height - rg.win_height_fixed) / 2 : 0);
	
	ret.y -= (rg.screen == screen_editor ? (CIRCLE_RADIUS / 2) : rg.mode == game_mode_mania ? margin_y_pix : 0);

    return ret;
}

float_rect song_map_float_rect_to_play_field(song *ptr, float_rect rect)
{
	v2f p1 = song_map_coord_to_play_field(ptr, V2F(rect.left, rect.top));
	v2f p2 = song_map_coord_to_play_field(ptr, V2F(rect.width, rect.height));
	return FLOATRECT
	(
		p1.x,
		p1.y,
		p2.x,
		p2.y
	);
}

v2f song_get_playfield_dimensions(song *ptr)
{
	v2f ret = V2F(SONG_PFW, SONG_PFH);

	float margin = song_get_pf_margin(ptr);
	float margin_x_pix = SONG_PFW * margin;
	ret.x = scale_value_to(SONG_PFW, 0, SONG_PFW, margin_x_pix, rg.win_width_fixed - (margin_x_pix * 2));
	float margin_y_pix = SONG_PFH * margin;
	ret.y = scale_value_to(SONG_PFH, 0, SONG_PFH, margin_y_pix, rg.win_height_fixed - (margin_y_pix * 2));

	return ret;
}

float song_get_pf_margin(song *ptr)
{
	if(rg.screen == screen_editor) return 0.08f;
	if(rg.mode == game_mode_mania) return 0.5f * ptr->object_size;
	return 0;
}

float song_mania_get_judge_line_y(song *ptr)
{
	return SONG_MANIA_JUDGEMENT_LINE_Y_BOTTOM;	
}

float song_mania_map_time_to_y(song *ptr, TIME_VAR rel_time_ms)
{
	float line_y = song_map_coord_to_play_field(ptr, V2F(0, song_mania_get_judge_line_y(ptr))).y;
	float zero_y = song_map_coord_to_play_field(ptr, V2F(0, SONG_PFH)).y;
	return scale_value_to(rel_time_ms, -(ptr->approach_time_ms / 1.79), 0, zero_y, line_y);
}

void song_sort_objects(song *ptr)
{
    list_node *sn;

	char swapped;

	do
	{
		swapped = 0;
		sn = ptr->all_objects->start;
		if(sn == NULL) break;
		list_node *ln = NULL;
		while(sn->next != ln)
		{
			object *o = sn->val;
			object *on = sn->next->val;
			if(o->start_time_ms > on->start_time_ms)
			{
				list_swap(sn, sn->next);
				swapped = 1;
			}
			sn = sn->next;
		}
		ln = sn;
	} while(swapped);

	do
	{
		swapped = 0;
		sn = ptr->all_objects->start;
		if(sn == NULL) break;
		list_node *ln = NULL;
		while(sn->next != ln)
		{
			object *o = sn->val;
			object *on = sn->next->val;
			if(!object_compatable_with_current_gm(o) && object_compatable_with_current_gm(on))
			{
				list_swap(sn, sn->next);
				swapped = 1;
			}
			sn = sn->next;
		}
		ln = sn;
	} while(swapped);

    int i = 0;
	int i2 = 0;
    for(sn = ptr->all_objects->start; sn != NULL; sn = sn->next)
    {
        object *o = sn->val;
		o->number = ++i2;
		if(o->circle != NULL)
		{
			if(o->new_combo) i = 0;
			circle_set_number(o->circle, ++i);
		}
    }

	song_calc_song_skip_and_end(ptr);

	song_reset_game_mode_objects_list(ptr);
}

void song_sort_selected_objects(song *ptr)
{
	if(ptr->selected_objects->count > 0)
	{
		list_node *sn;
		char swapped;

		do
		{
			swapped = 0;
			sn = ptr->selected_objects->start;
			list_node *ln = NULL;
			while(sn->next != ln)
			{
				object *o = sn->val;
				object *on = sn->next->val;
				if(o->start_time_ms > on->start_time_ms)
				{
					list_swap(sn, sn->next);
					swapped = 1;
				}
				sn = sn->next;
			}
			ln = sn;
		} while(swapped);
	}
}

void song_sort_timing_sections(song *ptr)
{
	if(ptr->timing_sections->count > 0)
	{
		list_node *tn;
		char swapped;

		do
		{
			swapped = 0;
			tn = ptr->timing_sections->start;
			list_node *ln = NULL;
			while(tn->next != ln)
			{
				timing_section *ts = tn->val;
				timing_section *tsn = tn->next->val;
				if(ts->start_ms > tsn->start_ms)
				{
					list_swap(tn, tn->next);
					swapped = 1;
				}
				tn = tn->next;
			}
			ln = tn;
		} while(swapped);
	}
}

object *song_get_obj_by_number(song *ptr, int num)
{
	object *ret = NULL;
	list_node *sn;
    for(sn = ptr->all_objects->start; sn != NULL; sn = sn->next)
    {
        object *o = sn->val;
		if(o->number == num) ret = o;
	}
	return ret;
}

void song_add_circle(song *ptr, float x, float y, TIME_VAR time_ms)
{
    list_push_back(ptr->all_objects, object_init(x, y, object_circle, time_ms, 0, ptr));
	song_capture_undo_state(ptr);
}

void song_add_mania_note(song *ptr, int lane, TIME_VAR time_ms)
{
	object *mo = object_init(0, 0, object_mania_note, time_ms, 0, ptr);
	mo->mania_note->lane = lane;
	list_push_back(ptr->all_objects, mo);
	song_capture_undo_state(ptr);
	song_sort_objects(ptr);
}

char song_reload_bg(song *ptr)
{
	char ret = 0;
	char *bgtfloc = dupfmt("%s%s", ptr->folder_loc, ptr->bg_filename);
	ptr->background_texture = rg.skin->bg;
	if(file_exists(bgtfloc))
	{
		ptr->background_texture = textures_get(bgtfloc, tex_load_loc_filesystem, rg.screen == screen_editor);
		ret = 1;
	}
	free(bgtfloc);
	return ret;
}

char song_load_music(song *ptr)
{
	free(ptr->music_file_loc);
	ptr->music_file_loc = dupfmt("%s%s", ptr->folder_loc, ptr->music_filename);
	char ret = ae_music_load_file(ptr->music, ptr->music_file_loc);
	if(ret) ptr->length_ms = ae_music_handle_get_length_ms(ptr->music);
	return ret;
}

char *song_serialize(song *ptr)
{
	char *ret = dupfmt
	(
		"%s"
		SONG_SER_SEC_SEP
		"%s"
		SONG_SER_SEC_SEP
		"%d"
		SONG_SER_SEC_SEP
		"%d"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%d"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP
		"%.2f"
		SONG_SER_SEC_SEP,
		ptr->music_filename,
		ptr->bg_filename,
		ptr->version,
		ptr->game_mode_in_editor,
		ptr->song_global_offset,
		ptr->grid_resolution,
		ptr->tl.timeline_scale,
		ptr->time_ms_on_editor_exit,
		ptr->preview_time_ms,
		ptr->raw_object_size,
		ptr->raw_approach_rate,
		ptr->speed_multiplier
	);

	list_node *tn;
	for(tn = ptr->timing_sections->start; tn != NULL; tn = tn->next)
	{
		append_to_str_free_append(&ret, timing_section_serialize(tn->val));
		if(tn != ptr->timing_sections->end) append_to_str(&ret, SONG_SER_TSEC_SEP);
	}

	append_to_str(&ret, SONG_SER_SEC_SEP);

    list_node *obn;
    for(obn = ptr->all_objects->start; obn != NULL; obn = obn->next)
    {
		char *oser = object_serialize(obn->val);
		if(oser)
		{
			append_to_str_free_append(&ret, oser);
			if(obn != ptr->all_objects->end) append_to_str(&ret, SONG_SER_OBJ_SEP);
		}
		else show_notif("song object save error occurred");
    }

    return ret;
}

void song_overwrite(song *ptr, char *serialized_data, char is_undo_redo)
{
	int i;
	int seg_start = 0;
	int found = 0;
	for(i = 0;; ++i)
	{
		char cc = serialized_data[i];
		char end = cc == '\0';
		if(cc == SONG_SER_SEC_SEP_C || end)
		{
			if(!end) serialized_data[i] = '\0';
			char *r = &serialized_data[seg_start];

			switch(found)
			{
				case 0:
				{
					if(ptr->music_filename) free(ptr->music_filename);
					ptr->music_filename = dupe_str(r);
					break;
				}
				case 1:
				{
					if(ptr->bg_filename) free(ptr->bg_filename);
					ptr->bg_filename = dupe_str(r);
					break;
				}
				case 2: { ptr->version = atoi(r); break; }
				case 3: { ptr->game_mode_in_editor = atoi(r); break; }
				case 4: { ptr->song_global_offset = atof(r); break; }
				case 5: { ptr->grid_resolution = atoi(r); break; }
				case 6: { if(!is_undo_redo) ptr->tl.timeline_scale = atof(r); break; }
				case 7: { ptr->time_ms_on_editor_exit = atof(r); break; }
				case 8: { ptr->preview_time_ms = atof(r); break; }
				case 9: { ptr->raw_object_size = atof(r); break; }
				case 10: { ptr->raw_approach_rate = atof(r); break; }
				case 11: { ptr->speed_multiplier = clamp(atof(r), SONG_MIN_SPEED, SONG_MAX_SPEED); break; }
				case 12:
				{
					song_clear_timing_sections(ptr);

					int ci2 = 0;
					int ss2 = 0;
					int f2 = 0;
					char g2 = 1;
					while(g2)
					{
						char cc2 = r[ci2];
						char e2 = cc2 == '\0';
						if(cc2 == SONG_SER_TSEC_SEP_C || e2)
						{
							if(!e2) r[ci2] = '\0';
							char *rt2 = &r[ss2];

							list_push_back(ptr->timing_sections, timing_section_deserialize(ptr, rt2));

							if(e2) g2 = 0;
							else
							{
								r[ci2] = SONG_SER_TSEC_SEP_C;
								++f2;
								ss2 = ci2 + 1;
							}
						}
						++ci2;
					}
					break;
				}
				case 13:
				{
					song_clear_objects(ptr, 0);

					int ci2 = 0;
					int ss2 = 0;
					int f2 = 0;
					char g2 = 1;
					while(g2)
					{
						char cc2 = r[ci2];
						char e2 = cc2 == '\0';
						if(cc2 == SONG_SER_OBJ_SEP_C || e2)
						{
							if(!e2) r[ci2] = '\0';
							char *rt2 = &r[ss2];

							list_push_back(ptr->all_objects, object_from_serialized(rt2, ptr));

							if(e2) g2 = 0;
							else
							{
								r[ci2] = SONG_SER_OBJ_SEP_C;
								++f2;
								ss2 = ci2 + 1;
							}
						}
						++ci2;
					}
					break;
				}
			}

			if(end) break;

			serialized_data[i] = SONG_SER_SEC_SEP_C;
			found++;
			seg_start = i + 1;
		}
	}

	if(found < 13) show_notif("song file is corrupt");

	song_clear_replays(ptr);

	song_sort_objects(ptr);

	if(!is_undo_redo)
	{
		if(rg.bg_dim_level < 100) song_reload_bg(ptr);

		song_load_music(ptr);
	}
}

char song_load(song *ptr)
{
    if(file_exists(ptr->data_file_loc))
    {
        char *data = file_to_str(ptr->data_file_loc);
        song_overwrite(ptr, data, 0);
		sha256into(ptr->hash, data);
        free(data);
        return 1;
    }
	else song_new(ptr, ptr->name);
    return 0;
}

char song_load_replay(song *ptr, char *data)
{
	replay_overwrite(ptr->replay, data);
	char good = strs_are_equal(ptr->hash, ptr->replay->song_hash);
	if(!good) show_notif("replay hash check failed...");
	return good;
}

void song_new(song *ptr, char *music_file_name)
{
	make_dir(ptr->folder_loc);
    free(ptr->music_filename);
    ptr->music_filename = dupe_str(music_file_name);
    song_load_music(ptr);
}

void song_save(song *ptr)
{
	if(strs_are_equal(ptr->name, SONG_UNKNOWN_NAME)) return;
	ptr->time_ms_on_editor_exit = ptr->current_time_ms;
	ptr->game_mode_in_editor = rg.mode;
    char *data = song_serialize(ptr);
    str_to_file(ptr->data_file_loc, data);
    free(data);
	show_center_status("song has been saved to disk.");
	char *log_msg = dupfmt("saved %s to %s", ptr->name, ptr->data_file_loc);
	log_normal(log_msg);
	free(log_msg);
	ptr->edit_count = 0;
}

void song_save_score(song *ptr)
{
	if(strs_are_equal(ptr->name, SONG_UNKNOWN_NAME)) return;
	if(!file_exists(ptr->data_file_loc)) return;
	if(ptr->game_mode_objects->count < 1) return;
	
	char *scores_dir = song_get_scores_dir(ptr);
	
	replay_set_player_name(ptr->replay, rg.bot_enabled ? "Bot" : rg.client->authed ? rg.client->me->name : "Player");
	
	char *ser = replay_serialize
	(
		ptr->replay,
		ptr->score,
		ptr->highest_combo,
		song_calc_accuracy(ptr, 0),
		song_get_real_raw_approach_rate(ptr),
		song_get_real_raw_object_size(ptr),
		ptr->hash,
		rg.mode
	);

	if(rg.client->authed && ptr->replay->score > 0 && !rg.bot_enabled) online_client_submit_replay(rg.client, ptr->replay, ser);

	char *to_file = dupfmt
	(
		"%s%s - %s - %dy%02dm%02dd - %02dh%02dm%02ds - %d score - %d combo - %.2f acc.score",
		scores_dir,
		ptr->replay->player_name,
		ptr->name,
		ptr->replay->year,
		ptr->replay->month,
		ptr->replay->day,
		ptr->replay->hour,
		ptr->replay->minute,
		ptr->replay->second,
		ptr->replay->score,
		ptr->replay->combo,
		ptr->replay->accuracy
	);
	str_to_file(to_file, ser);
	free(to_file);
	free(ser);
	free(scores_dir);
}

char *song_get_scores_dir(song *ptr)
{
	char *ret = dupfmt(SCORES_DIR "%s%d/", ptr->hash, rg.mode);
	make_dir(ret);
	return ret;
}

/*
song *song_convert_from_osu_beatmap(char *osu_file_loc)
{
	if(!file_exists(osu_file_loc)) return NULL;

	list *lines = list_init();
	FILE *fp = fopen(osu_file_loc, "r");

	if(!fp)
	{
		list_free(lines, list_dummy);
		return NULL;
	}

	fseek(fp, 0L, SEEK_END);
	long sz = ftell(fp);
	rewind(fp);

	char *buf = malloc(sz);
	size_t buflen = strlen(buf);
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		buf[clampi(buflen - 2, 0, buflen - 1)] = '\0';
		list_push_back(lines, dupe_str(&buf[0]));
	}

	free(buf);

	fclose(fp);

	song *ret = song_init(SONG_UNKNOWN_NAME, "", "");

	song_clear_timing_sections(ret);

	char found_approach_rate = 0;
	float cs = 0.0f;
	float od = 0.0f;
	float ar = 0.0f;
	float uninherited_beat_length_ms = 500.0f;
	float slider_multiplier = 1.0f;

	char found_general = 0;
	char found_metadata = 0;
	char found_difficulty = 0;
	char found_timing_points = 0;
	char found_hit_objects = 0;
	int i;
	for(i = 0; i < lines->count; i++)
	 {
		char *line = list_get_val_at(lines, i);
		size_t line_len = strlen(line);
		if(line_len == 0)
		{
			found_general = 0;
			found_metadata = 0;
			found_difficulty = 0;
			found_timing_points = 0;
			found_hit_objects = 0;
		}
		else if(found_general)
		{
			list *split = split_str(line, ":");
			if(split->count == 2)
			{
				char *first = list_get_val_at(split, 0);
				char *last = list_get_val_at(split, 1);
				int si = 0;
				while(last[si++] == ' ') last++;
				if(strs_are_equal(first, "AudioFilename"))
				{
					if(ret->music_filename) free(ret->music_filename);
					ret->music_filename = dupe_str(last);
				}
			}
			list_free(split, free);
		}
		else if(found_metadata)
		{
			list *split = split_str(line, ":");
			if(split->count == 2)
			{
				char *first = list_get_val_at(split, 0);
				char *last = list_get_val_at(split, 1);
				int si = 0;
				while(last[si++] == ' ') last++;
				if(strs_are_equal(first, "Title"))
				{
					if(ret->name) free(ret->name);
					ret->name = dupe_str(last);
				}
			}
			list_free(split, free);
		}
		else if(found_difficulty)
		{
			list *split = split_str(line, ":");
			if(split->count == 2)
			{
				char *first = list_get_val_at(split, 0);
				char *last = list_get_val_at(split, 1);
				if(strs_are_equal(first, "CircleSize"))
				{
					cs = atof(last);
					ret->raw_object_size = cs;
				}
				else if(strs_are_equal(first, "ApproachRate"))
				{
					ar = atof(last);
					ret->raw_approach_rate = ar;
					found_approach_rate = 1;
				}
				else if(strs_are_equal(first, "OverallDifficulty"))
				{
					od = atof(last);
				}
				else if(strs_are_equal(first, "SliderMultiplier"))
				{
					slider_multiplier = atof(last);
				}
			}
			list_free(split, free);
		}
		else if(found_timing_points)
		{
			list *split = split_str(line, ",");
			if(split->count >= 2)
			{
				TIME_VAR start_ms = atof(list_get_val_at(split, 0));
				TIME_VAR beat_length_ms = atof(list_get_val_at(split, 1));
				TIME_VAR beat_divisor = 4;
				float slider_time_pixel_ratio = 1.0;
				char this_tp_uninherited = 1;
				if(split->count == 2)
				{
					beat_divisor = 4;
					slider_time_pixel_ratio = ((beat_length_ms / (slider_multiplier * 100)) / slider_multiplier);
				}
				else
				{
					if(split->count >= 3)
					{
						beat_divisor = atof(list_get_val_at(split, 2));
					}
					if(split->count >= 7)
					{
						this_tp_uninherited = atoi(list_get_val_at(split, 6));
					}
				}
				if(!this_tp_uninherited)
				{
					slider_time_pixel_ratio = scale_value_to(beat_length_ms, -100, 0, 1, 4) * slider_multiplier;
					beat_length_ms = uninherited_beat_length_ms;
				}
				else uninherited_beat_length_ms = beat_length_ms;
				list_push_back(ret->timing_sections, timing_section_init(ret, beat_length_ms, start_ms, slider_time_pixel_ratio, beat_divisor));
			}
			list_free(split, free);
		}
		else if(found_hit_objects)
		{
			list *split = split_str(line, ",");
			if(split->count >= 6)
			{
				float x = scale_value_to(atof(list_get_val_at(split, 0)), 0, 512, 0, 640);
				float y = scale_value_to(atof(list_get_val_at(split, 1)), 0, 384, 0, 480);
				TIME_VAR time_ms = atof(list_get_val_at(split, 2));
				int8_t type = atoi(list_get_val_at(split, 3));
				char is_circle = (type >> 0) & 1;
				char is_slider = (type >> 1) & 1;
				char new_combo = (type >> 2) & 1;
				char is_spinner = (type >> 3) & 1;
				object_type conv_type = is_circle ? object_circle : is_slider ? object_slider : is_spinner ? object_spinner : -1;
				if(conv_type != -1 && !is_spinner)
				{
					object *obj = object_init(x, y, conv_type, time_ms, 0, ret);
					obj->new_combo = new_combo;

					if(is_slider && split->count >= 7)
					{
						slider *slider = obj->slider;
						slider_curve_type curve_type = slider_curve_catmull;
						list *control_point_strs = split_str(list_get_val_at(split, 5), "|");
						if(control_point_strs->count >= 3)
						{
							char *ctstr = list_get_val_at(control_point_strs, 0);
							char tc = ctstr[0];

							if(tc == 'B') curve_type = slider_curve_bezier;
							else if(tc == 'C') curve_type = slider_curve_catmull;
							else if(tc == 'L') curve_type = slider_curve_linear;
							else if(tc == 'P') curve_type = slider_curve_circle;
							slider->curve_type = curve_type;
							list_node *st = control_point_strs->start->next;
							for(; st != NULL; st = st->next)
							{
								char *cp_str = st->val;
								list *cp_sp = split_str(cp_str, ":");
								if(cp_sp->count == 2)
								{
									float cpx = scale_value_to(atof(list_get_val_at(cp_sp, 0)), 0, 512, 0, 640);
									float cpy = scale_value_to(atof(list_get_val_at(cp_sp, 1)), 0, 384, 0, 480);
									list_push_back(slider->control_points, slider_control_point_init(cpx, cpy));
								}
								if(slider->control_points->count == 3 && (tc == 'B' || tc == 'C'))
								{
									slider_control_point *last = (slider_control_point*) slider->control_points->end->val;
									list_push_back(slider->control_points, slider_control_point_init(last->pos.x, last->pos.y));
								}
								list_free(cp_sp, free);
							}
						}
						list_free(control_point_strs, free);
						slider->reverses = atoi(list_get_val_at(split, 6)) - 1;
					}
					
					list_push_back(ret->all_objects, obj);
				}
			}
			
			list_free(split, free);
		}
		else if(line_len > 0)
		{
			if(strs_are_equal(line, "[General]"))
			{
				found_general = 1;
			}
			else if(strs_are_equal(line, "[Metadata]"))
			{
				found_metadata = 1;
			}
			else if(strs_are_equal(line, "[Difficulty]"))
			{
				found_difficulty = 1;
			}
			else if(strs_are_equal(line, "[TimingPoints]"))
			{
				found_timing_points = 1;
			}
			else if(strs_are_equal(line, "[HitObjects]"))
			{
				found_hit_objects = 1;
			}
		}
	}
	if(!found_approach_rate) ret->raw_approach_rate = od;

	list_free(lines, free);

	return ret;
}
*/
