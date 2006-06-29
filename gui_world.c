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

static int           world_w;
static int           world_h;

static int           koth_x;
static int           koth_y;

static int          *map_sprites;
static int          *map_food;
static int           displaymode;

void gui_world_draw() {
    for (int y = 0; y < world_h; y++) {
        for (int x = 0; x < world_w; x++) {
            int val;
            switch (displaymode) {
                case 0:
                    video_draw(x * SPRITE_TILE_SIZE, 
                               y * SPRITE_TILE_SIZE, 
                               sprite_get(map_sprites[y * world_w + x]));
                    break;
                case 1:
                    val = (int)MAP_TILE(map_pathfind, x, y)->area;
                    video_rect(x * SPRITE_TILE_SIZE, 
                               y * SPRITE_TILE_SIZE,
                               (x+1) * SPRITE_TILE_SIZE,
                               (y+1) * SPRITE_TILE_SIZE,
                               val % 206, -val / 200, val, 0xFF);
                    break;
                case 2:
                    val = MAP_TILE(map_pathfind, x, y)->region;
                    video_rect(x * SPRITE_TILE_SIZE, 
                               y * SPRITE_TILE_SIZE,
                               (x+1) * SPRITE_TILE_SIZE,
                               (y+1) * SPRITE_TILE_SIZE,
                               val * 206, -val * 200, val, 0xFF);
                    break;
            }

            int food = map_food[y * world_w + x];
            if (food > 0) {
                video_draw(x * SPRITE_TILE_SIZE, 
                           y * SPRITE_TILE_SIZE, 
                           sprite_get(SPRITE_FOOD + food / 1000));
            }
        }
    }
}

void gui_world_set_display_mode(int mode) {
    if (mode >= 0 && mode <= 2)
        displaymode = mode;
}

void gui_world_from_network(packet_t *packet) {
    uint8_t x; uint8_t y;
    if (!packet_read08(packet, &x))         goto failed; 
    if (!packet_read08(packet, &y))         goto failed; 
    if (x >= world_w)                       goto failed;
    if (y >= world_h)                       goto failed;
    uint8_t spriteno;
    if (!packet_read08(packet, &spriteno))  goto failed; 
    if (!sprite_exists(spriteno))           goto failed; 
    map_sprites[x + world_w * y] = spriteno;
    uint16_t food; 
    if (!packet_read16(packet, &food))      goto failed;
    if (food > MAX_TILE_FOOD)               goto failed;
    map_food[x + world_w * y] = food;
    return;
failed:    
    printf("parsing world update packet failed\n");
    return;
}

void gui_world_init(int w, int h) {
    world_w = w;
    world_h = h;

    koth_x = w / 2;
    koth_y = h / 2;

    map_sprites = malloc(w * h * sizeof(int));

    map_food    = malloc(w * h * sizeof(int));
    memset(map_food, 0, w * h * sizeof(int));

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            map_sprites[x + w * y] = SPRITE_BORDER;
        }
    }

}

void gui_world_shutdown() {
    free(map_sprites);
    free(map_food);
}

