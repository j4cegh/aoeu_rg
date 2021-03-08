#pragma once

#include "list.h"
#include "utils.h"
#include "online_packet.h"
#include "online_player.h"
#include "online_globals.h"
#include "online_db.h"
#include "logger.h"
#include "popup.h"

#define ONLINE_SERVER_USERNAME "~"
#define ONLINE_SERVER_COMMAND_PREFIX '$'
#define ONLINE_SERVER_SAVE_INTERVAL_MS (60000 * 5)
#define ONLINE_SERVER_DATA_DIR "./server_data/"
#define ONLINE_SERVER_ACCOUNTS_DIR ONLINE_SERVER_DATA_DIR "accounts/"
#define ONLINE_SERVER_REPLAYS_DIR ONLINE_SERVER_DATA_DIR "replays/"
#define ONLINE_SERVER_LEADERBOARD_FILENAME "leaderboard"

#define rgsp(lev, msg) \
{ \
	printf("%s\n", msg); \
	log_msg(lev, msg); \
}

#define rgspf(lev, fmt, ...) \
{ \
	char *fmtd = dupfmt(fmt, __VA_ARGS__); \
	printf("%s\n", fmtd); \
	log_msg(lev, fmtd); \
	free(fmtd); \
}

typedef struct online_server
{
	char running;
	timer *uptime;
	timer *save_clock;
	list *users;
	list *users_remove;
	char *recv_data;
	struct sfTcpListener *tcpl;
	struct sfSocketSelector *socksel;
	online_packet spack, rpack;
	char pack_enc_scratch[ONLINE_PACKET_MAX_SIZE];
	online_db *db;
	SDL_Thread *th;
	online_player *serv_plr;
	list *cons_cmds_queue;
	char cons_cmds_queue_lock;
} online_server;

char *online_server_is_username_ok(online_server *ptr, char *name);

online_server *online_server_init();
void online_server_free(online_server *ptr);

char online_server_start(online_server *ptr);

int online_server_tick(void *vptr);

void online_server_run_cons_cmd(online_server *ptr, char *cmd);

char online_server_parse_packet(online_server *ptr, online_player *from, int recieved);
void online_server_parse_command(online_server *ptr, char *cmd, online_player *from);

void online_server_join_user(online_server *ptr, online_player *u, online_packet *pack, online_account *acc);
void online_server_kick_user(online_server *ptr, online_player *u, char *reason);
void online_server_rm_user(online_server *ptr, online_player *u);

char online_server_send(online_server *ptr, online_packet *pack, online_player *pl);
void online_server_send_msg_to_user(online_server *ptr, char *msg, online_player *user, char *sender_username);
void online_server_send_stats_to_all(online_server *ptr);
void online_server_send_stats_to_player(online_server *ptr, online_player *u);

void online_server_broadcast_msg(online_server *ptr, char *msg);

online_player *online_server_add_user(online_server *ptr, char *name, struct sfTcpSocket *sock);
online_player *online_server_get_user(online_server *ptr, char *name);

void online_server_send_to_all(online_server *ptr, online_packet *pack, online_player *exception);
void online_server_send_popup(online_server *ptr, char *popup_msg, popup_type type, online_player *user);
void online_server_send_popup_to_all(online_server *ptr, popup_type type, char *popup_msg);
