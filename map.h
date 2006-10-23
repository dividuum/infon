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

#ifndef PATHFINDING_MAP_H
#define PATHFINDING_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define TILE_SCALE 256

typedef struct area_s area_t;
typedef struct portalside_s portalside_t;
typedef struct portal_s portal_t;

/* Tile */

typedef struct tile_s {
    int     walkable;
    int     region;
    area_t *area;
#define REGION_NONE (0)
} tile_t;

#define TILE_WIDTH  TILE_SCALE
#define TILE_HEIGHT TILE_SCALE

#define TILE_X1(x) ((x) * TILE_WIDTH)
#define TILE_Y1(y) ((y) * TILE_HEIGHT)

#define TILE_X2(x) (TILE_X1(x) + TILE_WIDTH  - 1)
#define TILE_Y2(y) (TILE_Y1(y) + TILE_HEIGHT - 1)

#define TILE_XCENTER(x) (TILE_X1(x) + TILE_WIDTH  / 2)
#define TILE_YCENTER(y) (TILE_Y1(y) + TILE_HEIGHT / 2)

#define X_TO_TILEX(x) ((int)((x) / TILE_WIDTH))
#define Y_TO_TILEY(y) ((int)((y) / TILE_HEIGHT))


/* Area */

struct area_s {
    int x1;
    int y1;
    int x2;
    int y2;
    portalside_t *portals;
};

/* Portal */

struct portalside_s {
    area_t       *area;
    portalside_t *otherside;
    portal_t     *portal;
    portalside_t *next;
    portalside_t *prev;
};

struct portal_s {
    portalside_t *sides[2]; // 0 == LEFT/ABOVE  1 == RIGHT/BELOW
    int dir;
#define PORTAL_DIR_HORIZONTAL 0
#define PORTAL_DIR_VERTICAL   1
    int s1;
    int s2;
    int cx;
    int cy;

    // Von der Pfadsuche verwendet
    int       path_id;
    int       cost_from_start;
    portal_t *path_prev;
};

/* Map */

typedef struct map_s {
    int       path_id;
    int       width;
    int       height;
    tile_t   *tiles;
    int       next_region;
    int       dir[4];
} map_t;

#define  MAP_TILE(map, x, y)    (&(map)->tiles[(map)->width * (y) + (x)])

#define  MAP_IS_ON_MAP(map, x, y) ((x) >= 0 && (x) < (map)->width && \
                                   (y) >= 0 && (y) < (map)->height)

int     map_dig(map_t *map, int x, int y);
void    map_get_info(map_t *map, int x, int y);

int     map_get_region(map_t *map, int x, int y);
void    map_get_area_dimensions(map_t *map, int x, int y, int *x1, int *y1, int *x2, int *y2);
int     map_get_width(map_t *map);
int     map_get_height(map_t *map);
int     map_walkable(map_t *map, int x, int y);
int     map_get_area_x1(map_t *map, int x, int y);
int     map_get_area_y1(map_t *map, int x, int y);
int     map_get_area_x2(map_t *map, int x, int y);
int     map_get_area_y2(map_t *map, int x, int y);

map_t  *map_alloc();
void    map_init(map_t *map, int width, int height);
void    map_free(map_t *map);

#ifdef __cplusplus
}
#endif

#endif
