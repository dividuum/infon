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

#include <dlfcn.h>
#include <assert.h>

#include "misc.h"
#include "renderer.h"

#include "client.h"
#include "client_game.h"
#include "client_player.h"
#include "client_creature.h"
#include "client_world.h"
#include "common_player.h"

static renderer_api_t *renderer = 0;
static int             is_open  = 0;
static int             shutdown = 0;

static void           *dlhandle;

static void renderer_set_shutdown() {
    shutdown = 1;
}

static infon_api_t     infon = {
    .max_players        = MAXPLAYERS,
    .max_creatures      = MAXCREATURES,
    
    .each_creature      = client_each_creature,
    .each_player        = client_each_player,

    .get_creature       = client_get_creature,
    .get_player         = client_get_player,
    .get_king           = client_get_king,
    .get_world          = client_get_world,
    .get_world_info     = client_get_world_info,
    .get_intermission   = client_get_intermission,

    .get_traffic        = client_traffic,
    .shutdown           = renderer_set_shutdown,
};

void renderer_init_from_pointer(render_loader loader) {
    renderer = loader(&infon);

    if (renderer->version != RENDERER_API_VERSION)
        die("render is using api version %d, we provide %d", renderer->version, RENDERER_API_VERSION);
}


void renderer_init_from_file(const char *shared) {
    dlhandle = dlopen(shared, RTLD_NOW);

    if (!dlhandle) 
        die("dlopen(\"%s\") failed: %s", shared, dlerror());

    render_loader loader = (render_loader)dlsym(dlhandle, "load");

    if (!loader) 
        die("cannot find symbol 'load'");

    renderer_init_from_pointer(loader);
}

int renderer_open(int w, int h, int fs) {
    shutdown = 0;
    int ret = renderer->open(w, h, fs);
    if (ret) is_open = 1;
    return ret;
}

void renderer_close() {
    assert(is_open);
    renderer->close();
    is_open = 0;
}

void renderer_tick(int game_time, int delta) {
    renderer->tick(game_time, delta);
}

void renderer_world_change() {
    renderer->world_change();
}

void renderer_player_color_change(const client_player_t *player) {
    renderer->player_color_change(player);
}

void renderer_scroll_message(const char *buf) {
    renderer->scroll_message(buf);
}

int renderer_wants_shutdown() {
    return shutdown;
}

void renderer_shutdown() {
    if (is_open) 
        renderer_close();
        
    if (dlhandle)
        dlclose(dlhandle);
    renderer = NULL;
}
