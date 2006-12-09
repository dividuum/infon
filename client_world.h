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

#ifndef CLIENT_WORLD_H
#define CLIENT_WORLD_H

#include "packet.h"
#include "common_world.h"

typedef struct maptile_s {
    int       food;
    maptype_e type;
    mapgfx_e  gfx;
} client_maptile_t;

typedef struct world_info_s {
    int width;
    int height;

    int koth_x;
    int koth_y;
} client_world_info_t;

/* Renderer */

const client_world_info_t *client_world_get_info();
const client_maptile_t    *client_world_get();
const client_maptile_t    *client_world_get_tile(int x, int y);

/* Network */
void        client_world_from_network(packet_t *packet);
void        client_world_info_from_network(packet_t *packet);

void        client_world_init();
void        client_world_shutdown();

#endif
