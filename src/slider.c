#include "slider.h"

#include "utils.h"
#include "text3.h"
#include "curves.h"
#include "song.h"
#include "skin.h"
#include "circle.h"
#include "approach_circle.h"
#include "textures.h"
#include "explosion.h"
#include "timing_section.h"
#include "replay.h"

slider_control_point *slider_control_point_init(float x, float y)
{
	slider_control_point *ret = malloc(sizeof *ret);
	ret->pos.x = x;
	ret->pos.y = y;
	return ret;
}

void slider_control_point_free(slider_control_point *ptr)
{
	free(ptr);
}

slider *slider_init(object *host_obj)
{
    slider *ret = malloc(sizeof *ret);

    ret->host_obj = host_obj;

	ret->curve_type = slider_curve_bezier;

    ret->control_points = list_init();
    ret->curve_points = list_init();

	ret->bounds = FLOATRECTZERO;

	ret->tex = 0;
	ret->fbuf = 0;
	ret->tex_size = V2FZERO;

	ret->cur_point = V2FZERO;
	ret->next_point = V2FZERO;
	ret->cur_point_interpolated = V2FZERO;

	ret->slider_start_pos_op = V2FZERO;
	ret->slider_start_pos = V2FZERO;
	ret->slider_end_pos_op = V2FZERO;
	ret->slider_end_pos = V2FZERO;

	ret->reverses = 0;
	ret->segments_passed = 0;
	ret->segment_count = 1;
	ret->segment_index = 0;

	ret->rev_on_start = 0;
	ret->slider_failed = 0;
	ret->letgo = 0;
	ret->click_try = 0;
	ret->held_near_end = 0;

	ret->did_rev_expl = 0;
	ret->potato_state_on_regen = rg.potato_pc_enabled;

	ret->selected_control_point = NULL;

	ret->slider_hold_time_timer = timer_init();
	ret->not_good_time_timer = timer_init();
	ret->letgo_clock = timer_init();
	ret->follow_beat_timer = timer_init();
	timer_set_milliseconds(ret->letgo_clock, SLIDER_LETGO_ANIM_TIME);
	ret->slider_hold_time = 0.0f;
	ret->not_good_time = 0.0f;
	ret->follow_beat_time = 0.0f;
	ret->follow_do_beat = 0;
	ret->slider_hold_scale_at_letgo = 0.5f;
	ret->slider_hold_scale_at_letgo_set = 0;

	ret->last_is_good = 0;
	ret->is_good = 0;

    return ret;
}

void slider_delete_tex(slider *ptr)
{
	if(ptr->tex != 0)
	{
		glDeleteTextures(1, &(ptr->tex));
		ptr->tex = 0;
	}
	if(ptr->fbuf != 0)
	{
		glDeleteFramebuffers(1, &(ptr->fbuf));
		ptr->fbuf = 0;
	}
	ptr->tex_size = V2FZERO;
}

void slider_delete_control_points(slider *ptr)
{
    list_free(ptr->control_points, slider_control_point_free);
}

void slider_free(slider *ptr)
{
	timer_free(ptr->follow_beat_timer);
	timer_free(ptr->letgo_clock);
	timer_free(ptr->not_good_time_timer);
	timer_free(ptr->slider_hold_time_timer);
	slider_delete_tex(ptr);
	curves_delete_list(ptr->curve_points);
	slider_delete_control_points(ptr);
    free(ptr);
}

char slider_check_hovered(slider *ptr)
{
	if(ptr->tex == 0 || ptr->curve_points == NULL || ptr->curve_points->count < 1) return 0;
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	obj->hovered = 0;
	list_node *n;
	for(n = ptr->curve_points->start; n != NULL; n = n->next)
	{
		v2f point = song_map_coord_to_play_field(song, *((v2f*) n->val));
		v2f mouse_point = song_map_coord_to_play_field(song, V2F(rg.mop_x, rg.mop_y));
		if(get_distf(point.x, point.y, mouse_point.x, mouse_point.y) <= CIRCLE_RADIUS * song->object_size)
		{
			obj->host_song->mouse_hovering_slider_body = 1;
			obj->hovered = 1;
			return 1;
		}
	}
	return 0;
}

void slider_calc_start_end_pos(slider *ptr)
{
	if(ptr->curve_points == NULL || ptr->curve_points->count < 1) return;
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	float wiggle_xoff = obj->wiggle_xoff;
	ptr->slider_start_pos_op = *((v2f*) list_get_val_at(ptr->curve_points, 0));
	ptr->slider_start_pos_op.x += wiggle_xoff;
	ptr->slider_start_pos = song_map_coord_to_play_field(song, ptr->slider_start_pos_op);
	ptr->slider_end_pos_op = *((v2f*) list_get_val_at(ptr->curve_points, ptr->curve_points->count - 1));
	ptr->slider_end_pos_op.x += wiggle_xoff;
	ptr->slider_end_pos = song_map_coord_to_play_field(song, ptr->slider_end_pos_op);
}

void slider_draw_curve_points(slider *ptr, float alpha)
{
	glBegin(GL_POINTS);
	glColor4ub(255, 0, 0, alpha);
	list_node *n;
	for(n = ptr->curve_points->start; n != NULL; n = n->next)
	{
		v2f on_pf = song_map_coord_to_play_field(ptr->host_obj->host_song, *((v2f*) n->val));
		glVertex2f(on_pf.x, on_pf.y);
	}
	glEnd();
}

void slider_draw_texture(slider *ptr, color4 col)
{
	float wiggle_xoff = ptr->host_obj->wiggle_xoff;
	if(ptr->tex != 0)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, ptr->tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glColor4ub(color4_2func_alpha(col));
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0,0); glVertex2f(wiggle_xoff, 0);
			glTexCoord2f(1,0); glVertex2f(wiggle_xoff + rg.win_width, 0);
			glTexCoord2f(1,1); glVertex2f(wiggle_xoff + rg.win_width, rg.win_height);
			glTexCoord2f(0,1); glVertex2f(wiggle_xoff, rg.win_height);
		}
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}
}

void slider_draw_ball_and_follow(slider *ptr, color4 col)
{
	if(ptr->tex == 0 || ptr->curve_points == NULL || ptr->curve_points->count < 1) return;
	
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	TIME_VAR reltime = obj->relative_time_ms;
	TIME_VAR objlen = obj->length_time_ms;
	v2f pos = song_map_coord_to_play_field(song, ptr->cur_point);
	pos.x += obj->wiggle_xoff;

	float objsc = song->object_size;
	if(reltime > 0 || rg.enable_slider_debug)
	{
		if(reltime <= objlen || rg.enable_slider_debug)
		{
			v2f npos = song_map_coord_to_play_field(song, ptr->next_point);
			npos.x += obj->wiggle_xoff;
			float rot = rotate_towards(pos, npos) - 90.0f;
			GL_ROT(pos.x, pos.y, rot);
			textures_drawsc(rg.skin->sliderball, 0, 0, objsc, objsc, col);
			GL_ROT_END();
		}
		TIME_VAR lgc = timer_milliseconds(ptr->letgo_clock) * song->speed_multiplier;
		if(lgc > SLIDER_LETGO_ANIM_TIME && reltime < objlen - SLIDER_LETGO_ANIM_TIME)
		{
			ptr->letgo = 0;
			ptr->slider_hold_scale_at_letgo_set = 0;
		}
		float normal_scale =
			clamped_scale_value_to
			(
				clamp
				(
					ptr->slider_hold_time,
					0.0f,
					SLIDER_FOLLOW_OPEN_MS
				),
				0.0f,
				SLIDER_FOLLOW_OPEN_MS,
				0.85f,
				1.0f
			);
		if(ptr->letgo && !(ptr->slider_hold_scale_at_letgo_set))
		{
			ptr->slider_hold_scale_at_letgo_set = 1;
			ptr->slider_hold_scale_at_letgo = normal_scale;
			ptr->slider_hold_time = 0;
		}
		float letgo_scale =
			clamped_scale_value_to
			(
				clamp
				(
					lgc,
					0,
					SLIDER_LETGO_ANIM_TIME
				),
				0,
				SLIDER_LETGO_ANIM_TIME,
				ptr->slider_hold_scale_at_letgo,
				2.0f
			);
		if(ptr->letgo) col.a =
			scale_value_to
			(
				clamp
				(
					lgc,
					0,
					SLIDER_LETGO_ANIM_TIME
				),
				0,
				SLIDER_LETGO_ANIM_TIME,
				col.a,
				0
			);
		else col.a =
			scale_value_to
			(
				clamp
				(
					ptr->slider_hold_time,
					0,
					SLIDER_FOLLOW_OPEN_MS
				),
				0,
				SLIDER_FOLLOW_OPEN_MS,
				0,
				SLIDER_MAX_OPACITY
			);
		float real_scale = (ptr->letgo ? letgo_scale : normal_scale) * objsc;
		real_scale *= scale_value_to(ptr->follow_beat_time, 0.0f, SLIDER_FOLLOW_BEAT_TIME_MS, 1.0f, 1.08f);
		if(rg.screen == screen_player) textures_drawsc(rg.skin->followcircle, pos.x, pos.y, real_scale, real_scale, col);
	}
}

void slider_draw_reverse_arrows(slider *ptr, color4 col)
{
	if(ptr->tex == 0 || ptr->curve_points == NULL || ptr->curve_points->count < 1) return;
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	timing_section *tsec = song_get_timing_section(song, obj->start_time_ms);
	if(!tsec) return;
	TIME_VAR reltime = obj->relative_time_ms;
	TIME_VAR objlen = obj->length_time_ms;
	float wiggle_xoff = obj->wiggle_xoff;
	if(ptr->reverses >= 1 && reltime < objlen)
	{
		float mfv = fmodf(song->current_time_ms - tsec->start_ms, tsec->beat_length_ms);
		float objsc = song->object_size;
		float rev_rot_beat_off = 0.0f;

		if(rg.screen != screen_editor)
		{
			objsc = song->object_size * scale_value_to(mfv, tsec->beat_length_ms, 0.0f, 1.0f, 1.30f);
			rev_rot_beat_off = scale_value_to(mfv, tsec->beat_length_ms, 0.0f, 0.0f, 360.0f * 0.05f);
		}

		v2f sc = V2F(objsc, objsc);
		char on_last_reverse = (ptr->segment_index == ptr->reverses);
		char reverses_is_even = (ptr->reverses % 2 == 0);
		v2f slider_rot_pos_s = song_map_coord_to_play_field(song, *((v2f*) list_get_val_at(ptr->curve_points, clamp(10, 0, ptr->curve_points->count - 1))));
		slider_rot_pos_s.x += wiggle_xoff;

		loaded_texture *ra_sp = rg.skin->reversearrow;
		char should_reverse = ptr->segment_index == 1 || ptr->segment_index % 2 != 0;

		float rot_s_n = rotate_towards(ptr->slider_start_pos, slider_rot_pos_s) - 90;
		float rot_s = rot_s_n + rev_rot_beat_off;
		if(reltime >= 0 && !on_last_reverse && (on_last_reverse ? should_reverse : 1) && ptr->segment_index < ptr->reverses - (reverses_is_even ? 0 : 1))
		{
			color4 rot_s_col = color4_mod_alpha(col_white, scale_value_to(clamp(reltime, 0, 200), 0, 200, 0, col.a));
			GL_ROT(ptr->slider_start_pos.x, ptr->slider_start_pos.y, rot_s);
			textures_drawsc(ra_sp, 0, 0, sc.x, sc.y, rot_s_col);
			GL_ROT_END();
		}
		
		v2f slider_rot_pos_e = song_map_coord_to_play_field(song, *(v2f*) list_get_val_at(ptr->curve_points, clamp(ptr->curve_points->count - 2, 0, ptr->curve_points->count - 1)));
		slider_rot_pos_e.x += wiggle_xoff;
		float rot_e_n = rotate_towards(ptr->slider_end_pos, slider_rot_pos_e) - 90;
		float rot_e = rot_e_n + rev_rot_beat_off;
		if(ptr->segment_index < ptr->reverses - (!reverses_is_even ? 0 : 1) && !(on_last_reverse && should_reverse))
		{
			GL_ROT(ptr->slider_end_pos.x, ptr->slider_end_pos.y, rot_e);
			textures_drawsc(ra_sp, 0, 0, sc.x, sc.y, col);
			GL_ROT_END();
		}

		if(ptr->did_rev_expl && ptr->segment_index <= ptr->reverses)
		{
			float rotation = ptr->rev_on_start ? rot_s : rot_e;
			list_push_back(song->explosions, explosion_init_rev(ptr->rev_on_start ? ptr->slider_start_pos_op : ptr->slider_end_pos_op, song, song->current_time_ms, rotation));
		}
	}
}

void slider_tick_scoring(slider *ptr)
{
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	timing_section *tsec = song_get_timing_section(song, obj->start_time_ms);
	float r = CIRCLE_RADIUS * song->object_size;
	TIME_VAR objlen = obj->length_time_ms;
	TIME_VAR reltime = obj->relative_time_ms;
	char during_slider = reltime >= 0 && reltime <= objlen;
	if(reltime >= objlen && obj->end_judgement == judgement_unknown) obj->end_judgement = judgement_miss;
	ptr->did_rev_expl = 0;
	v2f cur_point_mapped = song_map_coord_to_play_field(song, ptr->cur_point);
	v2f mouse_pos_mapped = song_map_coord_to_play_field(song, V2F(rg.mop_x, rg.mop_y));
	char within_dist = get_distf(cur_point_mapped.x, cur_point_mapped.y, mouse_pos_mapped.x, mouse_pos_mapped.y) <= (r * 2.0f);
	if(during_slider)
	{
		char oc1 = (rg.ouendan_click_1 && (rg.ouendan_click_1_frame_1_time_ms >= obj->start_time_ms - MISS_TIME));
		char oc2 = (rg.ouendan_click_2 && (rg.ouendan_click_2_frame_1_time_ms >= obj->start_time_ms - MISS_TIME));
		if(!rg.replay_mode_enabled) ptr->is_good = ((oc1 || oc2) && within_dist) || rg.screen == screen_editor;
		if(ptr->is_good != ptr->last_is_good)
		{
			replay_add_frame(song->replay, song, song->current_time_ms, rg.mop_x, rg.mop_y, rg.ouendan_click_1, rg.ouendan_click_2, 0, 0, 0, replay_hold_toggle, obj->number);
		}
		ptr->last_is_good = ptr->is_good;
	}
	if(ptr->is_good) ptr->click_try = 1;
	if(ptr->is_good && reltime >= objlen - SLIDER_HELD_NEAR_END_MS) ptr->held_near_end = 1;
	TIME_VAR sholdtime = timer_milliseconds(ptr->slider_hold_time_timer);
	if(during_slider)
	{
		TIME_VAR lgtime = timer_milliseconds(ptr->not_good_time_timer);
		if(!ptr->is_good) ptr->not_good_time += lgtime * song->speed_multiplier;
		else ptr->not_good_time = 0;

		if(ptr->is_good && ptr->click_try) ptr->slider_hold_time += sholdtime * song->speed_multiplier;
		else if(ptr->not_good_time > SLIDER_HELD_NEAR_END_MS)
		{
			if(reltime > (tsec->beat_length_ms / tsec->beat_divisor) && !(ptr->letgo) && ptr->click_try)
			{
				song_break_combo(song);
				ptr->slider_failed = 1;
				ptr->letgo = 1;
				ptr->click_try = 0;
				ptr->slider_hold_scale_at_letgo_set = 0;
				timer_restart(ptr->letgo_clock);
			}
			ptr->slider_hold_time -= sholdtime * song->speed_multiplier;
		}
	}
	else if(reltime > objlen) ptr->slider_hold_time -= sholdtime * song->speed_multiplier;
	ptr->slider_hold_time = clamp(ptr->slider_hold_time, 0, SLIDER_FOLLOW_OPEN_MS);

	TIME_VAR folbms = timer_milliseconds(ptr->follow_beat_timer) * song->speed_multiplier;
	if(ptr->follow_do_beat)
	{
		ptr->follow_beat_time = SLIDER_FOLLOW_BEAT_TIME_MS;
		ptr->follow_do_beat = 0;
	}
	else ptr->follow_beat_time -= folbms;

	if(ptr->follow_beat_time < 0) ptr->follow_beat_time = 0;

	timer_restart(ptr->not_good_time_timer);
	timer_restart(ptr->slider_hold_time_timer);
	timer_restart(ptr->follow_beat_timer);
	
	if(song->music_state == song_playing && ptr->segments_passed < ptr->segment_index && reltime >= (ptr->segment_index * ptr->segment_len_ms))
	{
		if(ptr->segments_passed == -1) song->passed_segs++;

		char good_hne = (ptr->is_good || ptr->held_near_end);
		if(good_hne && ptr->segments_passed < ptr->segment_count)
		{
			if(ptr->segments_passed == -1) song->good_hits++;
			ptr->did_rev_expl = 1;
			v2f rev_expl_loc = song_map_coord_to_play_field(song, ptr->cur_point);
			ptr->rev_on_start = get_distf(ptr->slider_start_pos.x, ptr->slider_start_pos.y, rev_expl_loc.x, rev_expl_loc.y) < get_distf(ptr->slider_end_pos.x, ptr->slider_end_pos.y, rev_expl_loc.x, rev_expl_loc.y) ? 1 : 0;

			song->slider_startend_hit = 1;
			
			if(ptr->segments_passed == ptr->segment_count - 1)
			{
				ae_sound_effect_play(rg.skin->long_end);
				list_push_back(song->explosions, explosion_init_circ(ptr->slider_start_pos_op, song, song->current_time_ms));
				list_push_back(song->explosions, explosion_init_circ(ptr->slider_end_pos_op, song, song->current_time_ms));
				obj->end_judgement = judgement_perfect;
			}
			else
			{
				ae_sound_effect_play(rg.skin->long_middle);
				ptr->follow_do_beat = 1;
				list_push_back(song->explosions, explosion_init_circ(ptr->rev_on_start ? ptr->slider_start_pos_op : ptr->slider_end_pos_op, song, song->current_time_ms));
			}

			if(rg.screen == screen_player)
			{
				song_inc_combo(song);
				song->score += 18 + (song->current_combo * 0.729);
			}
		}
		else if(!good_hne && during_slider)
		{
			song_break_combo(song);
			ptr->slider_failed = 1;
		}

		ptr->segments_passed = ptr->segment_index;
	}
}

void slider_update_cur_point(slider *ptr)
{
	if(ptr->tex == 0 || ptr->curve_points == NULL || ptr->curve_points->count < 1) return;
	object *obj = ptr->host_obj;
	TIME_VAR objlen = obj->length_time_ms;
	TIME_VAR reltime = obj->relative_time_ms;
	TIME_VAR reltimec = clamp(obj->relative_time_ms, 0, objlen);
	int curve_point_count = ptr->curve_points->count;
	ptr->segment_index = clamp(reltime / ptr->segment_len_ms, 0, ptr->segment_count);
	char should_reverse = ptr->segment_index == 1 || ptr->segment_index % 2 != 0;
	int curve_point_index =
		clamp
		(
			scale_value_to
			(
				fmodf
				(
					reltimec,
					ptr->segment_len_ms
				),
				0,
				ptr->segment_len_ms,
				should_reverse ? curve_point_count - 1 : 0,
				should_reverse ? 0 : curve_point_count - 1
			), 
			0, 
			curve_point_count - 1
		);
	
	char slider_done = reltime >= objlen;
	if(slider_done) curve_point_index = ptr->segment_count % 2 == 0 ? 0 : curve_point_count - 1;

	if(rg.enable_slider_debug) {
		text3_fmt(ptr->slider_start_pos.x, ptr->slider_start_pos.y, rg.win_width - ptr->slider_start_pos.x, 255, origin_center, origin_top, text3_justification_left,
		"reltime: %f\nreltimec: %f\ncpc: %d\ncpi: %d\ndone: %d\nreverses: %d\nseg count: %d\ncur seg: %d\nreversing: %d", 
			reltime, reltimec, curve_point_count, curve_point_index, slider_done, ptr->reverses, ptr->segment_count, ptr->segment_index, should_reverse); }

	v2f *real_cur_point = list_get_val_at(ptr->curve_points, curve_point_index);

	ptr->next_point = *((v2f*) list_get_val_at(ptr->curve_points, clamp(curve_point_index + (should_reverse ? -1 : 1), 0, ptr->curve_points->count - 1)));
	
	if(!slider_done)
	{
		int cpir = should_reverse ? curve_point_count - curve_point_index : curve_point_index;
		TIME_VAR point_ms = (ptr->segment_len_ms / ((float) ptr->curve_points->count));
		float cur_point_ms = cpir * point_ms;
		float next_point_ms = (cpir + 1) * point_ms;
		ptr->cur_point_interpolated.x = scale_value_to(fmod(reltime, ptr->segment_len_ms), cur_point_ms, next_point_ms, real_cur_point->x, ptr->next_point.x);
		ptr->cur_point_interpolated.y = scale_value_to(fmod(reltime, ptr->segment_len_ms), cur_point_ms, next_point_ms, real_cur_point->y, ptr->next_point.y);
	}

	ptr->cur_point.x = real_cur_point->x;
	ptr->cur_point.y = real_cur_point->y;
}

void slider_update(slider *ptr)
{
	object *obj = ptr->host_obj;
	song *song = obj->host_song;

	if(rg.screen == screen_editor && rg.gui_render_end_of_frame == song)
	{
		if(obj->is_new)
		{
			float div = SONG_PFW / song->grid_resolution;
			float sox = clamp(round(rg.mop_x / div) * div, 0, SONG_PFW);
			float soy = clamp(round(rg.mop_y / div) * div, 0, SONG_PFH);
			
			if(ptr->control_points->count > 1)
			{
				slider_control_point *last = (slider_control_point*) ptr->control_points->end->val;
				last->pos.x = sox;
				last->pos.y = soy;
				if(song->music_state == song_playing && obj->length_time_ms > 0 && song->current_time_ms >= obj->start_time_ms + obj->length_time_ms && song->current_time_ms < song->length_ms)
				{
					ptr->reverses++;
					slider_calculate_path(ptr, 1);
				}
			}

			if(left_click == 1)
			{
				slider_add_control_point(ptr, V2F(sox, soy));
				slider_calculate_path(ptr, 0);
				song_capture_undo_state(song);
			}
			else if(mouse.right_click == 1)
			{
				if (ptr->tex == 0)
				{
					obj->delete_me = 1;
				}
				else
				{
					slider_calculate_path(ptr, 0);
					obj->is_new = 0;
					song_capture_undo_state(song);
				}
			}

			if(mouse.moved || left_click == 1) slider_calculate_path(ptr, 0);
		}
		else if
		(
			(
				song->selected_object == obj
				||
				ptr->tex == 0
			)
			&&
			song->selected_objects->count == 0
		)
		{
			v2f box_size;
			if(ptr->tex != 0) box_size = V2F(SLIDER_CONTROL_POINT_SIZE, SLIDER_CONTROL_POINT_SIZE);
			else box_size = V2F(SLIDER_CONTROL_POINT_SIZE_NO_TEX, SLIDER_CONTROL_POINT_SIZE_NO_TEX);

			list_node *n;
			if(rg.kp[SDL_SCANCODE_LCTRL] && left_click == 1)
			{
				float dist_from_mouse = INFINITY;
				list_node *near_n = NULL;
				for(n = ptr->control_points->start; n != NULL; n = n->next)
				{
					slider_control_point *p = (slider_control_point*)n->val;
					v2f on_pf = song_map_coord_to_play_field(song, p->pos);
					float dist = get_distf(mouse.x, mouse.y, on_pf.x, on_pf.y);
					if(dist < dist_from_mouse)
					{
						dist_from_mouse = dist;
						near_n = n;
					}
				}
				if(near_n != NULL)
				{
					slider_control_point *ncp = slider_control_point_init(rg.mop_x, rg.mop_y);
					list_insert(ptr->control_points, near_n, ncp, 0);
					ptr->selected_control_point = ncp;
					slider_calculate_path(ptr, 0);
					song_capture_undo_state(song);
				}
			}
			else
			{
				int i = 0;
				for(n = ptr->control_points->end; n != NULL; n = n->before)
				{
					slider_control_point *p = (slider_control_point*)n->val;
					
					v2f on_pf = song_map_coord_to_play_field(song, p->pos);
					v2f box_pos = V2F(on_pf.x - (box_size.x / 2), on_pf.y - (box_size.y / 2));

					if(mouse.x >= box_pos.x && mouse.y >= box_pos.y && mouse.x <= box_pos.x + box_size.x && mouse.y <= box_pos.y + box_size.y)
					{
						song->slider_control_point_box_hovered = 1;

						if(ptr->selected_control_point == NULL && left_click == 1) ptr->selected_control_point = p;
						else if(ptr->selected_control_point == NULL && ptr->control_points->count > 2 && mouse.right_click == 1)
						{
							slider_control_point_free(p);
							list_erase(ptr->control_points, n);
							slider_calculate_path(ptr, 0);
							song_capture_undo_state(song);
							break;
						}
					}
					i++;
				}
			}
		}

		if(ptr->selected_control_point != NULL && !obj->is_new)
		{
			if(left_click == 0)
			{
				if(mouse.left_click_released) song_capture_undo_state(song);
				ptr->selected_control_point = NULL;
			}
			else
			{
				float div = SONG_PFW / song->grid_resolution;
				ptr->selected_control_point->pos.x = round(rg.mop_x / div) * div;
				ptr->selected_control_point->pos.y = round(rg.mop_y / div) * div;
				if(ptr->tex == 0 && ptr->control_points->count >= 1) obj->pos = (*((slider_control_point*) list_get_val_at(ptr->control_points, 0))).pos;
				song->slider_control_point_box_hovered = 1;
				song->selected_object = obj;
			}

			if(mouse.moved || left_click == 1)
			{
				GLuint bef = ptr->tex;
				slider_calculate_path(ptr, 0);
				if(bef == 0 && ptr->tex != 0) song->selected_object = obj;
			}
		}
	}

	if(ptr->curve_points != NULL && ptr->curve_points->count >= 1)
	{
		slider_update_cur_point(ptr);

		slider_calc_start_end_pos(ptr);

		slider_tick_scoring(ptr);
	}
}

void slider_render(slider *ptr)
{
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	
	TIME_VAR reltime = obj->relative_time_ms;
	TIME_VAR objlen = obj->length_time_ms;
	TIME_VAR apptime = song->approach_time_ms;

	color4 col = col_white;
	if(reltime < 0) col.a = clamp(scale_value_to(reltime, -apptime, -apptime + CIRCLE_FADE_IN_TIME_MS, 0, SLIDER_MAX_OPACITY), 0, SLIDER_MAX_OPACITY);
	else if(reltime <= objlen) col.a = SLIDER_MAX_OPACITY;
	else col.a = clamp(scale_value_to(reltime, objlen, objlen + (rg.screen == screen_editor ? CIRCLE_EDITOR_FADE_OUT_MS : CIRCLE_FADE_OUT_TIME_MS), SLIDER_MAX_OPACITY, 0), 0, SLIDER_MAX_OPACITY);

	if(col.a > 0)
	{
		if(!rg.enable_slider_debug)
		{
			slider_draw_texture(ptr, col);

			if(ptr->tex != 0) circle_render(ptr->host_obj->circle);

			slider_draw_reverse_arrows(ptr, col);
		}

		slider_draw_ball_and_follow(ptr, col);
	}

	if
	(
		rg.screen == screen_editor
		&&
		(
			song->selected_object == obj
			||
			ptr->tex == 0
		)
		&&
		song->selected_objects->count == 0
	)
	{
		v2f box_size;
		if(ptr->tex != 0) box_size = V2F(SLIDER_CONTROL_POINT_SIZE, SLIDER_CONTROL_POINT_SIZE);
		else box_size = V2F(SLIDER_CONTROL_POINT_SIZE_NO_TEX, SLIDER_CONTROL_POINT_SIZE_NO_TEX);

		v2f last_point = V2FZERO;
    	list_node *n;
		for(n = ptr->control_points->start; n != NULL; n = n->next)
		{
			slider_control_point *p = (slider_control_point*)n->val;
			
			v2f on_pf = song_map_coord_to_play_field(song, p->pos);
			v2f box_pos = V2F(on_pf.x - (box_size.x / 2), on_pf.y - (box_size.y / 2));
			color4 box_col = ptr->selected_control_point == p ? col_red : col_white;

			GL_LINEBOX(FLOATRECT(box_pos.x, box_pos.y, box_size.x, box_size.y), box_col);
			
			if(n != ptr->control_points->start)
			{
				glBegin(GL_LINES);
				glColor3ub(255, 255, 255);
				glVertex2f(last_point.x, last_point.y);
				glVertex2f(on_pf.x, on_pf.y);
				glEnd();
			}
			last_point = on_pf;
		}

		if(ptr->tex != 0) GL_LINESTRIP(song_map_float_rect_to_play_field(song, ptr->bounds), col_white);
	}

	if(rg.kp[SDL_SCANCODE_R] || ptr->tex == 0 || rg.enable_slider_debug)
		slider_draw_curve_points(ptr, col.a);
}

void slider_snap_to_grid(slider *ptr, char regen_path_tex)
{
	object *obj = ptr->host_obj;
	song *song = obj->host_song;
	if
	(
		obj->do_grid_snap
		&&
		song->grid_resolution != SONG_PFW
	)
	{
		float div = SONG_PFW / song->grid_resolution;

		list_node *cn;
		for(cn = ptr->control_points->start; cn != NULL; cn = cn->next)
		{
			slider_control_point *p = (slider_control_point*) cn->val;
			p->pos.x = round(p->pos.x / div) * div;
			p->pos.y = round(p->pos.y / div) * div;
		}

		slider_calculate_path(ptr, !regen_path_tex);
		slider_calc_bounds(ptr);
		obj->do_grid_snap = 0;
	}
}

void slider_add_control_point(slider *ptr, v2f point)
{
    list_push_back(ptr->control_points, slider_control_point_init(point.x, point.y));
}

void slider_calculate_path(slider *ptr, char dont_regen_texture)
{
	if(ptr == NULL || ptr->control_points == NULL || ptr->control_points->count < 2) return;

	object *obj = ptr->host_obj;
	timing_section *tsec = song_get_timing_section(obj->host_song, obj->start_time_ms);
	if(tsec == NULL) return;

	curves_delete_list(ptr->curve_points);
	if(ptr->curve_type == slider_curve_linear) ptr->curve_points = curves_create_linear(ptr->control_points);
	else if(ptr->curve_type == slider_curve_catmull) ptr->curve_points = curves_create_catmull(ptr->control_points);
	else if(ptr->curve_type == slider_curve_bezier) ptr->curve_points = curves_create_bezier(ptr->control_points);
	else if(ptr->curve_type == slider_curve_circle) ptr->curve_points = curves_create_perfect(ptr->control_points);

	if(ptr->curve_points != NULL && ptr->curve_points->count >= 1)
	{
		float btms = tsec->beat_length_ms / tsec->slider_time_pixel_ratio;
		float divl = (btms / tsec->beat_divisor);
		curves_limit_curve(ptr->curve_points, divl);
		float len_ms = (round(ptr->curve_points->count / divl) * divl);

		if(ptr->curve_points->count >= 1)
		{
			v2f *first = list_get_val_at(ptr->curve_points, 0);
			obj->pos.x = first->x;
			obj->pos.y = first->y;
		}

		obj->length_time_ms = (len_ms * (1 + ptr->reverses)) * tsec->slider_time_pixel_ratio;
		ptr->segment_len_ms = obj->length_time_ms / (1 + ptr->reverses);
		if(ptr->segment_len_ms < 1) ptr->segment_len_ms = 1;
		ptr->segment_count = ptr->reverses + 1;

		if(!dont_regen_texture) slider_regenerate_texture(ptr);
	}
}

void slider_calc_bounds(slider *ptr)
{
	if(ptr == NULL || ptr->curve_points == NULL || ptr->curve_points->count < 1) return;

	v2f *fp = (v2f*) list_get_val_at(ptr->curve_points, 0);
	v2f fpc = *fp;

	float r = CIRCLE_RADIUS * ptr->host_obj->host_song->object_size_pf;

	ptr->bounds.left = fpc.x;
	ptr->bounds.top = fpc.y;
	ptr->bounds.width = fpc.x;
	ptr->bounds.height = fpc.y;

	list_node *n;
	for(n = ptr->curve_points->start; n != NULL; n = n->next)
	{
		v2f *p = (v2f*)n->val;
		v2f pc = *p;
		float px = pc.x;
		float py = pc.y;
		if(px < ptr->bounds.left) ptr->bounds.left = px;
		else if(px > ptr->bounds.width) ptr->bounds.width = px;
		if(py < ptr->bounds.top) ptr->bounds.top = py;
		else if(py > ptr->bounds.height) ptr->bounds.height = py;
	}

	r *= 1.1f;
	ptr->bounds.left -= r;
	ptr->bounds.top -= r;
	ptr->bounds.width += r;
	ptr->bounds.height += r;
}

#if 1
void slider_regenerate_texture(slider *ptr)
{
	if(ptr->control_points == NULL || ptr->control_points->count < 1 || ptr->curve_points == NULL || ptr->curve_points->count < 1)
	{
		slider_delete_tex(ptr);
		ptr->bounds = FLOATRECTZERO;
		return;
	}

	slider_calc_bounds(ptr);
	
	if((ptr->tex_size.x != rg.win_width_real && ptr->tex_size.y != rg.win_height_real) || (ptr->potato_state_on_regen != rg.potato_pc_enabled))
	{
		glDeleteTextures(1, &(ptr->tex));
		ptr->tex = 0;

		glDeleteFramebuffers(1, &(ptr->fbuf));
		ptr->fbuf = 0;
		
		ptr->tex_size = V2FZERO;
	}
	
	float texdiv = rg.potato_pc_enabled ? SLIDER_POTATO_DIV : 1;

	if(ptr->tex == 0 && ptr->fbuf == 0)
	{
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &(ptr->tex));
		ptr->tex_size = V2F(rg.win_width_real / texdiv, rg.win_height_real / texdiv);
		glGenFramebuffers(1, &(ptr->fbuf));
		glBindTexture(GL_TEXTURE_2D, ptr->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ptr->tex_size.x, ptr->tex_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindFramebuffer(GL_FRAMEBUFFER, ptr->fbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ptr->tex, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, ptr->fbuf);
	glClear(GL_COLOR_BUFFER_BIT);

	float ro = (CIRCLE_RADIUS * ptr->host_obj->host_song->object_size) / texdiv;
	float r = ro;

	int slider_renderer_point_count_detail = r;
	int pass_count = 0;
	int slider_shading_level = rg.potato_pc_enabled ? 1 : 2;
	list_node *n;
	v2f pfp;
	color4 col = col_white;
	for(n = ptr->curve_points->start; n != NULL;)
	{
		v2f *p = (v2f*) n->val;

		pfp = song_map_coord_to_play_field(ptr->host_obj->host_song, *p);
		if(rg.potato_pc_enabled)
		{
			pfp.x /= texdiv;
			pfp.y /= texdiv;
		}
		pfp.y = rg.win_height - pfp.y;
		pfp.x += rg.gui_view_xoff;
		pfp.y += rg.gui_view_yoff;

		GL_CIRCLE(pfp.x, pfp.y, r, slider_renderer_point_count_detail, 1, col);

		if(n == ptr->curve_points->end && !rg.potato_pc_enabled)
		{
			r = scale_value_to(pass_count, 0, slider_shading_level, ro * (0.9), 0);

#define scol(sv) scale_value_to(pass_count, 0, slider_shading_level, sv, 220)
			col = color4_gen(scol(0), scol(0), scol(0));
#undef scol
			/* col = se_gen_color(254, 184, 198); */

			n = ptr->curve_points->start;
			pass_count += 1;
		}
		else n = n->next;

		if(pass_count >= slider_shading_level) break;
	}

	v2f slider_start_pos = song_map_coord_to_play_field(ptr->host_obj->host_song, *(v2f*) list_get_val_at(ptr->curve_points, 0));
	v2f slider_end_pos = song_map_coord_to_play_field(ptr->host_obj->host_song, *(v2f*) list_get_val_at(ptr->curve_points, ptr->curve_points->count - 1));
	
	if(rg.potato_pc_enabled)
	{
		slider_start_pos.x /= texdiv;
		slider_start_pos.y /= texdiv;
		slider_end_pos.x /= texdiv;
		slider_end_pos.y /= texdiv;
	}

	slider_start_pos.y = rg.win_height - slider_start_pos.y;
	slider_end_pos.y = rg.win_height - slider_end_pos.y;
	slider_start_pos.x += rg.gui_view_xoff;
	slider_start_pos.y += rg.gui_view_yoff;
	slider_end_pos.x += rg.gui_view_xoff;
	slider_end_pos.y += rg.gui_view_yoff;

	loaded_texture *c_sp = rg.skin->circle;
	v2f sc = V2F(ptr->host_obj->host_song->object_size / texdiv, ptr->host_obj->host_song->object_size / texdiv);
	
	textures_drawsc(c_sp, slider_end_pos.x, slider_end_pos.y, sc.x, -sc.y, col_white);
	textures_drawsc(c_sp, slider_start_pos.x, slider_start_pos.y, sc.x, -sc.y, col_white);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ptr->potato_state_on_regen = rg.potato_pc_enabled;
}

#else

void slider_regenerate_texture(slider *ptr)
{
	if(ptr->control_points == NULL || ptr->control_points->count < 1 || ptr->curve_points == NULL || ptr->curve_points->count < 1)
	{
		slider_delete_tex(ptr);
		ptr->bounds = FLOATRECTZERO;
		return;
	}
	perf_timer_start();

	slider_calc_bounds(ptr);
	
	if((ptr->tex_size.x != rg.win_width_real && ptr->tex_size.y != rg.win_height_real) || (ptr->potato_state_on_regen != rg.potato_pc_enabled))
	{
		glDeleteTextures(1, &(ptr->tex));
		ptr->tex = 0;

		glDeleteFramebuffers(1, &(ptr->fbuf));
		ptr->fbuf = 0;
		
		ptr->tex_size = V2FZERO;
	}
	
	float texdiv = rg.potato_pc_enabled ? SLIDER_POTATO_DIV : 1;

	if(ptr->tex == 0 && ptr->fbuf == 0)
	{
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &(ptr->tex));
		ptr->tex_size = V2F(rg.win_width_real / texdiv, rg.win_height_real / texdiv);
		glGenFramebuffers(1, &(ptr->fbuf));
		glBindTexture(GL_TEXTURE_2D, ptr->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ptr->tex_size.x, ptr->tex_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindFramebuffer(GL_FRAMEBUFFER, ptr->fbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ptr->tex, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, ptr->fbuf);
	glClear(GL_COLOR_BUFFER_BIT);
	float r = (CIRCLE_RADIUS * ptr->host_obj->host_song->object_size) / texdiv;

	list_node *n;
	v2f next_point;
	v2f fix;
	v2f po;
/* 	int i = 0; */
	GLenum glm = GL_QUAD_STRIP;
	glBegin(glm);
	for(n = ptr->curve_points->start; n != NULL; n = n->next)
	{
		
/* 		if(i++ % (int)(clamp(r / 16, 1, ptr->curve_points->count)) != 0 && n != ptr->curve_points->end)
		{
			n = n->next;
			continue;
		} */
		
		if(n->next != NULL)
		{
			v2f *npv2f = n->next->val;
			next_point = song_map_coord_to_play_field(ptr->host_obj->host_song, *npv2f);
			if(rg.potato_pc_enabled)
			{
				next_point.x /= texdiv;
				next_point.y /= texdiv;
			}
			next_point.y = rg.win_height - next_point.y;
			next_point.x += gui_view_xoff;
			next_point.y += gui_view_yoff;
		}

		v2f *p = n->val;

		fix = V2F(p->x, p->y);
		po = song_map_coord_to_play_field(ptr->host_obj->host_song, fix);
		if(rg.potato_pc_enabled)
		{
			po.x /= texdiv;
			po.y /= texdiv;
		}
		po.y = rg.win_height - po.y;
		po.x += gui_view_xoff;
		po.y += gui_view_yoff;

		if(n != ptr->curve_points->end)
		{
			float rot = (rotate_towards(po, next_point) + 90) * (PI_NUM / 180.0f);

			v2f p1 = V2F(po.x, po.y - r);
			v2f p1r = rotate_point_angle(po.x, po.y, rot, p1);
			v2f p2 = V2F(po.x, po.y + r);
			v2f p2r = rotate_point_angle(po.x, po.y, rot, p2);
			glColor3ub(color4_2func(color4_gen_alpha(100, 100, 100, 200)));
			glVertex2f(p1r.x, p1r.y);
			glVertex2f(p2r.x, p2r.y);
		}
	}
	glEnd();
#ifndef SLIDER_DEBUG
	v2f slider_start_pos = song_map_coord_to_play_field(ptr->host_obj->host_song, *(v2f*)list_get_val_at(ptr->curve_points, 0));
	v2f slider_end_pos = song_map_coord_to_play_field(ptr->host_obj->host_song, *(v2f*)list_get_val_at(ptr->curve_points, ptr->curve_points->count - 1));
	
	if(rg.potato_pc_enabled)
	{
		slider_start_pos.x /= texdiv;
		slider_start_pos.y /= texdiv;
		slider_end_pos.x /= texdiv;
		slider_end_pos.y /= texdiv;
	}

	slider_start_pos.y = rg.win_height - slider_start_pos.y;
	slider_end_pos.y = rg.win_height - slider_end_pos.y;
	slider_start_pos.x += gui_view_xoff;
	slider_start_pos.y += gui_view_yoff;
	slider_end_pos.x += gui_view_xoff;
	slider_end_pos.y += gui_view_yoff;

	loaded_texture *c_sp = rg.skin->circle;
	v2f sc = V2F(ptr->host_obj->host_song->object_size / texdiv, ptr->host_obj->host_song->object_size / texdiv);
	textures_drawsc(c_sp, slider_end_pos.x, slider_end_pos.y, sc.x, -sc.y, col_white);
	textures_drawsc(c_sp, slider_start_pos.x, slider_start_pos.y, sc.x, -sc.y, col_white);
#endif
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ptr->potato_state_on_regen = rg.potato_pc_enabled;

	perf_timer_end("slider gen");
}
#endif

char *slider_control_points_serialize(slider *ptr)
{
	char *ret = init_empty_string();
	list_node *n;
    for(n = ptr->control_points->start; n != NULL; n = n->next)
    {
        slider_control_point *p = n->val;
		append_to_str_free_append(&ret, float_to_str(p->pos.x));
		append_to_str(&ret, SLIDER_CONTROL_POINT_COORD_SEP);
		append_to_str_free_append(&ret, float_to_str(p->pos.y));
		if(n != ptr->control_points->end) append_to_str(&ret, SLIDER_CONTROL_POINT_DATA_SEP);
    }
	append_to_str(&ret, SLIDER_DATA_SEP);
	append_to_str_free_append(&ret, int_to_str(ptr->reverses));
	append_to_str(&ret, SLIDER_DATA_SEP);
	append_to_str_free_append(&ret, int_to_str(ptr->curve_type));
	return ret;
}

void slider_overwrite_control_points(slider *ptr, char *serialized_data)
{
	slider_delete_control_points(ptr);
	ptr->control_points = list_init();
	int ci1 = 0;
	int ss1 = 0;
	int f1 = 0;
	char g1 = 1;
	while(g1)
	{
		char cc1 = serialized_data[ci1];
		char e1 = cc1 == '\0';
		if(cc1 == SLIDER_DATA_SEP_C || e1)
		{
			if(!e1) serialized_data[ci1] = '\0';
			char *rt1 = &serialized_data[ss1];

			switch(f1)
			{
				case 0:
				{
					int ci2 = 0;
					int ss2 = 0;
					int f2 = 0;
					char g2 = 1;
					while(g2)
					{
						char cc2 = rt1[ci2];
						char e2 = cc2 == '\0';
						if(cc2 == SLIDER_CONTROL_POINT_DATA_SEP_C || e2)
						{
							if(!e2) rt1[ci2] = '\0';
							char *rt2 = &rt1[ss2];

							int ci3 = 0;
							int ss3 = 0;
							int f3 = 0;
							char g3 = 1;
							v2f point;
							while(g3)
							{
								char cc3 = rt2[ci3];
								char e3 = cc3 == '\0';
								if(cc3 == SLIDER_CONTROL_POINT_COORD_SEP_C || e3)
								{
									if(!e3) rt2[ci3] = '\0';
									char *rt3 = &rt2[ss3];
									
									switch(f3)
									{
										case 0:
										{
											point.x = atof(rt3);
											break;
										}
										case 1:
										{
											point.y = atof(rt3);
											break;
										}
									}

									if(e3) g3 = 0;
									else
									{
										rt2[ci3] = SLIDER_CONTROL_POINT_COORD_SEP_C;
										f3++;
										ss3 = ci3 + 1;
									}
								}
								++ci3;
							}
							slider_add_control_point(ptr, point);

							if(e2) g2 = 0;
							else
							{
								rt1[ci2] = SLIDER_CONTROL_POINT_DATA_SEP_C;
								++f2;
								ss2 = ci2 + 1;
							}
						}
						++ci2;
					}
					break;
				}
				case 1:
				{
					ptr->reverses = atoi(rt1);
					break;
				}
				case 2:
				{
					ptr->curve_type = atoi(rt1);
					break;
				}
			}

			if(e1) g1 = 0;
			else
			{
				serialized_data[ci1] = SLIDER_DATA_SEP_C;
				++f1;
				ss1 = ci1 + 1;
			}
		}
		++ci1;
	}
}
