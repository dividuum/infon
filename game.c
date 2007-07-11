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

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <lauxlib.h>

#include "scroller.h"
#include "world.h"
#include "global.h"
#include "player.h"
#include "creature.h"
#include "misc.h"

static int            should_end_game = 0;
static struct timeval game_start;
static time_t         server_start;
static char           intermission[256] = {0};
static int            realtime = 1;

static int get_tick(const struct timeval *ref) {
    struct timeval now;
    gettimeofday(&now , NULL);
    return (now.tv_sec - ref->tv_sec) * 1000 + (now.tv_usec - ref->tv_usec) / 1000;
}

void game_send_info(client_t *client) {
    packet_t packet;
    packet_init(&packet, PACKET_GAME_INFO);
    packet_write32(&packet, game_time);
    server_send_packet(&packet, client);
}

void game_send_round_info(client_t *client, int delta) {
    packet_t packet;
    packet_init(&packet, PACKET_ROUND);
    packet_write08(&packet, delta);
    server_send_packet(&packet, client);
}

void game_send_intermission(client_t *client) {
    packet_t packet;
    packet_init(&packet, PACKET_INTERMISSION);
    packet_writeXX(&packet, intermission, strlen(intermission));
    server_send_packet(&packet, client);
}

void game_send_initial_update(client_t *client) {
    game_send_info(client);
    game_send_intermission(client);
}

void game_call_rule_handler(const char *name, int params) {
    lua_pushliteral(L, "rules_call");   // arg* 'rules_call'
    lua_rawget(L, LUA_GLOBALSINDEX);    // arg* rules_call
    lua_insert(L, -1 - params);         // rules_call arg* 
    lua_pushstring(L, name);            // rules_call arg* name
    lua_insert(L, -1 - params);         // rules_call name arg* 
    if (lua_pcall(L, 1 + params, 0, 0) != 0) {
        fprintf(stderr, "error calling rules_call: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static int luaGameEnd(lua_State *L) {
    should_end_game = 1;
    return 0;
}

static int luaGameTime(lua_State *L) {
    lua_pushnumber(L, game_time);
    return 1;
}

static int luaRealTime(lua_State *L) {
    lua_pushnumber(L, real_time);
    return 1;
}

static int luaScrollerAdd(lua_State *L) {
    add_to_scroller(luaL_checkstring(L, 1));
    return 0;
}

static int luaShutdown(lua_State *L) {
    game_exit = 1;
    return 0;
}

static int luaSetRealtime(lua_State *L) {
    realtime = lua_toboolean(L, 1);
    return 0;
}

static int luaGetRealtime(lua_State *L) {
    lua_pushboolean(L, realtime);
    return 1;
}

static int luaSetPaused(lua_State *L) {
    game_paused = lua_toboolean(L, 1);
    return 0;
}

static int luaGameIntermission(lua_State *L) {
    snprintf(intermission, sizeof(intermission), "%s", luaL_checkstring(L, 1));
    game_send_intermission(SEND_BROADCAST);
    return 0;
}

static int luaHexDecode(lua_State *L) {
    size_t left; const char *ptr = luaL_checklstring(L, 1, &left);
    luaL_Buffer out;
    luaL_buffinit(L, &out);
    int    hi   = 1;
    char   cur  = 0;
    while (left > 0) {
        int val = -1;
        switch (*ptr) {
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                val = *ptr - '0'; 
                break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                val = *ptr - 'a' + 10; 
                break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                val = *ptr - 'A' + 10; 
                break;
        }
        ptr++; left--;
        if (val < 0)
            continue;
        if (hi) {
            cur = val << 4;
        } else {
            luaL_addchar(&out, cur | val);
        }
        hi ^= 1;
    }
    luaL_pushresult(&out);
    return 1;
}

void game_init() {
    server_start = time(NULL);

    lua_register(L, "game_end",         luaGameEnd);
    lua_register(L, "game_time",        luaGameTime);
    lua_register(L, "real_time",        luaRealTime);

    lua_register(L, "set_intermission", luaGameIntermission);

    lua_register(L, "scroller_add",     luaScrollerAdd);
    
    lua_register(L, "hex_decode",       luaHexDecode);
    lua_register(L, "set_realtime",     luaSetRealtime);
    lua_register(L, "get_realtime",     luaGetRealtime);
    lua_register(L, "set_paused",       luaSetPaused);
    lua_register(L, "shutdown",         luaShutdown);
    lua_register(L, "pure_game",        luaCreatureConfigChanged);

    lua_register_constant       (L, MAXPLAYERS);
    lua_register_string_constant(L, GAME_NAME);
}

void game_one_game() {
    should_end_game = 0;
    game_time       = 0;
    
    // Zeitinfo an Client
    game_send_info(SEND_BROADCAST);

    // Regeln laden
    lua_pushliteral(L, "rules_init");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling rules_init: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Karte laden
    world_init();

    // Alle Viecher tot. vm_id = 0
    creature_init();

    // Initialer Worldtick. ruft level_init und ersten level_tick auf.
    world_tick();

    // Beginn der Zeitrechnung...
    int lasttick = 0;
    gettimeofday(&game_start, NULL);

    // onPlayerCreated Event an Rules, PLAYER_CREATED an VM
    player_game_start();

    // Spiel gestartet
    lua_pushliteral(L, "on_game_started");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling on_game_started: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    } 

    // Rule Handler
    game_call_rule_handler("onNewGame", 0);

    while (!game_exit && !should_end_game) {
        int tick = get_tick(&game_start);
        int delta;
        if (realtime) {
            delta = tick - lasttick;
            if (delta < 0 || delta > 200) {
                // Timewarp?
                lasttick = tick;
                continue;
            } else if (delta < 100) {
#ifdef WIN32
                Sleep(max(95 - delta, 10));
#else
                usleep(max(95000 - delta * 1000, 1000));
#endif
                continue;
            }
        } else {
            delta = 100;
        }
        lasttick = tick;

        // Realtime update
        real_time = time(NULL) - server_start; 
        
        // GC
        lua_gc(L, LUA_GCSTEP, 1);

        if (!game_paused) {
            // Runde starten
            game_send_round_info(SEND_BROADCAST, delta);

            // Rule Handler
            game_call_rule_handler("onRound", 0);

            // World Zeugs
            world_tick();
        }

        player_round();

        if (!game_paused) {
            // Spielerprogramme ausfuehren
            player_think();
        }

        player_sync();

        if (!game_paused) {
            // Viecher bewegen
            creature_moveall(delta);

            // Zeit weiterlaufen lassen
            game_time += delta;
        }
        
        // IO Lesen/Schreiben
        server_tick();
    }

    // Spiel beendet
    lua_pushliteral(L, "on_game_ended");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling on_game_ended: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    } 

    server_game_end();
    
    creature_shutdown();
    world_shutdown();
}
