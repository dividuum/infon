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
#include <winerror.h>
#else
#include <dlfcn.h>
#endif

#include <assert.h>

#include "misc.h"
#include "global.h"
#include "renderer.h"

#include "client.h"
#include "client_game.h"
#include "client_player.h"
#include "client_creature.h"
#include "client_world.h"
#include "common_player.h"

static const renderer_api_t *renderer  = 0;
static int             is_open         = 0;
static int             render_shutdown = 0;

#ifdef WIN32
static HINSTANCE       dlhandle;
#else
static void           *dlhandle;
#endif

static void renderer_set_shutdown() {
    render_shutdown = 1;
}

static const infon_api_t infon_api = {
    .version            = GAME_NAME,

    .max_players        = MAXPLAYERS,
    .max_creatures      = MAXCREATURES,
    
    .each_creature      = client_creature_each,
    .each_player        = client_player_each,

    .get_creature       = client_creature_get,
    .get_player         = client_player_get,
    .get_king           = client_player_get_king,
    .get_world          = client_world_get,
    .get_world_info     = client_world_get_info,
    .get_world_tile     = client_world_get_tile,
    .get_intermission   = client_get_intermission,

    .get_traffic        = client_traffic,
    .shutdown           = renderer_set_shutdown,
};

void renderer_init_from_pointer(render_loader loader) {
    renderer = loader(&infon_api);

    if (renderer->version != RENDERER_API_VERSION)
        die("version mismatch between renderer and engine:\n"
            "renderer provided api version %d\n"
            "engine   required api version %d", 
            renderer->version, 
            RENDERER_API_VERSION);
}


void renderer_init_from_file(const char *shared) {
#ifdef WIN32
    dlhandle = LoadLibrary(shared);

    if (!dlhandle) 
        die("opening renderer failed: dlopen(\"%s\") failed: %d", shared, GetLastError());

    render_loader loader = (render_loader)GetProcAddress(dlhandle, TOSTRING(RENDERER_SYM));

    if (!loader) 
        die("cannot find symbol '" TOSTRING(RENDERER_SYM) "' in renderer %s", shared);
#else
    dlhandle = dlopen(shared, RTLD_NOW);

    if (!dlhandle)
        die("opening renderer failed: dlopen(\"%s\") failed: %s", shared, dlerror());

    render_loader loader = (render_loader)dlsym(dlhandle, TOSTRING(RENDERER_SYM));

    if (!loader)
        die("cannot find symbol '" TOSTRING(RENDERER_SYM) "' in renderer %s", shared);
#endif
    renderer_init_from_pointer(loader);
}

int renderer_open(int w, int h, int fs) {
    render_shutdown = 0;
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

void renderer_world_info_changed(const client_world_info_t *info) {
    if (renderer->world_info_changed)
        renderer->world_info_changed(info);
}

void renderer_world_changed(int x, int y) {
    if (renderer->world_changed)
        renderer->world_changed(x, y);
}

void renderer_player_joined(client_player_t *player) {
    if (renderer->player_joined) 
        player->userdata = renderer->player_joined(player);
}

void renderer_player_changed(const client_player_t *player, int changed) {
    if (renderer->player_changed) 
        renderer->player_changed(player, changed);
}

void renderer_player_left(const client_player_t *player) {
    if (renderer->player_left)
        renderer->player_left(player);
}

void renderer_creature_spawned(client_creature_t *creature) {
    if (renderer->creature_spawned)
        creature->userdata = renderer->creature_spawned(creature);
}

void renderer_creature_changed(const client_creature_t *creature, int changed) {
    if (renderer->creature_changed)
        renderer->creature_changed(creature, changed);
}

void renderer_creature_died(const client_creature_t *creature) {
    if (renderer->creature_died)
        renderer->creature_died(creature);
}

void renderer_scroll_message(const char *buf) {
    if (renderer->scroll_message)
        renderer->scroll_message(buf);
}

int renderer_wants_shutdown() {
    return render_shutdown;
}

void renderer_shutdown() {
    if (is_open) 
        renderer_close();
        
#ifdef WIN32
    if (dlhandle)
        FreeLibrary(dlhandle);
#else
    if (dlhandle)
        dlclose(dlhandle);
#endif
    renderer = NULL;
}
