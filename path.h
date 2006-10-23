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

#ifndef PATHFINDING_PATH_H
#define PATHFINDING_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "map.h"

#define DEST_WEIGHT 1

typedef struct pathnode_s pathnode_t;
typedef struct open_s     open_t;

typedef struct pathfinder_s {
    int         path_id;
    int         numopen;
    int         maxopen;
    open_t     *openset;
    int         randcnt;
    int         random[256];
} pathfinder_t;

struct open_s {
    portal_t *portal;
    int fromside;
    int cost;
};

struct pathnode_s {
    int x;
    int y;
    pathnode_t *next;
};

pathnode_t *finder_find(pathfinder_t *finder, map_t *map, int sx, int sy, int ex, int ey);
pathnode_t *pathnode_new(int x, int y);

void        path_delete(pathnode_t *path);

void        finder_init    (pathfinder_t *finder);
void        finder_shutdown(pathfinder_t *finder);

#ifdef __cplusplus
}
#endif

#endif
