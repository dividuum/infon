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
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "renderer.h"
#include "client_world.h"
#include "misc.h"

static int                  initialized;
static client_world_t       map;
static client_world_info_t  info;

#define MAPTILE(x, y) (map[(y) * info.width + (x)])

const client_world_info_t *client_get_world_info() {
    return initialized ? &info : NULL;
}

const client_world_t client_get_world() {
    return initialized ? map : NULL;
}

void client_world_destroy() {
    if (!initialized)
        return;

    free(map);
    initialized = 0;
}

void client_world_from_network(packet_t *packet) {
    if (!initialized)                       PROTOCOL_ERROR();
    uint8_t x; uint8_t y;
    if (!packet_read08(packet, &x))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &y))         PROTOCOL_ERROR(); 
    if (x >= info.width)                    PROTOCOL_ERROR();
    if (y >= info.height)                   PROTOCOL_ERROR();
    uint8_t type;
    if (!packet_read08(packet, &type))      PROTOCOL_ERROR(); 
    if (type > TILE_WATER)                  PROTOCOL_ERROR();
    MAPTILE(x,y).map = type;
    uint8_t food; 
    if (!packet_read08(packet, &food))      PROTOCOL_ERROR();
    if (food == 0xFF) {
        MAPTILE(x, y).food = -1;
    } else {
        if (food >= 10)                     PROTOCOL_ERROR();
        MAPTILE(x, y).food = food;
    }
}

void client_world_info_from_network(packet_t *packet) {
    if (initialized)    
        client_world_destroy();

    uint8_t w, h, kx, ky;
    if (!packet_read08(packet, &w))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &h))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &kx))        PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &ky))        PROTOCOL_ERROR(); 
    
    info.width  = w;
    info.height = h;
    info.koth_x  = kx;
    info.koth_y  = ky;

    if (info.koth_x >= info.width)         PROTOCOL_ERROR();
    if (info.koth_y >= info.height)        PROTOCOL_ERROR();

    map  =  malloc(info.width * info.height * sizeof(maptile_t));
    memset(map, 0, info.width * info.height * sizeof(maptile_t));

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            MAPTILE(x,y).map = TILE_SOLID;
        }
    }

    initialized = 1;
    renderer_world_change();
}

void client_world_init() {
    initialized = 0;
}

void client_world_shutdown() {
    client_world_destroy();
}

