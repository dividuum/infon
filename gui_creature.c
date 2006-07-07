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
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "global.h"
#include "gui_creature.h"
#include "gui_world.h"
#include "video.h"

static gui_creature_t creatures[MAXCREATURES];

#define CREATURE_USED(creature) ((creature)->used)

void gui_creature_draw() {
    Uint32 time = SDL_GetTicks();
    gui_creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;

        const int x = X_TO_SCREENX(creature->x) - 7;
        const int y = Y_TO_SCREENY(creature->y) - 7;
        const int hw = creature->health;
        const int fw = creature->food;

        if (fw != 15) video_rect(x + fw, y - 4, x + 15, y - 2, 0x00, 0x00, 0x00, 0xB0);
        if (fw !=  0) video_rect(x,      y - 4, x + fw, y - 2, 0xFF, 0xFF, 0xFF, 0xB0);
                                               
        if (hw != 15) video_rect(x + hw, y - 2, x + 15, y,     0xFF, 0x00, 0x00, 0xB0);
        if (hw !=  0) video_rect(x,      y - 2, x + hw, y,     0x00, 0xFF, 0x00, 0xB0);

        video_draw(x, y, sprite_get(CREATURE_SPRITE(creature->color,
                                                    creature->type,
                                                    creature->dir,
                                                    (time >> 7) % 2)));
        
        //if (game_time > creature->last_state_change +  300 && 
        //    game_time < creature->last_state_change + 1000)
            video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + creature->state));

        //if (time < creature->last_msg_set + 2000) 
            video_tiny(x - strlen(creature->message) * 6 / 2 + 9, y + 14, creature->message);
        
        /*
        if (creature->path) {
            int lastx = X_TO_SCREENX(creature->x);
            int lasty = Y_TO_SCREENY(creature->y);
            pathnode_t *node = creature->path;
            while (node) {
                int curx = node->x * SPRITE_TILE_SIZE / TILE_WIDTH;
                int cury = node->y * SPRITE_TILE_SIZE / TILE_HEIGHT;
                video_line(lastx, lasty, curx, cury);
                lastx = curx, lasty = cury;
                node = node->next;
            }
        }
        */

        if (creature->state == CREATURE_ATTACK) {
            assert(creature->target >= 0 && creature->target < MAXCREATURES);
            const gui_creature_t *target = &creatures[creature->target];
            if (CREATURE_USED(target)) { // Das Angegriffene Vieh koennte in dieser Runde bereits getoetet sein.
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3, Y_TO_SCREENY(target->y) - 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3, Y_TO_SCREENY(target->y) + 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3, Y_TO_SCREENY(target->y) - 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3, Y_TO_SCREENY(target->y) + 3);
            }
        }
    }
}

void gui_creature_move(int delta) {
    gui_creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {

        if (!CREATURE_USED(creature)) 
            continue;

        const int dx = creature->path_x - creature->x;
        const int dy = creature->path_y - creature->y;
        //printf("%d,%d\n", creature->x, creature->y);
        const int dist_to_waypoint = sqrt(dx*dx + dy*dy);
        if (dist_to_waypoint < 16) {
            creature->x = creature->path_x;
            creature->y = creature->path_y;
            continue;
        }

        //printf("%d %d\n", creature->health, gui_creature_speed(creature));
        const int travelled = creature->speed * delta / 1000;

        creature->x += dx * travelled / dist_to_waypoint;
        creature->y += dy * travelled / dist_to_waypoint;

        int winkel_to_waypoint = 16 * ((atan2(-dx, dy == 0 ? 1 : dy) + M_PI) / M_PI);
        int dw = winkel_to_waypoint - creature->dir;

        if (dw < -16) dw += 32;
        if (dw >  16) dw -= 32;

        if (dw < 0) {
            creature->dir -= 1;
        } else if (dw > 0) {
            creature->dir += 1; 
        }

        if (creature->dir >= 32)
            creature->dir -= 32;

        if (creature->dir < 0)
            creature->dir += 32;
    }
}

void gui_creature_from_network(packet_t *packet) {
    uint16_t creatureno;

    if (!packet_read16(packet, &creatureno))    goto failed;
    if (creatureno >= MAXCREATURES)             goto failed;
    gui_creature_t *creature = &creatures[creatureno];

    uint8_t  updatemask;
    if (!packet_read08(packet, &updatemask))    goto failed;

    if (updatemask & CREATURE_DIRTY_ALIVE) {
        uint8_t alive;
        if (!packet_read08(packet, &alive))     goto failed;
        if (alive == 0xFF) {
            creature->used = 0;
            return;
        } else {
            if (alive >= CREATURE_COLORS)       goto failed;
            memset(creature, 0, sizeof(gui_creature_t));
            creature->used = 1;
            creature->color = alive;

            uint16_t x, y;
            if (!packet_read16(packet, &x))     goto failed;
            if (!packet_read16(packet, &y))     goto failed;
            // XXX: x, y checken?
            creature->x = creature->old_x = x * CREATURE_NETWORK_RESOLUTION;
            creature->y = creature->old_y = y * CREATURE_NETWORK_RESOLUTION;
        }
    }
    
    if (!CREATURE_USED(creature))               goto failed;

    if (updatemask & CREATURE_DIRTY_POS) {
        uint16_t x, y;
        if (!packet_read16(packet, &x))         goto failed;
        if (!packet_read16(packet, &y))         goto failed;
        // XXX: x, y checken?
        creature->path_x = x * CREATURE_NETWORK_RESOLUTION;
        creature->path_y = y * CREATURE_NETWORK_RESOLUTION;
    }

    if (updatemask & CREATURE_DIRTY_TYPE) {
        uint8_t type;
        if (!packet_read08(packet, &type))      goto failed;
        if (type >= CREATURE_TYPES)             goto failed;
        creature->type = type;
    }

    if (updatemask & CREATURE_DIRTY_FOOD_HEALTH) {
        uint8_t food_health;
        if (!packet_read08(packet, &food_health)) 
                                                goto failed;
        creature->food   = food_health >> 4;
        creature->health = food_health & 0x0F;
    }

    if (updatemask & CREATURE_DIRTY_STATE) {
        uint8_t state;
        if (!packet_read08(packet, &state))     goto failed;
        if (state >= CREATURE_STATES)           goto failed;
        creature->state = state;
    }

    if (updatemask & CREATURE_DIRTY_TARGET) {
        uint16_t target;
        if (!packet_read16(packet, &target))    goto failed;
        if (target >= MAXCREATURES)             goto failed;
        creature->target = target;
    }

    if (updatemask & CREATURE_DIRTY_MESSAGE) {
        uint8_t len; char buf[256];
        if (!packet_read08(packet, &len))       goto failed;
        if (!packet_readXX(packet, buf, len))   goto failed;
        buf[len] = '\0';
        snprintf(creature->message, sizeof(creature->message), "%s", buf);
        // creature->last_msg_set = SDL_GetTicks();
    }

    if (updatemask & CREATURE_DIRTY_SPEED) {
        uint8_t speed;
        if (!packet_read08(packet, &speed))     goto failed;
        creature->speed = speed * 4;
    }
    return;
failed:
    printf("parsing creature packet failed\n");
}

void gui_creature_init() {
    memset(creatures, 0, sizeof(creatures));
}

void gui_creature_shutdown() {
}

