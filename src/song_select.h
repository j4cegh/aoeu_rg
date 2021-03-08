#pragma once

#include "includes.h"
#include "online_replay_list.h"
#include "list.h"

#define SONG_SELECT_ENTRY_WIDTH 600
#define SONG_SELECT_ENTRY_HEIGHT 100
#define SONG_SELECT_ENTRY_PADDING -10

typedef struct song_select_entry
{
	char *name;
	char *folder_loc;
	char *data_file_loc;
	char no_data_files;
	float scale;
	float_rect rect;
	float ry;
	char hovered;
	char hovered_oc;
	char should_render;
	float x;
	char traveled_to;
} song_select_entry;

song_select_entry *song_select_entry_init(char *name, char *folder_loc, char *data_file_loc, char no_data_files);
void song_select_entry_click(song_select_entry* ptr, char enter);
void song_select_entry_free(song_select_entry *ptr);

typedef struct song_select
{
	list *entries;
	float y_scroll;
	float y_scroll_speed;
	float y_max_height;
	timer *scroll_clock, *scan_clock;
	song_select_entry *load_next;
	struct text *no_songs_text;
	int last_song_count;
	float list_global_x;
	char show_ents;
	float bef_travel_y;
	online_replay_list *orl;
} song_select;

extern song_select *ss;

void song_select_init();
void song_select_free(char clear_only);
song_select_entry *song_select_refresh(char no_status);
char song_select_load(song_select_entry *e);
void song_select_tick();
void song_select_load_random();
