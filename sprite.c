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

#include <assert.h>

#include <SDL_image.h>
#include <sge.h>

#include "sprite.h"
#include "misc.h"
#include "video.h"

static sprite_t sprites[SPRITE_NUM] = {{0}};

static void sprite_load_background() {
    const int tile_size = SPRITE_TILE_SIZE;

    SDL_Surface *tileset = IMG_Load("gfx/blocks1.gif");
    
    if (!tileset)
        die("Couldn't load gfx: %s", SDL_GetError());

    const int tilepos[][2] = { // SOLID
                               {  0,  6 }, {  1,  6 }, {  2,  6 }, {  3,  6 }, 
                               {  4,  6 }, {  5,  6 }, {  6,  6 }, {  7,  6 }, 
                               {  8,  6 }, {  9,  6 }, {  8,  7 }, {  9,  7 }, 
                               { 10,  7 }, { 11,  7 }, { 12,  7 }, { 13,  7 },
                               // PLAIN 
                               {  5, 10 }, {  6, 10 }, {  5, 10 }, {  6, 10 }, 
                               {  5, 10 }, {  6, 10 }, {  9, 10 }, { 10, 10 },
                               // SOLID_BORDER
                               {  0,  8 }, {  1,  8 }, {  2,  8 }, {  3,  8 }, 
                               {  4,  8 }, {  5,  8 },
                               // KOTH
                               { 13,  9 }};

    for (int i = 0; i < SPRITE_NUM_TILES; i++) {
        sprites[i].surface = video_new_surface(tile_size, tile_size);
        SDL_Rect srcrect = { tilepos[i][0] * 17 + 1, 
                             tilepos[i][1] * 17 + 1,  tile_size, tile_size };
        SDL_BlitSurface(tileset, &srcrect, sprites[i].surface, NULL);
    }

    SDL_FreeSurface(tileset);
}

static void sprite_load_creature() {
    SDL_Surface *tileset = IMG_Load("gfx/invader1.gif");
    if (!tileset)
        die("Couldn't load gfx: %s", SDL_GetError());

    const int creaturepos[CREATURE_TYPES][CREATURE_ANIMS][2] = { 
                                                                 { {   3, 155 }, {  20, 155 } },
                                                                 { { 105, 144 }, { 122, 144 } } ,
                                                                 { { 207, 132 }, { 224, 133 } },
                                                                 { { 138,  88 }, { 155,  88 } },
                                                               };
    for (int t = 0; t < CREATURE_TYPES; t++) {
        for (int a = 0; a < CREATURE_ANIMS; a++) {
            // XXX: video_new_surface verwenden. Allerdings funktioniert
            // dann bei 16bit das voodoozeugs hier nicht mehr.
            SDL_Surface *frame = SDL_CreateRGBSurface(SDL_HWSURFACE, 12, 12,
                                                      32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
            Uint32 *pixels = (Uint32*)frame->pixels;

            if (!frame) die("Could not allocate frames");

            SDL_Rect srcrect = { creaturepos[t][a][0], creaturepos[t][a][1], 12, 11 };
            for (int c = 0; c < CREATURE_COLORS; c++) { 
                SDL_BlitSurface(tileset, &srcrect, frame, NULL);
                for (int y = 0; y < 12; y++) {
                    for (int x = 0; x < 12; x++) {
                        //printf("0x%08x ", pixels[y * 12 + x]);
                        int col[CREATURE_COLORS][2][3] = { 
                                                           {{ 0xFF, 0x00, 0x00 }, { 0x00, 0xFF, 0x00 }}, 
                                                           {{ 0x00, 0xFF, 0x00 }, { 0x00, 0x00, 0xFF }},
                                                           {{ 0x00, 0x00, 0xFF }, { 0xFF, 0x00, 0x80 }},
                                                           {{ 0xFF, 0xFF, 0x00 }, { 0x80, 0x80, 0xFF }}, 
                                                           {{ 0x00, 0xFF, 0xFF }, { 0xFF, 0x80, 0x80 }}, 
                                                           {{ 0xFF, 0x00, 0xFF }, { 0x80, 0xFF, 0x80 }}, 
                                                           {{ 0xFF, 0xFF, 0xFF }, { 0xFF, 0x00, 0x00 }}, 
                                                           {{ 0x20, 0x20, 0x20 }, { 0xFF, 0xFF, 0xFF }},

                                                           {{ 0xFF, 0x00, 0x00 }, { 0x80, 0x00, 0x00 }}, 
                                                           {{ 0x00, 0xFF, 0x00 }, { 0x00, 0x80, 0x00 }},
                                                           {{ 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x80 }},
                                                           {{ 0xFF, 0xFF, 0x00 }, { 0x80, 0x80, 0x00 }}, 
                                                           {{ 0x00, 0xFF, 0xFF }, { 0x00, 0x80, 0x80 }}, 
                                                           {{ 0xFF, 0x00, 0xFF }, { 0x80, 0x00, 0x80 }}, 
                                                           {{ 0xFF, 0xFF, 0xFF }, { 0x80, 0x80, 0x80 }}, 
                                                           {{ 0x20, 0x20, 0x20 }, { 0x10, 0x10, 0x10 }},

                                                         };

                        int alpha  =   pixels[y * 12 + x] &       0xFF;
                        int bright = ((pixels[y * 12 + x] >> 24 & 0xFF) + 
                                      (pixels[y * 12 + x] >> 16 & 0xFF) + 
                                      (pixels[y * 12 + x] >>  8 & 0xFF)) / 3;

                        pixels[y * 12 + x] = (int)(alpha / 1.5) |
                                             col[c][bright > 64][0] << 24 | 
                                             col[c][bright > 64][1] << 16 | 
                                             col[c][bright > 64][2] <<  8;
                        //printf("0x%08x\n ", pixels[y * 12 + x]);
                    }
                }
                
                for (int d = 0; d < CREATURE_DIRECTIONS; d++) {
                    SDL_Surface **target = &sprites[CREATURE_SPRITE(c, t, d, a)].surface;
                    *target = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                   32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);

                    sge_transform(frame, *target, 360.0 * d / 32, 1.0, 1.0, 6, 6, 8 , 8, 0);
                }
            }
            SDL_FreeSurface(frame);
        }
    }
    SDL_FreeSurface(tileset);
}

static void sprite_load_food() {
    SDL_Surface *tileset = IMG_Load("gfx/food.gif");
    if (!tileset)
        die("Couldn't load gfx: %s", SDL_GetError());

    for (int f = 0; f < SPRITE_NUM_FOOD; f++) {
        // XXX: Geht nicht mit video_new_surface? alpha kaputt
        sprites[SPRITE_FOOD + f].surface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
        SDL_Rect srcrect = { f * 16, 0, 16, 16 };
        SDL_BlitSurface(tileset, &srcrect, sprites[SPRITE_FOOD + f].surface, NULL);
    }
    SDL_FreeSurface(tileset);
}

static void sprite_load_thought() {
    SDL_Surface *tileset = IMG_Load("gfx/states.gif");
    if (!tileset)
        die("Couldn't load gfx: %s", SDL_GetError());

    for (int t = 0; t < SPRITE_NUM_THOUGHT; t++) {
        // Mit video_new_alpha geht wahrscheinlich das bitgefuddel bei 16 bit nicht mehr.
        sprites[SPRITE_THOUGHT + t].surface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
        SDL_Rect srcrect = { t * 16, 0, 16, 16 };
        SDL_BlitSurface(tileset, &srcrect, sprites[SPRITE_THOUGHT + t].surface, NULL);

        Uint32 *pixels = (Uint32*)sprites[SPRITE_THOUGHT + t].surface->pixels;
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                Uint32 pixel = pixels[y * 16 + x];
                pixels[y * 16 + x] = (pixel & 0xFFFFFF00) | (int)((pixel & 0xFF) / 3);
            }
        }

    }
    SDL_FreeSurface(tileset);
}

static void sprite_load_images() {
    sprites[SPRITE_CROWN].surface = IMG_Load("gfx/koth.png");
    if (!sprites[SPRITE_CROWN].surface)
        die("Couldn't load gfx: %s", SDL_GetError());

    sprites[SPRITE_LOGO].surface = IMG_Load("gfx/logo.png");
    if (!sprites[SPRITE_LOGO].surface)
        die("Couldn't load gfx: %s", SDL_GetError());
}

sprite_t *sprite_get(int i) {
    if (i < 0 || i >= SPRITE_NUM)
        return NULL;
    return &sprites[i];
}

int sprite_num(sprite_t *sprite) {
    return sprite - sprites;
}

void sprite_init() {
    sprite_load_background();
    sprite_load_creature();
    sprite_load_food();
    sprite_load_thought();
    sprite_load_images();
}

void sprite_shutdown() {
    for (int i = 0; i < SPRITE_NUM; i++) {
        if (sprites[i].surface)
            SDL_FreeSurface(sprites[i].surface);
    }
}
