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

#include "world.h"
#include "video.h"
#include "path.h"
#include "sprite.h"

static int           world_w;
static int           world_h;

static int           koth_x;
static int           koth_y;

static map_t        *map_pathfind;
static pathfinder_t  finder;
static sprite_t    **map_sprites;
static int          *map_food;
static maptype_e    *map_type;
static int           displaymode;

void world_draw() {
    for (int y = 0; y < world_h; y++) {
        for (int x = 0; x < world_w; x++) {
            int val;
            switch (displaymode) {
                case 0:
                    video_draw(x * SPRITE_TILE_SIZE, 
                               y * SPRITE_TILE_SIZE, 
                               map_sprites[y * world_w + x]);
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

int world_walkable(int x, int y) {
    return MAP_IS_ON_MAP(map_pathfind, x, y) &&
           map_walkable(map_pathfind, x, y);
}

int world_dig(int x, int y, maptype_e type) {
    if (x < 1 || x >= world_w - 1 ||
        y < 1 || y >= world_h - 1)
        return 0;
    
    if (map_walkable(map_pathfind, x, y))
        return 1;
    printf("world_dig(%d, %d)\n", x, y);

    map_dig(map_pathfind, x, y);
    map_sprites[y * world_w + x] = sprite_get(SPRITE_PLAIN + rand() % SPRITE_NUM_PLAIN);
    
    world_to_network(x, y);
    return 1;
}

void world_set_display_mode(int mode) {
    if (mode >= 0 && mode <= 2)
        displaymode = mode;
}

pathnode_t *world_findpath(int x1, int y1, int x2, int y2) {
    return finder_find(&finder, map_pathfind, x1, y1, x2, y2);
}

int world_get_food(int x, int y) {
    if (!MAP_IS_ON_MAP(map_pathfind, x, y))
        return 0;
    return map_food[y * world_w + x];
}

int world_add_food(int x, int y, int amount) {
    if (!world_walkable(x, y))
        return 0;

    int old = map_food[y * world_w + x];
    int new = old + amount;
    if (new > MAX_TILE_FOOD) new = MAX_TILE_FOOD;
    if (new < 0)             new = 0;
    map_food[y * world_w + x] = new;

    if (new != old) 
        world_to_network(x, y);

    return new - old;
}

int world_food_eat(int x, int y, int amount) {
    if (!MAP_IS_ON_MAP(map_pathfind, x, y))
        return 0;

    int ontile = map_food[y * world_w + x];
    if (amount > ontile)
        amount = ontile;

    map_food[y * world_w + x] -= amount;

    if (amount != 0) 
        world_to_network(x, y);
    
    return amount;
}

int world_width() {
    return world_w;
}

int world_height() {
    return world_h;
}

int world_koth_x() {
    return koth_x;
}

int world_koth_y() {
    return koth_y;
}

void world_find_digged(int *x, int *y) {
    // Terminiert, da kothtile immer freigegraben
    while (1) {
        int xx = rand() % world_w;
        int yy = rand() % world_h;
        if (world_walkable(xx, yy)) {
            *x = xx;
            *y = yy;
            return;
        }
    }
}

void world_send_initial_update(client_t *client) {
    for (int x = 0; x < world_w; x++) {
        for (int y = 0; y < world_h; y++) {
            world_to_network(x, y);
        }
    }
}

void world_to_network(int x, int y) {
    packet_t packet;
    packet_reset(&packet);

    packet_write08(&packet, x);
    packet_write08(&packet, y);
    packet_write08(&packet, sprite_num(map_sprites[x + world_w * y]));
    packet_write16(&packet, map_food[x + world_w * y]);
    packet_send(PACKET_WORLD_UPDATE, &packet);
}

void world_from_network(packet_t *packet) {
    uint8_t x; uint8_t y;
    if (!packet_read08(packet, &x))         goto failed; 
    if (!packet_read08(packet, &y))         goto failed; 
    if (x >= world_w)                       goto failed;
    if (y >= world_h)                       goto failed;
    uint8_t spriteno;
    if (!packet_read08(packet, &spriteno))  goto failed; 
    sprite_t *sprite = sprite_get(spriteno);
    if (!sprite)                            goto failed; 
    map_sprites[x + world_w * y] = sprite;
    uint16_t food; 
    if (!packet_read16(packet, &food))      goto failed;
    if (food > MAX_TILE_FOOD)               goto failed;
    map_food[x + world_w * y] = food;
    return;
failed:    
    printf("parsing world update packet failed\n");
    return;
}

void world_init(int w, int h) {
    world_w = w;
    world_h = h;

    koth_x = w / 2;
    koth_y = h / 2;

    map_pathfind = map_alloc();
    map_init(map_pathfind, w, h);
    finder_init(&finder);

    map_sprites = (sprite_t**)malloc(w * h * sizeof(sprite_t*));
    map_food    = malloc(w * h * sizeof(int));
    memset(map_food, 0, w * h * sizeof(int));

    // Koth Tile freigraben
    world_dig(koth_x, koth_y, SOLID);

    // Tile Texturen setzen
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
                map_sprites[x + w * y] = sprite_get(SPRITE_BORDER + rand() % SPRITE_NUM_BORDER);
            } else if (x == koth_x && y == koth_y) {
                map_sprites[x + w * y] = sprite_get(SPRITE_KOTH);
            } else {
                map_sprites[x + w * y] = sprite_get(SPRITE_SOLID + rand() % SPRITE_NUM_SOLID);
            }
        }
    }

}

void world_shutdown() {
    finder_shutdown(&finder);
    free(map_sprites);
    free(map_food);
    map_free(map_pathfind);
}

