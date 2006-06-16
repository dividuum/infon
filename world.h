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

#ifndef WORLD_H
#define WORLD_H

#include "path.h"

// Begrenzt durch die Anzahl vorhandener Food Sprites
#define MAX_TILE_FOOD 9999

// XXX
typedef enum {
    SOLID = 0,
    PLAIN = 1,
    WATER = 2,
    LAVA  = 3,
} maptype_e;

int         world_dig(int x, int y, maptype_e type);
int         world_walkable(int x, int y);
int         world_get_food(int x, int y);
int         world_add_food(int x, int y, int amount);
int         world_food_eat(int x, int y, int amount);

int         world_width();
int         world_height();
int         world_koth_x();
int         world_koth_y();

pathnode_t *world_findpath(int x1, int y1, int x2, int y2);
void        world_find_digged(int *x, int *y);

void        world_set_display_mode(int mode);

void        world_draw();

void        world_init(int w, int h);
void        world_shutdown();

#endif
