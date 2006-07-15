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

void player_score(player_t *player, int scoredelta, char *reason) {
    static char buf[1024];
    snprintf(buf, sizeof(buf), "%s %s %d point%s: %s",
                               player->name, 
                               scoredelta > 0 ? "gained" : "lost",
                               abs(scoredelta),
                               abs(scoredelta) == 1 ? "" : "s",
                               reason);
                               
    add_to_scroller(buf);
    player->score += scoredelta;
    player->dirtymask |= PLAYER_DIRTY_SCORE;

    printf("stat: %10d %3d '%10s' %5d: %s\n", game_time, player_num(player), player->name, player->score, reason);
}

void player_on_creature_spawned(player_t *player, int idx, int points) {
    player->num_creatures++;
        
    lua_pushliteral(player->L, "_spawned_creatures");
    lua_rawget(player->L, LUA_GLOBALSINDEX);         
    if (!lua_isnil(player->L, -1)) {
        lua_pushnumber(player->L,  game_round);
        lua_rawseti(player->L, -2, idx);
    } else {
        static char msg[] = "cannot locate _spawned_creatures table. did you delete it?\n";
        player_writeto(player, msg, sizeof(msg) - 1);
    }
    lua_pop(player->L, 1);

    if (points > 0)
        player_score(player, points, "Creature Spawned");
}

void player_on_creature_killed(player_t *player, int victim, int killer) {
    lua_pushliteral(player->L, "_killed_creatures");
    lua_rawget(player->L, LUA_GLOBALSINDEX);         
    if (!lua_isnil(player->L, -1)) {
        lua_pushnumber(player->L,  killer);
        lua_rawseti(player->L, -2, victim);
    } else {
        static char msg[] = "cannot locate _killed_creatures table. did you delete it?\n";
        player_writeto(player, msg, sizeof(msg) - 1);
    }
    lua_pop(player->L, 1);

    player->num_creatures--;

    if (player->num_creatures == 0)
        player->all_dead_time = game_time;

    if (killer == victim) {
        player_score(player, CREATURE_SUICIDE_POINTS, "Creature suicides");
    } else if (killer == -1) {
        player_score(player, CREATURE_DIED_POINTS,    "Creature died");
    } else {
        creature_t *victim_creature = creature_by_num(victim);
        if (victim_creature->type == 0) {
            player_score(player, CREATURE_VICTIM_POINTS_0,  "Creature was killed");
        } else {
            player_score(player, CREATURE_VICTIM_POINTS_1,  "Creature was killed");
        }
        creature_t *killer_creature = creature_by_num(killer);
        player_score(killer_creature->player, CREATURE_KILLED_POINTS, "Killed a creature");
    }
}

void player_on_creature_attacked(player_t *player, int victim, int attacker) {
    lua_pushliteral(player->L, "_attacked_creatures");
    lua_rawget(player->L, LUA_GLOBALSINDEX);         
    if (!lua_isnil(player->L, -1)) {
        lua_pushnumber(player->L,  victim);
        lua_rawseti(player->L, -2, attacker);
    } else {
        static char msg[] = "cannot locate _attacked_creatures table. did you delete it?\n";
        player_writeto(player, msg, sizeof(msg) - 1);
    }
    lua_pop(player->L, 1);
}

static int luaPrint(lua_State *L) {
    player_t *player = lua_touserdata(L, lua_upvalueindex(1));
    lua_consumecycles(L, 100);
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
    player_writeto(player, "\n", 1);
    return 0;
}

#define get_player_and_creature()                                  \
    player_t   *player   = lua_touserdata(L, lua_upvalueindex(1)); \
    (void)player; /* Unused Warning weg */                         \
    const int creature_idx     = (int)luaL_checklong(L, 1);        \
    creature_t *creature = creature_by_num(creature_idx);          \
    if (!creature) return luaL_error(L, "%d isn't a valid creature", creature_idx);

#define assure_is_players_creature()                                            \
    do { if (creature->player != player)                                        \
        return luaL_error(L, "%d isn't your creature", creature_idx);           \
    } while(0)                                                                  

static int luaCreatureSuicide(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_consumecycles(L, 100);
    creature_kill(creature, creature);
    return 0;
}

static int luaCreatureSetPath(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_consumecycles(L, 500);
    lua_pushboolean(L, creature_set_path(creature, 
                                         luaL_checklong(L, 2), 
                                         luaL_checklong(L, 3)));
    return 1;
}

static int luaCreatureSetTarget(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_consumecycles(L, 20);
    lua_pushboolean(L, creature_set_target(creature, 
                                           luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetConvert(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_consumecycles(L, 20);
    lua_pushboolean(L, creature_set_conversion_type(creature, 
                                                    luaL_checklong(L, 2)));
    return 1;
}

static int luaCreatureSetMessage(lua_State *L) {
    get_player_and_creature();
    assure_is_players_creature();
    lua_consumecycles(L, 500);
    creature_set_message(creature, luaL_checkstring(L, 2));
    return 0;
}

static int luaCreatureNearestEnemy(lua_State *L) {
    get_player_and_creature();
    if (RESTRICTIVE) assure_is_players_creature();
    lua_consumecycles(L, 500);
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
    const creature_t *target = creature_by_num(luaL_checklong(L, 2));
    if (!target)
        return 0;
    lua_consumecycles(L, 100);
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

static int luaAreaSize(lua_State *L) {
    lua_pushnumber(L, TILE_X1(1));
    lua_pushnumber(L, TILE_Y1(1));
    lua_pushnumber(L, TILE_X2(world_width()  - 2));
    lua_pushnumber(L, TILE_Y1(world_height() - 2));
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

static int luaPlayerScore(lua_State *L) {
    lua_pushnumber(L, player_get_checked_lua(L, luaL_checklong(L, 1))->score);
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

#define lua_register_player(p,n,f) \
    (lua_pushstring((p)->L, n), \
     lua_pushlightuserdata((p)->L, p), \
     lua_pushcclosure((p)->L, f, 1), \
     lua_settable((p)->L, LUA_GLOBALSINDEX))    

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
    player->L = lua_open();

    player->color         = playerno  % CREATURE_COLORS;
    player->all_dead_time = game_time - PLAYER_CREATURE_RESPAWN_DELAY;
    player->all_disconnected_time = game_time;

    player->max_cycles    = LUA_MAX_CPU;
    
    lua_baselibopen(player->L);
    lua_dblibopen(player->L);
    lua_tablibopen(player->L);
    lua_strlibopen(player->L);
    lua_mathlibopen(player->L);

    lua_register_player(player, "print",        luaPrint);
    lua_register_player(player, "suicide",      luaCreatureSuicide);
    lua_register_player(player, "set_path",     luaCreatureSetPath);
    lua_register_player(player, "get_pos",      luaCreatureGetPos);
    lua_register_player(player, "get_state",    luaCreatureGetState);
    lua_register_player(player, "set_state",    luaCreatureSetState);
    lua_register_player(player, "set_target",   luaCreatureSetTarget);
    lua_register_player(player, "set_convert",  luaCreatureSetConvert);
    lua_register_player(player, "nearest_enemy",luaCreatureNearestEnemy);
    lua_register_player(player, "get_type",     luaCreatureGetType);
    lua_register_player(player, "get_food",     luaCreatureGetFood);
    lua_register_player(player, "get_health",   luaCreatureGetHealth);
    lua_register_player(player, "get_speed",    luaCreatureGetSpeed);
    lua_register_player(player, "get_tile_food",luaCreatureGetTileFood);
    lua_register_player(player, "get_max_food", luaCreatureGetMaxFood);
    lua_register_player(player, "get_distance", luaCreatureGetDistance);
    lua_register_player(player, "set_message",        luaCreatureSetMessage);
    lua_register_player(player, "get_hitpoints",      luaCreatureGetHitpoints);
    lua_register_player(player, "get_attack_distance",luaCreatureGetAttackDistance);

    lua_register(player->L,     "world_size",       luaAreaSize);
    lua_register(player->L,     "game_time",        luaGameTime);
    lua_register(player->L,     "get_koth_pos",     luaGetKothPos);
    lua_register(player->L,     "exists",           luaCreatureExists);
    lua_register(player->L,     "creature_exists",  luaCreatureExists);
    lua_register(player->L,     "player_exists",    luaPlayerExists);
    lua_register(player->L,     "king_player",      luaKingPlayer);
    lua_register(player->L,     "player_score",     luaPlayerScore);

    lua_pushliteral(player->L, "CREATURE_IDLE");
    lua_pushnumber(player->L,   CREATURE_IDLE);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_WALK");
    lua_pushnumber(player->L,   CREATURE_WALK);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_HEAL");
    lua_pushnumber(player->L,   CREATURE_HEAL);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_EAT");
    lua_pushnumber(player->L,   CREATURE_EAT);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_ATTACK");
    lua_pushnumber(player->L,   CREATURE_ATTACK);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_CONVERT");
    lua_pushnumber(player->L,   CREATURE_CONVERT);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_SPAWN");
    lua_pushnumber(player->L,   CREATURE_SPAWN);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "CREATURE_FEED");
    lua_pushnumber(player->L,   CREATURE_FEED);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(player->L, "player_number");
    lua_pushnumber(player->L,   playerno);
    lua_settable(player->L, LUA_GLOBALSINDEX);

    lua_pushliteral(L, "MAXPLAYERS");
    lua_pushnumber(L,   MAXPLAYERS);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_setmaxmem(player->L, LUA_MAX_MEM);
    lua_dofile(player->L, "player.lua");
    
    player_to_network(player, PLAYER_DIRTY_ALL, SEND_BROADCAST);
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

void player_mark_for_kill(player_t *player) {
    player->kill_me = 1;
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
    if (!PLAYER_USED(&players[playerno])) 
        return NULL;
    return &players[playerno];
}

player_t *player_get_checked_lua(lua_State *L, int playerno) {
    if (playerno < 0 || playerno >= MAXPLAYERS) 
        luaL_error(L, "player number %d out of range", playerno);

    if (!PLAYER_USED(&players[playerno])) 
        luaL_error(L, "player %d not in use", playerno);

    return &players[playerno];
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

void player_execute_client_lua(player_t *player, const char *source, const char *where) {
    int status = luaL_loadbuffer(player->L, source, strlen(source), where);
    
    if (status == 0) {
        int base = lua_gettop(player->L);
        lua_pushliteral(player->L, "_TRACEBACK");
        lua_rawget(player->L, LUA_GLOBALSINDEX);
        lua_insert(player->L, base);
        lua_setcycles(player->L, player->max_cycles);
        status = lua_pcall(player->L, 0, 1, base); // Yield() faehig machen?
        lua_remove(player->L, base);
    }

    const char *msg = lua_tostring(player->L, -1);

    if (status && !msg) 
        msg = "(error with no message)";

    if (msg) { 
        if (strstr(msg, "attempt to yield across C function")) {
            player_writeto(player, msg, strlen(msg));
            player_writeto(player, "\n\n", 2); 
            player_writeto(player, "--------------------------------------------------------------\n", 63);
            player_writeto(player, "Long running operations are not allowed from interactive shell\n", 63);
            player_writeto(player, "--------------------------------------------------------------\n", 63);
        } else {
            player_writeto(player, msg, strlen(msg));
            player_writeto(player, "\n", 1); 
        }
    }
    lua_pop(player->L, 1);
}

void player_think() {
    static char errorbuf[1000];
    int playerno;
    player_t *player = &players[0];
    for (playerno = 0; playerno < MAXPLAYERS; playerno++, player++) {
        if (!PLAYER_USED(player))
            continue;
        
        // Killen?
        if (player->kill_me) {
            player_writeto(player, "killed\n", 7);
            player_destroy(player);
            continue;
        }

        // Zu wenig Score?
        if (player->score <= PLAYER_KICK_SCORE) {
            player_writeto(player, "score too low. try harder!\n", 27);
            player_destroy(player);
            continue;
        }

        // Kein Client mehr da?
        if (player->num_clients == 0 && 
            player->all_disconnected_time + NO_PLAYER_DISCONNECT < game_time) 
        {
            player_destroy(player);
            continue;
        }

        // Alle Viecher tot? neu spawnen.
        if (player->num_creatures == 0 && 
            game_time > player->all_dead_time + PLAYER_CREATURE_RESPAWN_DELAY) 
        {
            int x, y;
            world_find_digged(&x, &y);
            creature_spawn(player, TILE_XCENTER(x), TILE_YCENTER(y), 0, 0);
            world_find_digged(&x, &y);
            creature_spawn(player, TILE_XCENTER(x), TILE_YCENTER(y), 0, 0);
            //creature_t * first = creature_spawn(player, TILE_XCENTER(world_koth_x()), TILE_YCENTER(world_koth_y()), 1, 0);
            // first->food = creature_max_food(first);
        }
        
        // Spiele Luacode aufrufen
        lua_pushliteral(player->L, "_TRACEBACK");
        lua_rawget(player->L, LUA_GLOBALSINDEX); 
        
        lua_pushliteral(player->L, "player_think");
        lua_rawget(player->L, LUA_GLOBALSINDEX);         
        
        lua_setcycles(player->L, player->max_cycles);

        switch (lua_pcall(player->L, 0, 0, -2)) {
            case LUA_ERRRUN:
                snprintf(errorbuf, sizeof(errorbuf), "runtime error: %s", lua_tostring(player->L, -1));
                lua_pop(player->L, 1);
                player_writeto(player, errorbuf, strlen(errorbuf));
                break;
            case LUA_ERRMEM:
                snprintf(errorbuf, sizeof(errorbuf), "mem error: %s", lua_tostring(player->L, -1));
                lua_pop(player->L, 1);
                player_writeto(player, errorbuf, strlen(errorbuf));
                break;
            case LUA_ERRERR:
                snprintf(errorbuf, sizeof(errorbuf), "error calling errorhandler (_TRACEBACK)");
                lua_pop(player->L, 1);
                player_writeto(player, errorbuf, strlen(errorbuf));
                break;
            default: 
                break;
        }

        int cycles_left = lua_getcycles(player->L);

        if (cycles_left < 0)
            cycles_left = 0;

        int newcpu = 100 * (player->max_cycles - cycles_left) / player->max_cycles;

        if (newcpu != player->cpu_usage) {
            player->dirtymask |= PLAYER_DIRTY_CPU;
            player->cpu_usage = newcpu;
        }

        lua_pop(player->L, 1);

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
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player_mark_for_kill(player);
    return 0;
}

static int luaPlayerNumClients(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    lua_pushnumber(L, player->num_clients);
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
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player_set_name(player, luaL_checkstring(L, 2));
    return 0;
}

static int luaPlayerGetName(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    lua_pushstring(L, player->name);
    return 1;
}

static int luaPlayerSetScore(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player->score = luaL_checklong(L, 2);
    return 0;
}

static int luaPlayerKillAllCreatures(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    creature_kill_all_players_creatures(player);
    return 0;
}


void player_init() {
    memset(players, 0, sizeof(players));

    lua_register(L, "player_create",                luaPlayerCreate);
    lua_register(L, "player_num_clients",           luaPlayerNumClients);
    lua_register(L, "player_kill",                  luaPlayerKill);
    lua_register(L, "player_set_name",              luaPlayerSetName);
    lua_register(L, "player_get_name",              luaPlayerGetName);
    lua_register(L, "player_set_score",             luaPlayerSetScore);
    lua_register(L, "player_kill_all_creatures",    luaPlayerKillAllCreatures);
}

void player_shutdown() {
    int playerno;
    for (playerno = 0; playerno < MAXPLAYERS; playerno++) {
        if (PLAYER_USED(&players[playerno]))
            player_destroy(&players[playerno]);
    }
}
