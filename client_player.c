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
#include <stdio.h>

#include "global.h"
#include "rules.h"
#include "renderer.h"
#include "client_player.h"
#include "common_player.h"

#define PLAYER_USED(player) ((player)->used)

static client_player_t players[MAXPLAYERS];
static client_player_t *king_player = NULL;

const client_player_t *client_player_get(int num) {
    if (num < 0 || num >= MAXPLAYERS)
        return NULL;
    const client_player_t *player = &players[num];
    if (!PLAYER_USED(player))
        return NULL;
    return player;
}

const client_player_t *client_player_get_king() {
    return king_player;
}

void client_player_each(void (*callback)(const client_player_t *player, void *opaque), void *opaque) {
    for (int n = 0; n < MAXPLAYERS; n++) {
        const client_player_t *player = &players[n];
        if (!PLAYER_USED(player))
            continue;
        callback(player, opaque);
    }
}


void client_player_from_network(packet_t *packet) {
    uint8_t playerno;

    if (!packet_read08(packet, &playerno))      PROTOCOL_ERROR(); 
    if (playerno >= MAXPLAYERS)                 PROTOCOL_ERROR();
    client_player_t *player = &players[playerno]; 
           
    uint8_t  updatemask;
    if (!packet_read08(packet, &updatemask))    PROTOCOL_ERROR();

    if (updatemask & PLAYER_DIRTY_ALIVE) {
        uint8_t alive;
        if (!packet_read08(packet, &alive))     PROTOCOL_ERROR();
        if (!alive) {
            renderer_player_left(player);
            player->used = 0;
            return;
        } else {
            memset(player, 0, sizeof(client_player_t));
            player->used = 1;
            player->num  = playerno;
            renderer_player_joined(player);
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

    if (updatemask & ~PLAYER_DIRTY_ALIVE) 
        renderer_player_changed(player, updatemask & ~PLAYER_DIRTY_ALIVE);
}

void client_player_king_from_network(packet_t *packet) {
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

void client_player_init() {
    memset(players, 0, sizeof(players));
}

void client_player_shutdown() {
    for (int n = 0; n < MAXPLAYERS; n++) {
        const client_player_t *player = &players[n];
        if (!PLAYER_USED(player))
            continue;
        renderer_player_left(player);        
    }
}
