#include "online_packet.h"
#include "utils.h"
#include <string.h>

void online_packet_set(online_packet *pack, online_packet_type type, int mode, char *name, char *msg)
{
	pack->ver = ONLINE_PACKET_VERSION;
	pack->type = type;
	pack->mode = mode;
	strncpy(pack->name, name, sizeof(pack->name));
	strncpy(pack->msg, msg, sizeof(pack->msg));
}

char online_packet_decode(online_packet *into, char *encoded_text)
{
	if(!encoded_text) return 0;

	into->ver = 0;
	into->type = 0;
	into->mode = 0;
	into->name[0] = '\0';
	into->msg[0] = '\0';

	size_t len = strlen(encoded_text);

	int i;
	int i2 = 0;
	int e = 0;
	for(i = 0;; ++i)
	{
		char cc = encoded_text[i];
		if(cc == '\0') break;
		if(cc == ONLINE_PACKET_SEPERATOR)
		{
			encoded_text[i] = '\0';
			char *r = &encoded_text[i2];
			switch(e)
			{
				case 0: { into->ver  = atoi(r); break; }
				case 1: { into->type = atoi(r); break; }
				case 2: { into->mode  = atoi(r); break; }
				case 3: { strncpy(into->name, r, sizeof(into->name)); break; }
				case 4: { strncpy(into->msg, r, sizeof(into->msg)); break; }
				default: continue;
			}
			encoded_text[i] = ONLINE_PACKET_SEPERATOR;
			e++;
			i2 = i + 1;
			continue;
		}
	}

	if(e < 5) return 0;
	else return 1;
}

void online_packet_encode(online_packet *ptr, char *into)
{
#ifndef RELEASE
	if(str_contains(ptr->name, ONLINE_PACKET_SEPERATOR_STR, 0) || str_contains(ptr->msg, ONLINE_PACKET_SEPERATOR_STR, 0))
		printf("packet name or msg contains seperator character! replacing with \'?\'...");
#endif
	str_replace_chars(ptr->name, ONLINE_PACKET_SEPERATOR, '?');
	str_replace_chars(ptr->msg, ONLINE_PACKET_SEPERATOR, '?');
	snprintf
		(
			into,
			ONLINE_PACKET_MAX_SIZE,
			"%d"
			ONLINE_PACKET_SEPERATOR_STR
			"%d"
			ONLINE_PACKET_SEPERATOR_STR
			"%d"
			ONLINE_PACKET_SEPERATOR_STR
			"%s"
			ONLINE_PACKET_SEPERATOR_STR
			"%s"
			ONLINE_PACKET_SEPERATOR_STR,
			ptr->ver,
			ptr->type,
			ptr->mode,
			ptr->name,
			ptr->msg
		);
}
