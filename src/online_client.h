#pragma once

#include "includes.h"
#include "online_packet.h"
#include "online_player.h"
#include "replay.h"
#include "utils.h"

#define ONLINE_CLIENT_UPDATE_SEND_INTERVAL 40

#ifdef RELEASE
#define ONLINE_CLIENT_SERV_IP ""
#else
#define ONLINE_CLIENT_SERV_IP "localhost"
#endif

typedef enum online_client_after_connect_action
{
	online_client_after_connect_action_nothing,
	online_client_after_connect_action_login,
	online_client_after_connect_action_register
} online_client_after_connect_action;

typedef struct online_client
{
	char started, connected, authed;
	online_packet_type last_pack_type_sent;
	timer *update_send_timer;
	list *players;
	char *recv_data;
	struct sfTcpSocket *tcpsock;
	online_client_after_connect_action after_conn_act;
	online_player *me;
	online_packet spack, rpack;
	char pack_enc_scratch[ONLINE_PACKET_MAX_SIZE];
} online_client;

online_client *online_client_init();
void online_client_free(online_client *ptr);
void online_client_set_name_and_pass_hash(online_client *ptr, char *name, char *pass_hash);
void online_client_connect(online_client *ptr);
void online_client_login(online_client *ptr);
void online_client_logout(online_client *ptr);
void online_client_register(online_client *ptr);
void online_client_disconnect(online_client *ptr, char kicked);
void online_client_parse_packet(online_client *ptr, int packet_size);
void online_client_tick(online_client *ptr);
char online_client_send(online_client *ptr, online_packet *pack);
void online_client_send_chat(online_client *ptr, char *msg);
void online_client_send_updates(online_client *ptr);
void online_client_submit_replay(online_client *ptr, replay *rep, char *replay_data);
void online_client_download_replay(online_client *ptr, char *name_on_server);
void online_client_request_replay_list(online_client *ptr);
void online_client_add_player(online_client *ptr, online_packet *pack);
online_player *online_client_get_player(online_client *ptr, char *name);
void online_client_update_player(online_client *ptr, online_packet *pack);
void online_client_remove_player(online_client *ptr, char *name);
