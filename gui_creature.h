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

#ifndef GUI_CREATURE_H
#define GUI_CREATURE_H

#include "common_creature.h"
#include "packet.h"

typedef struct gui_creature_s {
    int             used;

    int             x;
    int             y;
    int             speed;

    int             old_x;
    int             old_y;
    int             path_x;
    int             path_y;

    int             dir;
    int             type;
    int             food;
    int             health;
    int             target;
    int             color;
    creature_state  state;

    char message[9];
} gui_creature_t;

void        gui_creature_draw();
void        gui_creature_move(int delta);

/* Network */
void gui_creature_from_network(packet_t *packet);

void        gui_creature_init();
void        gui_creature_shutdown();

#endif
