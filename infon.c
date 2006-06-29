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

#include "global.h"
#include "client.h"
#include "video.h"
#include "sprite.h"
#include "misc.h"

//#include "gui_creature.h"
#include "gui_world.h"
#include "gui_scroller.h"
//#include "gui_player.h"

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
            printf("%d fps\n", frames);
        }
        frames = 0;
        ticks = SDL_GetTicks();
    }
}   

int main(int argc, char *argv[]) {
    if (argc != 2)
        die("%s <ip:port>");

    // const int width = 320, height = 208;
    const int width = 640, height = 480;
    // const int width = 800, height = 600;
    // const int width = 1024, height = 768;
    // const int width = 1280, height = 1024;

    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);

    srand(time(0));

    client_init(argv[1]);

    video_init(width, height);
    sprite_init();
    gui_scroller_init();
    gui_world_init(width / SPRITE_TILE_SIZE, height / SPRITE_TILE_SIZE - 2);
    //gui_player_init();
    //gui_creature_init();

    game_round = 0;
    game_time  = 0;

    while (running && client_is_connected()) {
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
                        default: ;
                    }
                    break;
                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        print_fps();

        // IO Lesen/Schreiben
        client_tick();

        // Anzeigen
        gui_world_draw();
        creature_draw();
        gui_scroller_draw();
        player_draw();

        video_flip();
    }
    
    //gui_creature_shutdown();
    //gui_player_shutdown();
    gui_world_shutdown();
    gui_scroller_shutdown();

    sprite_shutdown();
    video_shutdown();
    client_shutdown();

    return EXIT_SUCCESS; 
}

