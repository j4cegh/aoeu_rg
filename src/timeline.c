#include "timeline.h"
#include "utils.h"
#include "circle.h"
#include "slider.h"
#include "spinner.h"
#include "song.h"
#include "skin.h"
#include "rg.h"
#include "mania_note.h"

void timeline_init(timeline *tl)
{
	tl->selected_obj = NULL;
    tl->sel_ms = 0.0f;
    tl->mouse_ms_oc = 0.0f;
    tl->selecting = 0;
	tl->long_drag = 0;
	tl->drag_revs = 0;
	tl->long_drag_ms = 0;
    tl->moved_object = 0;
    tl->moved_object_once = 0;
	tl->timeline_scale = 0.1f;
}

void timeline_tick(timeline *tl, struct song *song)
{
	if(rg.screen == screen_editor)
	{
		float timeline_scale = tl->timeline_scale;
		float timeline_view = (song->current_time_ms * timeline_scale) - rg.win_width_mid;
		float timeline_circle_scale = 0.5;
		float obj_radius = CIRCLE_RADIUS;
		float timeline_circle_pos = rg.win_height - (obj_radius * timeline_circle_scale);
		TIME_VAR mouse_ms = song->current_time_ms + ((mouse.x - rg.win_width_mid) / timeline_scale);
		if(left_click == 1 || middle_click == 1 || right_click == 1) tl->mouse_ms_oc = mouse_ms;
		TIME_VAR mouse_ms_drag = mouse_ms - tl->mouse_ms_oc;
		if(left_click == 0) mouse_ms_drag = 0;
		TIME_VAR beat_div = song->beat_length_ms / song->beat_divisor;
		if(left_click == 0 && tl->long_drag)
		{
			tl->long_drag = 0;
			tl->selected_obj = NULL;
			tl->drag_revs = 0;
			tl->long_drag_ms = 0;
		}
		if(left_click_released || !rg.focus)
		{
			if(tl->moved_object) song_capture_undo_state(song);
			tl->selected_obj = NULL;
			tl->selecting = 0;
			tl->moved_object = 0;
		}
		tl->moved_object_once = 0;

		float obj_scale_modifier = 0.8f;
		float obj_scale_modifier_inner_margin = ((1.0f - obj_scale_modifier) / 2.0f);
		float long_y_top = rg.win_height - ((obj_radius * (1.0f + obj_scale_modifier_inner_margin) * obj_scale_modifier));
		float long_y_bottom = rg.win_height - (obj_radius * obj_scale_modifier_inner_margin);
		float long_middle_y = long_y_top + ((long_y_bottom - long_y_top) / 2.0f);

		char del = 0;
		char regen_obj_nums_aftr = 0;
		list_node *obn;

		for(obn = song->game_mode_objects->start; obn != NULL; obn = obn->next)
		{
			object *obj = obn->val;
			obj->relative_time_ms = song->current_time_ms - obj->start_time_ms;
			if(left_click == 1) obj->tl_drag_rel_time_ms = mouse_ms - obj->start_time_ms;
			
			v2f sc = V2F(timeline_circle_scale * obj_scale_modifier, timeline_circle_scale * obj_scale_modifier);

			float radius = obj_radius * sc.x;
			float rend_dist = obj->length_time_ms > 0 ? (rg.win_width + obj->length_time_ms) * rg.gui_view_scale : rg.win_width_mid;
			if
			(
				obj->relative_time_ms >= -((rend_dist + (obj_radius / 2)) / timeline_scale)
				&&
				obj->relative_time_ms <= ((rend_dist + (obj_radius / 2)) / timeline_scale)
			)
			{
				float x_on_timeline = (obj->start_time_ms * timeline_scale) - timeline_view;
				char long_hovered = 0;

				if(obj->length_time_ms > 0)
				{
					float long_sx = (obj->start_time_ms * timeline_scale) - timeline_view;
					float long_ex = ((obj->start_time_ms * timeline_scale) + (obj->length_time_ms * timeline_scale)) - timeline_view;

					char long_drag_hovered = (get_distf(long_ex, long_middle_y, mouse.x, mouse.y) <= radius / 2) && song->selected_objects->count == 0;

					long_hovered =
					!long_drag_hovered
					&&
					(
						(
							mouse.y_on_click >= long_y_top
							&&
							mouse.y_on_click <= long_y_bottom
							&&
							tl->mouse_ms_oc >= obj->start_time_ms
							&&
							tl->mouse_ms_oc <= obj->start_time_ms + obj->length_time_ms
						)
						||
						get_distf(long_ex, long_middle_y, mouse.x_on_click, mouse.y_on_click) <= radius
					);
				}

				v2f note_pos = V2F(x_on_timeline, timeline_circle_pos);
				
				char hovered = get_distf(note_pos.x, note_pos.y, mouse.x_on_click, mouse.y_on_click) <= radius;

				char me_selected = 0;
				if(song->selected_objects->count > 0)
				{
					list_node *sn;
					for(sn = song->selected_objects->start; sn != NULL; sn = sn->next)
					{
						object *so = sn->val;
						if(obj == so)
						{
							me_selected = 1;
							break;
						}
					}
				}
				if((hovered || long_hovered) && !tl->selecting)
				{
					if(left_click >= 1 && tl->selected_obj == NULL && (song->selected_objects->count > 0 ? me_selected : 1)) tl->selected_obj = obj;
					else if(left_click == 0)
					{
						if(middle_click == 1 && !regen_obj_nums_aftr)
						{
							obj->new_combo = !obj->new_combo;
							regen_obj_nums_aftr = 1;
						}
						else if(right_click == 1 && !del)
						{
							if(song->selected_objects->count > 0)
							{
								song_delete_selected_objects(song);
								break;
							}
							else obj->delete_me = 1;
							del = 1;
						}
					}
				}
			}
		}

		for(obn = song->game_mode_objects->end; obn != NULL; obn = obn->before)
		{
			object *obj = obn->val;
			float rend_dist = obj->length_time_ms > 0 ? (rg.win_width + obj->length_time_ms) * rg.gui_view_scale : rg.win_width_mid;
			if
			(
				obj->relative_time_ms >= -((rend_dist + (obj_radius / 2)) / timeline_scale)
				&&
				obj->relative_time_ms <= ((rend_dist + (obj_radius / 2)) / timeline_scale)
			)
			{
				float x_on_timeline = (obj->start_time_ms * timeline_scale) - timeline_view;

				skin *sk = rg.skin;

				color4 obj_color = col_white;
				if(song->selected_objects->count > 0)
				{
					list_node *sn;
					for(sn = song->selected_objects->start; sn != NULL; sn = sn->next)
					{
						object *so = sn->val;
						if(obj == so)
						{
							obj_color = col_cyan;
							break;
						}
					}
				}

				v2f sc = V2F(timeline_circle_scale * obj_scale_modifier, timeline_circle_scale * obj_scale_modifier);

				if(obj->length_time_ms > 0)
				{
					float long_sx = (obj->start_time_ms * timeline_scale) - timeline_view;
					float long_ex = ((obj->start_time_ms * timeline_scale) + (obj->length_time_ms * timeline_scale)) - timeline_view;

					color4 long_rect_color = color4_mod_alpha(obj_color, 100);

					GL_RECT2QUAD(
						FLOATRECT(
							clamp(long_sx, 0, rg.win_width),
							long_y_top,
							clamp(long_ex, 0, rg.win_width),
							long_y_bottom
						),
						long_rect_color
					);

					slider *sl = obj->slider;
					if(sl != NULL && sl->reverses > 0)
					{
						int ri;
						for(ri = 0; ri < sl->reverses; ri++)
						{
							float revpos = ((ri + 1) * ((long_ex - long_sx) / (sl->reverses + 1)));

							v2f rapos = V2F(long_sx + revpos, timeline_circle_pos);

							if(rapos.x > -obj_radius && rapos.x < rg.win_width + obj_radius)
							{
								v2f rsc = V2F(timeline_circle_scale * obj_scale_modifier, timeline_circle_scale * obj_scale_modifier);
								textures_drawsc(sk->circle, rapos.x, rapos.y, rsc.x, rsc.y, obj_color);
								char should_reverse = ri == 1 || ri % 2 != 0;
								float rot = 90 * 2 + (should_reverse ? 180 : 0);
								glPushMatrix();
								glTranslatef(rapos.x, rapos.y, 0);
								glRotatef(rot, 0.0, 0.0, 1.0);
								textures_drawsc(sk->reversearrow, 0, 0, rsc.x, rsc.y, obj_color);
								glPopMatrix();
							}
						}
					}

					v2f end_cap_pos = V2F(long_ex, long_middle_y);
					if(end_cap_pos.x > -obj_radius && end_cap_pos.x < rg.win_width + obj_radius)
					{
						GL_ROT(end_cap_pos.x, end_cap_pos.y, -90);
						GL_CIRCLE(0, 0, (long_y_bottom - long_y_top) / 2, obj_radius, 0.5, long_rect_color);
						GL_ROT_END();

						textures_drawsc(sk->circle, end_cap_pos.x, end_cap_pos.y, sc.x, sc.y, obj_color);
					}
					
					v2f start_cap_pos = V2F(long_sx, long_middle_y);
					if(start_cap_pos.x > -obj_radius && start_cap_pos.x < rg.win_width + obj_radius)
					{
						GL_ROT(start_cap_pos.x, start_cap_pos.y, 90);
						GL_CIRCLE(0, 0, (long_y_bottom - long_y_top) / 2, obj_radius, 0.5, long_rect_color);
						GL_ROT_END();
					}

					float radius = obj_radius * sc.x;
					char long_drag_hovered = (left_click == 0 || get_distf(long_ex, long_middle_y, mouse.x_on_click, mouse.y_on_click) <= radius / 2) && get_distf(long_ex, long_middle_y, mouse.x, mouse.y) <= radius / 2;
					if(long_drag_hovered && !tl->long_drag && song->selected_objects->count == 0 && rg.gui_render_end_of_frame == song)
					{
						if(left_click == 1)
						{
							tl->long_drag = 1;
							tl->selected_obj = obj;
							if(tl->selected_obj->type == object_slider)
							{
								slider *slider = tl->selected_obj->slider;
								tl->drag_revs = slider->reverses;
							}
							if(tl->selected_obj->type == object_spinner)
							{
								tl->long_drag_ms = tl->selected_obj->length_time_ms;
							}
						}
						mouse.cursor_type = SDL_SYSTEM_CURSOR_SIZEWE;
					}
				}

				v2f note_pos = V2F(x_on_timeline, timeline_circle_pos);
				
				if(note_pos.x > -obj_radius && note_pos.x < rg.win_width + obj_radius)
				{
					textures_drawsc(sk->circle, note_pos.x, note_pos.y, sc.x, sc.y, obj_color);

					if(obj->circle)
					{
						circle_render_number(obj->circle, note_pos.x, note_pos.y, obj_radius, 0.5, obj_color);
					}
					if(obj->mania_note)
					{
						glPushMatrix();
						glTranslatef(note_pos.x, note_pos.y, 0);
						glScalef(sc.x, sc.y, 0);
						textures_drawsc(mania_note_get_tex(obj->mania_note), 0, 0, sc.x, sc.y, obj_color);
						glPopMatrix();
					}
				}
			}
		}

		if(tl->long_drag)
		{
			if(tl->selected_obj && left_click && rg.gui_render_end_of_frame == song)
			{
				mouse.cursor_type = SDL_SYSTEM_CURSOR_SIZEWE;
				if(tl->selected_obj->type == object_slider)
				{
					slider *slider = tl->selected_obj->slider;
					int revs_bef = slider->reverses;
					int mdr = ((mouse_ms_drag + (slider->segment_len_ms / 2 * (mouse_ms_drag > 0 ? 1 : -1))) / (slider->segment_len_ms));
					slider->reverses = clamp(tl->drag_revs + mdr, 0, INFINITY);
					if(slider->reverses != revs_bef) slider_calculate_path(slider, 1);
				}
				if(tl->selected_obj->type == object_spinner)
				{
					tl->selected_obj->length_time_ms = song_align_ms(song, mouse_ms_drag + tl->long_drag_ms);
				}
			}
		}

		char pf_drag = (song->selected_object != NULL && song->selected_object->dragging) || (song->dragging_selected_objects);
		if(!mouse.outside_window_during_drag && !pf_drag && !tl->long_drag)
		{
			if(mouse_ms - tl->mouse_ms_oc != 0 && tl->selected_obj && rg.gui_render_end_of_frame == song)
			{
				if(song->selected_objects->count > 0)
				{
					list_node *sn;
					char good = mouse_ms_drag < 0;
					for(sn = good ? song->selected_objects->start : song->selected_objects->end; sn != NULL; sn = good ? sn->next : sn->before)
					{
						object *so = sn->val;
						TIME_VAR bef_stms = so->start_time_ms;
						so->start_time_ms = song_align_ms(song, mouse_ms - so->tl_drag_rel_time_ms);
						if(so->start_time_ms != bef_stms)
						{
							tl->moved_object = 1;
							tl->moved_object_once = 1;
						}
						if(so->start_time_ms + so->length_time_ms > song->length_ms)
						{
							so->start_time_ms = bef_stms;
							break;
						}
						if(so->start_time_ms == bef_stms) break;
					}
				}
				else
				{
					TIME_VAR bef_sel_obj_stms = tl->selected_obj->start_time_ms;
					tl->selected_obj->start_time_ms = clamp(song_align_ms(song, mouse_ms - tl->selected_obj->tl_drag_rel_time_ms), 0, song->length_ms - tl->selected_obj->length_time_ms);
					if(tl->selected_obj->start_time_ms != bef_sel_obj_stms)
					{
						tl->moved_object = 1;
						tl->moved_object_once = 1;
					}
				}
				if(tl->moved_object_once) song_sort_objects(song);
			}
			else if(!tl->selected_obj && left_click > 0 && mouse.y_on_click >= rg.win_height - obj_radius && rg.gui_render_end_of_frame == song)
			{
				song_clear_selected_objects(song);
				if(left_click == 1) tl->sel_ms = mouse_ms;
				float sel_start_x = (clamp(tl->sel_ms, 0, song->length_ms) * timeline_scale) - timeline_view;
				float sel_end_x = (clamp(mouse_ms, 0, song->length_ms) * timeline_scale) - timeline_view;
				float lst = return_lowest(tl->sel_ms, mouse_ms);
				float hst = return_highest(tl->sel_ms, mouse_ms);

				GL_RECT2QUAD(
					FLOATRECT(
						clamp(sel_start_x, 0, rg.win_width),
						long_y_top,
						clamp(sel_end_x, 0, rg.win_width),
						long_y_bottom
					),
					color4_mod_alpha(col_yellow, 50)
				);

				list_node *sn;
				for(sn = song->game_mode_objects->start; sn != NULL; sn = sn->next)
				{
					object *so = sn->val;
					if
					(
						(
							so->start_time_ms >= lst
							&&
							so->start_time_ms <= hst
						)
						||
						(
							so->start_time_ms + so->length_time_ms >= lst
							&&
							so->start_time_ms + so->length_time_ms <= hst
						)
						||
						(
							lst >= so->start_time_ms
							&&
							hst <= so->start_time_ms + so->length_time_ms
						)
					) list_push_back(song->selected_objects, so);
				}

				tl->selecting = 1;

				song_sort_selected_objects(song);
			}
		}
		
		if(regen_obj_nums_aftr) song_sort_objects(song);

		song_apply_timing_sec(song, song->current_time_ms);

		TIME_VAR li;
		int id = 0;
		for(li = song->timing_section_start_ms; li <= song->length_ms; li += beat_div)
		{
			float x_on_timeline = (li * timeline_scale) - timeline_view;
			if(x_on_timeline >= 0.0f && x_on_timeline <= rg.win_width)
			{
				char down_beat = id % (int)(song->beat_divisor) == 0;
				GL_RECT2BOX
				(
					FLOATRECT
					(
						x_on_timeline,
						down_beat ? timeline_circle_pos : rg.win_height - (obj_radius / 4.0f),
						1.0f,
						(obj_radius / (down_beat ? 2.0f : 4.0f))
					),
					col_white
				);
			}
			id++;
		}

		GL_RECT2BOX(FLOATRECT(rg.win_width_mid - 1, rg.win_height - obj_radius, 1, obj_radius), col_white);
		GL_RECT2BOX(FLOATRECT(rg.win_width_mid + 1, rg.win_height - obj_radius, 1, obj_radius), col_white);
	}
}
