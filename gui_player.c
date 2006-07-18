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

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "global.h"
#include "sprite.h"
#include "video.h"
#include "rules.h"
#include "gui_player.h"
#include "common_player.h"

#define PLAYER_USED(player) ((player)->used)

static gui_player_t players[MAXPLAYERS];
static gui_player_t *king_player = NULL;

static int player_sort_by_score(const void *a, const void *b) {
    const gui_player_t *pa = *(gui_player_t **)a;
    const gui_player_t *pb = *(gui_player_t **)b;
    return -(pa->score > pb->score) + (pa->score < pb->score);
}

void gui_player_draw() {
    static int lastturn = 0;
    static int page = 0;

    if (SDL_GetTicks() > lastturn + 5000) {
        lastturn = SDL_GetTicks();
        page++;
    }

    int per_page    = video_width() / 128; 
    if (per_page == 0) per_page = 1;

    int n;
    int player_displayed = 0;

    int num_players = 0;
    gui_player_t *sorted[MAXPLAYERS];

    for (n = 0; n < MAXPLAYERS; n++) {
        gui_player_t *player = &players[n];

        if (!PLAYER_USED(player))
            continue;
        sorted[num_players++] = player;
    }
    //printf("%d\n", num_players);

    if (king_player) king_player->score += 1000000; // HACKHACK
    qsort(sorted, num_players, sizeof(gui_player_t*), player_sort_by_score);
    if (king_player) king_player->score -= 1000000;

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
        gui_player_t *player = sorted[n];
        assert(PLAYER_USED(player));

        // King in dieser Runde?
        if (player == king_player) {
            video_draw(player_displayed * 128 + 10,
                       video_height() - 86 - abs(20.0 * sin(M_PI * (SDL_GetTicks() % 550) / 550.0)),
                       sprite_get(SPRITE_CROWN));
        }
        
        // Rotierendes Vieh
        video_draw(player_displayed * 128,
                   video_height() - 32, 
                   sprite_get(CREATURE_SPRITE(player->color,
                                              0, 
                                              (SDL_GetTicks() / 1000) % CREATURE_DIRECTIONS,
                                              (SDL_GetTicks() /  123) % 2)));
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

void gui_player_from_network(packet_t *packet) {
    uint8_t playerno;

    if (!packet_read08(packet, &playerno))      PROTOCOL_ERROR(); 
    if (playerno >= MAXPLAYERS)                 PROTOCOL_ERROR();
    gui_player_t *player = &players[playerno]; 
           
    uint8_t  updatemask;
    if (!packet_read08(packet, &updatemask))    PROTOCOL_ERROR();

    if (updatemask & PLAYER_DIRTY_ALIVE) {
        uint8_t alive;
        if (!packet_read08(packet, &alive))     PROTOCOL_ERROR();
        if (!alive) {
            player->used = 0;
            return;
        } else {
            memset(player, 0, sizeof(gui_player_t));
            player->used = 1;
        }
    }

    if (!PLAYER_USED(player))                   PROTOCOL_ERROR();

    if (updatemask & PLAYER_DIRTY_NAME) {
        uint8_t len; char buf[256];
        if (!packet_read08(packet, &len))       PROTOCOL_ERROR();
        if (!packet_readXX(packet, buf, len))   PROTOCOL_ERROR();
        buf[len] = '\0';
        snprintf(player->name, sizeof(player->name), "%s", buf);
    }
    if (updatemask & PLAYER_DIRTY_COLOR) { 
        uint8_t col;
        if (!packet_read08(packet, &col))       PROTOCOL_ERROR();
        if (col >= CREATURE_COLORS)             PROTOCOL_ERROR();
        player->color = col;
    }
    if (updatemask & PLAYER_DIRTY_CPU) {
        uint8_t cpu;
        if (!packet_read08(packet, &cpu))       PROTOCOL_ERROR();
        if (cpu > 100)                          PROTOCOL_ERROR();
        player->cpu_usage = cpu;
    }
    if (updatemask & PLAYER_DIRTY_SCORE) {
        uint16_t score;
        if (!packet_read16(packet, &score))     PROTOCOL_ERROR();
        player->score = score + PLAYER_KICK_SCORE;
    }
}

void gui_player_king_from_network(packet_t *packet) {
    uint8_t kingno;
    if (!packet_read08(packet, &kingno))        PROTOCOL_ERROR(); 
    if (kingno == 0xFF) {
        king_player = NULL;  
    } else {
        if (kingno >= MAXPLAYERS)               PROTOCOL_ERROR();
        if (!PLAYER_USED(&players[kingno]))     PROTOCOL_ERROR();
        king_player = &players[kingno];
    }
}

void gui_player_init() {
    memset(players, 0, sizeof(players));
}

void gui_player_shutdown() {
}
