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

#ifndef SDL_VIDEO_H
#define SDL_VIDEO_H

#include <SDL.h>

#include "sdl_sprite.h"

#define X_TO_SCREENX(x) ((x) * SPRITE_TILE_SIZE / TILE_WIDTH)
#define Y_TO_SCREENY(y) ((y) * SPRITE_TILE_SIZE / TILE_HEIGHT)

void         video_draw(int x, int y, SDL_Surface *sprite);
void         video_fullscreen_toggle();
void         video_flip();
void         video_set_title(const char *title);

int          video_width();
int          video_height();

void         video_resize(int w, int h);

void         video_line_green_red(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2);
void         video_line_green(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2);
void         video_hline(Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void         video_rect(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void         video_write(Sint16 x, Sint16 y, const char *text);
void         video_tiny(Sint16 x, Sint16 y, const char *text);

SDL_Surface *video_new_surface(int w, int h);

void         video_init(int w, int h, int fs);
void         video_shutdown();

#endif
