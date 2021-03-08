#include "replay.h"
#include "utils.h"
#ifndef SERV

#include "song.h"
#include "rg.h"
#endif

replay_frame *replay_frame_init(TIME_VAR time_ms, float cursor_x, float cursor_y, int click_1, int click_2, int click_3, int click_4, char paint, replay_event_type event, int objnum)
{
	replay_frame *ret = malloc(sizeof *ret);
	ret->time_ms = time_ms;
	ret->cursor_x = cursor_x;
	ret->cursor_y = cursor_y;
	ret->click_1 = click_1;
	ret->click_2 = click_2;
	ret->click_3 = click_3;
	ret->click_4 = click_4;
	ret->paint = paint;
	ret->event = event;
	ret->objnum = objnum;
	ret->next = NULL;
	ret->done = 0;
	return ret;
}

replay_frame *replay_frame_from_serialized(char *data)
{
	replay_frame *ret = NULL;

	list *split = split_str(data, REPLAY_FRAME_VAR_SEP);

	if(split->count == REPLAY_FRAME_SERIALIZED_VAR_COUNT)
	{
		ret = malloc(sizeof *ret);
		int i = 0;
		ret->time_ms = atof(list_get_val_at(split, i++));
		ret->cursor_x = atof(list_get_val_at(split, i++));
		ret->cursor_y = atof(list_get_val_at(split, i++));
		ret->click_1 = atoi(list_get_val_at(split, i++));
		ret->click_2 = atoi(list_get_val_at(split, i++));
		ret->click_3 = atoi(list_get_val_at(split, i++));
		ret->click_4 = atoi(list_get_val_at(split, i++));
		ret->paint = atoi(list_get_val_at(split, i++));
		ret->event = atoi(list_get_val_at(split, i++));
		ret->objnum = atoi(list_get_val_at(split, i++));
		ret->next = NULL;
		ret->done = 0;
	}

	list_free(split, free);

	return ret;
}

char *replay_frame_serialize(replay_frame *ptr)
{
	return dupfmt
	(
		"%.2f"
		REPLAY_FRAME_VAR_SEP
		"%.2f"
		REPLAY_FRAME_VAR_SEP
		"%.2f"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d"
		REPLAY_FRAME_VAR_SEP
		"%d",
		ptr->time_ms,
		ptr->cursor_x,
		ptr->cursor_y,
		ptr->click_1,
		ptr->click_2,
		ptr->click_3,
		ptr->click_4,
		ptr->paint,
		ptr->event,
		ptr->objnum
	);
}

replay *replay_init()
{
	replay *ret = malloc(sizeof *ret);
	ret->score = 0;
	ret->combo = 0;
	ret->accuracy = 0.0f;
	ret->year = 0;
	ret->month = 0;
	ret->day = 0;
	ret->hour = 0;
	ret->minute = 0;
	ret->second = 0;
	ret->frames = list_init();
	ret->fps_limit_clock = timer_init();
	ret->latest_ms = -100;
	ret->player_name = NULL;
#ifndef SERV
	ret->stats_text = NULL;
	ret->watch_btn = NULL;
#endif
	ret->file_loc = NULL;
	ret->date_str = NULL;
	ret->time_str = NULL;
	ret->raw_approach_rate = 0.0f;
	ret->raw_object_size = 0.0f;
	ret->mode = 0;

#ifndef SERV
	list_push_back(ret->frames, replay_frame_init(-1000, SONG_PFW / 2, SONG_PFH / 2, 0, 0, 0, 0, 0, replay_neutral, 0));
#else
	se_list_push_back(ret->frames, replay_frame_init(0, 0, 0, 0, 0, 0, 0, 0, replay_neutral, 0));
#endif
	return ret;
}

void replay_free(replay *ptr)
{
	if(ptr->time_str) free(ptr->time_str);
	if(ptr->date_str) free(ptr->date_str);
	if(ptr->file_loc) free(ptr->file_loc);
	if(ptr->player_name) free(ptr->player_name);
	timer_free(ptr->fps_limit_clock);
	list_free(ptr->frames, free);
	free(ptr);
}

void replay_clear_frames(replay *ptr)
{
	list_clear(ptr->frames, free);
}

void replay_reset(replay *ptr)
{
	list_node *n;
	for(n = ptr->frames->start; n != NULL; n = n->next)
	{
		replay_frame *fr = n->val;
		fr->done = 0;
	}
}

#ifndef SERV
int last_click_1 = -1;
int last_click_2 = -1;
int last_click_3 = -1;
int last_click_4 = -1;

void replay_add_frame(replay *ptr, struct song *song, TIME_VAR time_ms, float cursor_x, float cursor_y, int click_1, int click_2, int click_3, int click_4, char paint, replay_event_type event, int objnum)
{
	if(rg.screen != screen_player || song->music_state != song_playing || rg.replay_mode_enabled) return;

	TIME_VAR ms = timer_milliseconds(ptr->fps_limit_clock);
	char o1 = click_1 == 1 || click_2 == 1 || click_3 == 1 || click_4 == 1;
	char o2 = (click_1 > 1 || click_2 > 1 || click_3 > 1 || click_4 > 1) && (ms >= (1000 / REPLAY_MAX_FPS));
	char o3 = (ms > (1000 / REPLAY_IDLE_FPS));
	char o4 = song->slider_startend_hit;
	char o5 = (last_click_1 >= 1 && click_1 == 0) || (last_click_2 >= 1 && click_2 == 0) || (last_click_3 >= 1 && click_3 == 0) || (last_click_4 >= 1 && click_4 == 0);
	char o6 = event != replay_neutral;
	if(o1 || o2 || o3 || o4 || o5 || o6)
	{
/* #ifndef RELEASE
		c("---------------------");
		if(o1) printf("Button first frame.");
		if(o2) printf("Button held down (limited to max fps).");
		if(o3) printf("Idle frame (limited to idle fps).");
		if(o4) printf("Slider start or end hit.");
		if(o5) printf("Button released.");
		if(o6) printf("Event happened.");
#endif */

		replay_frame *nf = replay_frame_init(time_ms, cursor_x, cursor_y, click_1 > 0, click_2 > 0, click_3 > 0, click_4 > 0, paint, event, objnum);
		if(ptr->frames->count > 0)
		{
			replay_frame *last = ptr->frames->end->val;
			last->next = nf;
		}
		list_push_back(ptr->frames, nf);
		timer_restart(ptr->fps_limit_clock);

		last_click_1 = click_1;
		last_click_2 = click_2;
		last_click_3 = click_3;
		last_click_4 = click_4;
	}
}
#endif

replay_frame *replay_get_frame(replay *ptr, TIME_VAR time_ms, char events_only)
{
	replay_frame *closest = NULL;
	list_node *n;
	for(n = ptr->frames->start; n != NULL; n = n->next)
	{
		replay_frame *f = n->val;
		if(events_only)
		{
			if(f->event == replay_neutral) continue;
			if(!f->done && f->time_ms <= time_ms) closest = f;
			if(f->time_ms > time_ms) break;
		}
		else
		{
			if(f->time_ms > time_ms) break;
			if(f->time_ms <= time_ms) closest = f;
		}
	}
	return closest;
}

void replay_set_player_name(replay *ptr, char *player_name)
{
	if(ptr->player_name != NULL) free(ptr->player_name);
	ptr->player_name = dupe_str(player_name);
}

char *replay_serialize(replay *ptr, int score, int combo, float accuracy, float raw_approach_rate, float raw_object_size, char *song_hash, game_mode mode)
{
	ptr->score = score;
	ptr->combo = combo;
	ptr->accuracy = accuracy;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	ptr->year = 1900 + tm.tm_year;
	ptr->month = 1 + tm.tm_mon;
	ptr->day = tm.tm_mday;
	ptr->hour = tm.tm_hour;
	ptr->minute = tm.tm_min;
	ptr->second = tm.tm_sec;
	ptr->raw_approach_rate = raw_approach_rate;
	ptr->raw_object_size = raw_object_size;
	strncpy(ptr->song_hash, song_hash, SHA256_STRING_LEN);
	ptr->mode = mode;

	char *ret =
		dupfmt
		(
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%.2f"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP
			"%s"
			REPLAY_SER_DATA_SEP
			"%.2f"
			REPLAY_SER_DATA_SEP
			"%.2f"
			REPLAY_SER_DATA_SEP
			"%s"
			REPLAY_SER_DATA_SEP
			"%d"
			REPLAY_SER_DATA_SEP,
			ptr->score,
			ptr->combo,
			ptr->accuracy,
			ptr->year,
			ptr->month,
			ptr->day,
			ptr->hour,
			ptr->minute,
			ptr->second,
			ptr->player_name,
			ptr->raw_approach_rate,
			ptr->raw_object_size,
			ptr->song_hash,
			ptr->mode
		);
	
	list_node *n;
	for(n = ptr->frames->start; n != NULL; n = n->next)
	{
		append_to_str_free_append(&ret, replay_frame_serialize(n->val));
		if(n != ptr->frames->end) append_to_str(&ret, REPLAY_SER_FDATA_SEP);
	}

	return ret;
}

void replay_overwrite(replay *ptr, char *data)
{
	list *split = split_str(data, REPLAY_SER_DATA_SEP);
	if(split)
	{
		if(split->count == REPLAY_SERIALIZED_VAR_COUNT)
		{
			int i = 0;
			ptr->score = atoi(list_get_val_at(split, i++));
			ptr->combo = atoi(list_get_val_at(split, i++));
			ptr->accuracy = atof(list_get_val_at(split, i++));
			ptr->year = atoi(list_get_val_at(split, i++));
			ptr->month = atoi(list_get_val_at(split, i++));
			ptr->day = atoi(list_get_val_at(split, i++));
			ptr->hour = atoi(list_get_val_at(split, i++));
			ptr->minute = atoi(list_get_val_at(split, i++));
			ptr->second = atoi(list_get_val_at(split, i++));
			if(ptr->player_name) free(ptr->player_name);
			ptr->player_name = dupe_str(list_get_val_at(split, i++));
			ptr->raw_approach_rate = atof(list_get_val_at(split, i++));
			ptr->raw_object_size = atof(list_get_val_at(split, i++));
			strncpy(ptr->song_hash, list_get_val_at(split, i++), SHA256_STRING_LEN);
			ptr->mode = atoi(list_get_val_at(split, i++));

			list_clear(ptr->frames, free);

			list *frames_split = split_str(list_get_val_at(split, i++), REPLAY_SER_FDATA_SEP);

			if(frames_split != NULL)
			{
				if(frames_split->count > 0)
				{
					list_node *n;
					for(n = frames_split->start; n != NULL; n = n->next)
					{
						char *ns = n->val;
						if(ns != NULL)
						{
							replay_frame *nf = replay_frame_from_serialized(ns);
							if(nf)
							{
								if(ptr->frames->count > 0)
								{
									replay_frame *last = ptr->frames->end->val;
									if(last) last->next = nf;
								}
								list_push_back(ptr->frames, nf);
							}
						}
					}
				}

				list_free(frames_split, free);
			}
		}
#ifdef SERV
		else c("unable to load replay...");
#else
		else show_notif("unable to load replay...");
#endif

		list_free(split, free);
	}
}
