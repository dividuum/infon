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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "path.h"
#include "global.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

/* TILE Functions ******************************************/                               

void tile_init(tile_t *tile) {
    assert(tile);
    tile->walkable = 0;
    tile->region   = REGION_NONE;
    tile->area     = NULL;
}

/* PORTAL Functions ********************************************/

int portal_width(portal_t *portal) {
    return portal->s2 - portal->s1 + 1;
}

void portal_destroy(portal_t *portal) {
    free(portal);
}

/* Berechnet den veraenderlichen Koordinatenanteil
 * eines Portals neu. */
void portal_update_center(portal_t *portal) {
    if (portal->dir == PORTAL_DIR_VERTICAL) {
        portal->cy  = TILE_Y1(portal->s1) + portal_width(portal) * TILE_HEIGHT / 2;
    } else {
        portal->cx  = TILE_X1(portal->s1) + portal_width(portal) * TILE_WIDTH  / 2;
    }
}

/* PORTALSIDE Functions ********************************************/

void portalside_destroy(portalside_t *side) {
    free(side);
}

/* Liefert von einer uebergebenen Portalseite deren Richtung */
int portalside_direction(const portalside_t *side) {
    assert(side->prev->next == side);
    assert(side->next->prev == side);
    assert(side->portal->sides[0] == side || 
           side->portal->sides[1] == side);
    if (side->portal->sides[0] == side) {
        return side->portal->dir == PORTAL_DIR_HORIZONTAL ? DIR_DOWN : DIR_RIGHT;
    } else {
        return side->portal->dir == PORTAL_DIR_HORIZONTAL ? DIR_UP : DIR_LEFT;
    }
}

/* Ordnet einer Portalseite einen Wert zu, nach dem 
 * unterschiedliche Portale reihum sortiert 
 * werden koennen. */
int portalside_to_value(const portalside_t *side) {
    assert(side->portal->s1 <= side->portal->s2);
    switch (portalside_direction(side)) {
        case DIR_UP:    return          side->portal->s1; 
        case DIR_RIGHT: return 100000 + side->portal->s1;
        case DIR_DOWN:  return 300000 - side->portal->s2;
        case DIR_LEFT:  return 400000 - side->portal->s2;
    }
    assert(0);
    return 0;
}

/* Verbindet das Portal der uebergebenen Portalseite mit deren
 * Nachbarn. side2, side3 und side->next->portal werden geloescht.
 *
 *                          prev-->
 *               4          <--next          3
 * |--- side->otherside ----|   |---- side->next->otherside----|
 *
 *          [portal]                   [side->next->portal]
 *              1                              2
 * 
 * |----------- side -------|   |------- side->next -----------|
 *               1         next-->           2
 *                         <--prev
 */
void portalside_merge_with_neighbour(portalside_t *side) {
    portalside_t *side1 = side;
    portalside_t *side2 = side->next;
    portalside_t *side3 = side2->otherside;
    portalside_t *side4 = side1->otherside;

    portal_t *portal1 = side1->portal;
    portal_t *portal2 = side2->portal;

    assert(side1->next->otherside->next->otherside == side1);
    assert(side1->otherside->prev->otherside->prev == side1);
    assert(side1->area == side2->area);
    assert(side3->area == side4->area);

    assert(side2->prev == side1);
    assert(side4->prev == side3);
    assert(side3->portal == side2->portal);
    assert(side1->portal == side4->portal);

    assert(side1->prev->next == side1);
    assert(side1->next->prev == side1);
    assert(side4->prev->next == side4);
    assert(side4->next->prev == side4);

    side4->prev = side3->prev;
    side3->prev->next = side4;

    side1->next = side2->next;
    side2->next->prev = side1;

    if (side2->area->portals == side2)
        side2->area->portals = side1;

    if (side3->area->portals == side3)
        side3->area->portals = side4;

    portalside_destroy(side2);
    portalside_destroy(side3);

    // Neue Dimension des Portals bestimmen
    portal1->s1 = MIN(portal1->s1, portal2->s1);
    portal1->s2 = MAX(portal1->s2, portal2->s2);

    portal_update_center(portal1);

    portal_destroy(portal2);

    assert((portal1->sides[0] == side1 && portal1->sides[1] == side4) ||
           (portal1->sides[1] == side1 && portal1->sides[0] == side4));

    assert(side1->prev->next == side1);
    assert(side1->next->prev == side1);
    assert(side4->prev->next == side4);
    assert(side4->next->prev == side4);
}

/* AREA Functions ********************************************/

void area_destroy(area_t *area) {
    free(area);
}

/*
void area_get_info(area_t *area) {
    printf("     area %p, %d,%d - %d,%d\n",
           area, area->x1, area->y1, area->x2, area->y2);
    portalside_t *f, *side;
    f = side= area->portals;
    if (side) do {
        assert(side->area == area);
        printf("\n     portalside %p, value=%d\n", side, portalside_to_value(side));
        printf("         otherside: %p, portal: %p\n", side->otherside, side->portal);
        printf("         next: %p, prev: %p\n", side->next, side->prev);
        printf("\n");
        printf("         s1=%d, s2=%d\n", side->portal->s1, side->portal->s2);
        printf("         cx=%d, cy=%d\n", side->portal->cx, side->portal->cy);
        printf("         otherside->area %p, %d,%d - %d,%d\n",
               side->otherside->area, 
               side->otherside->area->x1, 
               side->otherside->area->y1, 
               side->otherside->area->x2, 
               side->otherside->area->y2);
        side = side->next;
    } while (side != f);
}
*/

int area_count_portals(const area_t *area) {
    const portalside_t *tmp = area->portals;
    const portalside_t *cur = area->portals;
    int count = 0;
    if (cur) do {
        assert(cur->next->prev == cur);
        count++;
        cur = cur->next;
    } while (cur != tmp);
    return count;
}

/* Verbindet zwei Areas durch ein Portal miteinander.
 * - Die beiden Areas liegen auf genau einer Seite aneinander.
 * - Es gibt eine gemeinsame Schnittflaeche
 * - Area1 liegt links/ueber Area2. 
 *
 * Es werden zwei Portalseiten sowie ein Portal angelegt. */
portal_t *area_connect(area_t *a1, area_t *a2) {
    portalside_t *side1 = (portalside_t*)malloc(sizeof(portalside_t));
    portalside_t *side2 = (portalside_t*)malloc(sizeof(portalside_t));
    portal_t *portal = (portal_t*)malloc(sizeof(portal_t));

    portal->path_id   = 0;
    portal->path_prev = NULL;

    portal->sides[0] = side1;
    portal->sides[1] = side2;

    // Portal Richtung und Ausdehnung berechnen
    if (a1->x2 + 1 == a2->x1) {
        portal->dir = PORTAL_DIR_VERTICAL;
        portal->s1  = MAX(a1->y1, a2->y1);
        portal->s2  = MIN(a1->y2, a2->y2);
        portal->cx  = TILE_X2(a1->x2);
    } else {
        portal->dir = PORTAL_DIR_HORIZONTAL;
        portal->s1  = MAX(a1->x1, a2->x1);
        portal->s2  = MIN(a1->x2, a2->x2);
        portal->cy  = TILE_Y2(a1->y2);
    }

    portal_update_center(portal);

    // side1 eintragen in area a1
    side1->area      = a1;
    side1->otherside = side2;
    side1->portal    = portal;

    if (!a1->portals) {
        a1->portals = side1->next = side1->prev = side1;
    } else {
        portalside_t *end = a1->portals;
        portalside_t *cur = a1->portals;
        if (cur == cur->next) {
            side1->next = cur->next;
            cur->next->prev = side1;
            cur->next = side1;
            side1->prev = cur;
        } else { 
            a1->portals = side1->next = side1->prev = side1; // HACK fuer die Asserts
            int side1_value = portalside_to_value(side1);
            // Mindestens zwei andere Portale vorhanden
            do {
                int cur_value  = portalside_to_value(cur);
                int next_value = portalside_to_value(cur->next);
                if ((next_value < cur_value  && side1_value > cur_value) ||
                    (cur_value < side1_value && side1_value < next_value) ||
                    (next_value < cur_value  && side1_value < next_value))
                {
                    // side1 hinter cur einhaengen
                    side1->next = cur->next;
                    cur->next->prev = side1;
                    cur->next = side1;
                    side1->prev = cur;
                    break;
                }
                cur = cur->next;
            } while (cur != end);
        }
    }

    assert(side1->next);
    assert(side1->prev);

    // side2 eintragen in area a2
    side2->area      = a2;
    side2->otherside = side1;
    side2->portal    = portal;

    if (!a2->portals) {
        a2->portals = side2->next = side2->prev = side2;
    } else {
        assert(a2->portals->next->prev == a2->portals);
        assert(a2->portals->prev->next == a2->portals);

        portalside_t *end = a2->portals;
        portalside_t *cur = a2->portals;
        if (cur == cur->next) {
            side2->next = cur->next;
            cur->next->prev = side2;
            cur->next = side2;
            side2->prev = cur;
        } else { 
            a2->portals = side2->next = side2->prev = side2; // HACK fuer die Asserts
            int side2_value = portalside_to_value(side2);
            // Mindestens zwei ander Portale vorhanden
            do {
                int cur_value  = portalside_to_value(cur);
                int next_value = portalside_to_value(cur->next);
                if ((next_value < cur_value  && side2_value > cur_value) ||
                    (cur_value < side2_value && side2_value < next_value) ||
                    (next_value < cur_value  && side2_value < next_value))
                {
                    // side2 hinter cur einhaengen
                    side2->next = cur->next;
                    cur->next->prev = side2;
                    cur->next = side2;
                    side2->prev = cur;
                    break;
                }
                cur = cur->next;
            } while (cur != end);
        }
    }

    assert(side2->next);
    assert(side2->prev);

    assert(a1->portals->next->prev == a1->portals);
    assert(a1->portals->prev->next == a1->portals);
    assert(a1->portals->otherside->otherside == a1->portals);
    assert(a2->portals->next->prev == a2->portals);
    assert(a2->portals->prev->next == a2->portals);
    assert(a2->portals->otherside->otherside == a2->portals);

    return portal;
}

/* Prueft, ob ein Portal der ueberlieferten Portalside mit
 * dessen naechsten Portal verbunden werden kann. */
int portal_is_mergable_with_neighbour(const portalside_t *side) {
    const portalside_t *neighbour = side->next;

    if (side->otherside->area != neighbour->otherside->area)
        return 0;

    int this_value      = portalside_to_value(side);
    int neighbour_value = portalside_to_value(neighbour);
    
    return this_value + portal_width(side->portal) == neighbour_value;
}

/* Geht alle Portalsides eines Area durch und prueft, ob zwei
 * hintereinander (siehe portalside_to_value) liegen. Liegen
 * sie hintereinander, so werden diese zu einem Portal 
 * verschmolzen. */
void area_merge_portals(area_t *area) {
    portalside_t *first, *cur;
again:    
    first = cur = area->portals;
    if (cur) do {
        assert(cur->next->prev == cur);
        assert(cur->prev->next == cur);
        
#ifndef NDEBUG
        int cur_value  = portalside_to_value(cur);
        int next_value = portalside_to_value(cur->next);
        
        assert(cur == cur->next || cur_value != next_value);
#endif
        
        if (portal_is_mergable_with_neighbour(cur)) {
            portalside_merge_with_neighbour(cur);
            goto again;
        }
        cur = cur->next;
    } while (cur != first);
}

/* MAP Functions ********************************************/

map_t *map_alloc() {
    return (map_t*)malloc(sizeof(map_t));
}

void map_free(map_t *map) {
    assert(map);
    // Uuah!
    for (int x = 0; x < map->width; x++) {
        for (int y = 0; y < map->height; y++) {
            area_t *area = MAP_TILE(map, x, y)->area;
            if (!area) continue;

            portalside_t *portal = area->portals;
            
            for (int xx = area->x1; xx <= area->x2; xx++) {
                for (int yy = area->y1; yy <= area->y2; yy++) {
                    MAP_TILE(map, xx, yy)->area = NULL;
                }
            }

            free(area);

            if (!portal) continue;

            portalside_t *cur = portal;
            do {
                if (cur->otherside) {
                    free(cur->portal);
                    cur->otherside->otherside = NULL;
                }
                portalside_t *tmp = cur; 
                cur = cur->next; 
                free(tmp);
            } while (cur != portal);
        }
    }

    free(map->tiles);
    free(map);
}

void map_init(map_t *map, int width, int height) {
    assert(map);
    map->width       = width;
    map->height      = height;
    map->next_region = 1;
    map->tiles       = (tile_t*)malloc(width * height * sizeof(tile_t));
    map->path_id     = 0;
    assert(map->tiles);

    for (int n = 0; n < map->width * map->height; n++) {
        tile_init(&map->tiles[n]);
    }

    map->dir[0] = -width;
    map->dir[1] = 1;
    map->dir[2] = width;
    map->dir[3] = -1;
}

void map_changeregion(map_t *map, int oldregion, int newregion) {
    for (int n = 0; n < map->width * map->height; n++) {
        tile_t *tile = &map->tiles[n];
        if (tile->region == oldregion)
            tile->region = newregion;
    }
}

/* Bestimmt die Region des Tile an x, y. Hat das
 * Tile keine Nachbarn, so wird eine neue Region
 * erstellt. Gibt es einen oder mehrere Nachbarn,
 * so werden die Regionen zusammengefuehrt. */
void map_set_region(map_t *map, int x, int y) {
    int new_tile_region = REGION_NONE;
    tile_t *tile = MAP_TILE(map, x, y);

    for (int d = 0; d < 4; d++) {
        tile_t *ntile = tile + map->dir[d];

        if (ntile->region == REGION_NONE) 
            continue;

        if (new_tile_region != REGION_NONE && new_tile_region != ntile->region) {
            map_changeregion(map, ntile->region, new_tile_region);
        } else {
            new_tile_region = ntile->region;
        }
    }
            
    if (new_tile_region == REGION_NONE)
        new_tile_region = map->next_region++;

    tile->region = new_tile_region;
}

/* Die beiden der ueberlieferten und der gegenueberliegenden 
 * Portalseiten werden verschmolzen. */
void map_zap_portal(map_t *map, portalside_t *side1, int dir) {
    portalside_t *side2 = side1->otherside;

    area_t *a1 = side1->area;
    area_t *a2 = side2->area;

    assert(side1 != side2);
    assert(a1 != a2);

    assert(a1);
    assert(a2);
    assert(side1);
    assert(side2);

    assert(side1->portal == side2->portal);
    assert(side2->otherside == side1);

    portal_t *portal = side1->portal;

    assert((portal->sides[0] == side1 && portal->sides[1] == side2) ||
           (portal->sides[1] == side1 && portal->sides[0] == side2));

    // Area a1 fuer a2 Tiles eintragen 
    for (int x = a2->x1; x <= a2->x2; x++) {
        for (int y = a2->y1; y <= a2->y2; y++) {
            tile_t *tile = MAP_TILE(map, x, y);
            assert(tile->area == a2);
            tile->area = a1;
        }
    }

    // neue Dimension fuer a1
    if (dir == PORTAL_DIR_HORIZONTAL) {
        a1->y1 = MIN(a1->y1, a2->y1);
        a1->y2 = MAX(a1->y2, a2->y2);
    } else {
        a1->x1 = MIN(a1->x1, a2->x1);
        a1->x2 = MAX(a1->x2, a2->x2);
    } 

    // verweise von portalseiten von area 2 aendern
    portalside_t *first = a2->portals;
    portalside_t *cur   = a2->portals;
    do {
        assert(cur->next->prev == cur);
        assert(cur->prev->next == cur);
        assert(cur->area == a2);
        cur->area = a1;
        cur = cur->next;
    } while (cur != first);

    // portallisten von a1 und a2 verschmelzen
    //
    //  s11 ->-             ->- s21
    //   ^      \         /      v
    //  s12    side1    side2   s22
    //   ^      /         \      v
    //  s13 -<-             -<- s23 
    //
    //  side1 und side2 werden geloscht.
    
    if (side1->next == side1 && side2->next == side2) {
        // portale sind jeweils die einzigen auf beiden areas?
        a1->portals = NULL;
    } else if (side1->next == side1) {
        // side1 ist einziges Portal auf area1
        side2->next->prev = side2->prev; 
        side2->prev->next = side2->next;
        a1->portals = side2->next;
    } else if (side2->next == side2) {
        // side2 ist einziges Portal auf area2
        side1->next->prev = side1->prev;
        side1->prev->next = side1->next;
        a1->portals = side1->next;
    } else {
        // side1 und side2 sind jeweils nicht die 
        // einzigen portalsides
        assert(side1->next->prev == side1);
        assert(side1->prev->next == side1);
        assert(side2->next->prev == side2);
        assert(side2->prev->next == side2);

        assert(a1->portals->next->prev == a1->portals);
        assert(a1->portals->next->next->prev->prev == a1->portals);
        assert(a1->portals->prev->next == a1->portals);
        assert(a1->portals->prev->prev->next->next == a1->portals);
        assert(a2->portals->next->prev == a2->portals);
        assert(a2->portals->next->next->prev->prev == a2->portals);
        assert(a2->portals->prev->next == a2->portals);
        assert(a2->portals->prev->prev->next->next == a2->portals);

        side1->prev->next = side2->next;
        side2->next->prev = side1->prev;
        side1->next->prev = side2->prev;
        side2->prev->next = side1->next;
        a1->portals = side1->next;
        
        assert(a1->portals != side1);
        assert(a1->portals->next != side1);
        assert(a1->portals->next->next != side1);
        assert(a1->portals->next->next->next != side1);
        assert(a1->portals->next->prev == a1->portals);
        assert(a1->portals->prev->next == a1->portals);
    }

    assert(!a1->portals || a1->portals->next->prev == a1->portals);
    assert(!a1->portals || a1->portals->prev->next == a1->portals);
    assert(!a1->portals || a1->portals->otherside->otherside == a1->portals);
    
    // portal zwischen a1 und a2 loeschen
    portal_destroy(portal);

    // beide portalseiten Loeschen
    portalside_destroy(side1);
    portalside_destroy(side2);

    // uebernommenes Area loeschen
    area_destroy(a2);

    area_merge_portals(a1);
}
    
/* Prueft, ob ein Area mit seinen Nachbarn verschmelzbar ist.
 * Wenn ja, so wird das Area verschmolzen und das entstandene
 * Area wird erneut geprueft. */
void map_area_merge(map_t *map, area_t *area) {
    if (!area->portals)
        return;

    portalside_t *first, *cur;
again:
    first = cur = area->portals;

    if (cur) do {
        assert(cur->next->prev == cur);
        assert(cur->prev->next == cur);
        assert(cur->otherside->otherside == cur);

        area_t *otherarea = cur->otherside->area;

        assert(cur->area != otherarea);

        if (cur->portal->dir == PORTAL_DIR_HORIZONTAL) {
            if (area->x1 == otherarea->x1 &&
                area->x2 == otherarea->x2) {
                /* otherarea uebernehmen */
                map_zap_portal(map, cur, PORTAL_DIR_HORIZONTAL);
                goto again;
            } 
        } else {
            if (area->y1 == otherarea->y1 &&
                area->y2 == otherarea->y2) {
                /* otherarea uebernehmen */
                map_zap_portal(map, cur, PORTAL_DIR_VERTICAL);
                goto again;
            } 
        }

        cur = cur->next;
    } while (cur != first);
}

/* Erzeugt ein neues 1x1 Area und verbindet es ueber 
 * Portale mit seinen Nachbarn. */
area_t *map_area_add(map_t *map, int x, int y) {
    area_t *area = (area_t*)malloc(sizeof(area_t));
    area->x1 = area->x2 = x;
    area->y1 = area->y2 = y;
    area->portals = NULL;

    tile_t *tile = MAP_TILE(map, x, y);

    tile->area = area;

    for (int d = 0; d < 4; d++) {
        tile_t *ntile = tile + map->dir[d];

        if (ntile->region == REGION_NONE) 
            continue;

        assert(ntile->area);

        switch (d) {
            case 0: area_connect(ntile->area, area); break;
            case 1: area_connect(area, ntile->area); break;
            case 2: area_connect(area, ntile->area); break;
            case 3: area_connect(ntile->area, area); break;
        }
    }

    return area;
}

void map_unsolid(map_t *map, int x, int y) {
    map_set_region(map, x, y);
    map_area_merge(map, map_area_add(map, x, y));
}

/* Legt ein neues Feld an x, y frei, setzt Region, erzeugt
 * ein neues Area und verbindet es mit benachbarten Areas. */
int map_dig(map_t *map, int x, int y) {
    if (!MAP_IS_ON_MAP(map, x, y))
        return 0;
    
    tile_t *tile = MAP_TILE(map, x, y);

    if (tile->walkable)
        return 1;

    tile->walkable = 1;

    map_set_region(map, x, y);
#ifdef PATHFIND_AREA_MERGE
    map_area_merge(map, map_area_add(map, x, y));
#else
    map_area_add(map, x, y);
#endif
    return 1;
}

/*
void map_get_info(map_t *map, int x, int y) {
    tile_t *tile = MAP_TILE(map, x, y);
    printf("tile %p, region=%d\n", tile, tile->region);
    area_get_info(tile->area);
}
*/

int map_get_region(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->region;
}

void map_get_area_dimensions(map_t *map, int x, int y, int *x1, int *y1, int *x2, int *y2) {
    area_t *area = MAP_TILE(map, x, y)->area;
    *x1 = area->x1;
    *y1 = area->y1;
    *x2 = area->x2;
    *y2 = area->y2;
}

int map_get_width(map_t *map) {
    return map->width;
}

int map_get_height(map_t *map) {
    return map->height;
}

int map_walkable(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->walkable;
}

int map_get_area_x1(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->area->x1;
}

int map_get_area_y1(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->area->y1;
}

int map_get_area_x2(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->area->x2;
}

int map_get_area_y2(map_t *map, int x, int y) {
    return MAP_TILE(map, x, y)->area->y2;
}
