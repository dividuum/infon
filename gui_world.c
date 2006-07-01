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

static int           world_w;
static int           world_h;

static int           koth_x;
static int           koth_y;

static sprite_t    **map_sprites  = NULL;
static sprite_t    **food_sprites = NULL;

void gui_world_draw() {
    for (int y = 0; y < world_h; y++) {
        for (int x = 0; x < world_w; x++) {
            video_draw(x * SPRITE_TILE_SIZE, 
                       y * SPRITE_TILE_SIZE, 
                       map_sprites[y * world_w + x]);

            sprite_t *food_sprite = food_sprites[y * world_w + x];
            if (food_sprite) {
                video_draw(x * SPRITE_TILE_SIZE, 
                           y * SPRITE_TILE_SIZE, 
                           food_sprite);
            }
        }
    }
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
    map_sprites[x + world_w * y] = sprite_get(spriteno);
    uint8_t food; 
    if (!packet_read08(packet, &food))      goto failed;
    if (food == 0xFF) {
        food_sprites[x + world_w * y] = NULL;
    } else {
        if (food >= SPRITE_NUM_FOOD)        goto failed;
        food_sprites[x + world_w * y] = sprite_get(SPRITE_FOOD + food);
    }
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

    map_sprites  = malloc(w * h * sizeof(sprite_t*));
    food_sprites = malloc(w * h * sizeof(sprite_t*));

    memset(food_sprites, 0, w * h * sizeof(int));

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            map_sprites[x + w * y] = sprite_get(SPRITE_BORDER);
        }
    }
}

void gui_world_shutdown() {
    free(map_sprites);
    free(food_sprites);
}

