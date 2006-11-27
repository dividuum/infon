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

#include <stdio.h>

#include "misc.h"
#include "client_game.h"
#include "client_creature.h"
#include "client_world.h"
#include "client_player.h"
#include "global.h"
#include "client.h"

static char intermission [256] = {0};

void client_game_intermission_from_network(packet_t *packet) {
    snprintf(intermission, sizeof(intermission),
             "%.*s", packet->len, packet->data);
}

const char *client_get_intermission() {
    return intermission;
}

void client_game_init() {
    client_world_init();
    client_player_init();
    client_creature_init();
}

void client_game_shutdown() {
    client_creature_shutdown();
    client_player_shutdown();
    client_world_shutdown();
}
