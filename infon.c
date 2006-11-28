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

#ifdef WIN32
#include <windows.h>
#endif

#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "global.h"
#include "client.h"
#include "misc.h"
#include "renderer.h"
#include "client_game.h"

int        game_time       = 0;
static int signal_received = 0;
static struct timeval start;

static int get_tick() {
    static struct timeval now;
    gettimeofday(&now , NULL);
    return (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000;
}

void sighandler(int sig) {
    signal_received = 1;
}

int main(int argc, char *argv[]) {
    int width = 800, height = 600, fullscreen = 0;
    char *host = NULL;
#ifdef EVENT_HOST
    host = EVENT_HOST;
#else
#ifdef WIN32
    char *sep = strrchr(argv[0], '\\');
    if (sep) { *sep = '\0'; chdir(argv[0]); }

    if (argc == 2 && stricmp(argv[1], "/s") == 0) {
        host  = "infon.dividuum.de";
        width = 1024, height = 768, fullscreen = 1;
    } else if (argc == 3 && stricmp(argv[1], "/p") == 0) {
        exit(0);
    } else if (argc == 2 && strstr(argv[1], "/c:") == argv[1]) {
        die("There are no settings");
    } else if (argc != 2) {
        if (yesno("You did not specify a game server.\nConnect to 'infon.dividuum.de:1234'?"))
            host = "infon.dividuum.de";
        else
            die("You must supply the game servers hostname\n"
                "as first command line parameter.\n\n"
                "Example: 'infon.exe infon.dividuum.de'\n\n"
                "Visit http://infon.dividuum.de/ for help.");
    } else {
        host = argv[1];
    }
#else
    if (argc != 2)
        die("usage: %s <serverip[:port]>", argv[0]);
    host = argv[1];
#endif
#endif

#ifndef WIN32
    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);
#endif

    srand(time(0));
    gettimeofday(&start, NULL);

    const char *gui = "sdl_gui";
    if (getenv("GUI"))
        gui = getenv("GUI");

    renderer_init(gui);
    if (!renderer_open(width, height, fullscreen))
        die("cannot open renderer. sorry");

    client_init(host, NULL);
    client_game_init();

    int lastticks = get_tick();
    while (!signal_received && !renderer_wants_shutdown() && client_is_connected()) {
        int nowticks = get_tick();
        int delta = nowticks - lastticks;

        if (nowticks < lastticks || nowticks > lastticks + 1000) {
            // Timewarp?
            lastticks = nowticks;
#ifdef WIN32
            Sleep(2);
#else
            usleep(5000);
#endif
            continue;
        } else if (delta < 20) {
#ifdef WIN32
            Sleep(2);
#else
            usleep(5000);
#endif
            continue;
        }
        lastticks = nowticks;

        // IO Lesen/Schreiben
        client_tick(delta);
        client_creature_move(delta);
        renderer_tick(game_time, delta);

        game_time += delta;
    }

    client_game_shutdown();
    client_shutdown();

    renderer_close();
    renderer_shutdown();
    return EXIT_SUCCESS; 
}
