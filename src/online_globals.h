#pragma once

#ifdef __linux__
#define ENABLE_RAND_PORT
#endif

#ifdef RELEASE
#define ONLINE_SERVER_PORT 7500
#else
#ifdef ENABLE_RAND_PORT
extern int ONLINE_SERVER_PORT;
#else
#define ONLINE_SERVER_PORT 7501
#endif
#endif
#define ONLINE_SERVER_MIN_USERNAME_LEN 4
#define ONLINE_SERVER_MAX_USERNAME_LEN 24
#define ONLINE_SERVER_MIN_CHAT_LEN 1
#define ONLINE_SERVER_MAX_CHAT_LEN 500
#define ONLINE_SERVER_ALLOWED_NAME_CHARS " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

/*
int ci1 = 0;
int ss1 = 0;
int f1 = 0;
char g1 = 1;
while(g1)
{
	char cc1 = serialized_data[ci1];
	char e1 = cc1 == '\0';
	if(cc1 == ??? || e1)
	{
		if(!e1) serialized_data[ci1] = '\0';
		char *rt1 = &serialized_data[ss1];

		switch(f1)
		{
			case 0:
			{
				
				break;
			}
		}

		if(e1) g1 = 0;
		else
		{
			serialized_data[ci1] = ???;
			++f1;
			ss1 = ci1 + 1;
		}
	}
	++ci1;
}
*/
