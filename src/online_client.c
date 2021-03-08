#include "online_client.h"
#include "utils.h"
#include "console.h"
#include "online_server.h"
#include "online_globals.h"
#include "song_select.h"
#include "popup.h"
#include <sodium.h>
#include <SFML/Network.h>

online_client *online_client_init()
{
	online_client *ret = malloc(sizeof *ret);

	ret->connected = 0;
	ret->authed = 0;
	ret->last_pack_type_sent = -1;

	ret->update_send_timer = timer_init();

	ret->players = list_init();

	ret->recv_data = malloc(ONLINE_PACKET_MAX_SIZE * sizeof(char));

	ret->tcpsock = sfTcpSocket_create();

	ret->after_conn_act = online_client_after_connect_action_nothing;

	ret->me = online_player_init("");

	online_packet_set(&ret->spack, 0, 0, "", "");
	online_packet_set(&ret->rpack, 0, 0, "", "");

	return ret;
}

void online_client_free(online_client *ptr)
{
	online_client_disconnect(ptr, 0);
	online_player_free(ptr->me);
	list_free(ptr->players, list_dummy);
	sfTcpSocket_destroy(ptr->tcpsock);
	free(ptr->recv_data);
	timer_free(ptr->update_send_timer);
	free(ptr);
}

void online_client_set_name_and_pass_hash(online_client *ptr, char *name, char *pass_hash)
{
	free(ptr->me->name);
	ptr->me->name = dupe_str(name);
	if(ptr->me->pass_hash) free(ptr->me->pass_hash);
	ptr->me->pass_hash = dupe_str(pass_hash);
}

void online_client_connect(online_client *ptr)
{
	online_client_disconnect(ptr, 1);
	sfTcpSocket_setBlocking(ptr->tcpsock, sfTrue);
	sfSocketStatus st = sfTcpSocket_connect(ptr->tcpsock, sfIpAddress_fromString(ONLINE_CLIENT_SERV_IP), ONLINE_SERVER_PORT, sfSeconds(3));
	
	if(st == sfSocketDone)
	{
		ptr->connected = 1;
		ptr->started = 1;

		switch(ptr->after_conn_act)
		{
			case online_client_after_connect_action_nothing: break;
			case online_client_after_connect_action_login:
			{
				online_client_login(ptr);
				break;
			}
			case online_client_after_connect_action_register:
			{
				online_client_register(ptr);
				break;
			}
		}

		ptr->after_conn_act = online_client_after_connect_action_nothing;

		timer_restart(ptr->update_send_timer);
	}
	else show_center_status("server unreachable.");
	sfTcpSocket_setBlocking(ptr->tcpsock, sfFalse);
}

void online_client_login(online_client *ptr)
{
	if(ptr->me->pass_hash == NULL) return;
	online_packet_set(&ptr->spack, online_packet_join, 0, ptr->me->name, ptr->me->pass_hash);
	online_client_send(ptr, &ptr->spack);
}

void online_client_logout(online_client *ptr)
{
	if(!ptr->authed) return;
	online_packet_set(&ptr->spack, online_packet_quit, 0, ptr->me->name, "");
	online_client_send(ptr, &ptr->spack);
	ptr->authed = 0;
}

void online_client_register(online_client *ptr)
{
	if(ptr->me->pass_hash == NULL) return;
	online_packet_set(&ptr->spack, online_packet_register, 0, ptr->me->name, ptr->me->pass_hash);
	online_client_send(ptr, &ptr->spack);
}

void online_client_disconnect(online_client *ptr, char kicked)
{
	if(ptr->connected)
	{
		if(!kicked) online_client_logout(ptr);

		list_clear(ptr->players, online_player_free);

		ptr->connected = 0;
		ptr->authed = 0;
		ptr->last_pack_type_sent = (online_packet_type) -1;
	}
	sfTcpSocket_disconnect(ptr->tcpsock);
}

void online_client_yes_no_cback(void *ptr, char yes)
{
	online_client *cli = ptr;
	online_packet_set(&cli->spack, online_packet_popup_yes_no, 0, cli->me->name, yes ? "y" : "n");
	online_client_send(ptr, &cli->spack);
}

void online_client_tbox_cback(void *ptr, char *text)
{
	online_client *cli = ptr;
	online_packet_set(&cli->spack, online_packet_popup_input, 0, cli->me->name, text);
	online_client_send(ptr, &cli->spack);
}

void online_client_parse_packet(online_client *ptr, int packet_size)
{
	if(packet_size < 1) return;
#ifndef RELEASE
	printf("client recv (%d): %s\n", packet_size, ptr->recv_data);
#endif
	online_packet *rpack = &ptr->rpack;
	char r = online_packet_decode(rpack, ptr->recv_data);
	if(r)
	{
		switch (rpack->type)
		{
			case online_packet_join:
			{
				online_client_add_player(ptr, rpack);
				break;
			}
			case online_packet_chat:
			{
				char *name_appended = dupfmt("%s: %s", rpack->name, rpack->msg);
				console_print(cons_ptr, name_appended, col_white);
				if(!cons_ptr->activated) show_notif(name_appended);
				free(name_appended);
				break;
			}
			case online_packet_quit:
			{
				online_client_remove_player(ptr, rpack->name);
				break;
			}
			case online_packet_kick:
			{
				online_client_disconnect(ptr, 1);
				break;
			}
			case online_packet_popup:
			{
				popup_open(rg.global_popup, popup_type_dismiss, rpack->msg);
				break;
			}
			case online_packet_popup_input:
			{
				popup_open(rg.global_popup, popup_type_input, rpack->msg);
				rg.global_popup->txtbox_callback = &online_client_tbox_cback;
				rg.global_popup->ptr = ptr;
				break;
			}
			case online_packet_popup_yes_no:
			{
				popup_open(rg.global_popup, popup_type_yes_no, rpack->msg);
				rg.global_popup->yes_no_callback = &online_client_yes_no_cback;
				rg.global_popup->ptr = ptr;
				break;
			}
			case online_packet_show_notif:
			{
				show_notif(rpack->msg);
				break;
			}
			case online_packet_center_status:
			{
				show_center_status(rpack->msg);
				break;
			}
			case online_packet_join_ack:
			{
				ptr->authed = 1;
				strncpy(ptr->me->name, rpack->msg, strlen(ptr->me->name));
				break;
			}
			case online_packet_user_stats:
			{
				list *split = split_str(rpack->msg, ":");
				if(split->count == 5)
				{
					int i = 0;
					ptr->me->score = strtoul(list_get_val_at(split, i++), NULL, 10);
					ptr->me->play_count = strtoul(list_get_val_at(split, i++), NULL, 10);
					ptr->me->is_admin = atoi(list_get_val_at(split, i++));
					ptr->me->is_mod = atoi(list_get_val_at(split, i++));
					ptr->me->rank = strtoul(list_get_val_at(split, i++), NULL, 10);
				}
				list_free(split, free);
				break;
			}
			case online_packet_leaderboard:
			{
				online_replay_list_overwrite(ss->orl, rpack->msg);
				break;
			}
		}
	}
}

void online_client_tick(online_client *ptr)
{
	if(!ptr->started) return;

	sfPacket *pack = sfPacket_create();
	sfSocketStatus st = sfTcpSocket_receivePacket(ptr->tcpsock, pack);
	if(st == sfSocketDone)
	{
		sfBool is_replay = sfPacket_readBool(pack);
		if (is_replay)
		{
			char *data = malloc(sfPacket_getDataSize(pack));
			sfPacket_readString(pack, data);
			if(song_load_replay(rg.song, data))
			{
				rg.replay_mode_enabled = 1;
				rg.mode = rg.song->replay->mode;
				change_screen(screen_player);
			}
			free(data);
		}
		else
		{
			sfPacket_readString(pack, ptr->recv_data);
			size_t recv = strlen(ptr->recv_data);
			online_client_parse_packet(ptr, recv);
		}
	}
	else if(st == sfSocketDisconnected || st == sfSocketError)
		online_client_disconnect(ptr, 1);

	sfPacket_destroy(pack);
	if (ptr->authed)
	{
		if(timer_milliseconds(ptr->update_send_timer) > ONLINE_CLIENT_UPDATE_SEND_INTERVAL)
		{
			online_client_send_updates(ptr);
			timer_restart(ptr->update_send_timer);
		}
	}
}

char online_client_send(online_client *ptr, online_packet *pack)
{
	char ret = 0;
	online_packet_encode(pack, ptr->pack_enc_scratch);
	sfTcpSocket_setBlocking(ptr->tcpsock, sfTrue);
	sfPacket *sfpack = sfPacket_create();
	sfPacket_writeBool(sfpack, sfFalse);
	sfPacket_writeString(sfpack, ptr->pack_enc_scratch);
	if(sfTcpSocket_sendPacket(ptr->tcpsock, sfpack) == sfSocketDone) ret = 1;
	sfTcpSocket_setBlocking(ptr->tcpsock, sfFalse);
#ifndef RELEASE
	printf("client send: %s\n", ptr->pack_enc_scratch);
#endif
	sfPacket_destroy(sfpack);
	return ret;
}

void online_client_send_chat(online_client *ptr, char *msg)
{
	if(ptr->connected && ptr->authed)
	{
		online_packet_set(&ptr->spack, online_packet_chat, 0, ptr->me->name, msg);
		if(!online_client_send(ptr, &ptr->spack)) console_print(cons_ptr, "unable to send message to server!", col_red);
	}
	else console_print(cons_ptr, "you're not connected to a server!", col_red);
}

void online_client_send_updates(online_client *ptr)
{

}

void online_client_submit_replay(online_client *ptr, replay *rep, char *replay_data)
{
	sfTcpSocket_setBlocking(ptr->tcpsock, sfTrue);
	sfPacket *sfpack = sfPacket_create();
	sfPacket_writeBool(sfpack, sfTrue);
	sfPacket_writeString(sfpack, replay_data);
	sfTcpSocket_sendPacket(ptr->tcpsock, sfpack);
	sfPacket_destroy(sfpack);
	sfTcpSocket_setBlocking(ptr->tcpsock, sfFalse);
}

void online_client_download_replay(online_client *ptr, char *name_on_server)
{
	online_packet_set(&ptr->spack, online_packet_download_replay, 0, ptr->me->name, name_on_server);
	online_client_send(ptr, &ptr->spack);
}

void online_client_request_replay_list(online_client *ptr)
{
	if(ptr->authed && rg.song->game_mode_objects->count > 0)
	{
		online_packet_set(&ptr->spack, online_packet_leaderboard, rg.mode, ptr->me->name, rg.song->hash);
		online_client_send(ptr, &ptr->spack);
	}
}

void online_client_add_player(online_client *ptr, online_packet *pack)
{
	online_player *n = online_player_init(pack->name);
	list_push_back(ptr->players, n);
}

online_player *online_client_get_player(online_client *ptr, char *name)
{
	list_node *n;
	for(n = ptr->players->start; n != NULL; n = n->next)
	{
		online_player *np = n->val;
		if(strs_are_equal(np->name, name)) return np;
	}
	return NULL;
}

void online_client_update_player(online_client *ptr, online_packet *pack)
{

}

void online_client_remove_player(online_client *ptr, char *name)
{
	list_node *n;
	for(n = ptr->players->start; n != NULL; n = n->next)
	{
		online_player *p = n->val;
		if(strs_are_equal(p->name, name))
		{
			online_player_free(p);
			list_erase(ptr->players, n);
			return;
		}
	}
}
