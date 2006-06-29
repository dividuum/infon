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

#ifndef GUI_PLAYER_H
#define GUI_PLAYER_H

#include <string.h>

#include <lua.h>

#include "packet.h"

#define MAXPLAYERS 32

typedef struct player_s {
    char          name[16];
    char          pass[16];
    int           color;
    lua_State    *L;
    int           num_creatures;

    int           num_clients;
    client_t     *clients;

    int           score;
    int           koth_time;
    int           last_koth_time;

    int           all_dead_time;
    int           all_disconnected_time;

    int           max_cycles;
    int           cpu_usage;

    int           kill_me;

    unsigned char dirtymask;
#define PLAYER_DIRTY_ALIVE   (1 << 0)
#define PLAYER_DIRTY_NAME    (1 << 1)
#define PLAYER_DIRTY_COLOR   (1 << 2)
#define PLAYER_DIRTY_CPU     (1 << 3)
#define PLAYER_DIRTY_SCORE   (1 << 4)

#define PLAYER_DIRTY_ALL         0x1F
#define PLAYER_DIRTY_NONE        0x00
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
player_t   *player_create(const char *pass);
void        player_mark_for_kill(player_t *player);

void        player_think();
void        player_draw();

void        player_is_king_of_the_hill(player_t *player, int delta);
void        player_there_is_no_king();
player_t   *player_king();

/* Network */
void        player_send_initial_update(client_t *client);
void        player_to_network(player_t *player, int dirtymask, client_t *client);
void        player_from_network(packet_t *packet);


void        player_init();
void        player_shutdown();

#endif
