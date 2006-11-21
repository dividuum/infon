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
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <event.h>
#include <assert.h>
#include <math.h>

#include "global.h"
#include "map.h"
#include "misc.h"
#include "renderer.h"
#include "sdl_video.h"
#include "common_player.h"
#include "client_world.h"

static const infon_api_t *infon;

static Uint32       render_real_time;
static int          render_game_time;

static int          center_x;
static int          center_y;

static int          offset_x;
static int          offset_y;

static struct evbuffer *scrollbuffer;
static int          rand_table[256];

static void recenter() {
    const client_world_info_t *info = infon->get_world_info();
    if (!info) return;
    center_x = info->width  * SPRITE_TILE_SIZE / 2;
    center_y = info->height * SPRITE_TILE_SIZE / 2;
}

static void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RETURN:
                        if (event.key.keysym.mod & KMOD_ALT)
                            video_fullscreen_toggle();
                        break;
                    case SDLK_1: video_resize( 640,  480); break;
                    case SDLK_2: video_resize( 800,  600); break;
                    case SDLK_3: video_resize(1024,  768); break;
                    case SDLK_4: video_resize(1280, 1024); break;
                    case SDLK_5: video_resize(1600, 1200); break;
                    case SDLK_c:
                        recenter();
                        break;
                    case SDLK_ESCAPE:
                        infon->shutdown();
                        break;
                    default: ;
                }
                break;
           case SDL_MOUSEMOTION: 
                if (event.motion.state & 1) {
                    center_x -= event.motion.xrel;
                    center_y -= event.motion.yrel;
                }
                break;
           case SDL_VIDEORESIZE:
                video_resize(event.resize.w, event.resize.h);
                break;
           case SDL_QUIT:
                infon->shutdown();
                break;
        }
    }
}

static void draw_scroller() {
    static int last_time    = 0;
    static float  x         = 0;
    static float  curspeed  = 0;
    int chars_per_screen = video_width() / 6 + 1;

    float speed = ((float)render_real_time - last_time) * 
                   (float)(EVBUFFER_LENGTH(scrollbuffer) - chars_per_screen - 3) / 200.0;

    if (curspeed < speed) curspeed += 0.1; //(speed - curspeed)/30.0;
    if (curspeed > speed) curspeed = speed;
    if (curspeed < 0.1)   curspeed = 0;

    x        -= curspeed;
    last_time = render_real_time;

    while (x < -6) {
        evbuffer_drain(scrollbuffer, 1);
        x += 6;
    }

    while (EVBUFFER_LENGTH(scrollbuffer) <= chars_per_screen + 1) 
        evbuffer_add(scrollbuffer, " ", 1);

    video_rect(0, video_height() - 16, video_width(), video_height(), 0, 0, 0, 0);

    assert(chars_per_screen < EVBUFFER_LENGTH(scrollbuffer));
    char *end = &EVBUFFER_DATA(scrollbuffer)[chars_per_screen];
    char saved = *end; *end = '\0';
    video_write(x, video_height() - 15, EVBUFFER_DATA(scrollbuffer));
    *end = saved;
}

static void draw_creature(const client_creature_t *creature, void *opaque) {
    const int x = X_TO_SCREENX(creature->x) - 7 + offset_x;
    const int y = Y_TO_SCREENY(creature->y) - 7 + offset_y;

    const int max_x = video_width()  + 10;
    const int max_y = video_height() - 32 + 10;
    

    if (x < -30 || x > max_x || y < -20 || y > max_y)
        return;

    const int hw = creature->health;
    const int fw = creature->food;

    if (fw != 15) video_rect(x + fw, y - 4, x + 15, y - 2, 0x00, 0x00, 0x00, 0xB0);
    if (fw !=  0) video_rect(x,      y - 4, x + fw, y - 2, 0xFF, 0xFF, 0xFF, 0xB0);
                                           
    if (hw != 15) video_rect(x + hw, y - 2, x + 15, y,     0xFF, 0x00, 0x00, 0xB0);
    if (hw !=  0) video_rect(x,      y - 2, x + hw, y,     0x00, 0xFF, 0x00, 0xB0);

    video_draw(x, y, sprite_get(CREATURE_SPRITE(creature->player,
                                                creature->type,
                                                creature->dir,
                                                (render_real_time >> 7) % 2)));
    
    if (creature->smile_time + 1000 > render_game_time) {
        video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + 8));
    } else {
        video_draw(x + 15, y - 10, sprite_get(SPRITE_THOUGHT + creature->state));
    }

    video_tiny(x - strlen(creature->message) * 6 / 2 + 9, y + 14, creature->message);
        
    if (creature->state == CREATURE_ATTACK) {
        const client_creature_t *target = infon->get_creature(creature->target);
        if (target) { 
            video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3 + offset_x, Y_TO_SCREENY(target->y) - 3 + offset_y);
            video_line(x + 6, y + 6, X_TO_SCREENX(target->x) - 3 + offset_x, Y_TO_SCREENY(target->y) + 3 + offset_y);
            video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3 + offset_x, Y_TO_SCREENY(target->y) - 3 + offset_y);
            video_line(x + 6, y + 6, X_TO_SCREENX(target->x) + 3 + offset_x, Y_TO_SCREENY(target->y) + 3 + offset_y);
        }
    }
}

static int player_sort_by_score(const void *a, const void *b) {
    const client_player_t *pa = *(client_player_t **)a;
    const client_player_t *pb = *(client_player_t **)b;
    return -(pa->score > pb->score) + (pa->score < pb->score);
}

static void draw_player_row() {
    static int lastturn = 0;
    static int page     = 0;

    if (render_real_time > lastturn + 5000) {
        lastturn = render_real_time;
        page++;
    }

    int per_page    = video_width() / 128; 
    if (per_page == 0) per_page = 1;

    int n;
    int player_displayed = 0;

    int num_players = 0;
    const client_player_t *sorted[infon->max_players];

    for (n = 0; n < infon->max_players; n++) {
        const client_player_t *player = infon->get_player(n);
        if (!player) 
            continue;
        sorted[num_players++] = player;
    }
    //printf("%d\n", num_players);

    //if (infon->get_king()) king_player->score += 1000000; // HACKHACK
    qsort(sorted, num_players, sizeof(client_player_t*), player_sort_by_score);
    //if (king_player) king_player->score -= 1000000;

    int offset = per_page * page;
    if (offset >= num_players) {
        page    = 0;
        offset  = 0;
    }

    int num = num_players - offset;
    if (num > per_page)
        num = per_page;

    video_rect(0, video_height() - 32, video_width(), video_height() - 16, 0, 0, 0, 0);

    for (n = offset; n < offset + num; n++) {
        const client_player_t *player = sorted[n];

        // King in dieser Runde?
        if (player == infon->get_king()) {
            video_draw(player_displayed * 128 + 32,
                       video_height() - 86 - abs(20.0 * sin(M_PI * (render_real_time % 550) / 550.0)),
                       sprite_get(SPRITE_CROWN));
        }
        
        // Rotierendes Vieh
        video_draw(player_displayed * 128,
                   video_height() - 32, 
                   sprite_get(CREATURE_SPRITE(player->num,
                                              0, 
                                              (render_real_time /  64) % CREATURE_DIRECTIONS,
                                              (render_real_time / 128) % 2)));
        // CPU Auslastung Anzeigen
        const int cpu = 80 * player->cpu_usage / 100;
        video_rect(player_displayed * 128 + 16, 
                   video_height() - 32, 
                   player_displayed * 128 + 16 + cpu,
                   video_height() - 16,
                   2 * cpu, 160 - 2 * cpu , 0x00, 0x00);

        // Name / Punkte
        static char buf[18];
        snprintf(buf, sizeof(buf), "%2d. %4d %s", n + 1, player->score, player->name);
        video_write(player_displayed * 128 + 16,
                    video_height() - 30, 
                    buf);
                
        player_displayed++;
    }
}

static void draw_scores(int xcenter, int y) {
    int n;
    int num_players = 0;

    const client_player_t *sorted[MAXPLAYERS];
    for (n = 0; n < MAXPLAYERS; n++) {
        const client_player_t *player = infon->get_player(n);
        if (!player);
            continue;
        sorted[num_players++] = player;
    }
    qsort(sorted, num_players, sizeof(client_player_t*), player_sort_by_score);

    for (n = 0; n < num_players; n++) {
        const client_player_t *player = sorted[n];
        video_draw(xcenter - 48,
                   y + 14 * n,
                   sprite_get(CREATURE_SPRITE(player->num,
                                              0, 
                                              (render_real_time / 60) % CREATURE_DIRECTIONS,
                                              (render_real_time / 123) % 2)));
        
        static char buf[40];
        snprintf(buf, sizeof(buf), "%2d.     %4d %s", n + 1, player->score, player->name);
        video_write(xcenter - 70,
                    y + 14 * n + 1,
                    buf);
    }
}


static void draw_world() {
    const client_world_info_t *info = infon->get_world_info();
    if (!info) return;

    const client_maptile_t *world = infon->get_world();

    int screen_w  = video_width(); 
    int screen_cx = screen_w / 2;

    int screen_h  = video_height() - 32;
    int screen_cy = screen_h / 2;

    int mapx1 = offset_x = screen_cx  - center_x;
    int x1    = 0;
    if (mapx1 <= -SPRITE_TILE_SIZE) {
        x1     =  -mapx1 / SPRITE_TILE_SIZE;
        mapx1 = -(-mapx1 % SPRITE_TILE_SIZE);
    }
    int x2 = x1 + (screen_w - mapx1) / SPRITE_TILE_SIZE + 1;
    if (x2 > info->width) x2 = info->width;
    int mapx2 = mapx1 + (x2 - x1) * SPRITE_TILE_SIZE;
    
    int mapy1 = offset_y = screen_cy - center_y;
    int y1    = 0;
    if (mapy1 <= -SPRITE_TILE_SIZE) {
        y1    =   -mapy1 / SPRITE_TILE_SIZE;
        mapy1 = -(-mapy1 % SPRITE_TILE_SIZE);
    }
    int y2 = y1 + (screen_h - mapy1) / SPRITE_TILE_SIZE + 1;
    if (y2 > info->height) y2 = info->height;
    int mapy2 = mapy1 + (y2 - y1) * SPRITE_TILE_SIZE;

    int screenx = mapx1;
    int screeny = mapy1;

    for (int y = y1; y < y2; y++) {
        const client_maptile_t *tile = &world[y * info->width + x1];
        for (int x = x1; x < x2; x++) {
            int pos_rand = rand_table[(x ^ y) & 0xFF];
            int floor_sprite;
            switch (tile->map) {
                case TILE_SOLID:
                    if (x == 0 || x == info->width - 1 || y == 0 || y == info->height - 1) {
                        floor_sprite = SPRITE_BORDER + pos_rand % SPRITE_NUM_BORDER;
                    } else {
                        floor_sprite = SPRITE_SOLID  + pos_rand % SPRITE_NUM_SOLID;
                    };
                    break;
                case TILE_PLAIN:
                    if (x == info->koth_x && y == info->koth_y) {
                        floor_sprite = SPRITE_KOTH;
                    } else {
                        floor_sprite = SPRITE_PLAIN + pos_rand % SPRITE_NUM_PLAIN;
                    }
                    break;
                case TILE_WATER:
                    floor_sprite = SPRITE_WATER + ((render_real_time >> 8) + x + y) % SPRITE_NUM_WATER;
                    break;
                default:
                    assert(0);
            }

            video_draw(screenx, screeny, sprite_get(floor_sprite));

            if (tile->food >= 0) 
                video_draw(screenx, screeny, sprite_get(SPRITE_FOOD + tile->food));
            
            screenx += SPRITE_TILE_SIZE;
            tile++;
        }
        screeny += SPRITE_TILE_SIZE;
        screenx  = mapx1;
    }

    if (mapx1 > 0) 
        video_rect(0, max(0, mapy1), mapx1, min(screen_h, mapy2), 30, 30, 30, 0);
    if (mapy1 > 0) 
        video_rect(0, 0, screen_w, mapy1, 30, 30, 30, 0);
    if (mapx2 <= screen_w) 
        video_rect(mapx2, max(0, mapy1), screen_w, min(screen_h, mapy2), 30, 30, 30, 0);
    if (mapy2 <= screen_h) 
        video_rect(0, mapy2, screen_w, screen_h, 30, 30, 30, 0);
}

static void sdl_world_info_changed(const client_world_info_t *info) {
    recenter();
}

static void sdl_player_changed(const client_player_t *player, int changed) {
    if (changed & PLAYER_DIRTY_COLOR) {
        int colors[16][3] = {
            { 0xFF, 0x00, 0x00 },
            { 0x00, 0xFF, 0x00 },
            { 0x00, 0x00, 0xFF },
            { 0xFF, 0xFF, 0x00 },
            { 0x00, 0xFF, 0xFF },
            { 0xFF, 0x00, 0xFF },
            { 0xFF, 0xFF, 0xFF },
            { 0x00, 0x00, 0x00 },

            { 0xFF, 0x80, 0x80 },
            { 0x80, 0xFF, 0x80 },
            { 0x80, 0x80, 0xFF },
            { 0xFF, 0xFF, 0x80 },
            { 0x80, 0xFF, 0xFF },
            { 0xFF, 0x80, 0xFF },
            { 0x80, 0x80, 0x80 },
            { 0x60, 0xA0, 0xFF },
        };

        int hi = (player->color & 0xF0) >> 4;
        int lo = (player->color & 0x0F);

        sprite_render_player_creatures(player->num,
                                       colors[hi][0],
                                       colors[hi][1],
                                       colors[hi][2],
                                       colors[lo][0],
                                       colors[lo][1],
                                       colors[lo][2]);
    }
}

static void sdl_scroll_message(const char *msg) {
    // Zuviel insgesamt?
    if (EVBUFFER_LENGTH(scrollbuffer) > 10000)
        return;

    evbuffer_add(scrollbuffer, (char*)msg, strlen(msg));
    evbuffer_add(scrollbuffer, "  -  ", 5);
}


static void sdl_tick(int gt, int delta) {
    static Uint32 frames    = 0;
    static int    last_net  = 0;
    static Uint32 last_info = 0;
    static int    stall     = 0;

    render_real_time = SDL_GetTicks();
    render_game_time = gt;

    handle_events();

    const char *intermission = infon->get_intermission();
    
    if (strlen(intermission) > 0) {
        int y = max(video_height() / 2 - 150, 20);
        video_rect(0, 0, video_width(), video_height() - 16, 0, 0, 0, 0);
        video_hline(0, video_width(), y + 20, 0x80, 0x80, 0x80, 0x80);
        video_write((video_width() - strlen(intermission) * 7) / 2, 
                    y, 
                    intermission);
        draw_scores(video_width() / 2, y + 30);
    } else {
        draw_world();
        infon->each_creature(draw_creature, NULL);
        draw_player_row();
    }

    draw_scroller();

    // Bitte drinlassen. Danke
    if (render_real_time / 1000 % 60 < 5) 
        video_draw(video_width() - 190, 20, sprite_get(SPRITE_LOGO));

    // Informationen
    if (render_real_time >= last_info + 1000) {
        char buf[128];
        int traffic = infon->get_traffic();
        snprintf(buf, sizeof(buf), GAME_NAME" - %d fps - %d byte/s", frames, traffic - last_net);
        video_set_title(buf);
        stall     = traffic > last_net ? 0 : stall + 1;
        last_net  = traffic;
        last_info = render_real_time;
        frames    = 0;
    }

    if (stall > 2) 
        video_write(4, 4, "connection stalled?");

    video_flip();
    frames++;
}

static int sdl_open(int w, int h, int fs) {
    video_init(w, h, fs);
    
    center_x = 0;
    center_y = 0;

    offset_x = 0;
    offset_y = 0;

    scrollbuffer = evbuffer_new();
    for (int i = 0; i < 20; i++)
        evbuffer_add(scrollbuffer, "          ", 10);

    sprite_init();

    for (int i = 0; i < 256; i++)
        rand_table[i] = rand();

    //evbuffer_add(GAME_NAME);
    return 1;
}

static void sdl_close() {
    sprite_shutdown();
    evbuffer_free(scrollbuffer);
    video_shutdown();
}

const static renderer_api_t sdl_api = {
    .version             = RENDERER_API_VERSION,
    .open                = sdl_open,
    .close               = sdl_close,
    .tick                = sdl_tick,
    .world_info_changed  = sdl_world_info_changed,
    .world_changed       = NULL,
    .player_joined       = NULL,
    .player_changed      = sdl_player_changed,
    .player_left         = NULL,
    .creature_spawned    = NULL,
    .creature_changed    = NULL,
    .creature_died       = NULL,
    .scroll_message      = sdl_scroll_message,
};

#ifdef WIN32
__declspec(dllexport) 
#endif 
const renderer_api_t *load(const infon_api_t *api) {
    infon = api;
    printf("Renderer loaded\n");
    return &sdl_api;
}
