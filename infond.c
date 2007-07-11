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

#ifdef DAEMONIZING
#include "daemonize.h"
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

int real_time   = 0; // Seconds since server start
int game_time   = 0; // Microseconds since game start

int game_paused = 0;
int game_exit   = 0;

void sighandler(int sig) {
    game_exit = 1;
}

int main(int argc, char *argv[]) {
#if defined(__linux__) && defined(WARN_COREDUMP)
    struct rlimit rlim;
    getrlimit(RLIMIT_CORE, &rlim);
    if (rlim.rlim_cur == 0) 
        fprintf(stderr, "no coredump possible. enable them to help debug crashes.\n");
#endif

    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);
#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    srand(time(NULL));

    L = luaL_newstate();
    luaL_openlibs(L);
    
    lua_register_string_constant(L, PREFIX);
    lua_register_string_constant(L, PLATFORM);

    game_init();
    server_init();
    player_init();
    
    if (luaL_loadfile(L, PREFIX "infond.lua") != 0) 
        die("startup failed: %s", lua_tostring(L, -1));

    if (!lua_checkstack(L, argc)) 
        die("too many arguments?");

    for (int i = 1; i < argc; i++) 
        lua_pushstring(L, argv[i]);

    if (lua_pcall(L, argc - 1, 0, 0) != 0)
        die("startup failed: %s", lua_tostring(L, -1));
    
    // XXX: HACK: stdin client starten
#ifndef NO_CONSOLE_CLIENT    
    server_accept(STDIN_FILENO, "special:console"); 
#endif

#ifdef DAEMONIZING
    daemonize(argc, argv);
#endif

    while (!game_exit) {
        game_one_game();
    }

    player_shutdown();
    server_shutdown();

    lua_close(L);

    return EXIT_SUCCESS; 
}

