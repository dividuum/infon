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

#define MAXPLAYERS 32

typedef struct player_s {
    char        name[32];
    char        pass[16];
    int         color;
    lua_State  *L;
    int         num_creatures;

    int         num_clients;
    client_t   *clients;

    int         score;
    int         koth_time;
    int         last_koth_time;

    int         all_dead_time;
    int         all_disconnected_time;

    int         max_cycles;
    int         cycles_left;

    int         kill_me;
} player_t;

player_t players[MAXPLAYERS];

int         player_attach_client(client_t *client, player_t *player, const char *pass);
int         player_detach_client(client_t *client, player_t *player);

void        player_on_creature_spawned(player_t *player,  int idx, int points);
void        player_on_creature_killed(player_t *player,   int victim, int killer);
void        player_on_creature_attacked(player_t *player, int victim, int attacker);

void        player_execute_client_lua(player_t *player, const char *source, const char *where);

player_t   *player_get_checked_lua(lua_State *L, int playerno);
player_t   *player_by_num(int playerno);
int         player_num(player_t *player);
void        player_writeto(player_t *player, const void *data, size_t size);

void        player_set_name(player_t *player, const char *name);
void        player_kill_all_creatures(player_t *player);
player_t   *player_create(const char *pass);
void        player_mark_for_kill(player_t *player);

void        player_think();
void        player_draw(int delta);

void        player_is_king_of_the_hill(player_t *player, int delta);
void        player_there_is_no_king();
player_t   *player_king();

/* Network */
void        player_send_initial_update(client_t *client);

int         packet_player_update_static(player_t *player, packet_t *packet);
int         packet_player_update_round(player_t *player, packet_t *packet);
int         packet_player_joined(player_t *player, packet_t *packet);
int         packet_player_left(player_t *player, packet_t *packet);

void        packet_handle_player_update_static(packet_t *packet);
void        packet_handle_player_update_round(packet_t *packet);
void        packet_handle_player_join_leave(packet_t *packet);

void        player_init();
void        player_shutdown();

#endif
