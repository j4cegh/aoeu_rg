#include "online_player.h"

online_player *online_player_init(char *name)
{
	online_player *ret = malloc(sizeof *ret);
	ret->name = dupe_str(name);
	ret->pass_hash = NULL;
	ret->score = 0;
	ret->play_count = 0;
	ret->is_admin = 0;
	ret->is_mod = 0;
	ret->rank = 0;
	ret->joined = 0;
	ret->tcpsock = NULL;
	return ret;
}

void online_player_free(online_player *ptr)
{
	if(ptr->tcpsock != NULL) sfTcpSocket_destroy(ptr->tcpsock);
	if(ptr->pass_hash) free(ptr->pass_hash);
	free(ptr->name);
	free(ptr);
}
