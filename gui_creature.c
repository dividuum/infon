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
#include <string.h>
#include <assert.h>

#include "global.h"
#include "gui_creature.h"
#include "gui_world.h"
#include "common_player.h"
#include "video.h"
#include "misc.h"
#include "map.h"

static gui_creature_t creatures[MAXCREATURES];

#define CREATURE_USED(creature) ((creature)->used)

void gui_creature_draw() {
    const int xoff = gui_world_x_offset();
    const int yoff = gui_world_y_offset();

    const int max_x = video_width()  + 10;
    const int max_y = video_height() - 32 + 10;
    
    Uint32 time = SDL_GetTicks();
    gui_creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;

        const int x = X_TO_SCREENX(creature->x) - 7 + xoff;
        const int y = Y_TO_SCREENY(creature->y) - 7 + yoff;

        if (x < -30 || x > max_x || y < -20 || y > max_y)
            continue;

        const int hw = creature->health;
        const int fw = creature->food;

        if (fw != 15) video_rect(x + fw, y - 4, x + 15, y - 2, 0x00, 0x00, 0x00, 0xB0);
        if (fw !=  0) video_rect(x,      y - 4, x + fw, y - 2, 0xFF, 0xFF, 0xFF, 0xB0);
                                               
        if (hw != 15) video_rect(x + hw, y - 2, x + 15, y,     0xFF, 0x00, 0x00, 0xB0);
        if (hw !=  0) video_rect(x,      y - 2, x + hw, y,     0x00, 0xFF, 0x00, 0xB0);

        video_draw(x, y, sprite_get(CREATURE_SPRITE(creature->player,
                                                    creature->type,
                                                    creature->dir,
                                                    (time >> 7) % 2)));
        
        //if (game_time > creature->last_state_change +  300 && 
        //    game_time < creature->last_state_change + 1000)
        if (creature->smile_time + 1000 > game_time) {
            video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + 8));
        } else {
            video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + creature->state));
        }

        //if (time < creature->last_msg_set + 2000) 
            video_tiny(x - strlen(creature->message) * 6 / 2 + 9, y + 14, creature->message);
            
        //{ 
        //  char debug[10]; sprintf(debug, "%d", creature->pathlen); 
        //  video_tiny(x - strlen(creature->message) * 6 / 2 + 9, y + 14, debug);
        //}
        
        //if (creature->path) {
        //    int lastx = X_TO_SCREENX(creature->x);
        //    int lasty = Y_TO_SCREENY(creature->y);
        //    gui_pathnode_t *node = creature->path;
        //    while (node) {
        //        int curx = node->x * SPRITE_TILE_SIZE / TILE_WIDTH;
        //        int cury = node->y * SPRITE_TILE_SIZE / TILE_HEIGHT;
        //        video_line(lastx, lasty, curx, cury);
        //        lastx = curx, lasty = cury;
        //        node = node->next;
        //    }
        //}

        if (creature->state == CREATURE_ATTACK) {
            assert(creature->target >= 0 && creature->target < MAXCREATURES);
            const gui_creature_t *target = &creatures[creature->target];
            if (CREATURE_USED(target)) { // Das Angegriffene Vieh koennte in dieser Runde bereits getoetet sein.
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3 + xoff, Y_TO_SCREENY(target->y) - 3 + yoff);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3 + xoff, Y_TO_SCREENY(target->y) + 3 + yoff);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3 + xoff, Y_TO_SCREENY(target->y) - 3 + yoff);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3 + xoff, Y_TO_SCREENY(target->y) + 3 + yoff);
            }
        }
        //gui_world_center_set(creature->x * SPRITE_TILE_SIZE / TILE_WIDTH,
        //                     creature->y * SPRITE_TILE_SIZE / TILE_HEIGHT);
    }
}

static void gui_creature_add_path(gui_creature_t *creature, int x, int y, int beam) {
    gui_pathnode_t *node = malloc(sizeof(gui_pathnode_t));

    node->x    = x;
    node->y    = y;
    node->beam = beam;
    node->next = NULL;

    if (creature->pathlen == 0) {
        assert(!creature->last && !creature->path);
        creature->last = creature->path = node;
    } else {
        assert(creature->last);
        assert(creature->path);
        assert(creature->last->next == NULL);
        creature->last = creature->last->next = node;
    } 

    creature->pathlen++;
}

static void gui_creature_del_path(gui_creature_t *creature) {
    assert(creature->pathlen > 0);
    assert(creature->path && creature->last);
    gui_pathnode_t *tmp = creature->path;
    creature->path = creature->path->next;
    if (--creature->pathlen == 0) {
        assert(creature->last == tmp);
        creature->last = NULL;
    }
    free(tmp);
}
        
static void gui_creature_kill(gui_creature_t *creature) {
    while (creature->path) 
        gui_creature_del_path(creature);
    creature->used = 0;
}

void gui_creature_move(int delta) {
    gui_creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {

        if (!CREATURE_USED(creature)) 
            continue;

        int travelled = (creature->speed + 150 * max(0, creature->pathlen - 1)) * delta / 1000;
again:        
        if (!creature->path)
            continue;

        if (creature->path->beam) {
            creature->x = creature->path->x;
            creature->y = creature->path->y;
            gui_creature_del_path(creature);
            goto again;
        }

        const int dx = creature->path->x - creature->x;
        const int dy = creature->path->y - creature->y;
        const int dist_to_waypoint = sqrt(dx*dx + dy*dy);

        if (travelled >= dist_to_waypoint) {
            creature->x = creature->path->x;
            creature->y = creature->path->y;
            travelled  -= dist_to_waypoint;
            gui_creature_del_path(creature);
            goto again;
        }

        creature->x += dx * travelled / dist_to_waypoint;
        creature->y += dy * travelled / dist_to_waypoint;

        int winkel_to_waypoint = 16 * ((atan2(-dx, dy == 0 ? 1 : dy) + M_PI) / M_PI);
        int dw = creature->dir - winkel_to_waypoint;

        if (dw < -16) dw += 32;
        if (dw >  16) dw -= 32;

        if (dw < -1) {
            creature->dir += 2;
        } else if (dw > 1) {
            creature->dir -= 2; 
        } else {
            creature->dir = winkel_to_waypoint;
        }

        if (creature->dir >= 32)
            creature->dir -= 32;

        if (creature->dir < 0)
            creature->dir += 32;
    }
}

void gui_creature_smile_from_network(packet_t *packet) {
    uint16_t creatureno;

    if (!packet_read16(packet, &creatureno))    PROTOCOL_ERROR();
    if (creatureno >= MAXCREATURES)             PROTOCOL_ERROR();
    gui_creature_t *creature = &creatures[creatureno];
    if (!CREATURE_USED(creature))               PROTOCOL_ERROR();
    creature->smile_time = game_time;
}

void gui_creature_from_network(packet_t *packet) {
    uint16_t creatureno;

    if (!packet_read16(packet, &creatureno))    PROTOCOL_ERROR();
    if (creatureno >= MAXCREATURES)             PROTOCOL_ERROR();
    gui_creature_t *creature = &creatures[creatureno];

    uint8_t  updatemask;
    if (!packet_read08(packet, &updatemask))    PROTOCOL_ERROR();

    if (updatemask & CREATURE_DIRTY_ALIVE) {
        uint8_t playerno;
        if (!packet_read08(packet, &playerno))  PROTOCOL_ERROR();
        if (playerno == 0xFF) {
            if (!CREATURE_USED(creature))       PROTOCOL_ERROR();
            gui_creature_kill(creature);
            return;
        } else {
            if (CREATURE_USED(creature))        PROTOCOL_ERROR();
            memset(creature, 0, sizeof(gui_creature_t));
            if (playerno >= MAXPLAYERS)         PROTOCOL_ERROR();
            creature->player = playerno;

            uint16_t x, y;
            if (!packet_read16(packet, &x))     PROTOCOL_ERROR();
            if (!packet_read16(packet, &y))     PROTOCOL_ERROR();

            creature->last_x = x;
            creature->last_y = y;
            creature->used   = 1;

            // XXX: x, y checken?
            gui_creature_add_path(creature,
                                  creature->last_x * CREATURE_POS_RESOLUTION, 
                                  creature->last_y * CREATURE_POS_RESOLUTION,
                                  1);
        }
    }
    
    if (!CREATURE_USED(creature))               PROTOCOL_ERROR();

    if (updatemask & CREATURE_DIRTY_TYPE) {
        uint8_t type;
        if (!packet_read08(packet, &type))      PROTOCOL_ERROR();
        if (type >= CREATURE_TYPES)             PROTOCOL_ERROR();
        creature->type = type;
    }

    if (updatemask & CREATURE_DIRTY_FOOD_HEALTH) {
        uint8_t food_health;
        if (!packet_read08(packet, &food_health)) 
                                                PROTOCOL_ERROR();
        creature->food   = food_health >> 4;
        creature->health = food_health & 0x0F;
    }
    
    if (updatemask & CREATURE_DIRTY_STATE) {
        uint8_t state;
        if (!packet_read08(packet, &state))     PROTOCOL_ERROR();
        if (state >= CREATURE_STATES)           PROTOCOL_ERROR();
        creature->state = state;
    }

    if (updatemask & CREATURE_DIRTY_PATH) {
        uint16_t x, y;
        if (!packet_read16(packet, &x))         PROTOCOL_ERROR();
        if (!packet_read16(packet, &y))         PROTOCOL_ERROR();

        int dx = x >> 1; if (x & 1) dx *= -1;
        int dy = y >> 1; if (y & 1) dy *= -1;

        creature->last_x += dx;
        creature->last_y += dy;
            
        // XXX: x, y checken?
        gui_creature_add_path(creature,
                              creature->last_x * CREATURE_POS_RESOLUTION, 
                              creature->last_y * CREATURE_POS_RESOLUTION,
                              0);
    }

    if (updatemask & CREATURE_DIRTY_TARGET) {
        uint16_t target;
        if (!packet_read16(packet, &target))    PROTOCOL_ERROR();
        if (target >= MAXCREATURES)             PROTOCOL_ERROR();
        creature->target = target;
    }

    if (updatemask & CREATURE_DIRTY_MESSAGE) {
        uint8_t len; char buf[256];
        if (!packet_read08(packet, &len))       PROTOCOL_ERROR();
        if (!packet_readXX(packet, buf, len))   PROTOCOL_ERROR();
        buf[len] = '\0';
        snprintf(creature->message, sizeof(creature->message), "%s", buf);
        // creature->last_msg_set = SDL_GetTicks();
    }

    if (updatemask & CREATURE_DIRTY_SPEED) {
        uint8_t speed;
        if (!packet_read08(packet, &speed))     PROTOCOL_ERROR();
        creature->speed = speed * CREATURE_SPEED_RESOLUTION;
    }
}

void gui_creature_init() {
    memset(creatures, 0, sizeof(creatures));
}

void gui_creature_shutdown() {
    gui_creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {

        if (!CREATURE_USED(creature)) 
            continue;

        gui_creature_kill(creature);
    }
}

