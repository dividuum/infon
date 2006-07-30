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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <lua.h>
#include <SDL.h>

lua_State *L;

Uint32     game_time;
int        game_round;

#define GAME_NAME "Infon Battle Arena 'Network Edition'"

// Pfadsuche, bei der mehrere beieinanderliegende
// Tiles zu einem Area zusammengefasst werden. Die
// Pfadsuche verwendet dann diese Areas fuer die 
// Pfadsuche. Dies ist schneller und die gefundenen
// Pfade sehen natuerlicher aus. Allerdings ergeben 
// sich gelegentlich aufgrund der zusammenfassung
// der Tiles komische Pfade.
#define PATHFIND_AREA_MERGE 

// Netzwerk Protokol
#define PROTOCOL_VERSION 4

#endif
