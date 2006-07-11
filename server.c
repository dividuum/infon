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

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <lauxlib.h>
#include <event.h>

#include "server.h"
#include "packet.h"
#include "global.h"
#include "player.h"
#include "world.h"
#include "listener.h"
#include "misc.h"
#include "scroller.h"
#include "creature.h"

#define CLIENT_USED(client) ((client)->in_buf)

static client_t *guiclients = NULL;

static void server_readable(int fd, short event, void *arg);
static void server_writable(int fd, short event, void *arg);

int client_num(client_t *client) {
    return client - clients;
}

int server_accept(int fd, struct sockaddr_in *peer) {
    static struct linger l = { 1, 0 };
    static const int one = 1;
    static char address[128];

    if (fd >= MAXCLIENTS) {
        fprintf(stderr, "cannot accept() new incoming connection: file descriptor too large\n");
        return 0;
    }

    if (peer)
        sprintf(address, "%s:%d", inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
    else
        sprintf(address, "locale console");

    lua_pushliteral(L, "on_new_client");  /* funcname */
    lua_rawget(L, LUA_GLOBALSINDEX);    /* func     */
    lua_pushstring(L, address);         /* addr     */

    if (lua_pcall(L, 1, 1, 0) != 0) {
        fprintf(stderr, "error calling on_new_client: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }

    if (!lua_toboolean(L, -1)) {
        fprintf(stderr, "rejected client %s\n", address);
        lua_pop(L, 1);
        return 0;
    }

    lua_pop(L, 1);

    /* Non Blocking setzen */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        fprintf(stderr, "cannot set accept()ed socket nonblocking: %s\n", strerror(errno));
        return 0;
    }

    if (peer) { 
        /* TCP_NODELAY setzen. Dadurch werden Daten frühestmöglich versendet */
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0) {
            fprintf(stderr, "cannot enable TCP_NODELAY: %s\n", strerror(errno));
            return 0;
        }

        /* SO_LINGER setzen. Falls sich noch Daten in der Sendqueue der Verbindung
           befinden, werden diese verworfen. */
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1) {
            fprintf(stderr, "cannot set SO_LINGER: %s\n", strerror(errno));
            return 0;
        }
    }

    // Aktivieren
    event_set(&clients[fd].rd_event, fd, EV_READ,  server_readable, &clients[fd].rd_event);
    event_set(&clients[fd].wr_event, fd, EV_WRITE, server_writable, &clients[fd].wr_event);
    clients[fd].in_buf  = evbuffer_new();
    clients[fd].out_buf = evbuffer_new();
    clients[fd].player  = NULL;

    clients[fd].next = NULL;
    clients[fd].prev = NULL;

    clients[fd].is_gui_client = 0;
    clients[fd].next_gui = NULL;
    clients[fd].prev_gui = NULL;

    lua_pushliteral(L, "on_client_accepted");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, fd);
    lua_pushstring(L, address);

    if (lua_pcall(L, 2, 0, 0) != 0) 
        fprintf(stderr, "error calling on_client_accepted: %s\n", lua_tostring(L, -1));

    event_add(&clients[fd].rd_event, NULL);
    return 1;
}

// TODO: Teilweise durch libevent Bufferfunktionen ersetzbar?
static void server_readable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;
    client_t *client = &clients[fd];

    int ret = evbuffer_read(client->in_buf, fd, 128);
    if (ret < 0) {
        server_destroy(client, strerror(errno));
    } else if (ret == 0) {
        server_destroy(client, "eof reached");
    } else if (EVBUFFER_LENGTH(client->in_buf) > 8192) {
        server_destroy(client, "line too long. go away.");
    } else {
        int pos;

nextline:
        for (pos = 0; pos < EVBUFFER_LENGTH(client->in_buf); pos++) {
            if (EVBUFFER_DATA(client->in_buf)[pos] == '\n') {
                lua_pushliteral(L, "on_client_input");   
                lua_rawget(L, LUA_GLOBALSINDEX);      
                lua_pushnumber(L, fd);               
                if (pos > 0 && EVBUFFER_DATA(client->in_buf)[pos - 1] == '\r') {
                    lua_pushlstring(L, (char*)EVBUFFER_DATA(client->in_buf), pos - 1);
                } else if (pos > 0 && EVBUFFER_DATA(client->in_buf)[0] == '\r') {
                    lua_pushlstring(L, (char*)EVBUFFER_DATA(client->in_buf) + 1, pos - 1);
                } else {
                    lua_pushlstring(L, (char*)EVBUFFER_DATA(client->in_buf), pos);
                }
                
                if (lua_pcall(L, 2, 1, 0) != 0) {
                    fprintf(stderr, "error calling on_client_input: %s\n", lua_tostring(L, -1));
                    server_writeto(client, lua_tostring(L, -1), lua_strlen(L, -1));
                }

                if (!lua_toboolean(L, -1)) {
                    lua_pop(L, 1);
                    server_destroy(client, "user closed");
                    return;
                }

                lua_pop(L, 1);

                evbuffer_drain(client->in_buf, pos + 1);
                goto nextline;
            }
        }
        event_add(cb_event, NULL);
    }
}

static void server_writable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;
    client_t *client = &clients[fd];

    // HACK um die Ausgabe des Consolenclients an
    // stdout statt stdin zu schicken.
    if (fd == STDIN_FILENO) fd = STDOUT_FILENO; 

    int ret = evbuffer_write(client->out_buf, fd);
    if (ret < 0) {
        server_destroy(client, strerror(errno));
    } else if (ret == 0) {
        server_destroy(client, "null write?");
    } else {
        if (EVBUFFER_LENGTH(client->out_buf) > 0) 
            event_add(cb_event, NULL);
    }
}

void server_writeto(client_t *client, const void *data, size_t size) {
    if (size == 0) 
        return;
    if (EVBUFFER_LENGTH(client->in_buf) > 1024*1024)
        return;
    evbuffer_add(client->out_buf, (void*)data, size);
    event_add(&client->wr_event, NULL);
}

void server_writeto_all_gui_clients(const void *data, size_t size) {
    client_t *client = guiclients;
    if (client) do {
        server_writeto(client, data, size);
        client = client->next_gui;
    } while (client != guiclients);
}

void server_send_packet(packet_t *packet, client_t *client) {
    packet->len  = packet->offset;
    if (!client) 
        server_writeto_all_gui_clients(packet, 1 + 1 + packet->len);
    else
        server_writeto(client, packet, 1 + 1 + packet->len);
}

void server_destroy(client_t *client, char *reason) {
    int fd = client_num(client);

    lua_pushliteral(L, "on_client_close");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, fd);
    lua_pushstring(L, reason);
    if (lua_pcall(L, 2, 0, 0) != 0) 
        fprintf(stderr, "error calling on_client_close: %s\n", lua_tostring(L, -1));

    // Versuchen, den Rest rauszuschreiben.
    if (client->is_gui_client) {
        packet_t packet;
        packet_init(&packet, PACKET_QUIT_MSG);
        packet_writeXX(&packet, reason, strlen(reason));
        server_send_packet(&packet, client);
    } else {
        evbuffer_add(client->out_buf, "connection terminating: ", 25);
        evbuffer_add(client->out_buf, reason, strlen(reason));
        evbuffer_add(client->out_buf, "\n", 1);
    }
    evbuffer_write(client->out_buf, fd);

    evbuffer_free(client->in_buf);
    evbuffer_free(client->out_buf);
    event_del(&client->rd_event);
    event_del(&client->wr_event);
    client->in_buf  = NULL;
    client->out_buf = NULL;

    if (client->player)
        player_detach_client(client, client->player);

    assert(client->next == NULL);
    assert(client->prev == NULL);

    if (client->is_gui_client) {
        if (client->next_gui == client) {
            assert(client->prev_gui == client);
            guiclients = NULL;
        } else {
            client->next_gui->prev_gui = client->prev_gui;
            client->prev_gui->next_gui = client->next_gui;
            guiclients = client->next_gui;
        }
    }
    close(fd);
}

static void initial_update(client_t *client) {
    // Protokoll Version senden
    packet_t packet;
    packet_init(&packet, PACKET_HANDSHAKE);
    packet_write08(&packet, PROTOCOL_VERSION);
    server_send_packet(&packet, client);

    world_send_initial_update(client);
    player_send_initial_update(client);
    creature_send_initial_update(client);
}

static void client_turn_into_gui_client(client_t *client) {
    assert(!client->is_gui_client);
    client->is_gui_client = 1;
    if (guiclients) {
        client->prev_gui = guiclients;
        client->next_gui = guiclients->next_gui;
        client->next_gui->prev_gui = client;
        client->prev_gui->next_gui = client;
    } else {
        client->prev_gui = client->next_gui = guiclients = client;
    }
    initial_update(client);
}

static client_t *client_get_checked_lua(lua_State *L, int clientno) {
    if (clientno < 0 || clientno >= MAXCLIENTS) 
        luaL_error(L, "client number %d out of range", clientno);

    if (!CLIENT_USED(&clients[clientno])) 
        luaL_error(L, "client %d not in use", clientno);

    return &clients[clientno];
}

static int luaWriteToClient(lua_State *L) {
    int clientno = luaL_checklong(L, 1);
    size_t msglen; const char *msg = luaL_checklstring(L, 2, &msglen);
    server_writeto(client_get_checked_lua(L, clientno), msg, msglen);
    return 0;
}

static int luaAttachClientToPlayer(lua_State *L) {
    lua_pushboolean(L, player_attach_client(client_get_checked_lua(L, luaL_checklong(L, 1)), 
                                            player_get_checked_lua(L, luaL_checklong(L, 2)), 
                                            luaL_checkstring(L, 3)));
    return 1;
}

static int luaDetachClientFromPlayer(lua_State *L) {
    lua_pushboolean(L, player_detach_client(client_get_checked_lua(L, luaL_checklong(L, 1)), 
                                            player_get_checked_lua(L, luaL_checklong(L, 2)))); 
    return 1;
}

static int luaPlayerKill(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player_mark_for_kill(player);
    return 0;
}

static int luaCreatePlayer(lua_State *L) {
    player_t *player = player_create(luaL_checkstring(L, 1));
    
    if (player) {
        lua_pushnumber(L, player_num(player));
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int luaPlayerNumber(lua_State *L) {
    client_t *client = client_get_checked_lua(L, luaL_checklong(L, 1));
    
    if (!client->player) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, player_num(client->player));
    }
    return 1;
}

static int luaPlayerNumClients(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    lua_pushnumber(L, player->num_clients);
    return 1;
}

static int luaSetPlayerName(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player_set_name(player, luaL_checkstring(L, 2));
    return 0;
}

static int luaSetPlayerScore(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    player->score = luaL_checklong(L, 2);
    return 0;
}

static int luaGetPlayerName(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    lua_pushstring(L, player->name);
    return 1;
}

static int luaTurnIntoGuiClient(lua_State *L) {
    client_t *client = client_get_checked_lua(L, luaL_checklong(L, 1));
    client_turn_into_gui_client(client);
    return 0;
}

static int luaIsGuiClient(lua_State *L) {
    client_t *client = client_get_checked_lua(L, luaL_checklong(L, 1));
    lua_pushboolean(L, client->is_gui_client);
    return 1;
}

static int luaWorldDig(lua_State *L) {
    lua_pushboolean(L, world_dig(luaL_checklong(L, 1), luaL_checklong(L, 2), SOLID));
    return 1;
}

static int luaWorldAddFood(lua_State *L) {
    lua_pushnumber(L, world_add_food(luaL_checklong(L, 1), 
                                     luaL_checklong(L, 2), 
                                     luaL_checklong(L, 3)));
    return 1;
}

static int luaWorldWalkable(lua_State *L) {
    lua_pushboolean(L, world_walkable(luaL_checklong(L, 1), luaL_checklong(L, 2)));
    return 1;
}

static int luaWorldFindDigged(lua_State *L) {
    int x, y;
    world_find_digged(&x, &y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int luaGameInfo(lua_State *L) {
    lua_pushnumber(L, game_time);
    lua_pushnumber(L, world_width());
    lua_pushnumber(L, world_height());
    return 3;
}

static int luaGameTime(lua_State *L) {
    lua_pushnumber(L, game_time);
    return 1;
}

static int luaPlayerKillAllCreatures(lua_State *L) {
    player_t *player = player_get_checked_lua(L, luaL_checklong(L, 1)); 
    creature_kill_all_players_creatures(player);
    return 0;
}

static int luaPlayerExecute(lua_State *L) {
    int clientno = luaL_checklong(L, 1);
    const char *code = luaL_checkstring(L, 2);

    client_t *client = client_get_checked_lua(L, clientno);
    char buf[128];
    snprintf(buf, sizeof(buf), "input from client %d", client_num(client));
    if (!client->player) 
        luaL_error(L, "client %d has no player", clientno);
    player_execute_client_lua(client->player, code, buf);
    return 0;
}

static int luaAddToScroller(lua_State *L) {
    add_to_scroller(luaL_checkstring(L, 1));
    return 0;
}

void server_tick() {
    lua_pushliteral(L, "server_tick");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling server_tick: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    event_loop(EVLOOP_NONBLOCK);
}

void server_init() {
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) 
        die("WSAStartup failed");
#endif

    event_init();

    if (!listener_init()) 
        die("error initializing listener");

    lua_dofile(L, "server.lua");

    memset(clients, 0, sizeof(clients));
    lua_register(L, "write_to_client",          luaWriteToClient);
    lua_register(L, "attach_client_to_player",  luaAttachClientToPlayer);
    lua_register(L, "detach_client_from_player",luaDetachClientFromPlayer);
    lua_register(L, "create_player",            luaCreatePlayer);
    lua_register(L, "set_player_name",          luaSetPlayerName);
    lua_register(L, "get_player_name",          luaGetPlayerName);
    lua_register(L, "player_number",            luaPlayerNumber);
    lua_register(L, "player_execute",           luaPlayerExecute);
    lua_register(L, "player_kill_all_creatures",luaPlayerKillAllCreatures);
    lua_register(L, "set_player_score",         luaSetPlayerScore);
    lua_register(L, "player_num_clients",       luaPlayerNumClients);
    lua_register(L, "turn_into_guiclient",      luaTurnIntoGuiClient);
    lua_register(L, "world_dig",                luaWorldDig);
    lua_register(L, "world_add_food",           luaWorldAddFood);
    lua_register(L, "world_is_walkable",        luaWorldWalkable);
    lua_register(L, "player_kill",              luaPlayerKill);
    lua_register(L, "game_info",                luaGameInfo);
    lua_register(L, "find_digged",              luaWorldFindDigged);
    lua_register(L, "add_to_scroller",          luaAddToScroller);
    lua_register(L, "game_time",                luaGameTime);
    lua_register(L, "is_gui_client",            luaIsGuiClient);

    lua_pushliteral(L, "MAXPLAYERS");
    lua_pushnumber(L,   MAXPLAYERS);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushliteral(L, "GAME_NAME");
    lua_pushliteral(L, GAME_NAME);
    lua_rawset(L, LUA_GLOBALSINDEX);

    // XXX: HACK: stdin client starten
    server_accept(STDIN_FILENO, NULL); 
}

void server_shutdown() {
    int clientno;
    for (clientno = 0; clientno < MAXCLIENTS; clientno++) {
        if (CLIENT_USED(&clients[clientno])) 
            server_destroy(&clients[clientno], "server shutdown");
    }

    listener_shutdown();

#ifdef WIN32
    WSACleanup();
#endif
}

