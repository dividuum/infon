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
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "path.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(x)   ((x)<0?-(x):(x))

inline int path_calc_cost(int x1, int y1, int x2, int y2) {
    //return ABS(x1 - x2) + ABS(y1 - y2);
    const int xd = x1 - x2;
    const int yd = y1 - y2;
    return (int)sqrt(xd * xd + yd * yd);
} 

pathnode_t *pathnode_new(int x, int y) {
    pathnode_t *node = (pathnode_t*)malloc(sizeof(pathnode_t));
    node->x = x;
    node->y = y;
    return node;
}

void finder_openset_moveup(open_t *set, int curidx) {
    // und in Richtung wurzel des Heaps verschieben,
    // bis richtige position erreicht ist
    while (curidx > 0) {
        int loweridx = (curidx - 1) >> 1;

        if (set[curidx].cost >= set[loweridx].cost)
            break;

        open_t tmp = set[curidx];
        set[curidx] = set[loweridx];
        set[loweridx] = tmp;

        curidx = loweridx;
    }
}

void finder_openset_add(pathfinder_t *finder, portal_t *portal, 
                        int side, int cost) 
{
    open_t *set = finder->openset;

    // Neues open_t fuellen
    set[finder->numopen].portal     = portal;
    set[finder->numopen].fromside   = side;
    set[finder->numopen].cost       = cost;

    int added_idx = finder->numopen;

    finder->numopen++;

    assert(finder->numopen < finder->maxopen); // XXX

    finder_openset_moveup(set, added_idx);
}

void finder_openset_update(pathfinder_t *finder, int idx, 
                        int side, int cost) 
{
    open_t *set = finder->openset;

    set[idx].fromside   = side;
    set[idx].cost       = cost;

    finder_openset_moveup(set, idx);
}

// Entfernt set[0], indem das letzte element an 
// stelle 0 verschoben wird und dann an die richtige
// stelle vertauscht wird.
void finder_openset_shift(pathfinder_t *finder) {
    assert(finder->numopen > 0);
    finder->numopen--;

    open_t *set = finder->openset;

    set[0] = set[finder->numopen];

    int curidx = 0;
    int maxidx = (finder->numopen >> 1) - 1;

    while (curidx <= maxidx) {
        int higheridx = (curidx << 1) + 1;

        if (higheridx < finder->numopen - 1 && 
            set[higheridx].cost >= set[higheridx + 1].cost)
            higheridx++;

        if (set[curidx].cost <= set[higheridx].cost) 
            break;

        open_t tmp = set[curidx];
        set[curidx] = set[higheridx];
        set[higheridx] = tmp;

        curidx = higheridx;
    }
}

int finder_openset_find(pathfinder_t *finder, portal_t *portal) {
    open_t *set = finder->openset;
    for (int idx = 0; idx < finder->numopen; idx++) {
        if (set[idx].portal == portal)
            return idx;
    }
    return -1;
}

void path_print(pathnode_t *path) {
    while (path) {
        printf("%d,%d\n", path->x, path->y);
        path = path->next;
    }
}

char *path_as_string(pathnode_t *path) {
    // XXX: unsicher
    static char buf[2000];
    char *pos = buf;
    while (path) {
        pos += sprintf(pos, "%d,%d|", path->x, path->y);
        path = path->next;
    }
    sprintf(pos, "0,0");
    return buf;
}

pathfinder_t *finder_alloc() {
    return (pathfinder_t*)malloc(sizeof(pathfinder_t));
}

void finder_init(pathfinder_t *finder) {
    finder->path_id = 0;
    finder->maxopen = 4096;
    finder->openset = (open_t*)malloc(finder->maxopen * sizeof(open_t));

    finder->randcnt = 0;
    for (int i = 0; i < 256; i++)
        finder->random[i] = rand() % 50;
}

void finder_shutdown(pathfinder_t *finder) {
    free(finder->openset);
}

pathnode_t *finder_find(pathfinder_t *finder, map_t *map, int sx, int sy, int ex, int ey) {
    const int tsx = X_TO_TILEX(sx);
    const int tsy = Y_TO_TILEY(sy);
    const int tex = X_TO_TILEX(ex);
    const int tey = Y_TO_TILEY(ey);

    const tile_t *stile = MAP_TILE(map, tsx, tsy);
    const tile_t *etile = MAP_TILE(map, tex, tey);

    // Ziel und Quelle gleich
    if (sx == ex && sy == ey) {
        pathnode_t *path = pathnode_new(ex, ey); 
        path->next = NULL;
        return path;
    }

    // Gibt es ueberhaupt eine Verbindung?
    if (stile->region != etile->region) 
        return NULL;

    assert(stile->walkable);
    assert(etile->walkable);

    // Spezialfall: Nur ein Tile
    if ((ABS(tsx - tex) == 1 && (tsy == tey)) ||
        ((tsx == tex) && ABS(tsy - tey) == 1)) 
    {
        pathnode_t *path = pathnode_new(ex, ey); 
        path->next = NULL;
        return path;
    }

    const area_t *sarea = stile->area;
    const area_t *earea = etile->area;

    // Ziel und Quelle im gleichen Area?
    if (sarea == earea) {
        pathnode_t *path = pathnode_new(ex, ey); 
        path->next = NULL;
        return path;
    }

    // Neue Path id zum eindeutigen erkennen, ob
    // path_prev in portal_t aktuell oder veraltet ist holen.
    finder->path_id = ++map->path_id;

    // Openset ruecksetzen
    finder->numopen = 0;

    const portalside_t *cur, *first;       

    // Vom Ziel aus vorgehen und in Richtung Start
    // vorarbeiten. Haette vielleicht den Vorteil, das
    // bei einem Teilberechneten Pfad bereits in Richtung
    // des letzten Pathnodes losgelaufen werden kann.
    cur = first = earea->portals;
    assert(cur);

    // Moegliche Portale fuer erste Iteration in Openset
    // eintragen.
    do {
        portal_t *portal        = cur->portal;

        portal->path_id         = finder->path_id;
        portal->cost_from_start = path_calc_cost(ex, ey, portal->cx, portal->cy);
        portal->path_prev = NULL;

        finder_openset_add(finder, portal,
                           portal->sides[1]->area == earea, 
                           portal->cost_from_start + 
                           path_calc_cost(portal->cx, portal->cy, sx, sy) * DEST_WEIGHT);
        cur = cur->next;
    } while (cur != first);

    portal_t *pathportal = NULL;
    
    while (finder->numopen > 0) {
        // Openset Element mit den geringsten Kosten holen
        open_t *open     = finder->openset;

        portal_t *portal = open->portal;
        int       side   = open->fromside;
        // int       costs  = open->cost; XXX: Wofuer war das da?

        // Von aktuellem Portal aus ist das StartArea erreichbar?
        if (portal->sides[0]->area == sarea ||
            portal->sides[1]->area == sarea) 
        {
            pathportal = portal;
            break;
        }

        // Erstes Element loeschen. Ab hier ist open ungueltig!
        finder_openset_shift(finder);

        // Das gerade betrachtete Portal verbindet 2 Areas
        // miteinander. In side steht, von welcher Seite 
        // aus wir das Portal betreten haben.
        // Area ist Area, dessen weitere Portale wir jetzt
        // durchschauen.
        area_t *area = portal->sides[1 - side]->area; 
        
        // Moegliche Zielportale durchgehen
        cur = first = portal->sides[1 - side];
        do {
            portal_t *nportal       = cur->portal;
            //printf("moegliches Ziel: %p ... ", nportal);

            // Kandidat ist Portal, durch das wird aktuelles
            // Area betreten haben?
            if (nportal == portal) {
                if (cur->next == first) 
                    break;
                cur = cur->next;
                continue;
            }

            // Kosten zu diesem Portal sind die Kosten
            // zum bisherigen Portal plus die Kosten
            // von Portal zu Portal.
            const int ncosts = portal->cost_from_start + 
                               path_calc_cost(portal->cx,  portal->cy, 
                                              nportal->cx, nportal->cy) +
                               //finder->random[++finder->randcnt & 0xFF];
                               0;

            // Noch nicht bekanntes Portal?
            if (nportal->path_id != finder->path_id) {
                nportal->path_id         = finder->path_id;
                nportal->cost_from_start = ncosts;
                nportal->path_prev       = portal;

                finder_openset_add(finder, nportal,
                                   nportal->sides[1]->area == area,
                                   ncosts + 
                                   path_calc_cost(nportal->cx, nportal->cy, sx, sy) * DEST_WEIGHT);
            } else if (ncosts < nportal->cost_from_start) {
                nportal->cost_from_start = ncosts;
                nportal->path_prev       = portal;

                const int idx = finder_openset_find(finder, nportal);

                // Nicht mehr im Openset?
                if (idx < 0) {
                    finder_openset_add(finder, nportal,
                                       nportal->sides[1]->area == area,
                                       ncosts + 
                                       path_calc_cost(nportal->cx, nportal->cy, sx, sy) * DEST_WEIGHT);
                } else {
                    finder_openset_update(finder, idx,
                                          nportal->sides[1]->area == area,
                                          ncosts + 
                                          path_calc_cost(nportal->cx, nportal->cy, sx, sy) * DEST_WEIGHT);
                }
            }
            cur = cur->next;
        } while (cur != first);
    }

    assert(finder->numopen > 0);

    // von pathportal zu pathportal vom pfandanfang zum
    // pfadende durchhangeln und dabei Ergebnispfad 
    // zusammenbauen
    pathnode_t pathstart;
    pathnode_t *curpath = &pathstart;
    int lastx = sx, lasty = sy;
    do {
        // Die cx und cy Koordinaten befinden sich jeweils an der linken bzw
        // oberen Kante eines Tiles. Kommt ein Pfad daher von Rechts oder von 
        // Unten zu einem solchen Portal, so wuerde der hier erzeugte Knoten noch im 
        // aktuell durchlaufenen Area liegen. Wird ein solcher Weg abgelaufen und
        // direkt nach dem Knoten folgt eine ~90 Grad Kurve, so kann es sein, dass 
        // die x bzw y Koordinate fuer eine kurze Zeit nicht veraendert wird, 
        // d.h. das Area, obwohl schon auf dem Weg zum naechsten Knoten, nicht 
        // verlassen wird. Wird in diesem Moment eine erneute Suche zum gleichen
        // Ziel begonnen, wird der zuvor bereits ueberquerte Knoten erneut
        // als Pfadknoten gefunden und es kommt zu einer haesslichen Endlosschleife.
        // Daher wird von x bzw y der Wert 1 subtrahiert, falls das Portal von 
        // Rechts bzw von unten angesteuert wird.
        curpath->next = pathnode_new(pathportal->cx - (pathportal->cx <= lastx), 
                                     pathportal->cy - (pathportal->cy <= lasty));
        lastx = pathportal->cx;
        lasty = pathportal->cy;
        curpath       = curpath->next;
        pathportal    = pathportal->path_prev;
    } while (pathportal);
    curpath->next = pathnode_new(ex, ey);
    curpath->next->next = NULL;

    return pathstart.next;
}

void path_delete(pathnode_t *path) {
    while (path) {
        pathnode_t *tmp = path;
        path = path->next;
        free(tmp);
    }
}

int map_visited(pathfinder_t *finder, map_t *map, int x, int y) {
    area_t *area = MAP_TILE(map, x, y)->area;
    if (!area)
        return 0;
    portalside_t *f, *c;
    f = c = area->portals;
    if (c) do {
        if (c->portal->path_id == finder->path_id)
            return 1;
        c = c->next;
    } while (c != f);
    return 0;
}
