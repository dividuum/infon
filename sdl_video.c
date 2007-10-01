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

#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_syswm.h>
#include <sge.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "global.h"
#include "sdl_sprite.h"
#include "sdl_video.h"

static SDL_Surface *screen;
static Uint32       flags = SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_HWACCEL | SDL_RESIZABLE;

static sge_bmpFont *font;

static char tiny_font[1792]; // XXX Hardcoded

void video_init(int w, int h, int fs) {
    if (SDL_Init(SDL_INIT_VIDEO) == -1) 
        die("Couldn't initialize SDL: %s", SDL_GetError());

    const SDL_VideoInfo *vi = SDL_GetVideoInfo();

    if (!vi)
        die("SDL_getVideoInfo() failed: %s", SDL_GetError());

    if (!(vi->vfmt->BitsPerPixel == 16 || vi->vfmt->BitsPerPixel == 32))
        die("insufficient color depth");

    if (fs) 
        flags |= SDL_FULLSCREEN;

    screen = SDL_SetVideoMode(w, h, vi->vfmt->BitsPerPixel, flags);

    if (!screen)
        die("Couldn't set display mode: %s", SDL_GetError());

    video_set_title(GAME_NAME);
    SDL_ShowCursor(1);
#if WIN32
    SDL_SysWMinfo wminfo;
    if (SDL_GetWMInfo(&wminfo) == 1) {
        HWND      hwnd   = wminfo.window;
        HINSTANCE handle = GetModuleHandle(NULL);
        HICON     icon   = LoadIcon(handle, "icon");
        SetClassLong(hwnd, GCL_HICON, (LONG) icon);
    }
#endif
    // SDL_EnableUNICODE(1);

    font = sge_BF_OpenFont(PREFIX "gfx/font.png", SGE_BFTRANSP|SGE_BFPALETTE);
    if(!font)
        die("Cannot open font font.png: %s", SDL_GetError());

    /* Load a font and draw with it */
    FILE *file = fopen(PREFIX "gfx/5x7.fnt","r");
    if (!file)
        die("Cannot open tiny font file 5x7.fnt: %s", strerror(errno));
    fread(&tiny_font,sizeof(tiny_font), 1, file);
    fclose(file);
    gfxPrimitivesSetFont(tiny_font, 5, 7);
}

void video_set_title(const char *title) {
    SDL_WM_SetCaption(title, "infon");
}

void video_shutdown() {
    sge_BF_CloseFont(font);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void video_fullscreen_toggle() {
#ifdef WIN32
    flags ^= SDL_FULLSCREEN;
    screen = SDL_SetVideoMode(0, 0, 0, flags);
    if (!screen)
        die("couldn't toggle fullscreen. sorry");
#else
    SDL_WM_ToggleFullScreen(screen);
#endif
}

void video_resize(int w, int h) {
    if (w < 320 || h < 200)
        return;
    screen = SDL_SetVideoMode(w, h, 0, flags);
    if (!screen)
        die("couldn't change resolution. sorry");
}

void video_flip() {
    //SDL_UpdateRect(screen, 0, 0, 0, 0);
    SDL_Flip(screen);
}

int  video_width() {
    return screen->w;
} 

int  video_height() {
    return screen->h;
}

void video_draw(int x, int y, SDL_Surface *sprite) {
    //if (x < 0 || y < 0) return;
    const int w = sprite->w;
    const int h = sprite->h;
    SDL_Rect dstrect = {x, y, x + w, y + h};
    SDL_BlitSurface(sprite, NULL, screen, &dstrect);
}

void video_rect(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Rect dstrect = {x1, y1, x2 - x1, y2 - y1};
    SDL_FillRect(screen, &dstrect, SDL_MapRGBA(screen->format, r, g, b, a));
}

void video_hline(Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    hlineRGBA(screen, x1, x2, y, r, g, b, a);        
}

void video_line_green_red(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2) {
    sge_AAmcLine(screen, x1, y1, x2, y2, 0, 0xFF, 0, 0xFF, 0, 0);
}

void video_line_green(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2) {
    sge_AAmcLine(screen, x1, y1, x2, y2, 0, 0xFF, 0, 0, 0x9F, 0);
}

void video_write(Sint16 x, Sint16 y, const char *text) {
    sge_BF_textout(screen, font, (char*)text, x, y);
}

void video_tiny(Sint16 x, Sint16 y, const char *text) {
    stringRGBA(screen, x,   y,   text,   0,   0,   0, 128);
    stringRGBA(screen, x+1, y+1, text, 255, 255, 255, 128);
}

SDL_Surface *video_new_surface(int w, int h) {
    return SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 
                                screen->format->BitsPerPixel, 
                                screen->format->Rmask, 
                                screen->format->Gmask,
                                screen->format->Bmask, 
                                screen->format->Amask);
}
