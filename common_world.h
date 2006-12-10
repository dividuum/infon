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

#ifndef COMMON_WORLD_H
#define COMMON_WORLD_H

// Begrenzt durch die Anzahl vorhandener Food Sprites
#define MAX_TILE_FOOD 9999

typedef enum {
    TILE_SOLID = 0,
    TILE_PLAIN,

    TILE_LAST_DEFINED,
} maptype_e;

typedef enum {
    TILE_GFX_SOLID = 0,
    TILE_GFX_PLAIN,
    TILE_GFX_BORDER,

    TILE_GFX_SNOW_SOLID,
    TILE_GFX_SNOW_PLAIN,
    TILE_GFX_SNOW_BORDER,

    TILE_GFX_WATER,
    TILE_GFX_LAVA,
    TILE_GFX_NONE,
    TILE_GFX_KOTH,
    TILE_GFX_DESERT,

    TILE_GFX_LAST_DEFINED
} mapgfx_e;

#endif
