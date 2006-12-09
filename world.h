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
#include "packet.h"
#include "server.h"
#include "common_world.h"

int         world_is_within_border(int x, int y);

int         world_walkable(int x, int y);
int         world_get_food(int x, int y);
maptype_e   world_get_type(int x, int y);
int         world_add_food(int x, int y, int amount);
int         world_food_eat(int x, int y, int amount);

int         world_width();
int         world_height();
int         world_koth_x();
int         world_koth_y();

pathnode_t *world_findpath(int x1, int y1, int x2, int y2);
int         world_find_plain(int *x, int *y);

void        world_tick();

/* Network */
void        world_send_initial_update(client_t *client);

void        world_init();
void        world_shutdown();

#endif
