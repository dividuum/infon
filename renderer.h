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

#ifdef __cplusplus
extern "C" {
#endif

#include "client_creature.h"
#include "client_player.h"
#include "client_world.h"

#define RENDERER_API_VERSION 4

typedef struct {
    /* the renderer version number. set this to RENDERER_API_VERSION. */
    int   version;

    /* called once before other rendering function. should initialize a new
     * output with size w x h and fullscreen if fs is set. should return 1
     * if successfull, 0 on failure. */
    int   (*open )(int w, int h, int fs);

    /* called once before the client exits. do cleanups here */
    void  (*close)(void);
    
    /* called for every frame. game_time and delta contain game time and delta 
     * to previous frame in milliseconds */
    void  (*tick)(int game_time, int delta);            

    /* called if the world changes its size (at startup or on level change). */ 
    void  (*world_info_changed)(const client_world_info_t *info);

    /* called for every world_tile that changed (from being solid) */
    void  (*world_changed   )(int x, int y);

    /* called after a player joins the game. you can return any pointer, which will
     * then be saved in player->userdata */
    void *(*player_joined   )(const client_player_t *player);

    /* called if some player value changed */
    void  (*player_changed  )(const client_player_t *player,     int changed);

    /* called before a player leaves the game */
    void  (*player_left     )(const client_player_t *player);

    /* called after a new creature spawned. you can return any pointer, which will
     * then be saved in creature->userdata */
    void *(*creature_spawned)(const client_creature_t *creature);

    /* called if some creature value changed */
    void  (*creature_changed)(const client_creature_t *creature, int changed);

    /* called before a creature died */
    void  (*creature_died   )(const client_creature_t *creature);

    /* called for each (scroll?) message. */
    void  (*scroll_message  )(const char *message);
} renderer_api_t;

typedef struct {
    /* infon version string */
    const char *version;
    
    /* maximum number of players. */
    int max_players;

    /* maximum number of creatures. */
    int max_creatures;

    /* calls function 'callback' for each creature in the game. opaque is passed to the callback function. */
    void (*each_creature)(void (*callback)(const client_creature_t *creature, void *opaque), void *opaque);

    /* calls function 'callback' for each player in the game. opaque is passed to the callback function. */
    void (*each_player  )(void (*callback)(const client_player_t   *player,   void *opaque), void *opaque);

    /* returns information on creature num or NULL if creature does not exist. */
    const client_creature_t*   (*get_creature)(int num);

    /* returns information on player num or NULL or the player does not exist. */
    const client_player_t*     (*get_player  )(int num);

    /* returns player information on the current king player or NULL if there is no king. */
    const client_player_t*     (*get_king    )();

    /* returns a pointer to world_with * world_height world tile information or NULL
     * if there is no world. you can reuse the returned pointer during one tick */
    const client_maptile_t   * (*get_world   )();

    /* returns information about the current world or NULL if there is no world. */
    const client_world_info_t* (*get_world_info)();

    /* returns information on world tile at x, y. must not be called if there 
     * is no world. x and y must be within world coordinates. */
    const client_maptile_t   * (*get_world_tile)(int x, int y);

    /* returns the current intermission message or NULL if there
     * is no message. */
    const char *               (*get_intermission)();

    /* return the amount of network traffic the client received. */
    int                        (*get_traffic)();

    /* tell the infon client to shutdown. */
    void                       (*shutdown)();

    /* write a string to the server */
    void                       (*printf)(const char *fmt, ...);

    /* Is it a demo replay? */
    int                        (*is_demo)();
} infon_api_t;

/* the function called after loading the renderer. it receives the infon api and must
 * return the renderer api. */
typedef const renderer_api_t *(*render_loader)(const infon_api_t *);

/* api provided by infon */
extern const  infon_api_t    *infon;

#ifdef WIN32
#define RENDERER_EXPORT_SPEC __declspec(dllexport)
#else
#define RENDERER_EXPORT_SPEC
#endif

#ifdef __cplusplus
#define RENDERER_EXTERN_SPEC extern "C"
#else 
#define RENDERER_EXTERN_SPEC extern
#endif

#define RENDERER_SYM renderer_load

    RENDERER_EXPORT_SPEC RENDERER_EXTERN_SPEC 
    const renderer_api_t *RENDERER_SYM (const infon_api_t *api);

#define RENDERER_EXPORT(renderer)                                   \
    const infon_api_t *infon;                                       \
    RENDERER_EXPORT_SPEC RENDERER_EXTERN_SPEC                       \
    const renderer_api_t *RENDERER_SYM (const infon_api_t *api) {   \
        infon = api; return &(renderer);                            \
    }                                                       

#ifdef __cplusplus
}
#endif

/* infon seitige API */

int  renderer_init(const char *name);
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
