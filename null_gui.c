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

#include "client_player.h"
#include "renderer.h"

static infon_api_t *infon;

static int null_open(int w, int h, int fs) {
	printf("open\n");
	return 1;
}

static void null_close() {
	printf("close\n");
}

static void null_tick(int gt, int delta) {
	printf("tick %d\n", delta);
}

static void null_world_change() {
    printf("world change\n");
}

static void null_player_color_change(const client_player_t *player) {
	printf("color\n");
}

static void null_scroll_message(const char *message) {
	printf("scroll: %s\n", message);
}

renderer_api_t null_api = {
    .version             = RENDERER_API_VERSION,
    .open                = null_open,
    .close               = null_close,
    .tick                = null_tick,
    .world_change        = null_world_change,
    .player_color_change = null_player_color_change,
    .scroll_message      = null_scroll_message,
};

renderer_api_t *load(infon_api_t *api) {
	infon = api;
    printf("null renderer loaded\n");
    return &null_api;
}
