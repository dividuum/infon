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

#ifndef WIN32
static void sighandler(int sig) {
    signal_received = 1;
}
#endif

static void info() {
    fprintf(stdout, "%s built %s %s\n", 
            GAME_NAME, __TIME__, __DATE__);
#ifdef NO_EXTERNAL_RENDERER
    fprintf(stdout, "Cannot load external renderers");
#else
    fprintf(stdout, "External renderers supported");
#endif
#ifdef BUILTIN_RENDERER
    fprintf(stdout, ", builtin renderer '%s'", TOSTRING(BUILTIN_RENDERER));
#endif
#ifdef DEFAULT_RENDERER
    fprintf(stdout, ", default renderer '%s'", TOSTRING(DEFAULT_RENDERER));
#endif
    fprintf(stdout, "\n");
}

int main(int argc, char *argv[]) {
    char *host       = NULL;
    char *renderer   = NULL;
    int   width      = 800;
    int   height     = 600;
    int   fullscreen = 0;

#ifdef EVENT_HOST
    host = EVENT_HOST;
#endif

#ifdef DEFAULT_RENDERER
    renderer = TOSTRING(DEFAULT_RENDERER);
#endif

    // GUI Environment setzt default Renderer um.
    if (getenv("GUI"))
        renderer = getenv("GUI");

#ifdef WIN32
    char *sep = strrchr(argv[0], '\\');
    if (sep) { *sep = '\0'; chdir(argv[0]); }

    // Spezialfaelle fuer Windows Screensaver Aufrufe
    if (argc == 2 && stricmp(argv[1], "/s") == 0) {
        host  = "infon.dividuum.de";
        width = 1024, height = 768, fullscreen = 1;
        goto screen_saver_start;
    } else if (argc == 3 && stricmp(argv[1], "/p") == 0) {
        exit(EXIT_SUCCESS);
    } else if (argc == 2 && strstr(argv[1], "/c:") == argv[1]) {
        die("There are no settings");
    }
#endif

    // Keine Fehler auf stderr
    opterr = 0;

    int opt; 
    while ((opt = getopt(argc, argv, ":fvx:y:r:h")) != -1) {
        switch (opt) {
            case '?': die("you specified an unknown option -%c.", optopt);
            case ':': die("missing argument to option -%c.", optopt);
            case 'r': renderer   = optarg;          break;
            case 'f': fullscreen = 1;               break;
            case 'x': width      = atoi(optarg);    break;
            case 'y': height     = atoi(optarg);    break;
            case 'h': die("usage: %s [-r <renderer>] [-f] [-x <width>] [-y <height>] [-v] [-h] <server[:port]>\n"
                          "\n"
                          " -r <renderer>    - renderer to use (sdl_gui, gl_gui, ...)\n"
                          " -x <width>       - initial screen width.\n"
                          " -y <height>      - initial screen height.\n"
                          " -f               - start in fullscreen mode.\n"
                          " -v               - display version information.\n"
                          " -h               - this help.\n"
                          "\n"
                          "<server[:port]>   - ip/hostname of an infon game server.\n"
                          "                    if no port is given, 1234 is used.\n", argv[0]);
            case 'v': info(); exit(EXIT_SUCCESS);
        }
    }

    switch (argc - optind) {
        case 0:  break;
        case 1:  host = argv[optind]; break;
        default: die("you specified more than one game server hostname");
    }

    if (!renderer)
        die("no renderer specified. use the '-r <renderer>' option");

#ifdef WIN32
    if (!host) {
        if (yesno("You did not specify a game server.\nConnect to 'infon.dividuum.de:1234'?"))
            host = "infon.dividuum.de";
        else
            die("You must supply the game servers hostname\n"
                "as a command line parameter.\n\n"
                "Example: 'infon.exe infon.dividuum.de'\n\n"
                "Visit http://infon.dividuum.de/ for help.");
    } 
#else
    if (!host)
        die("usage: %s [options] <server[:port]>\n"
            "see %s -h for a full list of options", argv[0], argv[0]);
#endif


#ifndef WIN32
    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);
    signal(SIGPIPE, SIG_IGN);
#else
screen_saver_start:
#endif

    srand(time(NULL));
    gettimeofday(&start, NULL);

    if (!renderer_init(renderer)) 
        die("cannot initialize the renderer '%s'", renderer);

    if (!renderer_open(width, height, fullscreen))
        die("cannot start the renderer '%s'. sorry", renderer);

    client_init(host);
    client_game_init();

    int lastticks = get_tick();
    while (!signal_received && !renderer_wants_shutdown() && client_is_connected()) {
        int nowticks = get_tick();
        int delta = nowticks - lastticks;

        if (nowticks < lastticks || nowticks > lastticks + 1000) {
            // Timewarp?
            lastticks = nowticks;
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
