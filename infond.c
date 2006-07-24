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

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <lualib.h>
#include <lauxlib.h>

#include "global.h"
#include "server.h"
#include "world.h"
#include "creature.h"
#include "scroller.h"

static int running = 1;
static int paused  = 0;

void sighandler(int sig) {
    running = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);

    srand(time(0));

    L = lua_open();

    lua_baselibopen(L);
    lua_tablibopen(L);
    lua_iolibopen(L);
    lua_strlibopen(L);
    lua_dblibopen(L);
    lua_mathlibopen(L);

    lua_dofile(L, "config.lua");
    lua_dofile(L, "server.lua");

    game_round = 0;
    game_time  = 0;

    world_init();
    server_init();
    player_init();
    creature_init();

    // Initialer Worldtick
    world_tick();

    Uint32 lastticks = SDL_GetTicks();
    while (running) {
        game_round++;
        Uint32 nowticks = SDL_GetTicks();
        Uint32 delta = nowticks - lastticks;

        if (nowticks < lastticks || nowticks > lastticks + 1000) {
            // Timewarp?
            lastticks = nowticks;
            SDL_Delay(5);
            continue;
        } else if (delta < 100) {
            SDL_Delay(5);
            continue;
        }

        lastticks = nowticks;

        // Zeit weiterlaufen lassen
        if (!paused) 
            game_time += delta;

        // IO Lesen/Schreiben
        server_tick();

        if (!paused) {
            // World Zeugs
            world_tick();

            // Spieler Programme ausfuehren
            player_think();

            // Viecher bewegen
            creature_moveall(delta);
        }
    }
    
    creature_shutdown();
    player_shutdown();
    server_shutdown();
    world_shutdown();

    lua_close(L);

    return EXIT_SUCCESS; 
}

