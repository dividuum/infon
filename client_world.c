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
static client_maptile_t    *map;
static client_world_info_t  info;

#define MAPTILE(x, y) (map[(y) * info.width + (x)])

const client_world_info_t *client_world_get_info() {
    return initialized ? &info : NULL;
}

const client_maptile_t *client_world_get() {
    return initialized ? map : NULL;
}

const client_maptile_t *client_world_get_tile(int x, int y) {
    return &MAPTILE(x, y);
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
    uint8_t food_type; 
    if (!packet_read08(packet, &food_type)) PROTOCOL_ERROR();

    uint8_t food = food_type & 0x0F;
    if (food > 10)                          PROTOCOL_ERROR();
    MAPTILE(x, y).food = food;

    uint8_t type = (food_type & 0xF0) >> 4;
    if (type >= TILE_LAST_DEFINED)          PROTOCOL_ERROR();
    MAPTILE(x,y).type = type;

    uint8_t gfx;
    if (!packet_read08(packet, &gfx))       PROTOCOL_ERROR();
    if (gfx >= TILE_GFX_LAST_DEFINED)       PROTOCOL_ERROR();
    MAPTILE(x,y).gfx = gfx;
    
    renderer_world_changed(x, y);
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

    map  =  malloc(info.width * info.height * sizeof(client_maptile_t));

    initialized = 1;

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            MAPTILE(x,y).food = 0;
            MAPTILE(x,y).type = TILE_SOLID;
            MAPTILE(x,y).gfx  = TILE_GFX_SOLID;
        }
    }

    renderer_world_info_changed(&info);

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            renderer_world_changed(x, y);
        }
    }
}

void client_world_init() {
    initialized = 0;
}

void client_world_shutdown() {
    client_world_destroy();
}

