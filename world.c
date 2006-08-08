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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <lauxlib.h>

#include "global.h"
#include "world.h"
#include "sprite.h"
#include "path.h"
#include "misc.h"

static int           world_w;
static int           world_h;

static int           koth_x;
static int           koth_y;

// TODO: Jeder Kreaturtype sollte eigene Pfadsuche haben
static map_t        *walkmap;
static pathfinder_t  finder;

typedef struct maptile_s {
    int         food;
    maptype_e   type;
} maptile_t;

static maptile_t *map;

#define MAPTILE(x, y) (map[(y) * world_w + (x)])

int world_is_on_map(int x, int y) {
    return x >= 0 && x < world_w &&
           y >= 0 && y < world_h;
}

int world_is_within_border(int x, int y) {
    return x >= 1 && x < world_w - 1 &&
           y >= 1 && y < world_h - 1;
}

int world_walkable(int x, int y) {
    return world_is_on_map(x, y) &&
           map_walkable(walkmap, x, y);
}

int world_dig(int x, int y, maptype_e type) {
    if (type == SOLID)
        return 0;

    if (!world_is_within_border(x, y))
        return 0;

    if (MAPTILE(x, y).type != SOLID)
        return 0;
    
    // fprintf(stderr, "world_dig(%d, %d, %d)\n", x, y, type);

    // Pfadsuche fuer Bodenbasierte Viecher aktualisieren
    if (type == PLAIN)
        map_dig(walkmap, x, y);
    
    MAPTILE(x, y).type = type;
    world_to_network(x, y, SEND_BROADCAST);
    return 1;
}

pathnode_t *world_findpath(int x1, int y1, int x2, int y2) {
    return finder_find(&finder, walkmap, x1, y1, x2, y2);
}

int world_get_food(int x, int y) {
    if (!world_is_on_map(x, y))
        return 0;

    return MAPTILE(x, y).food;
}

static int food_to_network_food(int food) {
    return food == 0 ? 0xFF : food / 1000;
}

int world_add_food(int x, int y, int amount) {
    if (!world_is_on_map(x, y))
        return 0;

    if (MAPTILE(x, y).type != PLAIN)
        return 0;

    int old = MAPTILE(x, y).food;
    int new = old + amount;
    if (new > MAX_TILE_FOOD) new = MAX_TILE_FOOD;
    if (new < 0)             new = 0;
    MAPTILE(x, y).food = new;

    if (food_to_network_food(new) != food_to_network_food(old))
        world_to_network(x, y, SEND_BROADCAST);

    return new - old;
}

int world_food_eat(int x, int y, int amount) {
    if (!world_is_on_map(x, y))
        return 0;

    if (MAPTILE(x, y).type != PLAIN)
        return 0;

    int ontile = MAPTILE(x, y).food;
    if (amount > ontile)
        amount = ontile;

    int old = MAPTILE(x, y).food;
    int new = MAPTILE(x, y).food -= amount;

    if (food_to_network_food(new) != food_to_network_food(old))
        world_to_network(x, y, SEND_BROADCAST);
    
    return amount;
}

int world_width() {
    return world_w;
}

int world_height() {
    return world_h;
}

int world_koth_x() {
    return koth_x;
}

int world_koth_y() {
    return koth_y;
}

void world_find_plain(int *x, int *y) {
    // Terminiert, da kothtile immer freigegraben
    while (1) {
        int xx = rand() % world_w;
        int yy = rand() % world_h;
        if (MAPTILE(xx, yy).type == PLAIN) {
            *x = xx;
            *y = yy;
            return;
        }
    }
}

void world_send_initial_update(client_t *client) {
    packet_t packet;
    packet_init(&packet, PACKET_WORLD_INFO);
    packet_write08(&packet, world_w);
    packet_write08(&packet, world_h);
    packet_write08(&packet, koth_x);
    packet_write08(&packet, koth_y);
    server_send_packet(&packet, client);

    for (int x = 0; x < world_w; x++) {
        for (int y = 0; y < world_h; y++) {
            world_to_network(x, y, client);
        }
    }
}

void world_to_network(int x, int y, client_t *client) {
    packet_t packet;
    packet_init(&packet, PACKET_WORLD_UPDATE);

    packet_write08(&packet, x);
    packet_write08(&packet, y);
    packet_write08(&packet, MAPTILE(x, y).type);
    packet_write08(&packet, food_to_network_food(MAPTILE(x, y).food));
    server_send_packet(&packet, client);
}

void world_tick() {
    lua_pushliteral(L, "world_tick");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling world_tick: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static int luaWorldDig(lua_State *L) {
    lua_pushboolean(L, world_dig(luaL_checklong(L, 1), 
                                 luaL_checklong(L, 2),
                                 luaL_checklong(L, 3)));
    return 1;
}

static int luaWorldAddFood(lua_State *L) {
    lua_pushnumber(L, world_add_food(luaL_checklong(L, 1), 
                                     luaL_checklong(L, 2), 
                                     luaL_checklong(L, 3)));
    return 1;
}

static int luaWorldWalkable(lua_State *L) {
    lua_pushboolean(L, world_walkable(luaL_checklong(L, 1), luaL_checklong(L, 2)));
    return 1;
}

static int luaWorldFindDigged(lua_State *L) {
    int x, y;
    world_find_plain(&x, &y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int luaWorldPosToTile(lua_State *L) {
    int x = luaL_checklong(L, 1);
    int y = luaL_checklong(L, 2);
    lua_pushnumber(L, X_TO_TILEX(x));
    lua_pushnumber(L, Y_TO_TILEY(y));
    return 2;
}

void world_init() {
    lua_register(L, "world_dig",            luaWorldDig);
    lua_register(L, "world_add_food",       luaWorldAddFood);
    lua_register(L, "world_is_walkable",    luaWorldWalkable);
    lua_register(L, "world_find_digged",    luaWorldFindDigged);
    lua_register(L, "world_pos_to_tile",    luaWorldPosToTile);

    lua_pushnumber(L, SOLID);
    lua_setglobal(L, "SOLID"); 

    lua_pushnumber(L, PLAIN);
    lua_setglobal(L, "PLAIN"); 

    lua_pushnumber(L, WATER);
    lua_setglobal(L, "WATER"); 
    
    lua_pushliteral(L, "world_init");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 4, 0) != 0) {
        fprintf(stderr, "error calling world_init: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    world_w = lua_tonumber(L, -4);
    world_h = lua_tonumber(L, -3);

    koth_x  = lua_tonumber(L, -2);
    koth_y  = lua_tonumber(L, -1);

    lua_pop(L, 4);

    if (world_w < 10 || world_w > 255 ||
        world_h < 10 || world_h > 255)
        die("world size invalid: %d x %d", world_w, world_h);

    if (koth_x <= 0 || koth_x >= world_w - 1 ||
        koth_y <= 0 || koth_y >= world_h - 1)
        die("koth pos invalid: %d, %d", koth_x, koth_y);
    
    walkmap = map_alloc();
    map_init(walkmap, world_w, world_h);
    finder_init(&finder);

    map  =  malloc(world_w * world_h * sizeof(maptile_t));
    memset(map, 0, world_w * world_h * sizeof(maptile_t));

    // Koth Tile freigraben
    world_dig(koth_x, koth_y, PLAIN);
}

void world_shutdown() {
    finder_shutdown(&finder);
    free(map);
    map_free(walkmap);
}

