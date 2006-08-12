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

#ifndef COMMON_CREATURE_H
#define COMMON_CREATURE_H

#define MAXCREATURES       256

#define CREATURE_COLORS     16

typedef enum {
    CREATURE_SMALL,
    CREATURE_BIG,
    CREATURE_FLYER,
    CREATURE_UNUSED
} creature_type;
#define CREATURE_TYPES       4

#define CREATURE_DIRECTIONS 32

#define CREATURE_ANIMS       2

typedef enum {
    CREATURE_IDLE,
    CREATURE_WALK,
    CREATURE_HEAL,
    CREATURE_EAT,
    CREATURE_ATTACK,
    CREATURE_CONVERT,
    CREATURE_SPAWN,
    CREATURE_FEED,
} creature_state;
#define CREATURE_STATES      8

#define CREATURE_POS_RESOLUTION   16
#define CREATURE_SPEED_RESOLUTION  4

#define CREATURE_DIRTY_ALIVE        (1 << 0) 
#define CREATURE_DIRTY_TYPE         (1 << 1)
#define CREATURE_DIRTY_FOOD_HEALTH  (1 << 2)
#define CREATURE_DIRTY_STATE        (1 << 3)
#define CREATURE_DIRTY_PATH         (1 << 4) 
#define CREATURE_DIRTY_TARGET       (1 << 5)
#define CREATURE_DIRTY_MESSAGE      (1 << 6)
#define CREATURE_DIRTY_SPEED        (1 << 7)

#define CREATURE_DIRTY_ALL         0xFF
#define CREATURE_DIRTY_NONE        0x00

#endif
