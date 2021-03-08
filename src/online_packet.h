#pragma once

#include "includes.h"
#include "online_globals.h"

#define ONLINE_PACKET_VERSION 0
#define ONLINE_PACKET_SEPERATOR '@'
#define ONLINE_PACKET_SEPERATOR_STR "@"
#define ONLINE_PACKET_ITEM_COUNT 5
#define ONLINE_PACKET_MAX_SIZE 65507

typedef enum online_packet_type
{
	online_packet_join,
	online_packet_chat,
	online_packet_quit,
	online_packet_join_ack,
	online_packet_kick,
	online_packet_register,
	online_packet_popup,
	online_packet_popup_input,
	online_packet_popup_yes_no,
	online_packet_show_notif,
	online_packet_center_status,
	online_packet_user_stats,
	online_packet_send_replay,
	online_packet_leaderboard,
	online_packet_download_replay
} online_packet_type;

typedef struct online_packet
{
	int ver;
	online_packet_type type;
	int mode;
	char name[ONLINE_SERVER_MAX_USERNAME_LEN];
	char msg[32768];
} online_packet;

void online_packet_set(online_packet *pack, online_packet_type type, int mode, char *name, char *msg);
char online_packet_decode(online_packet *into, char *encoded_text);
void online_packet_encode(online_packet *ptr, char *into);
