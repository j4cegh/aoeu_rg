#include "includes.h"
#ifdef USING_WINDOWS
#pragma once

#include <discord_rpc.h>

#define APPLICATION_ID "792274673906745344"

void discord_update_presence(char *status, char *state, char clear);
void discord_init();
void discord_done();
#endif