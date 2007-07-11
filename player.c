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
#include <signal.h>
#include <sys/time.h>

#include <lauxlib.h>
#include <lualib.h>

#include "infond.h"
#include "global.h"
#include "player.h"
#include "server.h"
#include "creature.h"
#include "world.h"
#include "misc.h"
#include "game.h"
#include "rules.h"
#include "scroller.h"

#define PLAYER_USED(player) (!!((player)->L))

static player_t players[MAXPLAYERS];
static int      num_players = 0;

static player_t *king_player   = NULL;

void player_change_score(player_t *player, int scoredelta, const char *reason) {
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
    lua_pushnumber(L, player_num(player));
    lua_pushnumber(L, player->score);
    lua_pushstring(L, reason);
    game_call_rule_handler("onPlayerScoreChange", 3);
}

void player_init_events(player_t *player) {
    lua_pushliteral(player->L, "events");
    lua_newtable(player->L);
    lua_settable(player->L, LUA_REGISTRYINDEX);
}

// erwartet values [key, value] Paare
void player_add_event(player_t *player, vm_event event, int values) {
    lua_pushliteral(player->L, "events");
    lua_rawget(player->L, LUA_REGISTRYINDEX);           // kv* _events        
    assert(!lua_isnil(player->L, -1));
    lua_createtable(player->L, 3, 0);                   // kv* _events event
    lua_pushliteral(player->L, "type");                 // kv* _events event 'type'
    lua_pushnumber(player->L, event);                   // kv* _events event 'type' id
    lua_rawset(player->L, -3);                          // kv* _events event
    lua_insert(player->L, -2);                          // kv* event _events 
    lua_insert(player->L, -2 - 2 * values);             // _events kv* event
    lua_insert(player->L, -1 - 2 * values);             // _events event kv*
    for (int i = 0; i < values; i++) 
        lua_rawset(player->L, - 1 - 2 * (values - i));  // _events event kv*
    size_t size = lua_objlen(player->L, -2);            // _events
    lua_rawseti(player->L, -2, size + 1);               // _events
    lua_pop(player->L, 1);                              //
}

void player_on_creature_spawned(player_t *player, creature_t *creature, creature_t *parent) {
    lua_pushliteral(player->L, "id");     lua_pushnumber(player->L, creature_id(creature));
    lua_pushliteral(player->L, "parent"); lua_pushnumber(player->L, parent ? creature_id(parent) : -1);
    player_add_event(player, CREATURE_SPAWNED, 2);

    player->num_creatures++;
        
    // Rule Handler aufrufen
    lua_pushnumber(L, creature_id(creature));
    if (parent)
        lua_pushnumber(L, creature_id(parent));
    else
        lua_pushnil(L);
    game_call_rule_handler("onCreatureSpawned", 2);
}

void player_on_creature_killed(player_t *player, creature_t *victim, creature_t *killer) {
    lua_pushliteral(player->L, "id");     lua_pushnumber(player->L, creature_id(victim));
    lua_pushliteral(player->L, "killer"); lua_pushnumber(player->L, killer ? creature_id(killer) : -1);
    player_add_event(player, CREATURE_KILLED, 2);

    player->num_creatures--;

    assert(player->num_creatures >= 0);

    if (player->num_creatures == 0)
        player->all_dead_time = game_time;

    // Rule Handler aufrufen
    lua_pushnumber(L, creature_id(victim));
    if (killer)
        lua_pushnumber(L, creature_id(killer));
    else
        lua_pushnil(L);
    game_call_rule_handler("onCreatureKilled", 2);
}

void player_on_creature_attacked(player_t *player, creature_t *victim, creature_t *attacker) {
    lua_pushliteral(player->L, "id");       lua_pushnumber(player->L, creature_id(victim));
    lua_pushliteral(player->L, "attacker"); lua_pushnumber(player->L, creature_id(attacker));
    player_add_event(player, CREATURE_ATTACKED, 2);
}

void player_on_all_dead(player_t *player, int time) {
    // Rule Handler aufrufen
    lua_pushnumber(L, player_num(player));
    lua_pushnumber(L, time);
    game_call_rule_handler("onPlayerAllCreaturesDead", 2);
}

void player_on_created(player_t *player) {
    // Zeiten zuruecksetzen
    player->all_dead_time = game_time;
    player->spawn_time    = game_time;

    // Event eintragen
    lua_pushliteral(player->L, "id"); lua_pushnumber(player->L, player_num(player));
    player_add_event(player, PLAYER_CREATED, 1);

    // Rule Handler aufrufen
    lua_pushnumber(L, player_num(player));
    game_call_rule_handler("onPlayerCreated", 1);
}

static int player_at_panic(lua_State *L) {
    fprintf(stderr, "Aeiik! LUA panic. Cannot continue. Please file a bug report\n");
    abort();
    return 0; // never reached
}

static void *player_allocator(void *ud, void *ptr, size_t osize, size_t nsize) {
    player_t *player = (player_t*)ud;
    (void)osize;  /* not used */

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

static const char *exceeded_message = NULL;

static int player_at_cpu_exceeded(lua_State *L) {
    lua_pushstring(L, exceeded_message);
    lua_pushliteral(L, "traceback");
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_call(L, 0, 1);
    lua_concat(L, 2);
    return 1;
}

static player_t *alarm_player = NULL;
#ifndef WIN32
static struct itimerval timer;
#endif

static void alarm_signal(int sig) {
    assert(sig == SIGVTALRM);
    assert(alarm_player);

    // Cycles auf 0 => Bei naechster Anweisung wird die Ausfuehrung beendet.
    exceeded_message = "cpu time exceeded at ";
    lua_set_cycles(alarm_player->L, 0);
  
    // Spieler kicken. Es riecht nach DOS.
    free(alarm_player->kill_me);
    alarm_player->kill_me = strdup("player killed: used excessive amount of cpu. what are you doing?");
} 

// Erwartet Funktion sowie params Parameter auf dem Stack
static int player_call_user_lua(const char *where, player_t *player, int params) {
    lua_pushliteral(player->L, "traceback");    // func params* 'traceback'
    lua_rawget(player->L, LUA_REGISTRYINDEX);   // func params* traceback
    lua_insert(player->L, -2 - params);         // traceback func params*

    // XXX: Geht davon aus, das vor dem installieren des Errorhandlers (setjmp, etc..) 
    //      kein Speicherbedingter Fehler auftritt.
    player->mem_enforce = 1;

    // Notfallzeitbeschraenkung einbauen. Kann auftreten, falls sehr viele
    // teure Lua Funktionen aufgerufen werden.
#ifndef WIN32
    alarm_player              = player;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec     = LUA_MAX_REAL_CPU_SECONDS;
    timer.it_value.tv_usec    = 0;
    signal(SIGVTALRM, alarm_signal);
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
#endif

    // Normalerweise liegt ein "cpu exceeded" Fehler an Ueberschreitung der lua VM Cycles.
    // Nur im CPU Rechenzeit-Fall wird diese Meldung entsprechend angepasst.
    exceeded_message = "lua vm cycles exceeded at ";
    
    // Usercode aufrufen
    int ret = lua_pcall(player->L, params, 0, -2 - params);

#ifndef WIN32
    // Alarm aufheben
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    alarm_player              = NULL;
#endif

    // Keine Speicherbeschraenkung mehr (ab hier wird die VM wieder von infon gesteuert)
    player->mem_enforce = 0;

    // Ausfuehrung geklappt. Aufraeumen & raus.
    if (ret == 0) {
        lua_pop(player->L, 1);
        return 1;
    }

    // Fehler bei der Ausfuehrung. Fehlermeldung holen
    const char *errmsg = lua_tostring(player->L, -1);
    if (!errmsg) 
        errmsg = "unknown error";

    const char *why;
    switch (ret) {
        case LUA_ERRRUN: why = "runtime";        break; 
        case LUA_ERRMEM: why = "memory";         break;
        case LUA_ERRERR: why = "error handling"; break;
        default:         why = "unknown";        break;
    }

    char errorbuf[4096];
    snprintf(errorbuf, sizeof(errorbuf), "%s error %s: %s\r\n", why, where, errmsg);
    player_writeto(player, errorbuf, strlen(errorbuf));
    lua_pop(player->L, 2);
    return 0;
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
                             creature_id(creature));            \
    } while(0)                                                                  

static int luaSaveInRegistry(lua_State *L) {
    get_player();
    lua_settable(player->L, LUA_REGISTRYINDEX);
    return 0;
}

void luaLineHook(lua_State *L, lua_Debug *ar) {
    lua_getinfo(L, "S", ar);
    lua_pushthread(L);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushstring(L, ar->source);
    lua_pushnumber(L, ar->currentline);
    lua_call(L, 2, 1);
    int yield = lua_toboolean(L, -1);
    lua_pop(L, 1);
    if (yield) lua_yield(L, 0);
}

static int luaSetLineHook(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTHREAD);
    lua_State *thread = lua_tothread(L, 1);
    if (lua_gettop(L) == 1) {
        lua_sethook(thread, luaLineHook, 0, 0);
    } else {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        lua_rawset(L, LUA_REGISTRYINDEX);
        lua_sethook(thread, luaLineHook, LUA_MASKLINE, 0);
    }
    return 0;
}

static int luaPrint(lua_State *L) {
    get_player();
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        if (i > 1) player_writeto(player, "\t", 1);
        if (lua_isstring(L,i))
            player_writeto(player, lua_tostring(L,i), lua_strlen(L, i));
        else if (lua_isnil(L,i))
            player_writeto(player, "nil", 3);
        else if (lua_isboolean(L,i))
            lua_toboolean(L,i) ? player_writeto(player, "true",  4): 
                                 player_writeto(player, "false", 5);
        else {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s:%p", lua_typename(L,lua_type(L,i)), lua_topointer(L,i));
            player_writeto(player, buffer, strlen(buffer));
        }
    }
    player_writeto(player, "\r\n", 2);
    return 0;
}

static int luaCreatureSuicide(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    creature_suicide(creature);
    return 0;
}

static int luaCreatureSetPath(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushboolean(L, creature_set_path(creature, 
                                         luaL_checklong(L, 2), 
                                         luaL_checklong(L, 3)));
    return 1;
}

static int luaCreatureSetTarget(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushboolean(L, creature_set_target(creature, 
                                           luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetConvert(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushboolean(L, creature_set_conversion_type(creature, 
                                                    luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetMessage(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    creature_set_message(creature, luaL_checkstring(L, 2));
    return 0;
}

static int luaCreatureGetNearestEnemy(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    int mindist;
    const creature_t *nearest = creature_nearest_enemy(creature, &mindist);
    if (!nearest)
        return 0;
    lua_pushnumber(L, creature_id(nearest));
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
    if (RESTRICTIVE) assure_is_players_creature();
    lua_pushnumber(L, creature->x);
    lua_pushnumber(L, creature->y);
    return 2;
}

static int luaCreatureGetDistance(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    const creature_t *target = creature_get_checked_lua(L, 2);
    lua_pushnumber(L, creature_dist(creature, target));
    return 1;
}

static int luaCreatureGetState(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_pushnumber(L, creature->state);
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
            luaL_error(L, "invalid state %d", state);
            break;
    }
    return 1;
}

static int luaGetCPUUsage(lua_State *L) { 
    get_player();
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
    if (RESTRICTIVE) luaL_error(L, "this function is not available in restricted mode");
    lua_pushboolean(L, !!creature_by_id(luaL_checklong(L, 1)));
    return 1;
}

static int luaPlayerExists(lua_State *L) {
    if (RESTRICTIVE) luaL_error(L, "this function is not available in restricted mode");
    lua_pushboolean(L, !!player_by_num(luaL_checklong(L, 1)));
    return 1;
}

static int luaCreatureGetPlayer(lua_State *L) {
    if (RESTRICTIVE) luaL_error(L, "this function is not available in restricted mode");
    const creature_t *creature = creature_get_checked_lua(L, 1);
    lua_pushnumber(L, player_num(creature->player));
    return 1;
}

static int luaPlayerScore(lua_State *L) {
    if (RESTRICTIVE) luaL_error(L, "this function is not available in restricted mode");
    lua_pushnumber(L, player_get_checked_lua(L, 1)->score);
    return 1;
}

static int luaPlayerChangeScore(lua_State *L) {
    player_change_score(player_get_checked_lua(L, 1), 
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

player_t *player_create(const char *name, const char *pass, const char *highlevel) {
    int playerno;

    for (playerno = 0; playerno < MAXPLAYERS; playerno++) {
        if (!PLAYER_USED(&players[playerno]))
            break;
    }

    if (playerno == MAXPLAYERS)
        return NULL;

    player_t *player = &players[playerno];
    memset(player, 0, sizeof(player_t));

    if (name) {
        snprintf(player->name, sizeof(player->name), "%s", name);
    } else {
        snprintf(player->name, sizeof(player->name), "player%d", playerno);
    }

    snprintf(player->pass, sizeof(player->pass), "%s", pass);

    player->max_mem             = LUA_MAX_MEM;
    player->mem_enforce         = 0;

    player->max_cycles          = LUA_MAX_CPU;

    player->no_client_kick_time   = NO_CLIENT_KICK_TIME;
    player->all_disconnected_time = real_time;
    
    player->L = lua_newstate(player_allocator, player);
    luaL_openlibs(player->L);

    lua_atcpu_exceeded(player->L, player_at_cpu_exceeded);
    lua_atpanic       (player->L, player_at_panic);

    player_set_color(player, rand() % 256);

    lua_register_string_constant(player->L, PREFIX);
    lua_register_player(player, "save_in_registry",     luaSaveInRegistry);
    lua_register_player(player, "linehook",             luaSetLineHook);
    lua_register_player(player, "client_print",         luaPrint);

    lua_register_player(player, "suicide",              luaCreatureSuicide);
    lua_register_player(player, "set_path",             luaCreatureSetPath);
    lua_register_player(player, "get_pos",              luaCreatureGetPos);
    lua_register_player(player, "get_state",            luaCreatureGetState);
    lua_register_player(player, "set_state",            luaCreatureSetState);
    lua_register_player(player, "set_target",           luaCreatureSetTarget);
    lua_register_player(player, "set_convert",          luaCreatureSetConvert);
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

#ifdef CHEATS
    lua_register_player(player, "cheat_give_all",       luaCreatureCheatGiveAll);
#endif

    lua_register_player(player, "get_cpu_usage",        luaGetCPUUsage);
    
    lua_register(player->L,     "world_size",           luaWorldSize);
    lua_register(player->L,     "game_time",            luaGameTime);
    lua_register(player->L,     "get_koth_pos",         luaGetKothPos);
    lua_register(player->L,     "creature_exists",      luaCreatureExists);
    lua_register(player->L,     "creature_player",      luaCreatureGetPlayer);
    lua_register(player->L,     "player_exists",        luaPlayerExists);
    lua_register(player->L,     "king_player",          luaKingPlayer);
    lua_register(player->L,     "player_score",         luaPlayerScore);
    lua_register(player->L,     "creature_get_config",  luaCreatureGetConfig);

    lua_register_constant(player->L, CREATURE_IDLE);
    lua_register_constant(player->L, CREATURE_WALK);
    lua_register_constant(player->L, CREATURE_HEAL);
    lua_register_constant(player->L, CREATURE_EAT);
    lua_register_constant(player->L, CREATURE_ATTACK);
    lua_register_constant(player->L, CREATURE_CONVERT);
    lua_register_constant(player->L, CREATURE_SPAWN);
    lua_register_constant(player->L, CREATURE_FEED);

    lua_register_constant(player->L, TILE_SOLID);
    lua_register_constant(player->L, TILE_PLAIN);

    lua_register_constant(player->L, CREATURE_SPAWNED);
    lua_register_constant(player->L, CREATURE_KILLED);
    lua_register_constant(player->L, CREATURE_ATTACKED);
    lua_register_constant(player->L, PLAYER_CREATED);

    lua_pushnumber(player->L, playerno);

    lua_setglobal(player->L, "player_number");

    // Initiale Cyclen setzen
    lua_set_cycles(player->L, player->max_cycles);
    player_init_events(player);

    // player.lua sourcen
    if (luaL_loadfile(player->L, PREFIX "player.lua")) {
        fprintf(stderr, "cannot load 'player.lua': %s\n", lua_tostring(player->L, -1));
        goto failed;
    }

    lua_pushstring(player->L, highlevel);

    // starten
    if (lua_pcall(player->L, 1, 0, 0) != 0) {
        fprintf(stderr, "cannot execute 'player.lua': %s\n", lua_tostring(player->L, -1));
        goto failed;
    }
    
    player_to_network(player, PLAYER_DIRTY_ALL, SEND_BROADCAST);

    num_players++;
    
    player_on_created(player);
    return player;

failed:
    lua_close(player->L);
    player->L = NULL;
    return NULL;
}

void player_destroy(player_t *player) {
    creature_kill_all_players_creatures(player);
            
    client_t *client;
    while ((client = player->clients)) {
        player_detach_client(client, player);
    };

    free(player->kill_me);

    lua_close(player->L);
    player->L = NULL;

    num_players--;
    
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
        if (!player->output_client || player->output_client == client)
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

    if (player->bot_output_client == client)
        player->bot_output_client = NULL;

    player->num_clients--;
    assert(player->num_clients >= 0);

    if (player->num_clients == 0)
        player->all_disconnected_time = real_time;

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

int player_execute_code(player_t *player, client_t *output_client, const char *code, size_t codelen, const char *where) {
    int success;
    player->output_client = output_client;
    if (luaL_loadbuffer(player->L, code, codelen, where) == 0) {
        success = player_call_user_lua("during interactive execution", player, 0);
    } else {
        const char *msg = lua_tostring(player->L, -1);
        if (!msg) 
            msg = "(error with no message)";
        player_writeto(player, msg, strlen(msg));
        player_writeto(player, "\r\n", 2); 
        lua_pop(player->L, 1);
        success = 0;
    }
    player->output_client = NULL;
    return success;
}

void player_round() {
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

        // Alle Viecher tot? 
        if (player->num_creatures == 0) 
            player_on_all_dead(player, game_time - player->all_dead_time);
        
        // Killen?
        if (player->kill_me) {
            player_writeto(player, player->kill_me, strlen(player->kill_me));
            player_writeto(player, "\r\n", 2);
            player_destroy(player);
            continue;
        }

        // Zu wenig Score?
        if (player->score <= PLAYER_KICK_SCORE) {
            player_writeto(player, "score too low. try harder!\r\n", 28);
            player_destroy(player);
            continue;
        }

        // Kein Client mehr da & soll in diesem Fall gekickt werden?
        if (player->num_clients == 0 && player->no_client_kick_time &&
            player->all_disconnected_time + player->no_client_kick_time < real_time) 
        {
            player_destroy(player);
            continue;
        }
    }
}

void player_think() {
    int playerno;
    player_t *player = &players[0];
    for (playerno = 0; playerno < MAXPLAYERS; playerno++, player++) {
        if (!PLAYER_USED(player))
            continue;

        // Player Code aufrufen
        lua_pushliteral(player->L, "player_think");
        lua_rawget(player->L, LUA_GLOBALSINDEX);         
        lua_pushliteral(player->L, "events");
        lua_rawget(player->L, LUA_REGISTRYINDEX);
        player->output_client = player->bot_output_client; 
        player_call_user_lua("during botcode execution", player, 1);
        player->output_client = NULL;

        // Events Strukur neu initialisieren
        player_init_events(player);
    }
}

void player_sync() {
    int playerno;
    player_t *player = &players[0];
    for (playerno = 0; playerno < MAXPLAYERS; playerno++, player++) {
        if (!PLAYER_USED(player))
            continue;

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
    
    lua_pushnumber(L, player_num(player));
    lua_pushnumber(L, delta);
    game_call_rule_handler("onKingPlayer", 2);

    if (king_player != old_king)
        player_send_king_update(SEND_BROADCAST);
}

void player_there_is_no_king() {
    player_t *old_king = king_player;
    king_player = NULL;

    game_call_rule_handler("onNoKing", 0);

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
    free(player->kill_me);
    player->kill_me = strdup(luaL_checkstring(L, 2));
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
    player_t *player = player_create(lua_isstring(L, 1) ? luaL_checkstring(L, 1) : NULL,
                                     luaL_checkstring(L, 2), 
                                     luaL_checkstring(L, 3));
    
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
    player_change_score(player, diff, buf);
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
        lua_pushnumber(L, creature_id(creature));
    return 1;
}

static int luaPlayerSetOutputClient(lua_State *L) {
    int success = 1;
    player_t *player = player_get_checked_lua(L, 1);
    if (lua_isnil(L, 2)) {
        player->bot_output_client = NULL;
    } else {
        client_t *output = client_get_checked_lua(L, 2);
        
        client_t *client = player->clients;
        do {
            if (client == output) {
                player->bot_output_client = output;
                goto out;
            }
            client = client->next;
        } while (client != player->clients);
        success = 0;
    }
out:
    lua_pushboolean(L, success);
    return 1;
}

static int luaPlayerSetNoClientKickTime(lua_State *L) {
    player_get_checked_lua(L, 1)->no_client_kick_time = luaL_checklong(L, 2);
    return 0;
}

static int luaPlayerGetNoClientKickTime(lua_State *L) {
    lua_pushnumber(L, player_get_checked_lua(L, 1)->no_client_kick_time);
    return 1;
}

static int luaPlayerIterator(lua_State *L) {
    if (lua_gettop(L) == 0) {
        lua_pushcfunction(L, luaPlayerIterator);
        return 1;
    }

    int pno = lua_isnumber(L, 2) ? lua_tonumber(L, 2) + 1 : 0;
    while (pno >= 0 && pno < MAXPLAYERS) {
        if (PLAYER_USED(&players[pno])) {
            lua_pushnumber(L, pno);
            return 1;
        }
        pno++;
    }
    return 0;
}

static int luaPlayerExecute(lua_State *L) {
    player_t *player = player_get_checked_lua(L, 1);
    client_t *client = lua_isnumber(L, 2) ? client_get_checked_lua(L, 2) : NULL;
    size_t codelen; const char *code = luaL_checklstring(L, 3, &codelen);
    const char *name = luaL_checkstring(L, 4);
    lua_pushboolean(L, player_execute_code(player, client, code, codelen, name));
    return 1;
}

int player_num_players() {
    return num_players;
}

void player_init() {
    memset(players, 0, sizeof(players));

    lua_register(L, "player_create",                 luaPlayerCreate);
    lua_register(L, "player_num_clients",            luaPlayerNumClients);
    lua_register(L, "player_num_creatures",          luaPlayerNumCreatures);
    lua_register(L, "player_kill",                   luaPlayerKill);
    lua_register(L, "player_set_color",              luaPlayerSetColor);
    lua_register(L, "player_set_name",               luaPlayerSetName);
    lua_register(L, "player_get_name",               luaPlayerGetName);
    lua_register(L, "player_set_score",              luaPlayerSetScore);
    lua_register(L, "player_kill_all_creatures",     luaPlayerKillAllCreatures);
    lua_register(L, "player_score",                  luaPlayerScore);
    lua_register(L, "player_exists",                 luaPlayerExists);
    lua_register(L, "player_spawntime",              luaPlayerSpawnTime);
    lua_register(L, "player_change_score",           luaPlayerChangeScore);
    lua_register(L, "player_get_used_mem",           luaPlayerGetUsedMem);
    lua_register(L, "player_get_used_cpu",           luaPlayerGetCPUUsage);
    lua_register(L, "player_set_output_client",      luaPlayerSetOutputClient);
    lua_register(L, "player_set_no_client_kick_time",luaPlayerSetNoClientKickTime);
    lua_register(L, "player_get_no_client_kick_time",luaPlayerGetNoClientKickTime);
    lua_register(L, "player_execute",                luaPlayerExecute);

    lua_register(L, "each_player",                   luaPlayerIterator);

    lua_register(L, "creature_spawn",                luaCreatureSpawn);
    lua_register(L, "creature_get_pos",              luaCreatureGetPos);
    lua_register(L, "creature_get_state",            luaCreatureGetState);
    lua_register(L, "creature_get_nearest_enemy",    luaCreatureGetNearestEnemy);
    lua_register(L, "creature_get_type",             luaCreatureGetType);
    lua_register(L, "creature_get_food",             luaCreatureGetFood);
    lua_register(L, "creature_get_health",           luaCreatureGetHealth);
    lua_register(L, "creature_get_speed",            luaCreatureGetSpeed);
    lua_register(L, "creature_get_tile_food",        luaCreatureGetTileFood);
    lua_register(L, "creature_get_max_food",         luaCreatureGetMaxFood);
    lua_register(L, "creature_get_distance",         luaCreatureGetDistance);
    lua_register(L, "creature_get_player",           luaCreatureGetPlayer);

    lua_register(L, "creature_set_food",             luaCreatureSetFood);
    lua_register(L, "creature_set_type",             luaCreatureSetType);
    lua_register(L, "creature_get_config",           luaCreatureGetConfig);
    lua_register(L, "creature_set_config",           luaCreatureSetConfig);

    lua_register_constant(L, CREATURE_SMALL);
    lua_register_constant(L, CREATURE_BIG);
    lua_register_constant(L, CREATURE_FLYER);
}

void player_game_start() {
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
