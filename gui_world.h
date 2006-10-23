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

#ifndef GUI_WORLD_H
#define GUI_WORLD_H

#include "packet.h"
#include "common_world.h"

void        gui_world_draw();
void        gui_world_recenter();
void        gui_world_center_change(int dx, int dy);
void        gui_world_center_set(int x, int y);

int         gui_world_x_offset();
int         gui_world_y_offset();

/* Network */
void        gui_world_from_network(packet_t *packet);
void        gui_world_info_from_network(packet_t *packet);

void        gui_world_init();
void        gui_world_shutdown();

#endif
