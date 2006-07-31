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

#include "gui_creature.h"
#include "gui_world.h"
#include "gui_scroller.h"
#include "gui_player.h"

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
        die("<ip:port>");

    const int width = 800, height = 600;

#ifndef WIN32
    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);
#endif

    srand(time(0));

    client_init(argv[1]);

    video_init(width, height);
    sprite_init();
    gui_scroller_init();
    gui_world_init();
    gui_player_init();
    gui_creature_init();

    game_round = 0;
    game_time  = 0;

    Uint32 lastticks = SDL_GetTicks();
    while (running && client_is_connected()) {
        game_round++;
        Uint32 nowticks = SDL_GetTicks();
        Uint32 delta = nowticks - lastticks;

        if (nowticks < lastticks || nowticks > lastticks + 1000) {
            // Timewarp?
            lastticks = nowticks;
            SDL_Delay(5);
            continue;
        } else if (delta < 20) {
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
                        case SDLK_1: video_resize( 640,  480); break;
                        case SDLK_2: video_resize( 800,  600); break;
                        case SDLK_3: video_resize(1024,  768); break;
                        case SDLK_4: video_resize(1280, 1024); break;
                        case SDLK_5: video_resize(1600, 1200); break;
                        case SDLK_c:
                            gui_world_center();
                            break;
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        default: ;
                    }
                    break;
               case SDL_MOUSEMOTION: 
                    if (event.motion.state & 1)
                    gui_world_center_change(-event.motion.xrel, -event.motion.yrel);
                    break;
               case SDL_VIDEORESIZE:
                    video_resize(event.resize.w, event.resize.h);
                    break;
               case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        game_time += delta;

        print_fps();

        // IO Lesen/Schreiben
        client_tick();
        gui_creature_move(delta);

        // Anzeigen
        gui_world_draw();
        gui_creature_draw();
        gui_scroller_draw();
        gui_player_draw();

        video_flip();
    }
    
    gui_creature_shutdown();
    gui_player_shutdown();
    gui_world_shutdown();
    gui_scroller_shutdown();

    sprite_shutdown();
    video_shutdown();
    client_shutdown();

    return EXIT_SUCCESS; 
}

#ifdef WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
                           LPSTR lpszArgument,      int nFunsterStil)
{
    if (strlen(lpszArgument) == 0) {
        die("you must supply the gameserver hostname\n"
            "as command line parameter.\n\n"
            "example: 'infon.exe bl0rg.net'");
    }
    char *argv[] = { "foobar", lpszArgument, NULL };
    return main(2, argv);
}
#endif


