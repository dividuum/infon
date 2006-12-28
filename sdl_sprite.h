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

#ifndef SDL_SPRITE_H
#define SDL_SPRITE_H

#include <SDL.h>

#include "common_creature.h"

#define SPRITE_NUM 16384

// Tiles Konstanten
#define SPRITE_BORDER           0
#define SPRITE_NUM_BORDER       16

#define SPRITE_SOLID            (SPRITE_BORDER + SPRITE_NUM_BORDER)
#define SPRITE_NUM_SOLID        16

#define SPRITE_PLAIN            (SPRITE_SOLID + SPRITE_NUM_SOLID)
#define SPRITE_NUM_PLAIN        16

#define SPRITE_SNOW_BORDER      (SPRITE_PLAIN + SPRITE_NUM_PLAIN)
#define SPRITE_NUM_SNOW_BORDER  16

#define SPRITE_SNOW_SOLID       (SPRITE_SNOW_BORDER + SPRITE_NUM_SNOW_BORDER)
#define SPRITE_NUM_SNOW_SOLID   16

#define SPRITE_SNOW_PLAIN       (SPRITE_SNOW_SOLID + SPRITE_NUM_SNOW_SOLID)
#define SPRITE_NUM_SNOW_PLAIN   16

#define SPRITE_KOTH             (SPRITE_SNOW_PLAIN + SPRITE_NUM_SNOW_PLAIN)
#define SPRITE_NUM_KOTH         1

#define SPRITE_WATER            (SPRITE_KOTH  + SPRITE_NUM_KOTH)
#define SPRITE_NUM_WATER        4

#define SPRITE_LAVA             (SPRITE_WATER + SPRITE_NUM_WATER)
#define SPRITE_NUM_LAVA         4

#define SPRITE_DESERT           (SPRITE_LAVA + SPRITE_NUM_LAVA)
#define SPRITE_NUM_DESERT       10

#define SPRITE_NUM_TILES (SPRITE_NUM_SOLID  + \
                          SPRITE_NUM_PLAIN  + \
                          SPRITE_NUM_BORDER + \
                          SPRITE_NUM_SNOW_SOLID  + \
                          SPRITE_NUM_SNOW_PLAIN  + \
                          SPRITE_NUM_SNOW_BORDER + \
                          SPRITE_NUM_KOTH   + \
                          SPRITE_NUM_WATER  + \
                          SPRITE_NUM_LAVA   + \
                          SPRITE_NUM_DESERT)

#define SPRITE_TILE_SIZE    16

// Food Konstanten
#define SPRITE_FOOD             256
#define SPRITE_NUM_FOOD         10

#define SPRITE_SNOW_FOOD        (SPRITE_FOOD + SPRITE_NUM_FOOD)
#define SPRITE_NUM_SNOW_FOOD    10

// Thought Konstanten
#define SPRITE_THOUGHT          (SPRITE_SNOW_FOOD + SPRITE_NUM_SNOW_FOOD)
#define SPRITE_NUM_THOUGHT      CREATURE_STATES + 1 /* + Smile */

// Koth Krone
#define SPRITE_CROWN            (SPRITE_THOUGHT + SPRITE_NUM_THOUGHT)
#define SPRITE_NUM_CROWN        1

// Logo
#define SPRITE_LOGO             (SPRITE_CROWN + SPRITE_NUM_CROWN)
#define SPRITE_HALO             (SPRITE_LOGO + 1)

#define SPRITE_CREATURE         512

// Creature Konstanten
#define CREATURE_SPRITE(player, type, direction, anim) \
        (SPRITE_CREATURE + (player) * CREATURE_TYPES * CREATURE_DIRECTIONS * CREATURE_ANIMS + \
                                              (type) * CREATURE_DIRECTIONS * CREATURE_ANIMS + \
                                                               (direction) * CREATURE_ANIMS + \
                                                                                     (anim))

SDL_Surface *sprite_get(int i);
int          sprite_exists(int i);
void         sprite_render_player_creatures(int playerno, int r1, int g1, int b1, int r2, int g2, int b2);

void         sprite_init();
void         sprite_shutdown();

#endif
