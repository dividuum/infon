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

#include "misc.h"
#include "video.h"
#include "gui_game.h"
#include "gui_creature.h"
#include "gui_world.h"
#include "gui_scroller.h"
#include "gui_player.h"

static char intermission [256] = {0};

void gui_game_intermission_from_network(packet_t *packet) {
    snprintf(intermission, sizeof(intermission),
             "%.*s", packet->len, packet->data);
}

void gui_game_draw() {
    Uint32 now = SDL_GetTicks();

    if (strlen(intermission) > 0) {
        int y = max(video_height() / 2 - 150, 20);
        video_rect(0, 0, video_width(), video_height() - 16, 0, 0, 0, 0);
        video_hline(0, video_width(), y + 20, 0x80, 0x80, 0x80, 0x80);
        video_write((video_width() - strlen(intermission) * 7) / 2, 
                y, 
                intermission);
        gui_player_draw_scores(video_width() / 2, y + 30);
    } else {
        gui_world_draw();
        gui_creature_draw();
        gui_player_draw();
    }

    gui_scroller_draw();

    // Bitte drinlassen. Danke
    if (now / 1000 % 60 < 5) 
        video_draw(video_width() - 190, 20, sprite_get(SPRITE_LOGO));

    video_flip();
}

void gui_game_init() {
    gui_scroller_init();
    gui_world_init();
    gui_player_init();
    gui_creature_init();
}

void gui_game_shutdown() {
    gui_creature_shutdown();
    gui_player_shutdown();
    gui_world_shutdown();
    gui_scroller_shutdown();
}
