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

#include <lauxlib.h>
#include <lualib.h>

#include "infond.h"
#include "global.h"
#include "creature.h"
#include "player.h"
#include "world.h"
#include "map.h"
#include "misc.h"

static creature_t creatures[MAXCREATURES];
static int        num_creatures = 0;

#define CREATURE_USED(creature) (!!((creature)->player))

#define HASHTABLE_SIZE  ((MAXCREATURES) * 4)

static creature_t *creature_hash[HASHTABLE_SIZE] = {0};
#define HASHVALUE(id)   ((id) % (HASHTABLE_SIZE))

static int next_free_vm_id = 0;

static creature_t *creature_find_unused() {
    for (int i = 0; i < MAXCREATURES; i++) {
        creature_t *creature = &creatures[i];
        if (!CREATURE_USED(creature))
            return creature;
    }
    return NULL;
}

static int creature_num(const creature_t *creature) {
    return creature - creatures;
}

creature_t *creature_by_num(int creature_num) {
    if (creature_num < 0 || creature_num >= MAXCREATURES)
        return NULL;
    creature_t *creature = &creatures[creature_num];
    if (!CREATURE_USED(creature))
        return NULL;
    return creature;
}

int creature_id(const creature_t *creature) {
    return creature->vm_id;
}

creature_t *creature_by_id(int vm_id) {
    if (vm_id < 0) return NULL;
    const int hash  = HASHVALUE(vm_id);
    creature_t *creature = creature_hash[hash];
    while (creature) {
        assert(CREATURE_USED(creature));
        if (creature->vm_id == vm_id) 
            return creature;
        creature = creature->hash_next;
    }
    return NULL;
}

creature_t *creature_get_checked_lua(lua_State *L, int idx) {
    const int vm_id = luaL_checklong(L, idx);
    creature_t *creature = creature_by_id(vm_id);
    if (creature)
        return creature;
    luaL_error(L, "creature %d not in use", vm_id);
    return NULL; // never reached
}

int creature_dist(const creature_t *a, const creature_t *b) {
    // XXX: Hier Pfadsuche?
    const int distx = a->x - b->x;
    const int disty = a->y - b->y;
    return sqrt(distx * distx + disty * disty);
}

static void creature_make_smile(const creature_t *creature, client_t *client) {
    packet_t packet;
    packet_init(&packet, PACKET_CREATURE_SMILE);
    packet_write16(&packet, creature_num(creature));
    server_send_packet(&packet, client);
}

int creature_groundbased(const creature_t *creature) {
    return creature->type != CREATURE_FLYER;
}

maptype_e creature_tile_type(const creature_t *creature) {
    return world_get_type(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y));
}

int creature_food_on_tile(const creature_t *creature) {
    return world_get_food(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y));
}

int creature_eat_from_tile(creature_t *creature, int amount) {
    return world_food_eat(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y), amount);
}

static int max_health[CREATURE_TYPES] = { 10000, 20000, 5000, 0 };

int creature_type_health(creature_type type) {
    return max_health[type];
}

int creature_max_health(const creature_t *creature) {
    return creature_type_health(creature->type);
}

static int max_food[CREATURE_TYPES] = { 10000, 20000, 5000, 0 };

int creature_max_food(const creature_t *creature) {
    return max_food[creature->type];
}

static int aging[CREATURE_TYPES] = { 5, 7, 5, 0 };

int creature_aging(const creature_t *creature) {
    return aging[creature->type];
}

creature_t *creature_nearest_enemy(const creature_t *reference, int *distptr) {
    int mindist = 1000000;
    creature_t *nearest  = NULL;
    creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
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


// ------------- Bewegung -------------

static int base_speed[CREATURE_TYPES] = { 200, 400, 800, 0 };
static int health_speed[CREATURE_TYPES] = { 625, 0, 0, 0 };

int creature_speed(const creature_t *creature) {
    int speed = base_speed[creature->type] + 
                (float)health_speed[creature->type] / creature_max_health(creature) * creature->health;

    // speed limit. needed since (speed / CREATURE_SPEED_RESOLUTION) is synchronized as byte.
    if (speed > 250 * CREATURE_SPEED_RESOLUTION)
        speed = 250 * CREATURE_SPEED_RESOLUTION;

    return speed;
}

static int creature_can_walk_path(creature_t *creature) {
    return creature->path != NULL && creature_speed(creature) > 0;
}

void creature_do_walk_path(creature_t *creature, int delta) {
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

    const int dist_to_waypoint = sqrt(dx*dx + dy*dy);

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
    
    assert(!creature_groundbased(creature) || world_walkable(X_TO_TILEX(creature->x), 
                                                             Y_TO_TILEY(creature->y)));
}

// ------------- Food -> Health -------------

int  creature_can_heal(const creature_t *creature) {
    return creature->health < creature_max_health(creature) &&
           creature->food   > 0;
}

static int heal_rate[CREATURE_TYPES] = {500, 300, 600, 0};

int creature_heal_rate(const creature_t *creature) {
    return heal_rate[creature->type];
}

void creature_do_heal(creature_t *creature, int delta) {
    int       max_heal = creature_heal_rate(creature) * delta / 1000;
    const int can_heal = creature_max_health(creature) - creature->health;
    int       finished = 0;

    // Nicht mehr als notwendig heilen
    if (max_heal >= can_heal) 
        max_heal  = can_heal, finished = 1;

    // Nicht mehr als moeglich heilen
    if (max_heal >= creature->food)
        max_heal  = creature->food, finished = 1;

    creature->health += max_heal;
    creature->food   -= max_heal;

    if (finished)
        creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Eat -------------

int creature_can_eat(const creature_t *creature) {
    return creature_food_on_tile(creature) > 0 &&
           creature->food < creature_max_food(creature);
}

static int eat_rate[CREATURE_TYPES] = { 800, 400, 600, 0 };

int creature_eat_rate(const creature_t *creature) {
    return eat_rate[creature->type];
}

void creature_do_eat(creature_t *creature, int delta) {
    int       max_eat = creature_eat_rate(creature) * delta / 1000;
    const int can_eat = creature_max_food(creature) - creature->food;

    if (max_eat > can_eat)
        max_eat = can_eat;

    const int eaten = creature_eat_from_tile(creature, max_eat);
    creature->food += eaten;

    if (eaten == 0)
        creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Attack -------------

static int hitpoints[CREATURE_TYPES][CREATURE_TYPES] = {
      // TARGET -> 
    {    0,      0,   1000,      0 },  // ATTACKER
    { 1500,   1500,   1500,      0 },  //   |
    {    0,      0,      0,      0 },  //   v
    {    0,      0,      0,      0 },
};

int creature_hitpoints(const creature_t *creature, const creature_t *target) {
    return hitpoints[creature->type][target->type];
}

static int attack_distance[CREATURE_TYPES][CREATURE_TYPES] = {
      // TARGET -> 
    {    0,      0,    768,      0 },  // ATTACKER
    {  512,    512,    512,      0 },  //   |
    {    0,      0,      0,      0 },  //   v
    {    0,      0,      0,      0 },
};

int creature_attack_distance(const creature_t *creature, const creature_t *target) {
    return attack_distance[creature->type][target->type];
}

void creature_do_attack(creature_t *creature, int delta) {
    creature_t *target = creature_by_id(creature->target_id);

    // Nicht gefunden?
    if (!target) 
        goto finished_attacking;

    // Eigenes Viech?
    if (target->player == creature->player)
        goto finished_attacking;

    const int hitpoints = creature_hitpoints(creature, target) * delta / 1000;

    // Kann nicht angreifen?
    if (hitpoints == 0)
        goto finished_attacking;

    // Zu weit weg?
    if (creature_dist(creature, target) > creature_attack_distance(creature, target))
        goto finished_attacking;

    target->health -= hitpoints;

    if (target->health > 0) {
        player_on_creature_attacked(target->player,
                                    target, 
                                    creature);
        return;
    }

    creature_make_smile(creature, SEND_BROADCAST);
    creature_kill(target, creature);
finished_attacking:
    creature_set_state(creature, CREATURE_IDLE);
}

// ------------- Creature Conversion -------------

static int conversion_speed[CREATURE_TYPES] = { 1000, 1000, 1000, 0 };

int creature_conversion_speed(const creature_t *creature) {
    return conversion_speed[creature->type];
}

static int conversion_food_needed[CREATURE_TYPES][CREATURE_TYPES] = 
                               // TO
    { {    0,   8000,   5000,      0 },// FROM
      { 8000,      0,      0,      0 },
      { 5000,      0,      0,      0 },
      {    0,      0,      0,      0 } };

int creature_conversion_food(const creature_t *creature, creature_type type) {
    return conversion_food_needed[creature->type][type];
}

int creature_can_convert(const creature_t *creature) {
    if (creature_conversion_food(creature, creature->convert_type) == 0)
        return 0;

    if (!world_walkable(X_TO_TILEX(creature->x), Y_TO_TILEY(creature->y)))
        return 0;

    return 1;
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
    creature->food         -= used_food;

    if (creature->convert_food == creature_conversion_food(creature, creature->convert_type)) {
        creature_set_type(creature, creature->convert_type);
        creature_set_state(creature, CREATURE_IDLE);
    }
}

int creature_set_conversion_type(creature_t *creature, creature_type type) {
    if (type < 0 || type >= CREATURE_TYPES)
        return 0;
    
    // Geht Umwandung ueberhaupt?
    if (creature_conversion_food(creature, type) == 0)
        return 0;

    if (creature_type_health(type) == 0)
        return 0;

    // Beim Type Wechsel geht Food verloren
    if (creature->convert_type != type) 
        creature->convert_food = 0;

    creature->convert_type = type;
    return 1;
}

int creature_set_type(creature_t *creature, creature_type type) {
    if (type == creature->type)
        return 1;

    if (type < 0 || type >= CREATURE_TYPES)
        return 0;

    creature->type         = type;
    creature->convert_type = type;
    
    creature_set_health(creature, creature->health);

    if (creature->food   > creature_max_food(creature))
        creature->food   = creature_max_food(creature);

    creature->dirtymask |= CREATURE_DIRTY_TYPE;
    return 1;
}

// ------------- Creature Spawning -------------

// Waehrend des Spawnens verbrauchtes Futter
static int spawn_food[CREATURE_TYPES] = { 0, 5000, 0, 0 };

int creature_spawn_food(const creature_t *creature) {
    return spawn_food[creature->type];
}

// Spawngeschwindigkeit (wie schnell wird das obige Futter verbraucht)
static int spawn_speed[CREATURE_TYPES] = { 0, 2000, 0, 0 };

int creature_spawn_speed(const creature_t *creature) {
    return spawn_speed[creature->type];
}

// Beim Spawnstart verlorene Lebensenergie
static int spawn_health[CREATURE_TYPES] = { 0, 4000, 0, 0 };

int creature_spawn_health(const creature_t *creature) {
    return spawn_health[creature->type];
}

static int spawn_type[CREATURE_TYPES] = { -1, CREATURE_SMALL, -1, -1 };

int creature_spawn_type(const creature_t *creature) {
    return spawn_type[creature->type];
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
        creature_t *baby = creature_spawn(creature->player, creature, creature->x, creature->y, CREATURE_SMALL);
        if (!baby) {
            static char msg[] = "uuh. sorry. couldn't spawn your new born creature\r\n";
            player_writeto(creature->player, msg, sizeof(msg) - 1);
        }
        creature_set_state(creature, CREATURE_IDLE);
    }
}

int creature_can_spawn(const creature_t *creature) {
    return creature_spawn_type(creature) != -1 &&
           creature->health > creature_spawn_health(creature);
}

// ---------------- Feeding -----------------

static int feed_distance[CREATURE_TYPES] = { 256, 0, 256, 0 };

int creature_can_feed(const creature_t *creature) {
    return feed_distance[creature->type] > 0 &&
           creature->food > 0;
}

int creature_feed_distance(const creature_t *creature) {
    return feed_distance[creature->type];
}

static int feed_speed[CREATURE_TYPES] = { 400, 0, 400, 0 };

int creature_feed_speed(const creature_t *creature) {
    return feed_speed[creature->type];
}

void creature_do_feed(creature_t *creature, int delta) {
    int food     = creature_feed_speed(creature) * delta / 1000;
    int finished = 0;
    
    creature_t *target = creature_by_id(creature->target_id);

    // Nicht gefunden?
    if (!target) 
        goto finished_feeding;

    // Zu weit weg?
    if (creature_dist(creature, target) > creature_feed_distance(creature))
        goto finished_feeding;

    if (food >= creature->food) 
        food  = creature->food, finished = 1;

    if (food >= creature_max_food(target) - target->food)
        food  = creature_max_food(target) - target->food, finished = 1;

    if (food == 0)
        goto finished_feeding;

    creature->food -= food;
    target->food   += food;

    if (!finished)
        return;

finished_feeding:
    creature_set_state(creature, CREATURE_IDLE);
}


int creature_set_target(creature_t *creature, int target_id) {
    // Sich selbst als Ziel macht nie Sinn.
    if (target_id == creature->vm_id)
        return 0;

    // Nicht vorhanden?
    if (!creature_by_id(target_id))
        return 0;

    // Nicht mehr pruefen, da sich das Ziel bereits in dieser
    // Runde toeten koennte und daher ein true hier nicht heisst,
    // dass tatsaechlich attackiert/gefuettert werden kann.
    creature->target_id = target_id;
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
            if (!creature_can_walk_path(creature))
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
            // if (!creature_can_attack(creature))
            //    return 0;
            break;
        case CREATURE_CONVERT:
            if (!creature_can_convert(creature))
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
    return 1;
}

void creature_update_network_variables(creature_t *creature) {
    // Leben/Food Prozentangaben aktualisieren
    int new_food_health = min(15, 16 * creature->food   / creature_max_food(creature)) << 4 |
                          min(15, 16 * creature->health / creature_max_health(creature));
    if (new_food_health != creature->network_food_health) {
        creature->network_food_health = new_food_health;
        creature->dirtymask |= CREATURE_DIRTY_FOOD_HEALTH;
    }

    if (creature->state != creature->network_state) {
        creature->network_state = creature->state;
        creature->dirtymask |= CREATURE_DIRTY_STATE;
    }

    int path_x = creature->x / CREATURE_POS_RESOLUTION;
    int path_y = creature->y / CREATURE_POS_RESOLUTION;
        
    if (path_x != creature->network_path_x ||
        path_y != creature->network_path_y) {
        creature->network_path_x = path_x;
        creature->network_path_y = path_y;
        creature->dirtymask |= CREATURE_DIRTY_PATH;
    }

    int new_speed = (creature_speed(creature) + 2) / CREATURE_SPEED_RESOLUTION;
    if (creature->state == CREATURE_WALK && 
        new_speed != creature->network_speed) 
    {
        creature->network_speed = new_speed;
        creature->dirtymask |= CREATURE_DIRTY_SPEED;
    }

    if ((creature->state == CREATURE_ATTACK || creature->state == CREATURE_FEED)) { 
        const creature_t *target = creature_by_id(creature->target_id);
        if (target && creature_num(target) != creature->network_target) {
            creature->network_target = creature_num(target);
            creature->dirtymask |= CREATURE_DIRTY_TARGET;
        }
    }
}


void creature_moveall(int delta) {
    int koth_score               = 1;
    creature_t *creature_on_koth = NULL;

    creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;

        if (creature->suicide) {
            creature_kill(creature, creature);
            continue;
        }

        // Alterungsprozess
        creature->age_action_deltas += delta;
        while (creature->age_action_deltas >= 100) {
            creature->age_action_deltas -= 100;
            creature->health -= creature_aging(creature);
        }

        // Tot?
        if (creature->health <= 0) {
            creature_kill(creature, NULL);
            continue;
        }
                
        // State Aktion
        switch (creature->state) {
            case CREATURE_IDLE:
                // Nichts machen
                break;
            case CREATURE_WALK:
                creature_do_walk_path(creature, delta);
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
                creature_do_convert(creature, delta);
                break;
            case CREATURE_SPAWN:
                creature_do_spawn(creature, delta);
                break;
            case CREATURE_FEED:
                creature_do_feed(creature, delta);
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
    }

    // Zweite Iteration fuer Network Sync
    creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;

        creature_update_network_variables(creature);
        
        creature_to_network(creature, creature->dirtymask, SEND_BROADCAST);
        creature->dirtymask = CREATURE_DIRTY_NONE;
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
    pathnode_t *newpath;

    if (creature_groundbased(creature)) {
        if (!world_walkable(X_TO_TILEX(x), Y_TO_TILEY(y)))
            return 0;
        
        newpath = world_findpath(creature->x, creature->y, x, y);

        if (!newpath)
            return 0;
    } else {
        // Fliegendes Vieh
        if (!world_is_within_border(X_TO_TILEX(x), Y_TO_TILEY(y)))
            return 0;

        newpath = pathnode_new(x, y); 
    }

    path_delete(creature->path);
    creature->path = newpath;
    return 1;
}

int creature_set_health(creature_t *creature, int health) {
    if (health > creature_max_health(creature))
        health = creature_max_health(creature);
    
    if (health < 0)
        health = 0;

    creature->health = health;

    return 1;
}

int creature_suicide(creature_t *creature) {
    creature->suicide = 1;
    return 1;
}

void creature_set_message(creature_t *creature, const char *message) {
    if (strncmp(creature->message, message, sizeof(creature->message) - 1)) {
        snprintf(creature->message, sizeof(creature->message), "%s", message);
        creature->dirtymask |= CREATURE_DIRTY_MESSAGE;
    } 
}

int creature_set_food(creature_t *creature, int food) {
    if (food < 0)
        return 0;

    if (food > creature_max_food(creature))
        return 0;

    creature->food = food;
    return 1;
}

creature_t *creature_spawn(player_t *player, creature_t *parent, int x, int y, creature_type type) {
    if (!world_walkable(X_TO_TILEX(x), Y_TO_TILEY(y))) 
        return NULL;

    creature_t *creature = creature_find_unused();

    if (!creature)
        return NULL;

    // spawning this type ok?
    if (creature_type_health(type) == 0)
        return NULL;

    num_creatures++;

    memset(creature, 0, sizeof(creature_t));

    creature->vm_id        = next_free_vm_id++;
    creature->x            = x;
    creature->y            = y;
    creature->type         = type;
    creature->food         = 0;
    creature->health       = creature_type_health(type);
    creature->player       = player;
    creature->target_id    = creature->vm_id; // muesste nicht gesetzt werden
    creature->path         = NULL;
    creature->convert_food = 0;
    creature->convert_type = type;
    creature->spawn_food   = 0;
    creature->state        = CREATURE_IDLE;
    creature->suicide      = 0;
    creature->message[0]   = '\0';
    creature->spawn_time   = game_time;
    creature->age_action_deltas = 0;

    const int hash = HASHVALUE(creature->vm_id);
    creature->hash_next = creature_hash[hash];
    creature_hash[hash] = creature;

    player_on_creature_spawned(player, creature, parent);

    creature_update_network_variables(creature);
    creature_to_network(creature, CREATURE_DIRTY_ALL, SEND_BROADCAST);
    creature->dirtymask = CREATURE_DIRTY_NONE;

    creature_make_smile(creature, SEND_BROADCAST);
    return creature;
}

void creature_kill(creature_t *creature, creature_t *killer) {
    player_on_creature_killed(creature->player, creature, killer);

    path_delete(creature->path);
    creature->player = NULL;

    num_creatures--;

    const int hash = HASHVALUE(creature->vm_id);
    creature_t *tmp = creature_hash[hash];
    if (tmp == creature) {
        creature_hash[hash] = creature_hash[hash]->hash_next;
    } else {
        while (tmp->hash_next != creature) {
            tmp = tmp->hash_next;
            assert(tmp);
        }
        tmp->hash_next = creature->hash_next;
    }
    
    creature_to_network(creature, CREATURE_DIRTY_ALIVE, SEND_BROADCAST);
}

void creature_kill_all_players_creatures(player_t *player) {
    creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;
        if (creature->player != player)
            continue;

        creature_kill(creature, NULL);
    }
}

void creature_send_initial_update(client_t *client) {
    creature_t *creature = &creatures[0];
    for (int i = 0; i < MAXCREATURES; i++, creature++) {
        if (!CREATURE_USED(creature)) 
            continue;
        creature_to_network(creature, CREATURE_DIRTY_ALL, client);
    }
}

void creature_to_network(creature_t *creature, int dirtymask, client_t *client) {
    if (dirtymask == CREATURE_DIRTY_NONE)
        return;

    packet_t packet;
    packet_init(&packet, PACKET_CREATURE_UPDATE);

    packet_write16(&packet, creature_num(creature));
    packet_write08(&packet, dirtymask);
    if (dirtymask & CREATURE_DIRTY_ALIVE) {
        if (CREATURE_USED(creature)) {
            packet_write08(&packet, player_num(creature->player));
            packet_write16(&packet, creature->vm_id);
            packet_write16(&packet, creature->network_last_x = creature->network_path_x);
            packet_write16(&packet, creature->network_last_y = creature->network_path_y);
        } else {
            packet_write08(&packet, 0xFF);
        }
    }
    if (dirtymask & CREATURE_DIRTY_TYPE) 
        packet_write08(&packet, creature->type);
    if (dirtymask & CREATURE_DIRTY_FOOD_HEALTH)
        packet_write08(&packet, creature->network_food_health);
    if (dirtymask & CREATURE_DIRTY_STATE)
        packet_write08(&packet, creature->network_state);
    if (dirtymask & CREATURE_DIRTY_PATH) {
        int dx = creature->network_path_x - creature->network_last_x;
        int dy = creature->network_path_y - creature->network_last_y;
        packet_write16(&packet, (dx < 0) | (abs(dx) << 1));
        packet_write16(&packet, (dy < 0) | (abs(dy) << 1));
        creature->network_last_x = creature->network_path_x;
        creature->network_last_y = creature->network_path_y;
    }
    if (dirtymask & CREATURE_DIRTY_TARGET)
        packet_write16(&packet, creature->network_target);
    if (dirtymask & CREATURE_DIRTY_MESSAGE) {
        packet_write08(&packet, strlen(creature->message));
        packet_writeXX(&packet, &creature->message, strlen(creature->message));
    }
    if (dirtymask & CREATURE_DIRTY_SPEED) 
        packet_write08(&packet, creature->network_speed);
    
    server_send_packet(&packet, client);
}

int creature_num_creatures() {
    return num_creatures;
}

static int pure = 1;

struct creature_option { 
    const char *name; 
    const int   min; 
    const int   max; 
    int        *value; 
};

#include "creature_config.h"

int luaCreatureGetConfig(lua_State *L) {
    size_t namelen; const char *name = luaL_checklstring(L, 1, &namelen);
    const struct creature_option *opt = in_word_set(name, namelen);
    if (!opt) luaL_error(L, "option '%s' does not exist", name);
    lua_pushnumber(L, *opt->value);
    return 1;
}

int luaCreatureSetConfig(lua_State *L) {
    if (num_creatures > 0) 
        luaL_error(L, "cannot change options while there are creatures");
    size_t namelen; const char *name = luaL_checklstring(L, 1, &namelen);
    int value = luaL_checklong(L, 2);
    const struct creature_option *opt = in_word_set(name, namelen);
    if (!opt) luaL_error(L, "option '%s' does not exist", name);
    if (value < opt->min || value > opt->max) 
        luaL_error(L, "value for '%s' out of range. (%d - %d)", name, opt->min, opt->max);
    *opt->value = value;
    pure = 0;
    return 0;
}

int luaCreatureConfigChanged(lua_State *L) {
    lua_pushboolean(L, pure);
    return 1;
}

void creature_init() {
    for (int i = 0; i < MAXCREATURES; i++) {
        creature_t *creature = &creatures[i];
        creature->player = NULL;
    }
    next_free_vm_id = 0;
    assert(num_creatures == 0);
}

void creature_shutdown() {
    for (int i = 0; i < MAXCREATURES; i++) {
        creature_t *creature = &creatures[i];
        if (CREATURE_USED(creature))
            creature_kill(&creatures[i], NULL);
    }
}

