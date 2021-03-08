#include "timing_section.h"

#include "utils.h"
#include "rg.h"
#include "song.h"

void timing_section_offseti_set_to_ctime(void *tsec)
{
	timing_section *ts = tsec;
	ts->start_ms = ts->host_song->current_time_ms;
	char *offset_str = float_to_str(ts->start_ms);
	input_box_set_text(ts->offseti, offset_str);
	free(offset_str);
}

void timing_section_bpmi_div_by_2(void *tsec)
{
	timing_section *ts = tsec;
	ts->beat_length_ms *= 2;
	char *bpm_str = float_to_str(beat_length_ms_to_beats_per_minute(ts->beat_length_ms));
	input_box_set_text(ts->bpmi, bpm_str);
	free(bpm_str);
}

void timing_section_bpmi_mult_by_2(void *tsec)
{
	timing_section *ts = tsec;
	ts->beat_length_ms /= 2;
	char *bpm_str = float_to_str(beat_length_ms_to_beats_per_minute(ts->beat_length_ms));
	input_box_set_text(ts->bpmi, bpm_str);
	free(bpm_str);
}

timing_section *timing_section_init(struct song *host_song, TIME_VAR beat_length_ms, TIME_VAR start_ms, TIME_VAR slider_time_pixel_ratio, TIME_VAR beat_divisor)
{
	timing_section *ret = malloc(sizeof *ret);

	ret->host_song = host_song;

	ret->beat_length_ms = beat_length_ms;
	ret->start_ms = start_ms;
	ret->slider_time_pixel_ratio = slider_time_pixel_ratio;
	ret->beat_divisor = beat_divisor;

	ret->bpmi = input_box_init(0, 0, 100, "", "bpm");
	ret->bpmi->force_numbers_only = 1;
	ret->bpmi->show_label_above = 1;
	context_menu_add_option_with_callback(ret->bpmi->menu, "div by 2", 0, &timing_section_bpmi_div_by_2, ret);
	context_menu_add_option_with_callback(ret->bpmi->menu, "mult by 2", 0, &timing_section_bpmi_mult_by_2, ret);

	ret->offseti = input_box_init(0, 0, 100, "", "start ms");
	ret->offseti->force_numbers_only = 1;
	ret->offseti->show_label_above = 1;
	context_menu_add_option_with_callback(ret->offseti->menu, "set to now", 0, &timing_section_offseti_set_to_ctime, ret);

	ret->slider_pix_ratioi = input_box_init(0, 0, 100, "", "slider pixel time");
	ret->slider_pix_ratioi->force_numbers_only = 1;
	ret->slider_pix_ratioi->show_label_above = 1;

	ret->beat_divi = input_box_init(0, 0, 100, "", "beat divisor");
	ret->beat_divi->force_numbers_only = 1;
	ret->beat_divi->show_label_above = 1;

	ret->deleteb = button_init(0, 0, "delete", 1, 0, NULL);
	ret->addaboveb = button_init(0, 0, "add above", 1, 0, NULL);
	ret->addbelowb = button_init(0, 0, "add below", 1, 0, NULL);
	ret->jumpb = button_init(0, 0, "jump to", 1, 0, NULL);

	ret->bpmi->before = ret->beat_divi;
	ret->bpmi->next = ret->offseti;
	ret->offseti->before = ret->bpmi;
	ret->offseti->next = ret->slider_pix_ratioi;
	ret->slider_pix_ratioi->before = ret->offseti;
	ret->slider_pix_ratioi->next = ret->beat_divi;
	ret->beat_divi->before = ret->slider_pix_ratioi;
	ret->beat_divi->next = ret->bpmi;

	return ret;
}

timing_section *timing_section_deserialize(struct song *host_song, char *serialized)
{
	list *sep = split_str(serialized, TIMING_SECTION_SERIALIZED_SEP);

	if(sep)
	{
		if(sep->count == 4)
		{
			TIME_VAR beat_length_ms = atof(list_get_val_at(sep, 0));
			TIME_VAR start_ms = atof(list_get_val_at(sep, 1));
			TIME_VAR slider_time_pixel_ratio = atof(list_get_val_at(sep, 2));
			TIME_VAR beat_divisor = atof(list_get_val_at(sep, 3));
			list_free(sep, free);

			return timing_section_init(host_song, beat_length_ms, start_ms, slider_time_pixel_ratio, beat_divisor);
		}

		list_free(sep, free);
	}

	return NULL;
}

void timing_section_free(timing_section *ptr)
{
	button_free(ptr->jumpb);
	button_free(ptr->addbelowb);
	button_free(ptr->addaboveb);
	button_free(ptr->deleteb);
	input_box_free(ptr->beat_divi);
	input_box_free(ptr->slider_pix_ratioi);
	input_box_free(ptr->offseti);
	input_box_free(ptr->bpmi);
	free(ptr);
}

timing_section_tick_action timing_section_tick(timing_section *ptr, float x, float y)
{
	if(y > rg.win_height || y < -TIMING_SECTION_HEIGHT) return timing_section_ta_nothing;
	timing_section_tick_action ta_ret = timing_section_ta_nothing;

	color4 bg_col = col_black;
	if(song_get_timing_section(ptr->host_song, ptr->host_song->current_time_ms) == ptr) bg_col = col_gray;

	GL_OLBOX(FLOATRECT(50, y, rg.win_width - (100), TIMING_SECTION_HEIGHT), col_white, bg_col, 1);

	float padding = 5;
	float gx = x + 50 + padding;
	float gy = y + ptr->bpmi->label_ptr->size.y + PADDING + (padding * 2);

	if(current_input_box != ptr->bpmi)
	{
		char *bpm_str = float_to_str(beat_length_ms_to_beats_per_minute(ptr->beat_length_ms));
		input_box_set_text(ptr->bpmi, bpm_str);
		free(bpm_str);
	}
	else
	{
		ptr->beat_length_ms = clamp(beats_per_minute_to_beat_length_ms(atof(ptr->bpmi->text)), 1, INFINITY);
	}
	input_box_update_vars(ptr->bpmi, gx, gy, ptr->bpmi->box_width_pixels);
	input_box_update(ptr->bpmi);
	input_box_render(ptr->bpmi);

	gy += ptr->bpmi->size.y + ptr->bpmi->label_ptr->size.y + (PADDING * 2) + padding;

	if(current_input_box != ptr->offseti)
	{
		char *offset_str = float_to_str(ptr->start_ms);
		input_box_set_text(ptr->offseti, offset_str);
		free(offset_str);
	}
	else
	{
		ptr->start_ms = clamp(atof(ptr->offseti->text), 0.0f, ptr->host_song->length_ms);
	}
	input_box_update_vars(ptr->offseti, gx, gy, ptr->offseti->box_width_pixels);
	input_box_update(ptr->offseti);
	input_box_render(ptr->offseti);

	gx = ptr->bpmi->pos.x + ptr->bpmi->size.x + padding;
	gy = ptr->bpmi->pos.y;

	if(current_input_box != ptr->slider_pix_ratioi)
	{
		char *sl_tpr_str = float_to_str(ptr->slider_time_pixel_ratio);
		input_box_set_text(ptr->slider_pix_ratioi, sl_tpr_str);
		free(sl_tpr_str);
	}
	else
	{
		ptr->slider_time_pixel_ratio = clamp(atof(ptr->slider_pix_ratioi->text), 0.1f, 48.0f);
	}
	input_box_update_vars(ptr->slider_pix_ratioi, gx, gy, ptr->slider_pix_ratioi->box_width_pixels);
	input_box_update(ptr->slider_pix_ratioi);
	input_box_render(ptr->slider_pix_ratioi);

	gy += ptr->slider_pix_ratioi->size.y + ptr->slider_pix_ratioi->label_ptr->size.y + (PADDING * 2) + padding;

	if(current_input_box != ptr->beat_divi)
	{
		char *dv_str = float_to_str(ptr->beat_divisor);
		input_box_set_text(ptr->beat_divi, dv_str);
		free(dv_str);
	}
	else
	{
		ptr->beat_divisor = clamp(atof(ptr->beat_divi->text), 1.0f, SONG_MAX_BEAT_DIVIDE);
	}
	input_box_update_vars(ptr->beat_divi, gx, gy, ptr->beat_divi->box_width_pixels);
	input_box_update(ptr->beat_divi);
	input_box_render(ptr->beat_divi);

	gx = ptr->slider_pix_ratioi->pos.x + ptr->slider_pix_ratioi->size.x + (padding * 4);
	gy = y + (padding * 2);

	if(ptr->host_song->timing_sections->count <= 1) ptr->deleteb->enabled = 0;
	else ptr->deleteb->enabled = 1;
	button_set_pos(ptr->deleteb, gx, gy);
	button_update(ptr->deleteb);
	button_render(ptr->deleteb);
	gy = ptr->deleteb->pos.y + ptr->deleteb->size.y + padding;

	button_set_pos(ptr->addaboveb, gx, gy);
	button_update(ptr->addaboveb);
	button_render(ptr->addaboveb);
	gy = ptr->addaboveb->pos.y + ptr->addaboveb->size.y + padding;

	button_set_pos(ptr->addbelowb, gx, gy);
	button_update(ptr->addbelowb);
	button_render(ptr->addbelowb);
	gy = ptr->addbelowb->pos.y + ptr->addbelowb->size.y + padding;

	button_set_pos(ptr->jumpb, gx, gy);
	button_update(ptr->jumpb);
	button_render(ptr->jumpb);

	if(ptr->deleteb->clicked)
	{
		current_input_box = NULL;
		current_context_menu = NULL;
		ta_ret = timing_section_ta_delete_me;
	}
	else if(ptr->addaboveb->clicked)
	{
		ta_ret = timing_section_ta_add_above;
	}
	else if(ptr->addbelowb->clicked)
	{
		ta_ret = timing_section_ta_add_below;
	}
	else if(ptr->jumpb->clicked)
	{
		song_stop(ptr->host_song);
		song_set_time(ptr->host_song, ptr->start_ms);
		change_screen(screen_editor);
	}

	return ta_ret;
}

char *timing_section_serialize(timing_section *ptr)
{
	return dupfmt
	(
		"%.2f"
		TIMING_SECTION_SERIALIZED_SEP
		"%.2f"
		TIMING_SECTION_SERIALIZED_SEP
		"%.2f"
		TIMING_SECTION_SERIALIZED_SEP
		"%.2f",
		ptr->beat_length_ms,
		ptr->start_ms,
		ptr->slider_time_pixel_ratio,
		ptr->beat_divisor
	);
}
