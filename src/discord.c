#include "discord.h"
#include "rg.h"
#ifdef USING_WINDOWS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <discord_rpc.h>


void discord_update_presence(char *status, char *state, char clear)
{
	if(clear) Discord_ClearPresence();
	else
	{
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));
		discordPresence.state = state;
		discordPresence.details = status;
		discordPresence.startTimestamp = 0;
		discordPresence.endTimestamp = 0;
		discordPresence.largeImageKey = "canary-large";
		discordPresence.smallImageKey = "ptb-small";
		discordPresence.partyId = "";
		discordPresence.partySize = 0;
		discordPresence.partyMax = 0;
		discordPresence.matchSecret = "";
		discordPresence.joinSecret = "";
		discordPresence.spectateSecret = "";
		discordPresence.instance = 0;
		Discord_UpdatePresence(&discordPresence);
	}
}

void handleDiscordReady(const DiscordUser* connectedUser)
{
    show_notif_fmt("\nDiscord: connected to user %s#%s - %s\n",
           connectedUser->username,
           connectedUser->discriminator,
           connectedUser->userId);
}

void handleDiscordDisconnected(int errcode, const char* message)
{
    show_notif_fmt("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

void handleDiscordError(int errcode, const char* message)
{
	show_notif_fmt("\nDiscord: error (%d: %s)\n", errcode, message);
}

void handleDiscordJoin(const char* secret)
{
	show_notif_fmt("\nDiscord: join (%s)\n", secret);
}

void handleDiscordSpectate(const char* secret)
{
	show_notif_fmt("\nDiscord: spectate (%s)\n", secret);
}

void handleDiscordJoinRequest(const DiscordUser* request)
{
    int response = -1;
    char yn[4];
    show_notif_fmt("\nDiscord: join request from %s#%s - %s\n",
           request->username,
           request->discriminator,
           request->userId);
    /*
    do {
        printf("Accept? (y/n)");
        if (!prompt(yn, sizeof(yn))) {
            break;
        }

        if (!yn[0]) {
            continue;
        }

        if (yn[0] == 'y') {
            response = DISCORD_REPLY_YES;
            break;
        }

        if (yn[0] == 'n') {
            response = DISCORD_REPLY_NO;
            break;
        }
    } while (1);
    if (response != -1) {
        Discord_Respond(request->userId, response);
    }
    */
}

void discord_init()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void discord_done()
{
	Discord_Shutdown();
}

/*
void gameLoop()
{
    char line[512];
    char* space;

    StartTime = time(0);

    printf("You are standing in an open field west of a white house.\n");
    while (prompt(line, sizeof(line))) {
        if (line[0]) {
            if (line[0] == 'q') {
                break;
            }

            if (line[0] == 't') {
                printf("Shutting off Discord.\n");
                Discord_Shutdown();
                continue;
            }

            if (line[0] == 'c') {
                if (SendPresence) {
                    printf("Clearing presence information.\n");
                    SendPresence = 0;
                }
                else {
                    printf("Restoring presence information.\n");
                    SendPresence = 1;
                }
                updateDiscordPresence();
                continue;
            }

            if (line[0] == 'y') {
                printf("Reinit Discord.\n");
                discordInit();
                continue;
            }

            if (time(NULL) & 1) {
                printf("I don't understand that.\n");
            }
            else {
                space = strchr(line, ' ');
                if (space) {
                    *space = 0;
                }
                printf("I don't know the word \"%s\".\n", line);
            }

            ++FrustrationLevel;

            updateDiscordPresence();
        }

#ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
#endif
        Discord_RunCallbacks();
    }
}
*/

/*
int discordmain(int argc, char* argv[])
{
    discordInit();

    gameLoop();

    Discord_Shutdown();
    return 0;
}
*/
#endif