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

#include "misc.h"
#include "sdl_sprite.h"
#include "sdl_video.h"

static SDL_Surface *sprites[SPRITE_NUM] = {0};
static SDL_Surface *gfx = NULL ;

static SDL_Surface *sprite_load_surface(const char *filename) {
    SDL_Surface *ret = IMG_Load(filename);
    if (!ret) 
        die("Cannot load file %s: %s", filename, SDL_GetError());
    return ret;
}

static void sprite_optimize(SDL_Surface **tmp, int transparent) {
    SDL_Surface *optimized = transparent ? SDL_DisplayFormatAlpha(*tmp) 
                                         : SDL_DisplayFormat(*tmp);
    SDL_FreeSurface(*tmp);
    *tmp = optimized;
}

static void sprite_load_background() {
    const int tilepos[][2] = { 
        // Border
        {  0,  0 }, {  1,  0 }, {  2,  0 }, {  3,  0 }, 
        {  4,  0 }, {  5,  0 }, {  6,  0 }, {  7,  0 }, 
        {  8,  0 }, {  9,  0 }, { 10,  0 }, { 11,  0 }, 
        { 12,  0 }, { 13,  0 }, { 14,  0 }, { 15,  0 }, 
        // Solid
        {  0,  1 }, {  1,  1 }, {  2,  1 }, {  3,  1 }, 
        {  4,  1 }, {  5,  1 }, {  6,  1 }, {  7,  1 }, 
        {  8,  1 }, {  9,  1 }, { 10,  1 }, { 11,  1 }, 
        { 12,  1 }, { 13,  1 }, { 14,  1 }, { 15,  1 }, 
        // Plain 
        {  0,  2 }, {  1,  2 }, {  2,  2 }, {  3,  2 }, 
        {  4,  2 }, {  5,  2 }, {  6,  2 }, {  7,  2 }, 
        {  8,  2 }, {  9,  2 }, { 10,  2 }, { 11,  2 }, 
        { 12,  2 }, { 13,  2 }, { 14,  2 }, { 15,  2 }, 
        // Border Snow
        { 16,  0 }, { 17,  0 }, { 18,  0 }, { 19,  0 }, 
        { 20,  0 }, { 21,  0 }, { 22,  0 }, { 23,  0 }, 
        { 24,  0 }, { 25,  0 }, { 26,  0 }, { 27,  0 }, 
        { 28,  0 }, { 29,  0 }, { 30,  0 }, { 31,  0 }, 
        // Solid Snow
        { 16,  1 }, { 17,  1 }, { 18,  1 }, { 19,  1 }, 
        { 20,  1 }, { 21,  1 }, { 22,  1 }, { 23,  1 }, 
        { 24,  1 }, { 25,  1 }, { 26,  1 }, { 27,  1 }, 
        { 28,  1 }, { 29,  1 }, { 30,  1 }, { 31,  1 }, 
        // Plain Snow
        { 16,  2 }, { 17,  2 }, { 18,  2 }, { 19,  2 }, 
        { 20,  2 }, { 21,  2 }, { 22,  2 }, { 23,  2 }, 
        { 24,  2 }, { 25,  2 }, { 26,  2 }, { 27,  2 }, 
        { 28,  2 }, { 29,  2 }, { 30,  2 }, { 31,  2 }, 
        // KOTH
        {  0,  3 },
        // Water
        {  0,  6 }, {  1,  6 }, {  2,  6 }, {  3,  6 }, 
        // Lava
        {  0,  7 }, {  1,  7 }, {  2,  7 }, {  3,  7 }, 
        // Desert
        {  6,  3 }, {  7,  3 }, {  8,  3 }, {  9,  3 }, 
        { 10,  3 }, { 11,  3 }, { 12,  3 }, { 13,  3 }, 
        { 14,  3 }, { 15,  3 }, 
    };

    for (int i = 0; i < SPRITE_NUM_TILES; i++) {
        sprites[i] = video_new_surface(SPRITE_TILE_SIZE, SPRITE_TILE_SIZE);
        SDL_Rect srcrect = {       tilepos[i][0] * 16, 
                             192 + tilepos[i][1] * 16, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE };
        SDL_BlitSurface(gfx, &srcrect, sprites[i], NULL);
        sprite_optimize(&sprites[i], 0);
    }
}

static void sprite_load_food() {
    for (int f = 0; f < SPRITE_NUM_FOOD; f++) {
        sprites[SPRITE_FOOD + f] = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
        SDL_Rect srcrect = { f * 16, 256, 16, 16 };
        SDL_BlitSurface(gfx, &srcrect, sprites[SPRITE_FOOD + f], NULL);
        sprite_optimize(&sprites[SPRITE_FOOD + f], 1);
    }
    for (int f = 0; f < SPRITE_NUM_SNOW_FOOD; f++) {
        sprites[SPRITE_SNOW_FOOD + f] = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                             32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
        SDL_Rect srcrect = { f * 16, 256 + 16, 16, 16 };
        SDL_BlitSurface(gfx, &srcrect, sprites[SPRITE_SNOW_FOOD + f], NULL);
        sprite_optimize(&sprites[SPRITE_SNOW_FOOD + f], 1);
    }
}

static void sprite_load_thought() {
    for (int t = 0; t < SPRITE_NUM_THOUGHT; t++) {
        // Mit video_new_alpha geht wahrscheinlich das bitgefuddel bei 16 bit nicht mehr.
        sprites[SPRITE_THOUGHT + t] = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
        SDL_Rect srcrect = { 0, 48 + t * 16, 16, 16 };
        SDL_BlitSurface(gfx, &srcrect, sprites[SPRITE_THOUGHT + t], NULL);

        Uint32 *pixels = (Uint32*)sprites[SPRITE_THOUGHT + t]->pixels;
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                Uint32 pixel = pixels[y * 16 + x];
                pixels[y * 16 + x] = (pixel & 0xFFFFFF00) | (int)((pixel & 0xFF) / 3);
            }
        }
        sprite_optimize(&sprites[SPRITE_THOUGHT + t], 1);
    }
}

static void sprite_load_images() {
    sprites[SPRITE_CROWN]= SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 64, 50,
                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
    SDL_Rect crect = { 0, 350, 64, 50 };
    SDL_BlitSurface(gfx, &crect, sprites[SPRITE_CROWN], NULL);
    sprite_optimize(&sprites[SPRITE_CROWN], 1);

    sprites[SPRITE_LOGO] = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 170, 80,
                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
    SDL_Rect lrect = { 0, 410, 170, 80};
    SDL_BlitSurface(gfx, &lrect, sprites[SPRITE_LOGO], NULL);
    sprite_optimize(&sprites[SPRITE_LOGO], 1);

    sprites[SPRITE_HALO] = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 32, 32,
                                                32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
    SDL_Rect hrect = {16, 48, 32, 32};
    SDL_BlitSurface(gfx, &hrect, sprites[SPRITE_HALO], NULL);
    sprite_optimize(&sprites[SPRITE_HALO], 1);
}

SDL_Surface *sprite_get(int i) {
    return sprites[i];
}

int sprite_exists(int i) {
    if (i < 0 || i >= SPRITE_NUM) return 0;
    if (!sprites[i])      return 0;
    return 1;
}

void sprite_render_player_creatures(int playerno, int r1, int g1, int b1, int r2, int g2, int b2) {
    for (int t = 0; t < CREATURE_TYPES; t++) {
        for (int a = 0; a < CREATURE_ANIMS; a++) {
            SDL_Surface *base = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                     32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
            SDL_Surface *over = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                     32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
            SDL_Surface *done = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                                     32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
            SDL_Rect baserect = { a * 16,      t * 16, 16, 16 };
            SDL_BlitSurface(gfx, &baserect, base, NULL);
            SDL_Rect overrect = { a * 16 + 32, t * 16, 16, 16 };
            SDL_BlitSurface(gfx, &overrect, over, NULL);
            
            Uint32 *basepixel = (Uint32*)base->pixels;
            Uint32 *donepixel = (Uint32*)done->pixels;
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    int sat1 = (*basepixel >> 24) & 0xFF;
                    int sat2 = (*basepixel >>  8) & 0xFF;
                    *donepixel = ((min(0xFF, (sat1 * r1 + sat2 * r2) >> 8)) << 24) | 
                                 ((min(0xFF, (sat1 * g1 + sat2 * g2) >> 8)) << 16) | 
                                 ((min(0xFF, (sat1 * b1 + sat2 * b2) >> 8)) <<  8) | 
                                 //((*basepixel & 0xFF) > 0 ? 0xFF : 0);
                                 (min(0xFF, (*basepixel & 0xFF) * 3));
                    basepixel++; donepixel++;
                }
            }
            SDL_BlitSurface(over, NULL, done, NULL);

            for (int d = 0; d < CREATURE_DIRECTIONS; d++) {
                SDL_Surface **target = &sprites[CREATURE_SPRITE(playerno, t, d, a)];
                if (*target) SDL_FreeSurface(*target);
                *target = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 16, 16,
                                               32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
                sge_transform(done, *target, 360.0 * d / 32, 0.75, 0.75, 7, 7, 7 , 7, SGE_TAA|SGE_TSAFE);
                sprite_optimize(target, 1);
            }
            SDL_FreeSurface(base);
            SDL_FreeSurface(over);
            SDL_FreeSurface(done);
        }
    }
}

void sprite_init() {
    gfx = sprite_load_surface(PREFIX "gfx/theme.png");
    SDL_SetAlpha(gfx, 0, 0);

    sprite_load_background();
    sprite_load_food();
    sprite_load_thought();
    sprite_load_images();
}

void sprite_shutdown() {
    for (int i = 0; i < SPRITE_NUM; i++) {
        if (sprites[i])
            SDL_FreeSurface(sprites[i]);
    }
    SDL_FreeSurface(gfx);
}
