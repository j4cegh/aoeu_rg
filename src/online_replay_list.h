#pragma once

#include "list.h"
#include "rect.h"
#include "checkbox.h"

#define ONLINE_RL_ENTH 45

typedef struct online_replay_list_ent
{
	char *name_on_server;
	char *name;
	int score;
	int combo;
	float accuracy;
	float_rect rect;
	char hovered;
	char is_local;
} online_replay_list_ent;

typedef struct online_replay_list
{
	list *ents;
	float scroll_y;
	checkbox *is_local_cbox;
} online_replay_list;

online_replay_list_ent *online_replay_list_ent_init(char *name_on_server, char *name, int score, int combo, float accuracy);
online_replay_list_ent *online_replay_list_ent_deserialize(char *data);
char *online_replay_list_ent_serialize(online_replay_list_ent *ptr);
void online_replay_list_ent_free(online_replay_list_ent *ptr);

online_replay_list *online_replay_list_init();
void online_replay_list_overwrite(online_replay_list *ptr, char *data);
void online_replay_list_sort(online_replay_list *ptr);
void online_replay_list_load_local(online_replay_list *ptr);
void online_replay_list_reload(online_replay_list *ptr);
char *online_replay_list_serialize(online_replay_list *ptr);
void online_replay_list_add(online_replay_list *ptr, online_replay_list_ent *ent);
void online_replay_list_clear(online_replay_list *ptr);
#ifndef SERV
void online_replay_list_tick(online_replay_list *ptr, float width_lim, float x, float y);
#endif
void online_replay_list_free(online_replay_list *ptr);
