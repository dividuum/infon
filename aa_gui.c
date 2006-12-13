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

#define SCALE 4

#include "renderer.h"

#include "aalib.h"

static aa_context        *context;
static unsigned char     *bitmap;
static aa_palette         palette;
static aa_renderparams   *params;

__AA_CONST static int pal[] =
{  0,  0,  0,  0,  0,  6,  0,  0,  6,  0,  0,  7,  0,  0,  8,  0,  0,  8,  0,  0,  9,  0,  0, 10,
   2,  0, 10,  4,  0,  9,  6,  0,  9,  8,  0,  8, 10,  0,  7, 12,  0,  7, 14,  0,  6, 16,  0,  5,
  18,  0,  5, 20,  0,  4, 22,  0,  4, 24,  0,  3, 26,  0,  2, 28,  0,  2, 30,  0,  1, 32,  0,  0,
  32,  0,  0, 33,  0,  0, 34,  0,  0, 35,  0,  0, 36,  0,  0, 36,  0,  0, 37,  0,  0, 38,  0,  0,
  39,  0,  0, 40,  0,  0, 40,  0,  0, 41,  0,  0, 42,  0,  0, 43,  0,  0, 44,  0,  0, 45,  0,  0,
  46,  1,  0, 47,  1,  0, 48,  2,  0, 49,  2,  0, 50,  3,  0, 51,  3,  0, 52,  4,  0, 53,  4,  0,
  54,  5,  0, 55,  5,  0, 56,  6,  0, 57,  6,  0, 58,  7,  0, 59,  7,  0, 60,  8,  0, 61,  8,  0,
  63,  9,  0, 63,  9,  0, 63, 10,  0, 63, 10,  0, 63, 11,  0, 63, 11,  0, 63, 12,  0, 63, 12,  0,
  63, 13,  0, 63, 13,  0, 63, 14,  0, 63, 14,  0, 63, 15,  0, 63, 15,  0, 63, 16,  0, 63, 16,  0,
  63, 17,  0, 63, 17,  0, 63, 18,  0, 63, 18,  0, 63, 19,  0, 63, 19,  0, 63, 20,  0, 63, 20,  0,
  63, 21,  0, 63, 21,  0, 63, 22,  0, 63, 22,  0, 63, 23,  0, 63, 24,  0, 63, 24,  0, 63, 25,  0,
  63, 25,  0, 63, 26,  0, 63, 26,  0, 63, 27,  0, 63, 27,  0, 63, 28,  0, 63, 28,  0, 63, 29,  0,
  63, 29,  0, 63, 30,  0, 63, 30,  0, 63, 31,  0, 63, 31,  0, 63, 32,  0, 63, 32,  0, 63, 33,  0,
  63, 33,  0, 63, 34,  0, 63, 34,  0, 63, 35,  0, 63, 35,  0, 63, 36,  0, 63, 36,  0, 63, 37,  0,
  63, 38,  0, 63, 38,  0, 63, 39,  0, 63, 39,  0, 63, 40,  0, 63, 40,  0, 63, 41,  0, 63, 41,  0,
  63, 42,  0, 63, 42,  0, 63, 43,  0, 63, 43,  0, 63, 44,  0, 63, 44,  0, 63, 45,  0, 63, 45,  0,
  63, 46,  0, 63, 46,  0, 63, 47,  0, 63, 47,  0, 63, 48,  0, 63, 48,  0, 63, 49,  0, 63, 49,  0,
  63, 50,  0, 63, 50,  0, 63, 51,  0, 63, 52,  0, 63, 52,  0, 63, 52,  0, 63, 52,  0, 63, 52,  0,
  63, 53,  0, 63, 53,  0, 63, 53,  0, 63, 53,  0, 63, 54,  0, 63, 54,  0, 63, 54,  0, 63, 54,  0,
  63, 54,  0, 63, 55,  0, 63, 55,  0, 63, 55,  0, 63, 55,  0, 63, 56,  0, 63, 56,  0, 63, 56,  0,
  63, 56,  0, 63, 57,  0, 63, 57,  0, 63, 57,  0, 63, 57,  0, 63, 57,  0, 63, 58,  0, 63, 58,  0,
  63, 58,  0, 63, 58,  0, 63, 59,  0, 63, 59,  0, 63, 59,  0, 63, 59,  0, 63, 60,  0, 63, 60,  0,
  63, 60,  0, 63, 60,  0, 63, 60,  0, 63, 61,  0, 63, 61,  0, 63, 61,  0, 63, 61,  0, 63, 62,  0,
  63, 62,  0, 63, 62,  0, 63, 62,  0, 63, 63,  0, 63, 63,  1, 63, 63,  2, 63, 63,  3, 63, 63,  4,
  63, 63,  5, 63, 63,  6, 63, 63,  7, 63, 63,  8, 63, 63,  9, 63, 63, 10, 63, 63, 10, 63, 63, 11,
  63, 63, 12, 63, 63, 13, 63, 63, 14, 63, 63, 15, 63, 63, 16, 63, 63, 17, 63, 63, 18, 63, 63, 19,
  63, 63, 20, 63, 63, 21, 63, 63, 21, 63, 63, 22, 63, 63, 23, 63, 63, 24, 63, 63, 25, 63, 63, 26,
  63, 63, 27, 63, 63, 28, 63, 63, 29, 63, 63, 30, 63, 63, 31, 63, 63, 31, 63, 63, 32, 63, 63, 33,
  63, 63, 34, 63, 63, 35, 63, 63, 36, 63, 63, 37, 63, 63, 38, 63, 63, 39, 63, 63, 40, 63, 63, 41,
  63, 63, 42, 63, 63, 42, 63, 63, 43, 63, 63, 44, 63, 63, 45, 63, 63, 46, 63, 63, 47, 63, 63, 48,
  63, 63, 49, 63, 63, 50, 63, 63, 51, 63, 63, 52, 63, 63, 52, 63, 63, 53, 63, 63, 54, 63, 63, 55,
  63, 63, 56, 63, 63, 57, 63, 63, 58, 63, 63, 59, 63, 63, 60, 63, 63, 61, 63, 63, 62, 63, 63, 63};

static int aarenderer_open(int w, int h, int fs) {
    int   argc = 1;
    char *argv[] = { "foo", NULL };
    int i;
    if (!aa_parseoptions(NULL, NULL, &argc, argv) || argc != 1) {
        printf("%s\n", aa_help);
        return 0;
    }
    context = aa_autoinit(&aa_defparams);
    if (context == NULL) {
        printf("Can not intialize aalib\n");
        return 0;
    }

    bitmap = aa_image (context);
    params = aa_getrenderparams ();
    for (i = 0; i < 256; i++)
        aa_setpalette(palette, i, pal[i * 3] * 4, pal[i * 3 + 1] * 4, pal[i * 3 + 2] * 4);
    aa_hidecursor(context);
    return 1;
}

static void aarenderer_close() {
    aa_close(context);
}

int ox = 0;
int oy = 0;

static void draw_creature(const client_creature_t *creature, void *opaque) {
    int x = creature->x / (256 / SCALE) - ox;
    int y = creature->y / (256 / SCALE) - oy;
    if (x < 0 || x >= aa_imgwidth(context) ||
        y < 0 || y >= aa_imgheight(context))
        return;
    bitmap[y * aa_imgwidth(context) + x] = 0xFF;
}

static void aarenderer_tick(int gt, int delta) {
    const client_world_info_t *worldsize = infon->get_world_info();
    if (!worldsize) return;
    const client_maptile_t *world = infon->get_world();
    unsigned char *p = bitmap;
    int x;
    int y;
    p--;
    for (y = 0; y < aa_imgheight(context); y++) {
        for (x = 0; x < aa_imgwidth(context); x++) {
            int terrainx = (x - ox) / SCALE;
            int terrainy = (y - oy) / SCALE;
            p++;
            if (terrainx < 0 || terrainx >= worldsize->width || 
                terrainy < 0 || terrainy >= worldsize->height)
                continue;
            const client_maptile_t *tile = &world[terrainy * worldsize->width + terrainx];
            switch (tile->type) {
                case TILE_SOLID:
                    *p = 90;
                    break;
                case TILE_PLAIN:
                    *p = 0;
                    break;
                default:
                    *p = 50;
                    break;
            }
        }
    }

    infon->each_creature(draw_creature, NULL);
    aa_renderpalette (context, palette, params, 0, 0, aa_scrwidth (context), aa_scrheight (context));
    aa_printf(context, 0, 0, AA_SPECIAL, infon->version);
    aa_flush(context);
}

const static renderer_api_t aa_api = {
    .version             = RENDERER_API_VERSION,
    .open                = aarenderer_open,
    .close               = aarenderer_close,
    .tick                = aarenderer_tick,
};

RENDERER_EXPORT(aa_api)
