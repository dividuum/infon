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

#ifndef RULES_H
#define RULES_H

#define PLAYER_CREATURE_RESPAWN_DELAY 2000
#define CREATURE_SUICIDE_POINTS        -40
#define CREATURE_DIED_POINTS            -3
#define CREATURE_VICTIM_POINTS_0        -3
#define CREATURE_VICTIM_POINTS_1       -20
#define CREATURE_KILLED_POINTS          10
#define CREATURE_KOTH_POINTS             5

#define RESTRICTIVE                      0

#define NO_PLAYER_DISCONNECT   (120 * 1000)

#define LUA_MAX_MEM       (8 * 1024 * 1024)
#define LUA_MAX_CPU                  400000

#endif
