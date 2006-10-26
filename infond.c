/*
   
    Copyright (c) 2006 Florian Wesch <fw@dividuum.de>. All Rights Reserved.
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <lualib.h>
#include <lauxlib.h>

#include "global.h"
#include "misc.h"
#include "server.h"
#include "world.h"
#include "game.h"
#include "player.h"

lua_State *L;

int game_time   = 0;
int game_paused = 0;
int game_exit   = 0;

void sighandler(int sig) {
    game_exit = 1;
}

int main(int argc, char *argv[]) {
#ifdef __linux__ 
    struct rlimit rlim;
    getrlimit(RLIMIT_CORE, &rlim);
    if (rlim.rlim_cur == 0) 
        fprintf(stderr, "no coredump possible. enable them to help debug crashes.\n");
#endif

    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);

    srand(time(0));

    L = luaL_newstate();

    luaL_openlibs(L);
    luaL_dofile(L, "infond.lua");

    game_init();
    server_init();
    player_init();

    while (!game_exit) {
        game_one_round();
    }

    player_shutdown();
    server_shutdown();

    lua_close(L);

    return EXIT_SUCCESS; 
}

