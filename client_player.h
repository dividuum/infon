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

#include "packet.h"

typedef struct client_player_s {
    int           num;
    int           used;
    void         *userdata;

    char          name[16];
    int           color;

    int           score;
    int           cpu_usage;
} client_player_t;

/* Renderer */
const client_player_t *client_player_get(int num);
const client_player_t *client_player_get_king();
void                   client_player_each(void (*callback)(const client_player_t *player, void *opaque), void *opaque);

/* Network */
void        client_player_from_network(packet_t *packet);
void        client_player_king_from_network(packet_t *packet);

void        client_player_init();
void        client_player_shutdown();

#endif
