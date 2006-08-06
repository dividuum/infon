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

#include "gui_world.h"
#include "video.h"
#include "misc.h"
#include "sprite.h"

static int           initialized = 0;

static int           world_w;
static int           world_h;

static int           koth_x;
static int           koth_y;

static int           center_x;
static int           center_y;

static int           offset_x;
static int           offset_y;

typedef struct maptile_s {
    sprite_t    *food;
    sprite_t    *map;
} maptile_t;

static maptile_t *map;

#define MAPTILE(x, y) (map[(y) * world_w + (x)])

void gui_world_draw() {
    if (!initialized)
        return;

    int screen_w  = video_width(); 
    int screen_cx = screen_w / 2;

    int screen_h  = video_height() - 32;
    int screen_cy = screen_h / 2;

    int mapx1 = offset_x = screen_cx  - center_x;
    int x1    = 0;
    if (mapx1 <= -SPRITE_TILE_SIZE) {
        x1     =  -mapx1 / SPRITE_TILE_SIZE;
        mapx1 = -(-mapx1 % SPRITE_TILE_SIZE);
    }
    int x2 = x1 + (screen_w - mapx1) / SPRITE_TILE_SIZE + 1;
    if (x2 > world_w) x2 = world_w;
    int mapx2 = mapx1 + (x2 - x1) * SPRITE_TILE_SIZE;
    
    int mapy1 = offset_y = screen_cy - center_y;
    int y1    = 0;
    if (mapy1 <= -SPRITE_TILE_SIZE) {
        y1    =   -mapy1 / SPRITE_TILE_SIZE;
        mapy1 = -(-mapy1 % SPRITE_TILE_SIZE);
    }
    int y2 = y1 + (screen_h - mapy1) / SPRITE_TILE_SIZE + 1;
    if (y2 > world_h) y2 = world_h;
    int mapy2 = mapy1 + (y2 - y1) * SPRITE_TILE_SIZE;

    int screenx = mapx1;
    int screeny = mapy1;

    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            video_draw(screenx, screeny, MAPTILE(x, y).map);

            sprite_t *food_sprite = MAPTILE(x, y).food;
            if (food_sprite) 
                video_draw(screenx, screeny, food_sprite);
            
            screenx += SPRITE_TILE_SIZE;
        }
        screeny += SPRITE_TILE_SIZE;
        screenx  = mapx1;
    }

    if (mapx1 > 0) 
        video_rect(0, max(0, mapy1), mapx1, min(screen_h, mapy2), 30, 30, 30, 0);
    if (mapy1 > 0) 
        video_rect(0, 0, screen_w, mapy1, 30, 30, 30, 0);
    if (mapx2 <= screen_w) 
        video_rect(mapx2, max(0, mapy1), screen_w, min(screen_h, mapy2), 30, 30, 30, 0);
    if (mapy2 <= screen_h) 
        video_rect(0, mapy2, screen_w, screen_h, 30, 30, 30, 0);
}

static int gui_world_settype(int x, int y, int type) {
    switch (type) {
        case SOLID:
            if (x == 0 || x == world_w - 1 || y == 0 || y == world_h - 1) {
                MAPTILE(x, y).map = sprite_get(SPRITE_BORDER + rand() % SPRITE_NUM_BORDER);
            } else {
                MAPTILE(x, y).map = sprite_get(SPRITE_SOLID  + rand() % SPRITE_NUM_SOLID);
            };
            break;
        case PLAIN:
            if (x == koth_x && y == koth_y) {
                MAPTILE(x, y).map = sprite_get(SPRITE_KOTH);
            } else {
                MAPTILE(x, y).map = sprite_get(SPRITE_PLAIN + rand() % SPRITE_NUM_PLAIN);
            }
            break;
        case WATER:
            MAPTILE(x, y).map = sprite_get(SPRITE_WATER + rand() % SPRITE_NUM_WATER);
            break;
        default:
            // XXX: Unsupported...
            MAPTILE(x, y).map = sprite_get(SPRITE_KOTH);
            return 0;
    }
    return 1;
}

void gui_world_center_change(int dx, int dy) {
    center_x += dx;
    center_y += dy;
}

void gui_world_center() {
    center_x = world_w * SPRITE_TILE_SIZE / 2;
    center_y = world_h * SPRITE_TILE_SIZE / 2;
}

int gui_world_x_offset() {
    return offset_x;
}

int gui_world_y_offset() {
    return offset_y;
}

void gui_world_from_network(packet_t *packet) {
    if (!initialized)                       PROTOCOL_ERROR();
    uint8_t x; uint8_t y;
    if (!packet_read08(packet, &x))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &y))         PROTOCOL_ERROR(); 
    if (x >= world_w)                       PROTOCOL_ERROR();
    if (y >= world_h)                       PROTOCOL_ERROR();
    uint8_t type;
    if (!packet_read08(packet, &type))      PROTOCOL_ERROR(); 
    if (!gui_world_settype(x, y, type))     PROTOCOL_ERROR();
    uint8_t food; 
    if (!packet_read08(packet, &food))      PROTOCOL_ERROR();
    if (food == 0xFF) {
        MAPTILE(x, y).food = NULL;
    } else {
        if (food >= SPRITE_NUM_FOOD)        PROTOCOL_ERROR();
        MAPTILE(x, y).food = sprite_get(SPRITE_FOOD + food);
    }
}

void gui_world_info_from_network(packet_t *packet) {
    if (initialized)                        PROTOCOL_ERROR();
    uint8_t w, h, kx, ky;
    if (!packet_read08(packet, &w))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &h))         PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &kx))        PROTOCOL_ERROR(); 
    if (!packet_read08(packet, &ky))        PROTOCOL_ERROR(); 
    
    world_w = w;
    world_h = h;

    if (koth_x >= world_w)                  PROTOCOL_ERROR();
    if (koth_y >= world_h)                  PROTOCOL_ERROR();

    koth_x  = kx;
    koth_y  = ky;

    map  =  malloc(world_w * world_h * sizeof(maptile_t));
    memset(map, 0, world_w * world_h * sizeof(maptile_t));

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            gui_world_settype(x, y, SOLID);
        }
    }

    gui_world_center();
    initialized = 1;
}

void gui_world_init() {
    center_x = 0;
    center_y = 0;

    offset_x = 0;
    offset_y = 0;
}

void gui_world_shutdown() {
    if (initialized) {
        free(map);
        initialized = 0;
    }
}

