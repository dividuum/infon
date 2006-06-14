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
#include "creature.h"
#include "player.h"
#include "world.h"
#include "video.h"
#include "map.h"

static creature_t creatures[CREATURE_NUM];

#define CREATURE_USED(creature) ((creature)->player)

static creature_t *creature_find_unused() {
    for (int i = 0; i < CREATURE_NUM; i++) {
        creature_t *creature = &creatures[i];
        if (!CREATURE_USED(creature))
            return creature;
    }
    return NULL;
}

creature_t *creature_by_num(int creature_num) {
    if (creature_num < 0 || creature_num >= CREATURE_NUM)
        return NULL;
    creature_t *creature = &creatures[creature_num];
    if (!CREATURE_USED(creature))
        return NULL;
    return creature;
}

int creature_dist(const creature_t *a, const creature_t *b) {
    // XXX: Hier Pfadsuche?
    const int distx = a->x - b->x;
    const int disty = a->y - b->y;
    return sqrt(distx * distx + disty * disty);
}

int creature_num(const creature_t *creature) {
    return creature - creatures;
}

int creature_food_on_tile(const creature_t *creature) {
    return world_get_food(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y));
}

int creature_eat_from_tile(creature_t *creature, int amount) {
    return world_food_eat(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y), amount);
}

int creature_max_health(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 10000;
        default:
            return 20000;
    }
}

int creature_max_food(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 10000;
        default:
            return 20000;
    }
}

creature_t *creature_nearest_enemy(const creature_t *reference, int *distptr) {
    int mindist = 1000000;
    creature_t *nearest  = NULL;
    creature_t *creature = &creatures[0];
    for (int i = 0; i < CREATURE_NUM; i++, creature++) {
        if (!CREATURE_USED(creature) || creature->player == reference->player) 
            continue;

        const int dist = creature_dist(creature, reference);

        if (dist > mindist) 
            continue;

        nearest = creature;
        mindist = dist;
    }

    if (distptr) *distptr = mindist;

    return nearest;
}

void creature_draw() {
    creature_t *creature = &creatures[0];
    for (int i = 0; i < CREATURE_NUM; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;
        
        const int x = X_TO_SCREENX(creature->x) - 7;
        const int y = Y_TO_SCREENY(creature->y) - 7;
        const int hw = creature->health * 16 / creature_max_health(creature);
        const int fw = creature->food   * 16 / creature_max_food(creature);

        if (fw != 16) video_rect(x + fw, y - 4, x + 16, y - 2, 0x00, 0x00, 0x00, 0xB0);
        if (fw !=  0) video_rect(x,      y - 4, x + fw, y - 2, 0xFF, 0xFF, 0xFF, 0xB0);
                                               
        if (hw != 16) video_rect(x + hw, y - 2, x + 16, y,     0xFF, 0x00, 0x00, 0xB0);
        if (hw !=  0) video_rect(x,      y - 2, x + hw, y,     0x00, 0xFF, 0x00, 0xB0);

        video_draw(x, y, sprite_get(CREATURE_SPRITE(creature->player->color,
                                                    creature->type,
                                                    creature->dir,
                                                    (game_time >> 7) % 2)));
        
        //if (game_time > creature->last_state_change +  300 && 
        //    game_time < creature->last_state_change + 1000)
            video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + creature->state));

        if (game_time < creature->last_msg_set + 2000) 
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

        if (creature->state == CREATURE_ATTACK && creature->spawn_time != game_time) {
            assert(creature->target >= 0 && creature->target < CREATURE_NUM);
            const creature_t *target = &creatures[creature->target];
            if (target) { // Das Angegriffene Vieh koennte in dieser Runde bereits getoetet sein.
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3, Y_TO_SCREENY(target->y) - 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3, Y_TO_SCREENY(target->y) + 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3, Y_TO_SCREENY(target->y) - 3);
                video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3, Y_TO_SCREENY(target->y) + 3);
            }
        }
    }
}


// ------------- Bewegung -------------

int  creature_can_move_to_target(creature_t *creature) {
    return creature->path != NULL;
}

int creature_speed(const creature_t *creature) {
    switch (creature->type) {
        case 0:
            //return 200 + creature->health / 50;
            return 200 + creature->health / 16;
        default:
            //return 300;
            return 400;
    }
}

int creature_turnspeed(const creature_t *creature) {
    switch (creature->type) {
        case 0:
            return 2;
        default:
            return 1;
    }
}

void creature_do_move_to_target(creature_t *creature, int delta) {
    int travelled = creature_speed(creature) * delta / 1000;

    if (travelled == 0)
        travelled = 1;

again:
    if (!creature->path) {
        creature_set_state(creature, CREATURE_IDLE);
        return;
    }

    const int dx = creature->path->x - creature->x;
    const int dy = creature->path->y - creature->y;

    int winkel_to_waypoint = 16 * ((atan2(-dx, dy == 0 ? 1 : dy) + M_PI) / M_PI);
    if (winkel_to_waypoint == 32) winkel_to_waypoint = 0;

    assert(winkel_to_waypoint >= 0 && winkel_to_waypoint < 32);
    int dw = winkel_to_waypoint - creature->dir;

    if (dw < -16) dw += 32;
    if (dw >  16) dw -= 32;

    if (dw < 0) {
        creature->dir -= -dw > creature_turnspeed(creature) ? creature_turnspeed(creature) : -dw;
    } else if (dw > 0) {
        creature->dir +=  dw > creature_turnspeed(creature) ? creature_turnspeed(creature) :  dw;
    }

    if (creature->dir >= 32)
        creature->dir -= 32;

    if (creature->dir < 0)
        creature->dir += 32;

    if (dw > 6 || dw < -6)
        return;
    
    int dist_to_waypoint = sqrt(dx*dx + dy*dy);

    if (travelled >= dist_to_waypoint) {
        creature->x = creature->path->x;
        creature->y = creature->path->y;
        travelled  -= dist_to_waypoint;
        pathnode_t *old = creature->path;
        creature->path = creature->path->next;
        free(old);
        goto again;
    }

    creature->x += dx * travelled / dist_to_waypoint;
    creature->y += dy * travelled / dist_to_waypoint;
}

// ------------- Food -> Health -------------

int  creature_can_heal(const creature_t *creature) {
    return creature->health < creature_max_health(creature) &&
           creature->food   > 0;
}

int creature_heal_rate(const creature_t *creature) {
    switch (creature->type) {
        case 0:
            return 500;
        default:
            return 300;
    }
}

void creature_do_heal(creature_t *creature, int delta) {
    int max_heal = creature_heal_rate(creature) * delta / 1000;
    const int can_heal = creature_max_health(creature) - creature->health;

    // Nicht mehr als notwendig heilen
    if (max_heal > can_heal) 
        max_heal = can_heal;

    // Nicht mehr als moeglich heilen
    if (max_heal > creature->food)
        max_heal = creature->food;

    creature->health += max_heal;    
    creature->food   -= max_heal;    

    if (max_heal == 0)
        creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Eat -------------

int creature_can_eat(const creature_t *creature) {
    return creature_food_on_tile(creature) > 0 &&
           creature->food < creature_max_food(creature);
}

int creature_eat_rate(const creature_t *creature) {
    switch (creature->type) {
        case 0:
            return 800;
        default:
            return 400;
    }
}

void creature_do_eat(creature_t *creature, int delta) {
    int max_eat = creature_eat_rate(creature) * delta / 1000;
    const int can_eat = creature_max_food(creature) - creature->food;

    if (max_eat > can_eat)
        max_eat = can_eat;

    const int eaten = creature_eat_from_tile(creature, max_eat);
    creature->food += eaten;

    if (eaten == 0)
        creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Attack -------------

int creature_can_attack(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 0;
        default:
            return 1;
    }
}

int creature_hitpoints(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 0;
        default:
            return 1500;
    }
}

int creature_attack_distance(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 0;
        default:
            return 2 * TILE_SCALE;
    }
}

void creature_do_attack(creature_t *creature, int delta) {
    const int hitpoints = creature_hitpoints(creature) * delta / 1000;
    if (hitpoints == 0)
        goto finished_attacking;
    
    creature_t *target = creature_by_num(creature->target);

    // Nicht gefunden?
    if (!target) 
        goto finished_attacking;
    
    // Ziel zu jung?
    if (target->spawn_time + 500 > game_time)
        goto finished_attacking;

    // Eigenes Viech?
    if (target->player == creature->player)
        goto finished_attacking;

    // Zu weit weg?
    if (creature_dist(creature, target) > creature_attack_distance(creature))
        goto finished_attacking;

    target->health -= hitpoints;

    player_on_creature_attacked(target->player,
                                creature_num(target), 
                                creature_num(creature));

    if (target->health > 0)
        return;

    creature_kill(target, creature);
finished_attacking:
    creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Creature Conversion -------------

int creature_conversion_speed(creature_t *creature) {
    return 1000;
}

static const int conversion_food_needed[CREATURE_TYPES][CREATURE_TYPES] = 
                               // TO
    { {    0,   8000,      0,      0 },// FROM
      {    0,      0,      0,      0 },
      {    0,      0,      0,      0 },
      {    0,      0,      0,      0 } };

int creature_conversion_food(const creature_t *creature, int type) {
    return conversion_food_needed[creature->type][type];
}

void creature_do_convert(creature_t *creature, int delta) {
    int used_food = creature_conversion_speed(creature) * delta / 1000;
    const int needed_food = creature_conversion_food(creature, creature->convert_type) - 
                            creature->convert_food;

    if (used_food > needed_food)
        used_food = needed_food;

    if (used_food > creature->food)
        used_food = creature->food;

    if (used_food == 0) 
        creature_set_state(creature, CREATURE_IDLE);

    creature->convert_food += used_food;
    creature->food -= used_food;

    if (creature->convert_food == creature_conversion_food(creature, creature->convert_type)) {
        creature->type = creature->convert_type;
        if (creature->health > creature_max_health(creature))
            creature->health = creature_max_health(creature);
        if (creature->food   > creature_max_food(creature))
            creature->food   = creature_max_food(creature);
        creature_set_state(creature, CREATURE_IDLE);
    }
}

int creature_set_conversion_type(creature_t *creature, int type) {
    if (type < 0 || type >= CREATURE_TYPES)
        return 0;
    
    // Geht Umwandung ueberhaupt?
    if (creature_conversion_food(creature, type) == 0)
        return 0;

    // Beim Type Wechsel geht Food verloren
    if (creature->convert_type != type) 
        creature->convert_food = 0;

    creature->convert_type = type;
    return 1;
}

// ------------- Creature Spawning -------------

// Waehrend des Spawnens verbrauchtes Futter
int creature_spawn_food(const creature_t *creature) {
    return 5000;
}

// Spawngeschwindigkeit (wie schnell wird das obige Futter verbraucht)
int creature_spawn_speed(const creature_t *creature) {
    return 2000;
}

// Beim Spawnstart verlorene Lebensenergie
int creature_spawn_health(const creature_t *creature) {
    return 4000;
}

void creature_do_spawn(creature_t *creature, int delta) {
    int used_food = creature_spawn_speed(creature) * delta / 1000;
    const int needed_food = creature_spawn_food(creature) - creature->spawn_food;

    if (used_food > needed_food) 
        used_food = needed_food;
        
    if (used_food > creature->food)
        used_food = creature->food;

    if (used_food == 0) 
        creature_set_state(creature, CREATURE_IDLE);

    creature->spawn_food += used_food;
    creature->food -= used_food;

    if (creature->spawn_food == creature_spawn_food(creature)) {
        creature_t *baby = creature_spawn(creature->player, creature->x, creature->y, 0, 10);
        if (!baby) {
            static char msg[] = "uuh. sorry. couldn't spawn your new born creature\n";
            player_writeto(creature->player, msg, sizeof(msg) - 1);
        } else {
            baby->food = 1000;
        }
        creature_set_state(creature, CREATURE_IDLE);
    }
}

int creature_can_spawn(const creature_t *creature) {
    return creature->type == 1 &&
           creature->food >= creature_spawn_food(creature) &&
           creature->health > creature_spawn_health(creature);
}

// ---------------- Feeding -----------------

int creature_can_feed(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return creature->food > 0;
        default:
            return 0;
    }
}

int creature_feed_distance(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return TILE_SCALE;
        default:
            return 0;
    }
}

int creature_feed_speed(const creature_t *creature) {
    switch (creature->type) {
        case 0: 
            return 400;
        default:
            return 0;
    }
}

void creature_do_feed(creature_t *creature, int delta) {
    int food = creature_feed_speed(creature) * delta / 1000;
    
    creature_t *target = creature_by_num(creature->target);

    // Nicht gefunden?
    if (!target) 
        goto finished_feeding;

    // Ziel zu jung?
    if (target->spawn_time + 500 > game_time)
        goto finished_feeding;

    // Zu weit weg?
    if (creature_dist(creature, target) > creature_feed_distance(creature))
        goto finished_feeding;

    if (food > creature->food) 
        food = creature->food;

    if (food + target->food > creature_max_food(target))
        food = creature_max_food(target) - target->food;

    if (food == 0)
        goto finished_feeding;

    creature->food -= food;
    target->food += food;
    return;

finished_feeding:
    creature_set_state(creature, CREATURE_IDLE);
}


int creature_set_target(creature_t *creature, int target) {
    // Sich selbst als Ziel macht nie Sinn.
    if (target == creature_num(creature))
        return 0;

    // Nicht vorhanden?
    if (!creature_by_num(target))
        return 0;

    // Nicht mehr pruefen, da sich das Ziel bereits in dieser
    // Runde toeten koennte und daher ein true hier nicht heisst,
    // dass tatsaechlich attackiert/gefuettert werden kann.
    creature->target = target;
    return 1;
}


// -----------------------------------------

int creature_set_state(creature_t *creature, int newstate) {
    // Wechsel in gleichen State immer ok
    if (newstate == creature->state)
        return 1;

    // Kann neuer State betreten werden? 
    switch (newstate) {
        case CREATURE_IDLE:
            break;
        case CREATURE_WALK:
            if (!creature_can_move_to_target(creature))
                return 0;
            break;
        case CREATURE_HEAL:
            if (!creature_can_heal(creature))
                return 0;
            break;
        case CREATURE_EAT:
            if (!creature_can_eat(creature))
                return 0;
            break;
        case CREATURE_ATTACK:
            if (!creature_can_attack(creature))
                return 0;
            break;
        case CREATURE_CONVERT:
            if (creature_conversion_food(creature, creature->convert_type) == 0)
                return 0;
            break;
        case CREATURE_SPAWN:
            if (!creature_can_spawn(creature))
                return 0;
            creature->health -= creature_spawn_health(creature);
            assert(creature->health > 0);
            break;
        case CREATURE_FEED:
            if (!creature_can_feed(creature))
                return 0;
            break;
    }

    // Alten State verlassen
    switch (creature->state) {
        case CREATURE_CONVERT:
            // Bei einem Statewechel geht in die Konvertierung 
            // investiertes Futter verloren
            creature->convert_food = 0;
            break;
        case CREATURE_SPAWN:
            // Dito fuer Spawnen
            creature->spawn_food = 0;
            break;
        default:
            break;
    }

    creature->state = newstate;
    creature->last_state_change = game_time;
    return 1;
}

void creature_moveall(int delta) {
    int koth_score               = 1;
    creature_t *creature_on_koth = NULL;

    creature_t *creature = &creatures[0];
    for (int i = 0; i < CREATURE_NUM; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;

        // Alterungsprozess
        creature->age_action_deltas += delta;
        while (creature->age_action_deltas >= 100) {
            creature->age_action_deltas -= 100;
            switch (creature->type) {
                case 0:
                    creature->health -= 1;
                case 1: // Fallthough
                    creature->health -= 4;
                    if (creature->health <= 0) {
                        creature_kill(creature, NULL);
                        goto next_creature;
                    }
                    break;
                default:
                    break;
            }
        }
                
        // State Aktion
        switch (creature->state) {
            case CREATURE_IDLE:
                // Nichts machen
                break;
            case CREATURE_WALK:
                creature_do_move_to_target(creature, delta);
                break;
            case CREATURE_HEAL:
                creature_do_heal(creature, delta);
                break;
            case CREATURE_EAT:
                creature_do_eat(creature, delta);
                break;
            case CREATURE_ATTACK:
                creature_do_attack(creature, delta);
                break;
            case CREATURE_CONVERT:
                creature_do_convert(creature, delta);;
                break;
            case CREATURE_SPAWN:
                creature_do_spawn(creature, delta);;
                break;
            case CREATURE_FEED:
                creature_do_feed(creature, delta);;
                break;
        }

        // Erst hier auf IDLE pruefen. Oben kann eine Kreatur in den Zustand
        // IDLE gewechselt haben. XXX: Wieder nach oben verschieben?
        if (creature->state == CREATURE_IDLE) {
            if (X_TO_TILEX(creature->x) == world_koth_x() &&
                Y_TO_TILEY(creature->y) == world_koth_y()) 
            {
                // Wenn schon eins drauf ist, und diese von unterschiedlichen Spielern
                // sind, so gibt es keine Punkte
                if (creature_on_koth && creature->player != creature_on_koth->player)
                    koth_score = 0;
                creature_on_koth = creature;
            }
        }
next_creature: ;
    }

    // Gibt es einen Koenig? :)
    // Eventuell wurde die Koth Kreatur in dieser Runde gekillt
    if (koth_score && creature_on_koth && CREATURE_USED(creature_on_koth)) {
        player_is_king_of_the_hill(creature_on_koth->player, delta);
    } else {
        player_there_is_no_king();
    }
}

int creature_set_path(creature_t *creature, int x, int y) {
    if (!world_walkable(X_TO_TILEX(x), Y_TO_TILEY(y)))
        return 0;

    pathnode_t *newpath = world_findpath(creature->x, creature->y, x, y);

    if (!newpath)
        return 0;

    /*
    pathnode_t *cur = newpath;
    do {
        printf("%4d,%4d=>", cur->x, cur->y);
        cur = cur->next;
    } while (cur);
    printf("done\n");
    */

    path_delete(creature->path);

    creature->path = newpath;
    return 1;
}

void creature_set_message(creature_t *creature, const char *message) {
    snprintf(creature->message, sizeof(creature->message), "%s", message);
    creature->last_msg_set = game_time;
}

creature_t *creature_spawn(player_t *player, int x, int y, int type, int points) {
    if (!world_walkable(X_TO_TILEX(x), Y_TO_TILEY(y))) 
        return NULL;

    creature_t *creature = creature_find_unused();

    if (!creature)
        return NULL;

    creature->x      = x;
    creature->y      = y;
    creature->dir    = 0;
    creature->type   = type;
    creature->food   = 0;
    creature->health = creature_max_health(creature);
    creature->player = player;
    creature->target = creature_num(creature); // muesste nicht gesetzt werden
    creature->path   = NULL;
    creature->convert_food = 0;
    creature->convert_type = type;
    creature->spawn_food   = 0;
    creature->message[0]   = '\0';
    creature->last_msg_set = 0;
    creature->spawn_time   = game_time;

    creature->last_state_change = game_time;
    creature->age_action_deltas = 0;

    player_on_creature_spawned(player, creature_num(creature), points);
    return creature;
}

void creature_kill(creature_t *creature, creature_t *killer) {
    int food_from_creature = creature->food;

    // Bei Selbstmord kommt nicht das ganze Futter raus.
    if (killer == creature)
        food_from_creature /= 3;

    world_add_food(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y), food_from_creature);

    player_on_creature_killed(creature->player, 
                              creature_num(creature), 
                              killer ? creature_num(killer) : -1);

    path_delete(creature->path);
    creature->player = NULL;
}

void creature_init() {
    for (int i = 0; i < CREATURE_NUM; i++) {
        creature_t *creature = &creatures[i];
        creature->player = NULL;
    }
}

void creature_shutdown() {
    for (int i = 0; i < CREATURE_NUM; i++) {
        creature_t *creature = &creatures[i];
        if (CREATURE_USED(creature))
            creature_kill(&creatures[i], NULL);
    }
}

