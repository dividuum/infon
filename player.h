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

#ifndef PLAYER_H
#define PLAYER_H

#include <string.h>

#include <lua.h>

#include "server.h"
#include "packet.h"
#include "common_player.h"

typedef struct player_s {
    char          name[16];
    char          pass[16];
    int           color;
    lua_State    *L;
    int           num_creatures;

    int           num_clients;
    client_t     *clients;

    client_t     *output_client;     // if set, output is limited to this client
    client_t     *bot_output_client; // output client used during botcode execution

    int           score;
    int           spawn_time;

    int           all_dead_time;
    int           all_disconnected_time;
    int           no_client_kick_time;

    int           max_cycles;
    int           cpu_usage;

    int           max_mem;
    int           mem_enforce;

    char         *kill_me;

    unsigned char dirtymask;
} player_t;

typedef enum {
    CREATURE_SPAWNED,
    CREATURE_KILLED,
    CREATURE_ATTACKED,
    PLAYER_CREATED
} vm_event;

struct creature_s;

int         player_num(player_t *player);
player_t   *player_by_num(int playerno);
player_t   *player_get_checked_lua(lua_State *L, int idx);

int         player_attach_client(client_t *client, player_t *player, const char *pass);
int         player_detach_client(client_t *client, player_t *player);

void        player_on_creature_spawned(player_t *player,  struct creature_s *creature, struct creature_s *parent);
void        player_on_creature_killed(player_t *player, struct creature_s *victim, struct creature_s *killer);
void        player_on_creature_attacked(player_t *player, struct creature_s *victim, struct creature_s *attacker);

void        player_writeto(player_t *player, const void *data, size_t size);

void        player_set_name(player_t *player, const char *name);
player_t   *player_create(const char *name, const char *pass, const char *highlevel);

void        player_round();
void        player_think();
void        player_sync();

void        player_is_king_of_the_hill(player_t *player, int delta);
void        player_there_is_no_king();
player_t   *player_king();
int         player_num_players();

/* Network */
void        player_send_initial_update(client_t *client);
void        player_to_network(player_t *player, int dirtymask, client_t *client);

void        player_init();
void        player_game_start();
void        player_shutdown();

#endif
