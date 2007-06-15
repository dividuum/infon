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

#ifndef CREATURE_H
#define CREATURE_H

#include <lua.h>

#include "path.h"
#include "player.h"
#include "server.h"
#include "common_creature.h"
#include "common_world.h"

typedef struct creature_s {
    int             x;
    int             y;
    creature_type   type;
    int             food;
    int             health;
    player_t       *player;
    int             target_id;
    pathnode_t     *path;
    int             convert_food;
    creature_type   convert_type;
    int             spawn_food;
    creature_state  state;
    int             suicide;

    int             age_action_deltas;
    int             spawn_time;

    char            message[9];
    unsigned char   dirtymask;

    int             network_food_health;
    int             network_state;
    int             network_path_x;
    int             network_path_y;
    int             network_speed;
    int             network_target;

    // Letzte uebermittelte Koordinate (fuer Delta Kompression)
    int             network_last_x;
    int             network_last_y;

    int                 vm_id;
    struct creature_s  *hash_next;
} creature_t;

creature_t *creature_by_id(int creature_num);
int         creature_id(const creature_t *creature);

creature_t *creature_get_checked_lua(lua_State *L, int idx);

creature_t *creature_spawn(player_t *player, creature_t *parent, int x, int y, creature_type type);
void        creature_kill(creature_t *creature, creature_t *killer);

int         creature_set_path(creature_t *creature, int x, int y);
int         creature_set_health(creature_t *creature, int health);
int         creature_set_target(creature_t *creature, int target);
int         creature_set_state(creature_t *creature, int state);
int         creature_set_conversion_type(creature_t *creature, creature_type type);
void        creature_set_message(creature_t *creature, const char *message);
int         creature_set_food(creature_t *creature, int food);
int         creature_set_type(creature_t *creature, creature_type type);
int         creature_suicide(creature_t *creature);

creature_t *creature_nearest_enemy(const creature_t *reference, int *distptr);
int         creature_max_health(const creature_t *creature);
int         creature_speed(const creature_t *creature);
int         creature_dist(const creature_t *a, const creature_t *b);
int         creature_food_on_tile(const creature_t *creature);
maptype_e   creature_tile_type(const creature_t *creature);
int         creature_max_food(const creature_t *creature);

void        creature_kill_all_players_creatures(player_t *player);
void        creature_moveall(int delta);
int         creature_num_creatures();

/* Network */
void        creature_send_initial_update(client_t *client);
void        creature_to_network(creature_t *creature, int dirtymask, client_t *client);

int         luaCreatureGetConfig(lua_State *L); 
int         luaCreatureSetConfig(lua_State *L);
int         luaCreatureConfigChanged(lua_State *L);

void        creature_init();
void        creature_shutdown();

#endif
