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

#ifndef COMMON_PLAYER_H
#define COMMON_PLAYER_H

#define MAXPLAYERS 32

#define PLAYER_DIRTY_ALIVE   (1 << 0)
#define PLAYER_DIRTY_NAME    (1 << 1)
#define PLAYER_DIRTY_COLOR   (1 << 2)
#define PLAYER_DIRTY_CPU     (1 << 3)
#define PLAYER_DIRTY_SCORE   (1 << 4)

#define PLAYER_DIRTY_ALL         0x1F
#define PLAYER_DIRTY_NONE        0x00

#endif
