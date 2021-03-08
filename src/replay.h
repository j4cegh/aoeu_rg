#pragma once

#include "includes.h"
#include "list.h"
#ifndef SERV
#include "object.h"
#include "rg.h"
#include "song.h"
#include "text.h"
#include "button.h"
#else
#include "globals.h"
#endif

#define REPLAY_SER_DATA_SEP "|"
#define REPLAY_SER_FDATA_SEP "_"
#define REPLAY_FRAME_VAR_SEP "~"

#define REPLAY_FRAME_SERIALIZED_VAR_COUNT 10
#define REPLAY_SERIALIZED_VAR_COUNT 15

#define REPLAY_IDLE_FPS 5
#define REPLAY_MAX_FPS 20

#define REPLAY_GUI_HEIGHT 55

typedef enum replay_event_type
{
	replay_neutral,
	replay_note_click,
	replay_hold_toggle
} replay_event_type;

typedef struct replay_frame
{
	TIME_VAR time_ms;
	float cursor_x;
	float cursor_y;
	int click_1;
	int click_2;
	int click_3;
	int click_4;
	char paint;
	struct replay_frame *next;
	replay_event_type event;
	int objnum;
	char done;
} replay_frame;

replay_frame *replay_frame_init(TIME_VAR time_ms, float cursor_x, float cursor_y, int click_1, int click_2, int click_3, int click_4, char paint, replay_event_type event, int objnum);
replay_frame *replay_frame_from_serialized(char *data);
char *replay_frame_serialize(replay_frame *ptr);

typedef struct replay
{
	int score;
	int combo;
	float accuracy;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	list *frames;
	timer *fps_limit_clock;
	TIME_VAR latest_ms;
	char *player_name;
#ifndef SERV
	text *stats_text;
	button *watch_btn;
#endif
	char *file_loc;
	char *date_str;
	char *time_str;
	float raw_approach_rate;
	float raw_object_size;
	char song_hash[SHA256_STRING_LEN];
	game_mode mode;
} replay;

replay *replay_init();
void replay_free(replay *ptr);

typedef enum replay_action
{
	replay_action_nothing,
	replay_action_watch
} replay_action;

void replay_clear_frames(replay *ptr);
void replay_reset(replay *ptr);
#ifndef SERV
void replay_add_frame(replay *ptr, struct song *song, TIME_VAR time_ms, float cursor_x, float cursor_y, int click_1, int click_2, int click_3, int click_4, char paint, replay_event_type event, int objnum);
#endif
replay_frame *replay_get_frame(replay *ptr, TIME_VAR time_ms, char events_only);
void replay_set_player_name(replay *ptr, char *player_name);
char *replay_serialize(replay *ptr, int score, int combo, float accuracy, float raw_approach_rate, float raw_object_size, char *song_hash, game_mode mode);
void replay_overwrite(replay *ptr, char *data);
