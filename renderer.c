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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "misc.h"
#include "global.h"
#include "renderer.h"

#include "client.h"
#include "client_game.h"
#include "client_player.h"
#include "client_creature.h"
#include "client_world.h"
#include "common_player.h"

#if defined(NO_EXTERNAL_RENDERER) && !defined(BUILTIN_RENDERER) 
#error "this build can neither load external renderers nor contains an internal renderer."
#endif

#ifndef NO_EXTERNAL_RENDERER
#ifdef WIN32
#include <windows.h>
#include <winerror.h>
static HINSTANCE dlhandle;
#else
#include <dlfcn.h>
static void     *dlhandle;
#endif
#endif

static const renderer_api_t *renderer  = 0;
static int             is_open         = 0;
static int             render_shutdown = 0;

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
    .printf             = client_printf,
    .is_demo            = client_is_file_source,
};

int renderer_init_from_pointer(render_loader loader) {
    renderer = loader(&infon_api);

    if (renderer->version == RENDERER_API_VERSION) 
        return 1;

    fprintf(stderr, "version mismatch between renderer and engine: renderer api %d != engine api %d\n", 
                    renderer->version, RENDERER_API_VERSION);
    return 0;
}

#ifndef NO_EXTERNAL_RENDERER

void renderer_close_file() {
#ifdef WIN32
    if (dlhandle)
        FreeLibrary(dlhandle);
    dlhandle = 0;
#else
    if (dlhandle)
        dlclose(dlhandle);
    dlhandle = NULL;
#endif
}

int renderer_open_file(const char *shared) {
    fprintf(stderr, "loading renderer plugin '%s'\n * ", shared);
#ifdef WIN32
    dlhandle = LoadLibrary(shared);

    if (!dlhandle) {
        fprintf(stderr, "LoadLibrary failed: %s\n", ErrorString(GetLastError()));
        goto failed;
    }

    render_loader loader = (render_loader)GetProcAddress(dlhandle, TOSTRING(RENDERER_SYM));
#else
    dlhandle = dlopen(shared, RTLD_NOW);

    if (!dlhandle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        goto failed;
    }

    render_loader loader = (render_loader)dlsym(dlhandle, TOSTRING(RENDERER_SYM));
#endif
    if (!loader) {
        fprintf(stderr, "cannot find symbol '" TOSTRING(RENDERER_SYM) "'\n");
        goto failed;
    }

    if (!renderer_init_from_pointer(loader)) 
        goto failed;

    fprintf(stderr, "ok (renderer struct @ %p)\n", (void*)renderer);
    return 1;

failed:
    renderer_close_file();
    return 0;
}

int renderer_search_and_load(const char *name) {
    char buf[4096];
    
#ifdef WIN32
    const char *ext = "dll";
#else
    const char *ext = "so";
#endif
    
#define renderer_try(fmt, ...)                          \
    do {                                                \
        snprintf(buf, sizeof(buf), fmt, __VA_ARGS__);   \
        if (renderer_open_file(buf)) return 1;          \
    } while (0)

    renderer_try("%s", name);

#ifdef WIN32
    renderer_try(".\\%s.%s", name, ext);

    if (getenv("USERPROFILE")) 
        renderer_try("%s\\infon\\%s.%s", getenv("USERPROFILE"), name, ext);
#else
    renderer_try("./%s.%s",  name, ext);
    
    if (getenv("INFON_PATH")) 
        renderer_try("%s/%s.%s", getenv("INFON_PATH"), name, ext);
    
    if (getenv("HOME"))        
        renderer_try("%s/.infon/%s.%s", getenv("HOME"), name, ext);
#endif
    
#ifdef RENDERER_PATH
    renderer_try("%s/%s.%s", RENDERER_PATH, name, ext);
#endif
    renderer_try("%s%s", PREFIX, name);
    return 0;
}

#endif

int renderer_init(const char *name) {
#ifndef NO_EXTERNAL_RENDERER
    if (renderer_search_and_load(name))
        return 1;
#endif

#ifdef BUILTIN_RENDERER
    if (!strcmp(name, TOSTRING(BUILTIN_RENDERER))) {
        fprintf(stderr, "using builtin renderer '%s'\n", TOSTRING(BUILTIN_RENDERER));
        renderer_init_from_pointer(RENDERER_SYM);
        return 1;
    }
#endif

    return 0;
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
#ifndef NO_EXTERNAL_RENDERER
    renderer_close_file();
#endif
    renderer = NULL;
}
