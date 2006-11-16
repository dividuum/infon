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

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <lauxlib.h>
#include <lualib.h>

#include "global.h"
#include "player.h"
#include "server.h"
#include "creature.h"
#include "world.h"
#include "misc.h"
#include "rules.h"
#include "scroller.h"

#define PLAYER_USED(player) (!!((player)->L))

static player_t players[MAXPLAYERS];
static player_t *king_player = NULL;

void player_score(player_t *player, int scoredelta, const char *reason) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s %s %d point%s: %s",
                               player->name, 
                               scoredelta > 0 ? "gained" : "lost",
                               abs(scoredelta),
                               abs(scoredelta) == 1 ? "" : "s",
                               reason);
                               
    add_to_scroller(buf);

    int oldscore = player->score;

    player->score += scoredelta;

    if (player->score < PLAYER_KICK_SCORE)
        player->score = PLAYER_KICK_SCORE;

    if (player->score > 9999)
        player->score = 9999;

    if (player->score != oldscore)
        player->dirtymask |= PLAYER_DIRTY_SCORE;

    // Rule Handler aufrufen
    lua_pushliteral(L, "onPlayerScoreChange");
    lua_rawget(L, LUA_GLOBALSINDEX);         
    lua_pushnumber(L, player_num(player));
    lua_pushnumber(L, player->score);
    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling onPlayerScoreChange: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    printf("stat: %10d %3d '%10s' %5d: %s\n", game_time, player_num(player), player->name, player->score, reason);
}

void player_init_events(player_t *player) {
    lua_pushliteral(player->L, "events");
    lua_newtable(player->L);
    lua_settable(player->L, LUA_REGISTRYINDEX);
}

// Erwartet Name an -2 und Wert an -1
void player_add_event(player_t *player, creature_event event) {
    lua_pushliteral(player->L, "events");
    lua_rawget(player->L, LUA_REGISTRYINDEX); // name val _events        
    assert(!lua_isnil(player->L, -1));
    lua_createtable(player->L, 3, 0);         // name val _events t
    lua_pushnumber(player->L, event);         // name val _events t e
    lua_rawseti(player->L, -2, 1);            // name val _events t
    lua_insert(player->L, -3);                // name t val _events 
    lua_insert(player->L, -4);                // _events name t val
    lua_rawseti(player->L, -2, 3);            // _events name t
    lua_insert(player->L, -2);                // _events t name
    lua_rawseti(player->L, -2, 2);            // _events t
    size_t size = lua_objlen(player->L, -2);
    lua_rawseti(player->L, -2, size + 1);     // _events
    lua_pop(player->L, 1);                    //
}

void player_on_creature_spawned(player_t *player, creature_t *creature, creature_t *parent) {
    lua_pushnumber(player->L, creature_num(creature));
    lua_pushnumber(player->L, parent ? creature_num(parent) : -1);
    player_add_event(player, CREATURE_SPAWNED);

    player->num_creatures++;
        
    // Rule Handler aufrufen
    lua_pushliteral(L, "onCreatureSpawned");
    lua_rawget(L, LUA_GLOBALSINDEX);         
    lua_pushnumber(L, creature_num(creature));
    if (parent)
        lua_pushnumber(L, creature_num(parent));
    else
        lua_pushnil(L);
    
    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling onCreaturSpawned: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void player_on_creature_killed(player_t *player, creature_t *victim, creature_t *killer) {
    lua_pushnumber(player->L, creature_num(victim));
    lua_pushnumber(player->L, killer ? creature_num(killer) : -1);
    player_add_event(player, CREATURE_KILLED);

    player->num_creatures--;

    assert(player->num_creatures >= 0);

    if (player->num_creatures == 0)
        player->all_dead_time = game_time;

    // Rule Handler aufrufen
    lua_pushliteral(L, "onCreatureKilled");
    lua_rawget(L, LUA_GLOBALSINDEX);         
    lua_pushnumber(L, creature_num(victim));
    if (killer)
        lua_pushnumber(L, creature_num(killer));
    else
        lua_pushnil(L);
    
    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling onCreatureKilled: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void player_on_creature_attacked(player_t *player, creature_t *victim, creature_t *attacker) {
    lua_pushnumber(player->L, creature_num(victim));
    lua_pushnumber(player->L, creature_num(attacker));
    player_add_event(player, CREATURE_ATTACKED);
}

void player_on_all_dead(player_t *player) {
    // Rule Handler aufrufen
    lua_pushliteral(L, "onPlayerAllCreaturesDied");
    lua_rawget(L, LUA_GLOBALSINDEX);         
    lua_pushnumber(L, player_num(player));
    if (lua_pcall(L, 1, 0, 0) != 0) {
        fprintf(stderr, "error calling onPlayerAllCreaturesDied: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void player_on_created(player_t *player) {
    // Zeiten zuruecksetzen
    player->all_dead_time         = game_time - PLAYER_CREATURE_RESPAWN_DELAY;
    player->all_disconnected_time = game_time;
    player->spawn_time            = game_time;

    // Rule Handler aufrufen
    lua_pushliteral(L, "onPlayerCreated");
    lua_rawget(L, LUA_GLOBALSINDEX);         
    lua_pushnumber(L, player_num(player));
    if (lua_pcall(L, 1, 0, 0) != 0) {
        fprintf(stderr, "error calling onPlayerCreated: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    
    lua_pushliteral(player->L, "_player_created");
    lua_pushnumber(player->L, player_num(player));
    lua_rawset(player->L, LUA_GLOBALSINDEX);         
}

static int player_at_panic(lua_State *L) {
    printf("Aeiik! LUA panic. Cannot continue. Please file a bug report\n");
    abort();
    return 0; // never reached
}

static int player_at_cpu_exceeded(lua_State *L) {
    lua_pushliteral(L, "traceback");
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushliteral(L, "cpu limit exceeded");
    lua_call(L, 1, 1);
    return 1;
}

static void *player_allocator(void *ud, void *ptr, size_t osize, size_t nsize) {
    player_t *player = (player_t*)ud;
    (void)osize;  /* not used */

    // printf("alloc %p, o=%d, n=%d\n", ptr, osize, nsize);

    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else {
        if (player->mem_enforce && 
            lua_gc(player->L, LUA_GCCOUNT, 0) >= player->max_mem)
            return NULL;

        return realloc(ptr, nsize);
    }
}


// Erwartet Funktion sowie params Parameter auf dem Stack
static int call_user_lua(player_t *player, int params) {
    char errorbuf[4096];
    lua_pushliteral(player->L, "traceback");    // func params* 'traceback'
    lua_rawget(player->L, LUA_REGISTRYINDEX);   // func params* traceback
    lua_insert(player->L, -2 - params);         // traceback func params*

    // XXX: Geht davon aus, das vor dem installieren des Errorhandlers (setjmp, etc..) 
    //      kein Speicherbedingter Fehler auftritt.
    player->mem_enforce = 1;
    int ret = lua_pcall(player->L, params, 0, -2 - params);
    player->mem_enforce = 0;

    switch (ret) {
        case LUA_ERRRUN:
            snprintf(errorbuf, sizeof(errorbuf), "runtime error: %s\r\n", lua_tostring(player->L, -1));
            player_writeto(player, errorbuf, strlen(errorbuf));
            lua_pop(player->L, 2);
            return 0;
        case LUA_ERRMEM:
            snprintf(errorbuf, sizeof(errorbuf), "mem error: %s\r\n", lua_tostring(player->L, -1));
            player_writeto(player, errorbuf, strlen(errorbuf));
            lua_pop(player->L, 2);
            return 0;
        case LUA_ERRERR:
            snprintf(errorbuf, sizeof(errorbuf), "error calling errorhandler (_TRACEBACK)\r\n");
            player_writeto(player, errorbuf, strlen(errorbuf));
            lua_pop(player->L, 2);
            return 0;
        default: 
            lua_pop(player->L, 1);
            return 1;
    }
}

static int player_get_cpu_usage(player_t *player) {
    int cycles_left = lua_get_cycles(player->L);
    return 100 * (player->max_cycles - cycles_left) / player->max_cycles;
}

// Makros fuer die Verwendung innerhalb der C-Lua Creature-Funktionen. 
// Innerhalb einer Spieler VM wird ein Pointer auf die player-Struktur
// als Upvalue abgelegt (siehe lua_register_player), welcher dann inner-
// halb der Funktion wieder als 'player' zur Verfuegung steht. 
//
// Werden die Funktionen von der HauptVM aus aufgerufen, so ist der
// Upvalue nil und der Wert von Player NULL.

#define get_player()                                            \
    player_t *player = lua_touserdata(L, lua_upvalueindex(1));  \
    (void)player; /* Unused Warning weg */                         

#define get_player_and_creature()                               \
    get_player();                                               \
    creature_t *creature = creature_get_checked_lua(L, 1);         

#define assure_is_players_creature()                            \
    do { if (player && creature->player != player)              \
        return luaL_error(L, "%d isn't your creature",          \
                             creature_num(creature));           \
    } while(0)                                                                  

static int luaSaveInRegistry(lua_State *L) {
    get_player();
    lua_settable(player->L, LUA_REGISTRYINDEX);
    return 0;
}

static int luaPrint(lua_State *L) {
    get_player();
    int n=lua_gettop(L);
    int i;
    for (i=1; i<=n; i++) {
        if (i>1) player_writeto(player, "\t", 1);
        if (lua_isstring(L,i))
            player_writeto(player, lua_tostring(L,i), lua_strlen(L, i));
        else if (lua_isnil(L,i))
            player_writeto(player, "nil", 3);
        else if (lua_isboolean(L,i))
            lua_toboolean(L,i) ? player_writeto(player, "true",  4): 
                                 player_writeto(player, "false", 5);
        else {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s:%p",lua_typename(L,lua_type(L,i)),lua_topointer(L,i));
            player_writeto(player, buffer, strlen(buffer));
        }
    }
    player_writeto(player, "\r\n", 2);
    return 0;
}

static int luaCreatureSuicide(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    // lua_consumecycles(L, 100);
    creature_suicide(creature);
    return 0;
}

static int luaCreatureSetPath(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    // lua_consumecycles(L, 500);
    lua_pushboolean(L, creature_set_path(creature, 
                                         luaL_checklong(L, 2), 
                                         luaL_checklong(L, 3)));
    return 1;
}

static int luaCreatureSetTarget(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    // lua_consumecycles(L, 20);
    lua_pushboolean(L, creature_set_target(creature, 
                                           luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetConvert(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    // lua_consumecycles(L, 20);
    lua_pushboolean(L, creature_set_conversion_type(creature, 
                                                    luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetMessage(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    // lua_consumecycles(L, 500);
    creature_set_message(creature, luaL_checkstring(L, 2));
    return 0;
}

static int luaCreatureGetNearestEnemy(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    // lua_consumecycles(L, 500);
    int mindist;
    const creature_t *nearest = creature_nearest_enemy(creature, &mindist);
    if (!nearest)
        return 0;
    lua_pushnumber(L, creature_num(nearest));
    lua_pushnumber(L, nearest->x);
    lua_pushnumber(L, nearest->y);
    lua_pushnumber(L, player_num(nearest->player));
    lua_pushnumber(L, mindist);
    return 5;
}

static int luaCreatureGetHealth(lua_State *L) {
    get_player_and_creature();
    lua_pushnumber(L, 100 * creature->health / creature_max_health(creature));
    return 1;
}

static int luaCreatureGetSpeed(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    lua_pushnumber(L, creature_speed(creature));
    return 1;
}

static int luaCreatureGetType(lua_State *L) {
    get_player_and_creature();
    lua_pushnumber(L, creature->type);
    return 1;
}

static int luaCreatureGetFood(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    lua_pushnumber(L, creature->food);
    return 1;
}

static int luaCreatureGetTileFood(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushnumber(L, creature_food_on_tile(creature));
    return 1;
}

static int luaCreatureGetTileType(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushnumber(L, creature_tile_type(creature));
    return 1;
}

static int luaCreatureGetMaxFood(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushnumber(L, creature_max_food(creature));
    return 1;
}

static int luaCreatureGetPos(lua_State *L) {
    get_player_and_creature();
    lua_pushnumber(L, creature->x);
    lua_pushnumber(L, creature->y);
    return 2;
}

static int luaCreatureGetDistance(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    const creature_t *target = creature_get_checked_lua(L, 2);
    // lua_consumecycles(L, 100);
    lua_pushnumber(L, creature_dist(creature, target));
    return 1;
}

static int luaCreatureGetState(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushnumber(L, creature->state);
    return 1;
}

static int luaCreatureGetHitpoints(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    lua_pushnumber(L, creature_hitpoints(creature));
    return 1;
}

static int luaCreatureGetAttackDistance(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    lua_pushnumber(L, creature_attack_distance(creature));
    return 1;
}

#ifdef CHEATS
static int luaCreatureCheatGiveAll(lua_State *L) {
    get_player_and_creature();
    creature->health = creature_max_health(creature);
    creature->food   = creature_max_food(creature);
    return 0;
}
#endif

static int luaCreatureSetState(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    int state = luaL_checklong(L, 2);
    switch (state) {
        case CREATURE_IDLE:
        case CREATURE_WALK:
        case CREATURE_HEAL:
        case CREATURE_EAT:
        case CREATURE_ATTACK:
        case CREATURE_CONVERT:
        case CREATURE_SPAWN:
        case CREATURE_FEED:
            lua_pushboolean(L, creature_set_state(creature, state));
            break;
        default:
            luaL_error(L, "invalid state %d", luaL_checklong(L, 2));
            break;
    }
    return 1;
}

static int luaGetCPUUsage(lua_State *L) { 
    get_player();
    // lua_consumecycles(L, 1000);
    lua_pushnumber(L, player_get_cpu_usage(player));
    return 1;
}

static int luaWorldSize(lua_State *L) {
    lua_pushnumber(L, TILE_X1(1));
    lua_pushnumber(L, TILE_Y1(1));
    lua_pushnumber(L, TILE_X2(world_width()  - 2));
    lua_pushnumber(L, TILE_Y2(world_height() - 2));
    return 4;
}

static int luaGameTime(lua_State *L) {
    lua_pushnumber(L, game_time);
    return 1;
}

static int luaGetKothPos(lua_State *L) {
    lua_pushnumber(L, TILE_XCENTER(world_koth_x()));
    lua_pushnumber(L, TILE_YCENTER(world_koth_y()));
    return 2;
}

static int luaCreatureExists(lua_State *L) {
    lua_pushboolean(L, !!creature_by_num(luaL_checklong(L, 1)));
    return 1;
}

static int luaPlayerExists(lua_State *L) {
    lua_pushboolean(L, !!player_by_num(luaL_checklong(L, 1)));
    return 1;
}

static int luaCreatureGetPlayer(lua_State *L) {
    const creature_t *creature = creature_get_checked_lua(L, 1);
    lua_pushnumber(L, player_num(creature->player));
    return 1;
}

static int luaPlayerScore(lua_State *L) {
    lua_pushnumber(L, player_get_checked_lua(L, 1)->score);
    return 1;
}

static int luaPlayerChangeScore(lua_State *L) {
    player_score(player_get_checked_lua(L, 1), 
                 luaL_checklong(L, 2),
                 luaL_checkstring(L, 3));
    return 0;
}

static int luaCreatureSetFood(lua_State *L) {
    get_player_and_creature();
    lua_pushnumber(L, creature_set_food(creature, luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetType(lua_State *L) {
    get_player_and_creature();
    lua_pushnumber(L, creature_set_type(creature, luaL_checklong(L, 2)));
    return 1;
}


static int luaKingPlayer(lua_State *L) {
    player_t *king = player_king();
    if (king) {
        lua_pushnumber(L, player_num(king));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static void player_set_color(player_t *player, int color) {
    player->color = color & 0xFF;
    player->dirtymask |= PLAYER_DIRTY_COLOR;
}

#define lua_register_player(p,n,f) \
    (lua_pushstring((p)->L, n), \
     lua_pushlightuserdata((p)->L, p), \
     lua_pushcclosure((p)->L, f, 1), \
     lua_settable((p)->L, LUA_GLOBALSINDEX))    

#define lua_register_player_global(p, name) \
    (lua_pushnumber((p)->L, name), \
     lua_setglobal((p)->L, #name))

player_t *player_create(const char *pass) {
    int playerno;

    for (playerno = 0; playerno < MAXPLAYERS; playerno++) {
        if (!PLAYER_USED(&players[playerno]))
            break;
    }

    if (playerno == MAXPLAYERS)
        return NULL;

    player_t *player = &players[playerno];
    memset(player, 0, sizeof(player_t));

    snprintf(player->name, sizeof(player->name), "player%d", playerno);
    snprintf(player->pass, sizeof(player->pass), "%s", pass);

    player->max_mem     = LUA_MAX_MEM;
    player->mem_enforce = 0;
    
    player->max_cycles = LUA_MAX_CPU;

    player->L = lua_newstate(player_allocator, player);

    lua_atcpu_exceeded(player->L, player_at_cpu_exceeded);
    lua_atpanic       (player->L, player_at_panic);

    player_set_color(player, rand() % 256);
    
    luaL_openlibs(player->L);

    lua_register_player(player, "save_in_registry",     luaSaveInRegistry);
    lua_register_player(player, "print",                luaPrint);
    lua_register_player(player, "suicide",              luaCreatureSuicide);
    lua_register_player(player, "set_path",             luaCreatureSetPath);
    lua_register_player(player, "get_pos",              luaCreatureGetPos);
    lua_register_player(player, "get_state",            luaCreatureGetState);
    lua_register_player(player, "set_state",            luaCreatureSetState);
    lua_register_player(player, "set_target",           luaCreatureSetTarget);
    lua_register_player(player, "set_convert",          luaCreatureSetConvert);
    lua_register_player(player, "nearest_enemy",        luaCreatureGetNearestEnemy);
    lua_register_player(player, "get_nearest_enemy",    luaCreatureGetNearestEnemy);
    lua_register_player(player, "get_type",             luaCreatureGetType);
    lua_register_player(player, "get_food",             luaCreatureGetFood);
    lua_register_player(player, "get_health",           luaCreatureGetHealth);
    lua_register_player(player, "get_speed",            luaCreatureGetSpeed);
    lua_register_player(player, "get_tile_food",        luaCreatureGetTileFood);
    lua_register_player(player, "get_tile_type",        luaCreatureGetTileType);
    lua_register_player(player, "get_max_food",         luaCreatureGetMaxFood);
    lua_register_player(player, "get_distance",         luaCreatureGetDistance);
    lua_register_player(player, "set_message",          luaCreatureSetMessage);
    lua_register_player(player, "get_hitpoints",        luaCreatureGetHitpoints);
    lua_register_player(player, "get_attack_distance",  luaCreatureGetAttackDistance);

#ifdef CHEATS
    lua_register_player(player, "cheat_give_all",       luaCreatureCheatGiveAll);
#endif

    lua_register_player(player, "get_cpu_usage",        luaGetCPUUsage);
    
    lua_register(player->L,     "world_size",           luaWorldSize);
    lua_register(player->L,     "game_time",            luaGameTime);
    lua_register(player->L,     "get_koth_pos",         luaGetKothPos);
    lua_register(player->L,     "exists",               luaCreatureExists);
    lua_register(player->L,     "creature_exists",      luaCreatureExists);
    lua_register(player->L,     "creature_player",      luaCreatureGetPlayer);
    lua_register(player->L,     "player_exists",        luaPlayerExists);
    lua_register(player->L,     "king_player",          luaKingPlayer);
    lua_register(player->L,     "player_score",         luaPlayerScore);

    lua_register_player_global(player, CREATURE_IDLE);
    lua_register_player_global(player, CREATURE_WALK);
    lua_register_player_global(player, CREATURE_HEAL);
    lua_register_player_global(player, CREATURE_EAT);
    lua_register_player_global(player, CREATURE_ATTACK);
    lua_register_player_global(player, CREATURE_CONVERT);
    lua_register_player_global(player, CREATURE_SPAWN);
    lua_register_player_global(player, CREATURE_FEED);

    lua_register_player_global(player, TILE_SOLID);
    lua_register_player_global(player, TILE_PLAIN);
    lua_register_player_global(player, TILE_WATER);
    lua_register_player_global(player, TILE_LAVA);

    lua_register_player_global(player, CREATURE_SPAWNED);
    lua_register_player_global(player, CREATURE_KILLED);
    lua_register_player_global(player, CREATURE_ATTACKED);

    lua_pushnumber(player->L, playerno);

    lua_setglobal(player->L, "player_number");

    // Initiale Cyclen setzen
    lua_set_cycles(player->L, player->max_cycles);
    player_init_events(player);

    luaL_dofile(player->L, "player.lua");
    
    player_to_network(player, PLAYER_DIRTY_ALL, SEND_BROADCAST);

    player_on_created(player);
    return player;
}

void player_destroy(player_t *player) {
    creature_kill_all_players_creatures(player);
            
    client_t *client;
    while ((client = player->clients)) {
        player_detach_client(client, player);
    };

    lua_close(player->L);
    player->L = NULL;
    
    player_to_network(player, PLAYER_DIRTY_ALIVE, SEND_BROADCAST);
}

void player_set_name(player_t *player, const char *name) {
    snprintf(player->name, sizeof(player->name), "%s", name);
    player->dirtymask |= PLAYER_DIRTY_NAME;
}

int player_num(player_t *player) {
    return player - players;
}

void player_writeto(player_t *player, const void *data, size_t size) {
    if (!player->clients) {
        assert(player->num_clients == 0);
        return;
    }
            
    client_t *client = player->clients;
    do {
        server_writeto(client, data, size);
        client = client->next;
    } while (client != player->clients);
}

player_t *player_by_num(int playerno) {
    if (playerno < 0 || playerno >= MAXPLAYERS) 
        return NULL;
    player_t *player = &players[playerno];
    if (!PLAYER_USED(player)) 
        return NULL;
    return player;
}

player_t *player_get_checked_lua(lua_State *L, int idx) {
    int playerno = luaL_checklong(L, idx);
    if (playerno < 0 || playerno >= MAXPLAYERS) 
        luaL_error(L, "player number %d out of range", playerno);
    player_t *player = &players[playerno];
    if (!PLAYER_USED(player)) 
        luaL_error(L, "player %d not in use", playerno);
    return player;
}

int player_attach_client(client_t *client, player_t *player, const char *pass) {
    if (!client || !player)
        return 0;

    if (!PLAYER_USED(player))
        return 0;
    
    // Passwort fhslca?
    if (strcmp(pass, player->pass))
        return 0;

    if (client->player) {
        if (client->player == player) 
            return 1;
        player_detach_client(client, client->player);
    }

    client->player = player;

    player->num_clients++;

    if (player->clients) {
        client->prev = player->clients;
        client->next = player->clients->next;
        client->next->prev = client;
        client->prev->next = client;
    } else {
        client->prev = client->next = player->clients = client;
    }
    assert(client->next->prev == client);
    assert(client->prev->next == client);
    assert(player->clients->next->prev == player->clients);
    assert(player->clients->prev->next == player->clients);

    return 1;
}

int player_detach_client(client_t *client, player_t *player) {
    if (!client || !player) 
        return 0;

    if (!client->player) 
        return 0;

    if (client->player != player) 
        return 0;

    player->num_clients--;
    assert(player->num_clients >= 0);

    if (player->num_clients == 0)
        player->all_disconnected_time = game_time;

    client->player = NULL;
    
    if (player->num_clients == 0) {
        player->clients = NULL;
    } else {
        client->next->prev = client->prev;
        client->prev->next = client->next;
        player->clients = client->next;
        assert(client->next->next->prev == client->next);
        assert(client->next->prev->next == client->next);
        assert(player->clients->next->prev == player->clients);
        assert(player->clients->prev->next == player->clients);
    }

    client->next = NULL;
    client->prev = NULL;

    return 1;
}

void player_execute_client_lua(player_t *player, const char *code, size_t codelen, const char *where) {
    int error = luaL_loadbuffer(player->L, code, codelen, where);

    if (!error) {
        call_user_lua(player, 0);
    } else {
        const char *msg = lua_tostring(player->L, -1);
        if (!msg) 
            msg = "(error with no message)";
        player_writeto(player, msg, strlen(msg));
        player_writeto(player, "\r\n", 2); 
        lua_pop(player->L, 1);
    }
}

void player_think() {
    int playerno;
    player_t *player = &players[0];
    for (playerno = 0; playerno < MAXPLAYERS; playerno++, player++) {
        if (!PLAYER_USED(player))
            continue;

        // Spieler CPU cyclen zuruecksetzen. Der Spieler
        // kann damit sowohl seine Kreaturen steuern, als
        // auch Befehle ueber Netzwerk ausfuehren.
        lua_set_cycles(player->L, player->max_cycles);

        // Incremental Garbage Collection
        lua_gc(player->L, LUA_GCSTEP, 1);

        // Alle Viecher tot? neu spawnen.
        if (player->num_creatures == 0 && 
            game_time > player->all_dead_time + PLAYER_CREATURE_RESPAWN_DELAY) 
        {
            player_on_all_dead(player);
        }
        
        // Killen?
        if (player->kill_me) {
            player_writeto(player, "player was killed\r\n", 19);
            player_destroy(player);
            continue;
        }

        // Zu wenig Score?
        if (player->score <= PLAYER_KICK_SCORE) {
            player_writeto(player, "score too low. try harder!\r\n", 28);
            player_destroy(player);
            continue;
        }

        // Kein Client mehr da?
        if (player->num_clients == 0 && 
            player->all_disconnected_time + NO_CLIENT_KICK_TIME < game_time) 
        {
            player_destroy(player);
            continue;
        }

        // Player Code aufrufen
        lua_pushliteral(player->L, "player_think");
        lua_rawget(player->L, LUA_GLOBALSINDEX);         
        lua_pushliteral(player->L, "events");
        lua_rawget(player->L, LUA_REGISTRYINDEX);
        call_user_lua(player, 1);

        // Events Strukur neu initialisieren
        player_init_events(player);

        // Verbrauchte CPU Zeit speichern:
        // Dies ist die rein fuer player_think verbrauchte
        // Zeit. Darauf folgende Konsolenbefehle werden fuer
        // die Anzeige im Client nicht mehr beachtet.
        int newcpu = player_get_cpu_usage(player); 

        if (newcpu != player->cpu_usage) {
            player->dirtymask |= PLAYER_DIRTY_CPU;
            player->cpu_usage = newcpu;
        }

        player_to_network(player, player->dirtymask, SEND_BROADCAST);
        player->dirtymask = PLAYER_DIRTY_NONE;
    }
}

void player_send_king_update(client_t *client) {
    // Network Sync
    packet_t packet;
    packet_init(&packet, PACKET_KOTH_UPDATE);
    if (king_player) 
        packet_write08(&packet, player_num(king_player));
    else
        packet_write08(&packet, 0xFF);
    server_send_packet(&packet, client); 
}

void player_is_king_of_the_hill(player_t *player, int delta) {
    player_t *old_king = king_player;

    king_player = player;
    player->last_koth_time = game_time;
    player->koth_time += delta;
    while (player->koth_time >= 2000) {
        player_score(player, CREATURE_KOTH_POINTS, "King of the Hill!");
        player->koth_time -= 2000;
    }

    if (king_player != old_king)
        player_send_king_update(SEND_BROADCAST);
}

void player_there_is_no_king() {
    player_t *old_king = king_player;
    king_player = NULL;

    if (old_king) 
        player_send_king_update(SEND_BROADCAST);
}

player_t *player_king() {
    if (king_player && PLAYER_USED(king_player))
        return king_player;
    else
        return NULL;
}

void player_send_initial_update(client_t *client) {
    int playerno;
    player_t *player = &players[0];
    for (playerno = 0; playerno < MAXPLAYERS; playerno++, player++) {
        if (!PLAYER_USED(player))
            continue;

        player_to_network(player, PLAYER_DIRTY_ALL, client);
    }

    player_send_king_update(client);
}

void player_to_network(player_t *player, int dirtymask, client_t *client) {
    if (dirtymask == PLAYER_DIRTY_NONE)
        return;

    packet_t packet;
    packet_init(&packet, PACKET_PLAYER_UPDATE);

    packet_write08(&packet, player_num(player));
    packet_write08(&packet, dirtymask);
    if (dirtymask & PLAYER_DIRTY_ALIVE) 
        packet_write08(&packet, PLAYER_USED(player));
    if (dirtymask & PLAYER_DIRTY_NAME) {
        packet_write08(&packet, strlen(player->name));
        packet_writeXX(&packet, &player->name, strlen(player->name));
    }
    if (dirtymask & PLAYER_DIRTY_COLOR) 
        packet_write08(&packet, player->color);
    if (dirtymask & PLAYER_DIRTY_CPU)
        packet_write08(&packet, player->cpu_usage);
    if (dirtymask & PLAYER_DIRTY_SCORE)
        packet_write16(&packet, player->score - PLAYER_KICK_SCORE);

    server_send_packet(&packet, client);
}

static int luaPlayerKill(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    player->kill_me = 1;
    return 0;
}

static int luaPlayerNumClients(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    lua_pushnumber(L, player->num_clients);
    return 1;
}

static int luaPlayerNumCreatures(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    lua_pushnumber(L, player->num_creatures);
    return 1;
}

static int luaPlayerSpawnTime(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    lua_pushnumber(L, player->spawn_time);
    return 1;
}

static int luaPlayerCreate(lua_State *L) {
    player_t *player = player_create(luaL_checkstring(L, 1));
    
    if (player) {
        lua_pushnumber(L, player_num(player));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int luaPlayerSetName(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    player_set_name(player, luaL_checkstring(L, 2));
    return 0;
}

static int luaPlayerSetColor(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    player_set_color(player, luaL_checklong(L, 2));
    return 0;
}

static int luaPlayerGetName(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    lua_pushstring(L, player->name);
    return 1;
}

static int luaPlayerGetUsedMem(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    lua_pushnumber(L, lua_gc(player->L, LUA_GCCOUNT,  0) << 10 |
                      lua_gc(player->L, LUA_GCCOUNTB, 0));
    return 1;
}

static int luaPlayerGetCPUUsage(lua_State *L) {
    lua_pushnumber(L, player_get_cpu_usage(player_get_checked_lua(L, 1)));
    return 1;
}

static int luaPlayerSetScore(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    int newscore = luaL_checklong(L, 2);
    int diff = newscore - player->score;
    char buf[128];
    snprintf(buf, sizeof(buf), "set to %d", newscore);
    player_score(player, diff, buf);
    return 0;
}

static int luaPlayerKillAllCreatures(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1); 
    creature_kill_all_players_creatures(player);
    return 0;
}

static int luaCreatureSpawn(lua_State *L) {
    player_t   *player = player_get_checked_lua(L, 1);
    creature_t *parent = lua_isnil(L, 2) ? NULL : creature_get_checked_lua(L, 2);
    int x              = luaL_checklong(L, 3);
    int y              = luaL_checklong(L, 4);
    creature_type type = luaL_checklong(L, 5);
    if (type < 0 || type >= CREATURE_TYPES)
        luaL_error(L, "creature type %d invalid", type);
    creature_t *creature = creature_spawn(player, parent, x, y, type);
    if (!creature)
        lua_pushnil(L);
    else
        lua_pushnumber(L, creature_num(creature));
    return 1;
}

void player_init() {
    memset(players, 0, sizeof(players));

    lua_register(L, "player_create",                luaPlayerCreate);
    lua_register(L, "player_num_clients",           luaPlayerNumClients);
    lua_register(L, "player_num_creatures",         luaPlayerNumCreatures);
    lua_register(L, "player_kill",                  luaPlayerKill);
    lua_register(L, "player_set_color",             luaPlayerSetColor);
    lua_register(L, "player_set_name",              luaPlayerSetName);
    lua_register(L, "player_get_name",              luaPlayerGetName);
    lua_register(L, "player_set_score",             luaPlayerSetScore);
    lua_register(L, "player_kill_all_creatures",    luaPlayerKillAllCreatures);
    lua_register(L, "player_score",                 luaPlayerScore);
    lua_register(L, "player_exists",                luaPlayerExists);
    lua_register(L, "player_spawntime",             luaPlayerSpawnTime);
    lua_register(L, "player_change_score",          luaPlayerChangeScore);
    lua_register(L, "player_get_used_mem",          luaPlayerGetUsedMem);
    lua_register(L, "player_get_used_cpu",          luaPlayerGetCPUUsage);

    lua_register(L, "creature_spawn",               luaCreatureSpawn);
    lua_register(L, "creature_get_pos",             luaCreatureGetPos);
    lua_register(L, "creature_get_state",           luaCreatureGetState);
    lua_register(L, "creature_get_nearest_enemy",   luaCreatureGetNearestEnemy);
    lua_register(L, "creature_get_type",            luaCreatureGetType);
    lua_register(L, "creature_get_food",            luaCreatureGetFood);
    lua_register(L, "creature_get_health",          luaCreatureGetHealth);
    lua_register(L, "creature_get_speed",           luaCreatureGetSpeed);
    lua_register(L, "creature_get_tile_food",       luaCreatureGetTileFood);
    lua_register(L, "creature_get_max_food",        luaCreatureGetMaxFood);
    lua_register(L, "creature_get_distance",        luaCreatureGetDistance);
    lua_register(L, "creature_get_hitpoints",       luaCreatureGetHitpoints);
    lua_register(L, "creature_get_attack_distance", luaCreatureGetAttackDistance);
    lua_register(L, "creature_get_player",          luaCreatureGetPlayer);

    lua_register(L, "creature_set_food",            luaCreatureSetFood);
    lua_register(L, "creature_set_type",            luaCreatureSetType);

    lua_pushnumber(L, CREATURE_SMALL);
    lua_setglobal(L, "CREATURE_SMALL");

    lua_pushnumber(L, CREATURE_BIG);
    lua_setglobal(L, "CREATURE_BIG");

    lua_pushnumber(L, CREATURE_FLYER);
    lua_setglobal(L, "CREATURE_FLYER");
}

void player_round_start() {
    int playerno;
    for (playerno = 0; playerno < MAXPLAYERS; playerno++) {
        if (PLAYER_USED(&players[playerno]))
            player_on_created(&players[playerno]);
    }
}

void player_shutdown() {
    int playerno;
    for (playerno = 0; playerno < MAXPLAYERS; playerno++) {
        if (PLAYER_USED(&players[playerno]))
            player_destroy(&players[playerno]);
    }
}
