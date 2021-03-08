#ifndef RGPUB
#include "online_server.h"
#include "online_client.h"
#include "online_replay_list.h"
#include "online_db.h"
#include "replay.h"
#include "online_globals.h"
#include "popup.h"
#include <sodium.h>
#include <SFML/Network.h>

#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <dirent.h>
#ifdef __linux__
#include <sys/socket.h>
#endif

#ifndef RELEASE
#ifdef ENABLE_RAND_PORT
int ONLINE_SERVER_PORT;
#endif
#endif

char *online_server_is_username_ok(online_server *ptr, char *name)
{
	if(name == NULL || name[0] == '\0') return dupe_str("your name is too short.");
	if(!is_str_legal(name, ONLINE_SERVER_ALLOWED_NAME_CHARS)) return dupe_str("your name has illegal characters.");
	online_player *u = online_server_get_user(ptr, name);
	int name_len = strlen(name);
	return dupe_str
		(
			u != NULL || strs_are_equal(name, ONLINE_SERVER_USERNAME) ? "name already in use." :
			strs_are_equal(name, ONLINE_SERVER_LEADERBOARD_FILENAME) ? "you can't use this name." :
			name_len < ONLINE_SERVER_MIN_USERNAME_LEN ? "name is too short!" :
			name_len > ONLINE_SERVER_MAX_USERNAME_LEN ? "name is too long!" :
			NULL
		);
}

online_server *online_server_init()
{
	make_dir(ONLINE_SERVER_DATA_DIR);
	make_dir(ONLINE_SERVER_ACCOUNTS_DIR);
	make_dir(ONLINE_SERVER_REPLAYS_DIR);

	online_server *ret = malloc(sizeof *ret);

	ret->running = 0;
	ret->uptime = timer_init();
	ret->save_clock = timer_init();

	ret->users = list_init();
	ret->users_remove = list_init();

	ret->socksel = sfSocketSelector_create();
	
	ret->tcpl = sfTcpListener_create();

	ret->db = online_db_init(ONLINE_SERVER_ACCOUNTS_DIR);

	ret->recv_data = malloc(ONLINE_PACKET_MAX_SIZE * sizeof(char));

	online_packet_set(&ret->spack, 0, 0, "", "");
	online_packet_set(&ret->rpack, 0, 0, "", "");

	ret->serv_plr = online_player_init(ONLINE_SERVER_USERNAME);
	ret->serv_plr->is_admin = 1;
	ret->serv_plr->is_mod = 1;
	ret->serv_plr->joined = 1;

	ret->cons_cmds_queue = list_init();

	ret->cons_cmds_queue_lock = 0;

	return ret;
}

void online_server_free(online_server *ptr)
{
	ptr->running = 0;

	int status = 0;
	SDL_WaitThread(ptr->th, &status);

	int seconds_open = timer_seconds(ptr->uptime);
	rgspf(loglev_normal, "going down after running for %d days, %d hours, %d minutes, and %d seconds.", seconds_open / (3600 * 24), (seconds_open % (3600 * 24)) / 3600, (seconds_open % 3600) / 60, seconds_open % 60);

	list_free(ptr->cons_cmds_queue, free);
	
	online_player_free(ptr->serv_plr);

	online_db_free(ptr->db);

	sfSocketSelector_destroy(ptr->socksel);

	sfTcpListener_destroy(ptr->tcpl);

	free(ptr->recv_data);

	list_free(ptr->users_remove, list_dummy);

	list_free(ptr->users, online_player_free);

	timer_free(ptr->save_clock);

	timer_free(ptr->uptime);

	free(ptr);

	rgsp(loglev_normal, "goodbye.");
}

#ifndef RELEASE
#ifdef ENABLE_RAND_PORT
void online_server_rand_port()
{
	ONLINE_SERVER_PORT = rand_num_ranged(7501, 35000);
	rgspf(loglev_normal, "randomized port to %d...", ONLINE_SERVER_PORT);
}
#endif
#endif

char online_server_start(online_server *ptr)
{
#if !defined(RELEASE) && defined(ENABLE_RAND_PORT)
	online_server_rand_port();
	while(sfTcpListener_listen(ptr->tcpl, ONLINE_SERVER_PORT, sfIpAddress_Any) != sfSocketDone)
		online_server_rand_port();
#else
	if(sfTcpListener_listen(ptr->tcpl, ONLINE_SERVER_PORT, sfIpAddress_Any) == sfSocketDone)
#endif
	{
		sfSocketSelector_addTcpListener(ptr->socksel, ptr->tcpl);
		rgspf(loglev_normal, "aoeu's rhythm game server (on port %d)", ONLINE_SERVER_PORT);
		ptr->running = 1;
		ptr->th = SDL_CreateThread(online_server_tick, "rgserv", ptr);
		timer_restart(ptr->uptime);
		return 1;
	}
#ifdef RELEASE
	else rgsp(loglev_error, "failed to start server...");
#endif
	return 0;
}

int online_server_tick(void *vptr)
{
	online_server *ptr = vptr;
	while(ptr->running)
	{
		if(sfSocketSelector_wait(ptr->socksel, sfSeconds(1)) == sfTrue)
		{
			if (sfSocketSelector_isTcpListenerReady(ptr->socksel, ptr->tcpl) == sfTrue)
			{
				sfTcpSocket *sock = sfTcpSocket_create();
				if (sfTcpListener_accept(ptr->tcpl, &sock) == sfSocketDone)
				{
					sfSocketSelector_addTcpSocket(ptr->socksel, sock);
					online_player *p = online_server_add_user(ptr, "user", sock);
					sfPacket *pack = sfPacket_create();
					sfSocketStatus st = sfTcpSocket_receivePacket(p->tcpsock, pack);
					if(st == sfSocketDone)
					{
						sfBool is_replay = sfPacket_readBool(pack);
						if(!is_replay)
						{
							sfPacket_readString(pack, ptr->recv_data);
							size_t recv = strlen(ptr->recv_data);
							online_server_parse_packet(ptr, p, recv);
						}
					}
					else online_server_rm_user(ptr, p);
					sfPacket_destroy(pack);
				}
				else sfTcpSocket_destroy(sock);
			}
			else
			{
				list_node *n;
				for (n = ptr->users->start; n != NULL; n = n->next)
				{
					online_player *p = n->val;
					if (sfSocketSelector_isTcpSocketReady(ptr->socksel, p->tcpsock))
					{
						online_account *acc = online_db_get_acc(ptr->db, p->name);
						sfPacket *pack = sfPacket_create();
						sfSocketStatus st = sfTcpSocket_receivePacket(p->tcpsock, pack);
						if (st == sfSocketDone)
						{
							sfBool is_replay = sfPacket_readBool(pack);
							if (is_replay)
							{
								char *data = malloc(sfPacket_getDataSize(pack));
								sfPacket_readString(pack, data);
								
								replay *rep = replay_init();
								replay_overwrite(rep, data);
								char same_name = strs_are_equal(rep->player_name, p->name);
								if(!same_name)
								{
									rgspf(loglev_warning, "%s uploaded a replay file from %s!!! not accepted.", p->name, rep->player_name);
									online_server_send_popup(ptr, "this replay isn't yours.", popup_type_dismiss, p);
								}
								else
								{
									char *sdir = dupfmt(ONLINE_SERVER_REPLAYS_DIR "%s/", rep->song_hash);
									make_dir(sdir);
									char *dir = dupfmt("%s%d/", sdir, rep->mode);
									make_dir(dir);

									++(acc->play_count);
									p->play_count = acc->play_count;

									char beat_score = 0;
									char *to_loc = dupfmt("%s%s", dir, p->name);
									if(file_exists(to_loc))
									{
										replay *ex = replay_init();
										char *exd = file_to_str(to_loc);
										replay_overwrite(ex, exd);
										free(exd);
										if (rep->score > ex->score)
										{
											beat_score = 1;
											remove(to_loc);
											acc->score -= ex->score;
											acc->score += rep->score;
											p->score = acc->score;
											online_db_sort_rank(ptr->db);
											p->rank = acc->rank;
											online_server_send_stats_to_all(ptr);
										}
										replay_free(ex);
									}
									else
									{
										beat_score = 2;
										acc->score += rep->score;
										p->score = acc->score;
										online_db_sort_rank(ptr->db);
										p->rank = acc->rank;
										online_server_send_stats_to_all(ptr);
									}
									
									if(beat_score == 0) online_server_send_stats_to_player(ptr, p);
									online_account_save(acc);

									online_packet_set(&ptr->spack, online_packet_center_status, 0, ONLINE_SERVER_USERNAME, beat_score == 1 ? "you beat your score!" : "upload successful!");
									online_server_send(ptr, &ptr->spack, p);

									if (beat_score > 0) str_to_file(to_loc, data);

									char *leaderboard_file = dupfmt("%s" ONLINE_SERVER_LEADERBOARD_FILENAME, dir);
									remove(leaderboard_file);
									free(leaderboard_file);

									free(to_loc);
									free(dir);
									free(sdir);
								}

								replay_free(rep);
								free(data);
							}
							else
							{
								sfPacket_readString(pack, ptr->recv_data);
								size_t recv = strlen(ptr->recv_data);
								online_server_parse_packet(ptr, p, recv);
							}
						}
						else online_server_rm_user(ptr, p);
						sfPacket_destroy(pack);
					}
				}
			}
		}

		list_node *n;
		for (n = ptr->users_remove->start; n != NULL; n = n->next)
		{
			online_player *ua = n->val;
			list_node *n2;
			char g = 1;
			for (n2 = ptr->users->start; n2 != NULL && g; n2 = n2->next)
			{
				online_player* ua2 = n2->val;
				if (ua == ua2)
				{
					sfSocketSelector_removeTcpSocket(ptr->socksel, ua2->tcpsock);
					online_player_free(ua2);
					list_erase(ptr->users, n2);
					g = 0;
				}
			}
		}
		list_clear(ptr->users_remove, list_dummy);

		for(n = ptr->cons_cmds_queue->start; n != NULL;)
		{
			char *cmd = n->val;
			online_server_parse_command(ptr, cmd, ptr->serv_plr);
			free(cmd);
			list_erase(ptr->cons_cmds_queue, n);
			n = ptr->cons_cmds_queue->start;
		}

		if(timer_milliseconds(ptr->save_clock) > ONLINE_SERVER_SAVE_INTERVAL_MS)
		{
			online_db_save_all(ptr->db);
			timer_restart(ptr->save_clock);
		}
	}
	return 0;
}

void online_server_run_cons_cmd(online_server *ptr, char *cmd)
{
	while(ptr->cons_cmds_queue_lock) {}
	ptr->cons_cmds_queue_lock = 1;
	list_push_back(ptr->cons_cmds_queue, dupfmt("$%s", cmd));
	ptr->cons_cmds_queue_lock = 0;
}

char online_server_parse_packet(online_server *ptr, online_player *from, int recieved)
{
	if(recieved < 1) return 0;
	
	online_packet *rpack = &ptr->rpack;
	char r = 0;
#ifndef RELEASE
	rgspf(loglev_normal, "server recv (%d): %", recieved, ptr->recv_data);
#endif
	r = online_packet_decode(rpack, ptr->recv_data);

	if(r)
	{
		switch (rpack->type)
		{
			case online_packet_join:
			{
				if(!from->joined)
				{
					char *reason = online_server_is_username_ok(ptr, rpack->name);
					if(reason == NULL)
					{
						char good_login = 0;
						online_account *acc = online_db_get_acc(ptr->db, rpack->name);
						if(acc)
						{
							if(strs_are_equal(acc->pass_hash, rpack->msg)) good_login = 2;
							else good_login = 1;
						}
						if(good_login == 2) online_server_join_user(ptr, from, rpack, acc);
						else
						{
							char *deny_msg = good_login == 0 ? "account doesn't exist!" : good_login == 1 ? "wrong password!" : "can't login.";
							rgspf(loglev_warning, "denied %s from logging in because: %s", rpack->name, deny_msg);
							online_packet_set(&ptr->spack, online_packet_popup, 0, ONLINE_SERVER_USERNAME, deny_msg);
							online_server_send(ptr, &ptr->spack, from);
						}
					}
					else
					{
						rgspf(loglev_warning, "username %s is blocked from logging in because: %s", rpack->name, reason);
						online_packet_set(&ptr->spack, online_packet_popup, 0, ONLINE_SERVER_USERNAME, reason);
						online_server_send(ptr, &ptr->spack, from);
						free(reason);
					}
				}
				else rgspf(loglev_warning, "%s sent a join packet after they were already logged in...", from->name);
				break;
			}
			case online_packet_register:
			{
				if(!from->joined)
				{
					char *reason = online_server_is_username_ok(ptr, rpack->name);
					if(reason == NULL)
					{
						online_account *acc = online_db_new(ptr->db, rpack->name, rpack->msg);
						if(acc != NULL) online_server_join_user(ptr, from, rpack, acc);
						else
						{
							if(acc == NULL) rgspf(loglev_error, "%s was blocked from registering twice", rpack->name)
							online_packet_set(&ptr->spack, online_packet_popup, 0, ONLINE_SERVER_USERNAME, acc == NULL ? "account already exists!" : "you're registered. login now.");
							online_server_send(ptr, &ptr->spack, from);
						}
					}
					else
					{
						rgspf(loglev_warning, "username %s is blocked from registering because: %s", rpack->name, reason);
						online_packet_set(&ptr->spack, online_packet_popup, 0, ONLINE_SERVER_USERNAME, reason);
						online_server_send(ptr, &ptr->spack, from);
						free(reason);
					}
				}
				else rgspf(loglev_warning, "%s sent a register packet after they were already logged in...", from->name);
				break;
			}
			case online_packet_chat:
			{
				if(from->joined)
				{
					int mlen = strlen(rpack->msg);
					if(mlen >= ONLINE_SERVER_MIN_CHAT_LEN && mlen <= ONLINE_SERVER_MAX_CHAT_LEN)
					{
						if(rpack->msg[0] == ONLINE_SERVER_COMMAND_PREFIX)
							online_server_parse_command(ptr, rpack->msg, from);
						else
						{
							rgspf(loglev_normal, "%s: %s", rpack->name, rpack->msg);
							online_server_send_to_all(ptr, rpack, NULL);
						}
					}
				}
				break;
			}
			case online_packet_quit:
			{
				if(from->joined)
				{
					char *leave_msg = dupfmt("%s logged out.", from->name);
					rgsp(loglev_normal, leave_msg);
					online_server_broadcast_msg(ptr, leave_msg);
					free(leave_msg);
					online_server_send_to_all(ptr, rpack, from);
					online_server_rm_user(ptr, from);
				}
				break;
			}
			case online_packet_popup_input:
			{
				char *resp = rpack->msg;

				break;
			}
			case online_packet_popup_yes_no:
			{
				char yes = rpack->msg[0] == 'y';
				
				break;
			}
			case online_packet_leaderboard:
			{
				if(from->joined && rpack->mode >= 0 && rpack->mode < game_mode_count)
				{
					size_t rml = strlen(rpack->msg);
					if(rml != SHA256_STRING_LEN - 1) break;
					char *dir = dupfmt(ONLINE_SERVER_REPLAYS_DIR "%s/%d/", rpack->msg, rpack->mode);
					char *leaderboard_file = dupfmt("%s" ONLINE_SERVER_LEADERBOARD_FILENAME, dir);
					if(file_exists(leaderboard_file))
					{
						char *data = file_to_str(leaderboard_file);
						online_packet_set(&ptr->spack, online_packet_leaderboard, 0, ONLINE_SERVER_USERNAME, data);
						online_server_send(ptr, &ptr->spack, from);
						free(data);
					}
					else
					{
						replay *sr = replay_init();
						online_replay_list *lb = online_replay_list_init();
						if(dir_exists(dir) && rpack->msg[0] != '.')
						{
							DIR *dr = opendir(dir);
							struct dirent *entry;
							while ((entry = readdir(dr)) != NULL)
							{
								if (entry->d_type == DT_REG && !strs_are_equal(entry->d_name, ONLINE_SERVER_LEADERBOARD_FILENAME))
								{
									char *loc = dupfmt("%s%s", dir, entry->d_name);
									char *data = file_to_str(loc);
									replay_overwrite(sr, data);
									char *name_on_server = dupfmt("%s/%d/%s", rpack->msg, rpack->mode, entry->d_name);
									online_replay_list_ent *ent = online_replay_list_ent_init(name_on_server, sr->player_name, sr->score, sr->combo, sr->accuracy);
									online_replay_list_add(lb, ent);
									free(name_on_server);
									free(data);
									free(loc);
								}
							}
							closedir(dr);
						}
						online_replay_list_sort(lb);
						char *lbs = online_replay_list_serialize(lb);
						online_packet_set(&ptr->spack, online_packet_leaderboard, 0, ONLINE_SERVER_USERNAME, lbs);
						online_server_send(ptr, &ptr->spack, from);
						str_to_file(leaderboard_file, lbs);
						free(lbs);
						replay_free(sr);
						online_replay_list_free(lb);
					}
					free(leaderboard_file);
					free(dir);
				}
				break;
			}
			case online_packet_download_replay:
			{
				if(from->joined && count_char_occurances(rpack->msg, '/') == 2 && strlen(rpack->msg) >= SHA256_STRING_LEN + 1 && rpack->msg[SHA256_STRING_LEN - 1] == '/')
				{
					char *loc = dupfmt(ONLINE_SERVER_REPLAYS_DIR "%s", rpack->msg);
					if(file_exists(loc))
					{
						char *replay_data = file_to_str(loc);
						sfPacket *pack = sfPacket_create();
						sfPacket_writeBool(pack, sfTrue);
						sfPacket_writeString(pack, replay_data);
						sfTcpSocket_sendPacket(from->tcpsock, pack);
						sfPacket_destroy(pack);
						free(replay_data);
					}
					free(loc);
				}
				break;
			}
		}
	}
	return r;
}

typedef struct cmd_help
{
	char cmd_name[100];
	char cmd_desc[1024];
} cmd_help;

list *rhelp_list;

char *rcmd_help_add(char *cmd_name, char *cmd_desc)
{
	cmd_help *ch = malloc(sizeof *ch);
	strncpy(&(ch->cmd_name[0]), cmd_name, sizeof(ch->cmd_name));
	strncpy(&(ch->cmd_desc[0]), cmd_desc, sizeof(ch->cmd_desc));
	list_push_back(rhelp_list, ch);
	return cmd_name;
}

void online_server_parse_command(online_server *ptr, char *cmd, online_player *from)
{
	if(strlen(cmd) < 2 || !from->joined)
	{
		online_server_send_msg_to_user(ptr, "your command is too short!", from, ONLINE_SERVER_USERNAME);
		return;
	}

	rgspf(loglev_normal, "%s executed command \"%s\"", from->name, cmd);

	char is_op = from->is_admin;

	list *split = split_str(&cmd[1], " ");

	char *a0 = list_get_val_at(split, 0);
	char *a1 = NULL;
	if(split->count >= 2) a1 = list_get_val_at(split, 1);

	char *after_a0 = NULL;
	char *after_a1 = NULL;
	if(split->count >= 2) after_a0 = cmd + (strlen(a0) + 2);
	if (split->count >= 3) after_a1 = after_a0 + (strlen(a1) + 1);

	rhelp_list = list_init();

	if(is_op)
	{
		if (strs_are_equal(a0, rcmd_help_add("popall", "show a popup message to all players on the server")) && after_a0)
		{
			online_server_send_popup_to_all(ptr, popup_type_dismiss, after_a0);
		}
		if (strs_are_equal(a0, rcmd_help_add("popiall", "show an input box to all players on the server")) && after_a0)
		{
			online_server_send_popup_to_all(ptr, popup_type_input, after_a0);
		}
		if (strs_are_equal(a0, rcmd_help_add("popcall", "show a yes/no choice box to all players on the server")) && after_a0)
		{
			online_server_send_popup_to_all(ptr, popup_type_yes_no, after_a0);
		}
		else if (strs_are_equal(a0, rcmd_help_add("pop", "show a popup message to one player")) && after_a0 && after_a1)
		{
			online_player *target = online_server_get_user(ptr, a1);
			if (target) online_server_send_popup(ptr, after_a1, popup_type_dismiss, target);
			else online_server_send_msg_to_user(ptr, "user not found.", from, ONLINE_SERVER_USERNAME);
		}
		else if (strs_are_equal(a0, rcmd_help_add("popi", "show an input box to one player")) && after_a0 && after_a1)
		{
			online_player *target = online_server_get_user(ptr, a1);
			if (target) online_server_send_popup(ptr, after_a1, popup_type_input, target);
			else online_server_send_msg_to_user(ptr, "user not found.", from, ONLINE_SERVER_USERNAME);
		}
		else if (strs_are_equal(a0, rcmd_help_add("popc", "show a yes/no choice popup message to one player")) && after_a0 && after_a1)
		{
			online_player *target = online_server_get_user(ptr, a1);
			if (target) online_server_send_popup(ptr, after_a1, popup_type_yes_no, target);
			else online_server_send_msg_to_user(ptr, "user not found.", from, ONLINE_SERVER_USERNAME);
		}
		else if(strs_are_equal(a0, rcmd_help_add("kick", "kick a player by name")) && after_a0)
		{
			online_player *target = online_server_get_user(ptr, after_a0);
			if(target)
			{
#ifdef RELEASE
				if (target == from) online_server_send_msg_to_user(ptr, "you can't kick yourself.", from, ONLINE_SERVER_USERNAME);
				else
#endif
				{
					online_server_send_popup(ptr, "you have been kicked!", popup_type_dismiss, target);
					online_server_kick_user(ptr, target, "kicked by admin");
				}
			}
		}
		else if(strs_are_equal(a0, rcmd_help_add("togmod", "toggle a user's mod status")) && after_a0)
		{
			online_player *target = online_server_get_user(ptr, after_a0);
			if(target)
			{
				online_account *acc = online_db_get_acc(ptr->db, target->name);
				acc->is_mod = !acc->is_mod;
				target->is_mod = acc->is_mod;
				online_server_send_stats_to_player(ptr, target);
			}
		}
		else if(strs_are_equal(a0, rcmd_help_add("togadmin", "toggle a user's admin status")) && after_a0)
		{
			online_player *target = online_server_get_user(ptr, after_a0);
			if(target)
			{
				online_account *acc = online_db_get_acc(ptr->db, target->name);
				acc->is_admin = !acc->is_admin;
				target->is_admin = acc->is_admin;
				online_server_send_stats_to_player(ptr, target);
			}
		}
	}

	if(strs_are_equal(a0, rcmd_help_add("online", "list all online users")))
	{
		online_server_send_msg_to_user(ptr, "online users:", from, ONLINE_SERVER_USERNAME);
		list_node *n;
		for(n = ptr->users->start; n != NULL; n = n->next)
		{
			online_player *u = n->val;
			char *fmt = dupfmt("%s #%lu (score: %lu, play count: %lu, role: %s)", u->name, u->rank, u->score, u->play_count, u->is_admin ? "admin" : u->is_mod ? "mod" : "user");
			online_server_send_msg_to_user(ptr, fmt, from, ONLINE_SERVER_USERNAME);
			free(fmt);
		}
	}

	if(strs_are_equal(a0, "help"))
	{
		online_server_send_msg_to_user(ptr, "list of usable commands:", from, ONLINE_SERVER_USERNAME);

		list_node *hn;
		for(hn = rhelp_list->start; hn != NULL; hn = hn->next)
		{
			cmd_help *ch = hn->val;
			char *cmd_hm = dupfmt("%s: %s", ch->cmd_name, ch->cmd_desc);
			online_server_send_msg_to_user(ptr, cmd_hm, from, ONLINE_SERVER_USERNAME);
			free(cmd_hm);
		}
	}

	list_free(rhelp_list, free);

	list_free(split, free);
}

void online_server_join_user(online_server *ptr, online_player *u, online_packet *pack, online_account *acc)
{
	free(u->name);
	u->name = dupe_str(acc->name);
	u->joined = 1;
	u->score = acc->score;
	u->play_count = acc->play_count;
	u->is_admin = acc->is_admin;
	u->is_mod = acc->is_mod;
	u->rank = acc->rank;
	
	online_packet_set(&ptr->spack, online_packet_join_ack, 0, ONLINE_SERVER_USERNAME, acc->name);
	online_server_send(ptr, &ptr->spack, u);

	online_server_send_stats_to_player(ptr, u);

	list_node *n;
	for(n = ptr->users->start; n != NULL; n = n->next)
	{
		online_player *uu = n->val;
		if(uu == u || uu->joined == 0) continue;
		online_packet_set(&ptr->spack, online_packet_join, 0, uu->name, "");
		online_server_send(ptr, &ptr->spack, u);
	}

	online_server_send_to_all(ptr, pack, u);

	char *jmsg = dupfmt("%s logged in.", u->name);
	rgsp(loglev_normal, jmsg);
	online_server_broadcast_msg(ptr, jmsg);
	free(jmsg);
}

void online_server_kick_user(online_server *ptr, online_player *u, char *reason)
{
	if(u->joined)
	{
		online_packet_set(&ptr->spack, online_packet_kick, 0, ONLINE_SERVER_USERNAME, reason);
		online_server_send(ptr, &ptr->spack, u);
		
		online_packet_set(&ptr->spack, online_packet_quit, 0, u->name, "");
		online_server_send_to_all(ptr, &ptr->spack, u);
	}

	online_server_rm_user(ptr, u);
}

void online_server_rm_user(online_server *ptr, online_player *u)
{
	list_push_back(ptr->users_remove, u);
}

char online_server_send(online_server *ptr, online_packet *pack, online_player *pl)
{
	if(pl == ptr->serv_plr)
	{
		rgsp(loglev_normal, pack->msg);
		return 1;
	}
	char ret = 0;
	online_packet_encode(pack, ptr->pack_enc_scratch);
	sfPacket *sfpack = sfPacket_create();
	sfPacket_writeBool(sfpack, sfFalse);
	sfPacket_writeString(sfpack, ptr->pack_enc_scratch);
	if(sfTcpSocket_sendPacket(pl->tcpsock, sfpack) == sfSocketDone) ret = 1;
#ifndef RELEASE
	rgspf(loglev_normal, "server send (%s): %s", ret ? "sent" : "failed", ptr->pack_enc_scratch);
#endif
	sfPacket_destroy(sfpack);
	return ret;
}

void online_server_send_msg_to_user(online_server *ptr, char *msg, online_player *user, char *sender_username)
{
	if(user != ptr->serv_plr) rgspf(loglev_normal, "sending msg \"%s\" from %s to %s", msg, sender_username, user->name);
	online_packet_set(&ptr->spack, online_packet_chat, 0, sender_username, msg);
	online_server_send(ptr, &ptr->spack, user);
}

void online_server_send_stats_to_all(online_server *ptr)
{
	list_node *n;
	for(n = ptr->users->start; n != NULL; n = n->next)
	{
		online_player *u = n->val;
		online_server_send_stats_to_player(ptr, u);
	}
}

void online_server_send_stats_to_player(online_server *ptr, online_player *u)
{
	char *stats_str = dupfmt("%lu:%lu:%d:%d:%lu", u->score, u->play_count, u->is_admin, u->is_mod, u->rank);
	online_packet_set(&ptr->spack, online_packet_user_stats, 0, ONLINE_SERVER_USERNAME, stats_str);
	online_server_send(ptr, &ptr->spack, u);
	free(stats_str);
}

void online_server_broadcast_msg(online_server *ptr, char *msg)
{
	online_packet_set(&ptr->spack, online_packet_chat, 0, ONLINE_SERVER_USERNAME, msg);
	online_server_send_to_all(ptr, &ptr->spack, NULL);
}

online_player *online_server_add_user(online_server *ptr, char *name, sfTcpSocket *sock)
{
	online_player *exists = online_server_get_user(ptr, name);
	if(exists != NULL)
	{
		sfTcpSocket_destroy(exists->tcpsock);
		exists->tcpsock = sock;
		return exists;
	}
	online_player *nu = online_player_init(name);
	nu->tcpsock = sock;
	list_push_back(ptr->users, nu);
	return nu;
}

online_player *online_server_get_user(online_server *ptr, char *name)
{
	list_node *n;
	for(n = ptr->users->start; n != NULL; n = n->next)
	{
		online_player *u = n->val;
		if(strs_are_equal(u->name, name)) return u;
	}
	return NULL;
}

void online_server_send_to_all(online_server *ptr, online_packet *pack, online_player *exception)
{
	list_node *n;
	for(n = ptr->users->start; n != NULL; n = n->next)
	{
		online_player *u = n->val;
		if(u != exception) online_server_send(ptr, pack, u);
	}
}

#define ONLINE_SERVER_POPUP_PACKET_SET online_packet_set(&ptr->spack, type == popup_type_dismiss ? online_packet_popup : type == popup_type_yes_no ? online_packet_popup_yes_no : type == popup_type_input ? online_packet_popup_input : online_packet_popup, 0, ONLINE_SERVER_USERNAME, popup_msg)

void online_server_send_popup(online_server *ptr, char *popup_msg, popup_type type, online_player *user)
{
	ONLINE_SERVER_POPUP_PACKET_SET;
	online_server_send(ptr, &ptr->spack, user);
}

void online_server_send_popup_to_all(online_server *ptr, popup_type type, char *popup_msg)
{
	ONLINE_SERVER_POPUP_PACKET_SET;
	online_server_send_to_all(ptr, &ptr->spack, NULL);
}

#ifdef SERV
int main(int argc, char *argv[])
{
	online_server *serv = online_server_init();
	online_server_start(serv);
	online_server_tick(serv);
	online_server_free(serv);
	return 0;
}
#endif

#endif
