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

#include <stdio.h>

#include "renderer.h"

static int null_open(int w, int h, int fs) {
    printf("open\n");
    return 1;
}

static void null_close() {
    printf("close\n");
}

static void null_tick(int gt, int delta) {
    printf("tick %d %d\n", delta, infon->get_traffic());
}

static void  null_world_info_changed(const client_world_info_t *info) {
    printf("new world\n");
}

static void  null_world_changed(int x, int y) {
    printf("change at %d,%d\n", x, y);
}

static void *null_player_joined(const client_player_t *player) {
    printf("new player %d\n", player->num);
    return NULL;
}

static void  null_player_changed(const client_player_t *player, int changed) {
    printf("player %d changed: %d\n", player->num, changed);
}

static void  null_player_left(const client_player_t *player) {
    printf("player %d left\n", player->num);
}

static void *null_creature_spawned(const client_creature_t *creature) {
    printf("creature spawned %d\n", creature->num);
    return NULL;
}

static void  null_creature_changed(const client_creature_t *creature, int changed) {
    printf("creature %d changed: %d\n", creature->num, changed);
}

static void  null_creature_died(const client_creature_t *creature) {
    printf("creature %d died\n", creature->num);
}

static void  null_scroll_message(const char *message) {
    printf("scroll: %s\n", message);
}

const static renderer_api_t null_api = {
    .version             = RENDERER_API_VERSION,
    .open                = null_open,
    .close               = null_close,
    .tick                = null_tick,
    .world_info_changed  = null_world_info_changed,
    .world_changed       = null_world_changed,
    .player_joined       = null_player_joined,
    .player_changed      = null_player_changed,
    .player_left         = null_player_left,
    .creature_spawned    = null_creature_spawned,
    .creature_changed    = null_creature_changed,
    .creature_died       = null_creature_died,
    .scroll_message      = null_scroll_message,
};

RENDERER_EXPORT(null_api)
