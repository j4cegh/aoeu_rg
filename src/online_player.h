#pragma once

#include "includes.h"

typedef struct online_player
{
	char *name;
	char *pass_hash;
	unsigned long score;
	unsigned long play_count;
	char is_admin;
	char is_mod;
	unsigned long rank;

	char joined;
	struct sfTcpSocket *tcpsock;
} online_player;

online_player *online_player_init(char *name);
void online_player_free(online_player *ptr);
