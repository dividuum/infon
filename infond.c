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
#include "video.h"
#include "sprite.h"
#include "creature.h"
#include "scroller.h"

static int running = 1;

void sighandler(int sig) {
    running = 0;
}

void print_fps() {
    static Uint32 frames = 0;
    static Uint32 ticks  = 0;
    frames++;
    if(SDL_GetTicks() >= ticks + 1000) {
        static int iteration = 0;
        if (++iteration % 35 == 0) {
            static char buf[16];
            snprintf(buf, sizeof(buf), "%d fps", frames);
            add_to_scroller(buf);
        }
        frames = 0;
        ticks = SDL_GetTicks();
    }
}   

int main(int argc, char *argv[]) {
    // const int width = 320, height = 208;
    const int width = 640, height = 480;
    // const int width = 800, height = 600;
    // const int width = 1024, height = 768;
    // const int width = 1280, height = 1024;

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

    video_init(width, height);
    sprite_init();
    scroller_init();
    world_init(width / SPRITE_TILE_SIZE, height / SPRITE_TILE_SIZE - 2);
    server_init();
    player_init();
    creature_init();

    game_round = 0;
    game_time  = 0;

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
        } else if (delta < 10) {
            SDL_Delay(5);
            continue;
        }

        lastticks = nowticks;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_RETURN:
                            if (event.key.keysym.mod & KMOD_ALT)
                                video_fullscreen_toggle();
                            break;
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_1: world_set_display_mode(0); break;
                        case SDLK_2: world_set_display_mode(1); break;
                        case SDLK_3: world_set_display_mode(2); break;
                        default: ;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (event.motion.state == 1) {
                        world_dig(event.motion.x / SPRITE_TILE_SIZE, event.motion.y / SPRITE_TILE_SIZE, SOLID);
                        world_dig(event.motion.x / SPRITE_TILE_SIZE + 1, event.motion.y / SPRITE_TILE_SIZE, SOLID);
                        world_dig(event.motion.x / SPRITE_TILE_SIZE, event.motion.y / SPRITE_TILE_SIZE + 1, SOLID);
                        world_dig(event.motion.x / SPRITE_TILE_SIZE + 1, event.motion.y / SPRITE_TILE_SIZE + 1, SOLID);
                        printf("%d %d %d\n", 
                               event.motion.x * TILE_SCALE / SPRITE_TILE_SIZE,
                               event.motion.y * TILE_SCALE / SPRITE_TILE_SIZE, 
                                world_get_food(event.motion.x / SPRITE_TILE_SIZE,
                                               event.motion.y / SPRITE_TILE_SIZE));

                    }
                    break;
                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        print_fps();

        // Zeit weiterlaufen lassen
        game_time += delta;

        // IO Lesen/Schreiben
        server_tick();

        // Spieler Programme ausfuehren
        player_think();

        // Viecher bewegen
        creature_moveall(delta);

        // Anzeigen
        world_draw();
        creature_draw();
        scroller_draw();
        player_draw(delta);

        video_flip();

        // GUI Client aktualisieren 
        // TODO: implement me
    }
    
    creature_shutdown();
    player_shutdown();
    server_shutdown();
    world_shutdown();
    scroller_shutdown();
    sprite_shutdown();
    video_shutdown();

    lua_close(L);

    return EXIT_SUCCESS; 
}

