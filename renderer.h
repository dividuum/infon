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

#ifndef RENDERER_H
#define RENDERER_H

#include "client_creature.h"
#include "client_player.h"
#include "client_world.h"

#define RENDERER_API_VERSION 2

typedef struct {
    int   version;

    int   (*open )(int w, int h, int fs);
    void  (*close)(void);
    
    void  (*tick)(int game_time, int delta);            

    void  (*world_info_changed)(const client_world_info_t *info);
    void  (*world_changed   )(int x, int y);

    void *(*player_joined   )(const client_player_t *player);
    void  (*player_changed  )(const client_player_t *player,     int changed);
    void  (*player_left     )(const client_player_t *player);

    void *(*creature_spawned)(const client_creature_t *creature);
    void  (*creature_changed)(const client_creature_t *creature, int changed);
    void  (*creature_died   )(const client_creature_t *creature);

    void  (*scroll_message  )(const char *message);
} renderer_api_t;

typedef struct {
    int  max_players;
    int  max_creatures;

    void (*each_creature)(void (*callback)(const client_creature_t *creature, void *opaque), void *opaque);
    void (*each_player  )(void (*callback)(const client_player_t   *player,   void *opaque), void *opaque);

    const client_creature_t*   (*get_creature)(int num);
    const client_player_t*     (*get_player  )(int num);
    const client_player_t*     (*get_king    )();
    const client_world_t       (*get_world   )();
    const client_world_info_t* (*get_world_info)();
    const client_maptile_t   * (*get_world_tile)(int x, int y);
    const char *               (*get_intermission)();

    int                        (*get_traffic)();
    void                       (*shutdown)();
} infon_api_t;

typedef const renderer_api_t *(*render_loader)(const infon_api_t *);

/* infon seitige API */

void renderer_init_from_pointer(render_loader loader);
void renderer_init_from_file(const char *shared);
void renderer_shutdown();

int  renderer_open(int w, int h, int fs);
void renderer_close();

void renderer_tick(int game_time, int delta);

void renderer_world_info_changed(const client_world_info_t *info);
void renderer_world_changed     (int x, int y);

void renderer_player_joined     (client_player_t *player);
void renderer_player_changed    (const client_player_t *player,     int changed);
void renderer_player_left       (const client_player_t *player);
void renderer_creature_spawned  (client_creature_t *creature);
void renderer_creature_changed  (const client_creature_t *creature, int changed);
void renderer_creature_died     (const client_creature_t *creature);

void renderer_scroll_message(const char *buf);

int  renderer_wants_shutdown();

#endif
