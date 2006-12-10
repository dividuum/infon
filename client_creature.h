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

#ifndef CLIENT_CREATURE_H
#define CLIENT_CREATURE_H

#include "common_creature.h"
#include "packet.h"

typedef struct client_pathnode_s client_pathnode_t;

struct client_pathnode_s {
    int                x;
    int                y;
    int                beam;
    client_pathnode_t *next;
};

typedef struct client_creature_s {
    int             num;
    int             vm_id;
    int             used;
    void           *userdata;

    int             x;
    int             y;
    int             speed;
    int             player;

    int             dir;
    creature_type   type;
    int             food;
    int             health;
    int             target;
    creature_state  state;

    client_pathnode_t *path;
    client_pathnode_t *last;
    int                pathlen;

    int             last_x;
    int             last_y;

    char            message[9];
    int             smile_time;
} client_creature_t;

/* Renderer */
const client_creature_t *client_creature_get(int num);
void                     client_creature_each(void (*callback)(const client_creature_t *creature, void *opaque), void *opaque);

/* Movement */
void        client_creature_move(int delta);

/* Network */
void        client_creature_from_network(packet_t *packet);
void        client_creature_smile_from_network(packet_t *packet);

void        client_creature_init();
void        client_creature_shutdown();

#endif
