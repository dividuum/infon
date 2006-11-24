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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>


#include <lauxlib.h>
#include <event.h>
#include <zlib.h>

#include "server.h"
#include "packet.h"
#include "global.h"
#include "player.h"
#include "world.h"
#include "listener.h"
#include "misc.h"
#include "game.h"
#include "creature.h"

#define CLIENT_USED(client) ((client)->in_buf)

static client_t *guiclients = NULL;

static void server_readable(int fd, short event, void *arg);
static void server_writable(int fd, short event, void *arg);

int client_num(client_t *client) {
    return client - clients;
}

client_t *server_accept(int fd, struct sockaddr_in *peer) {
    static struct linger l = { 1, 0 };
    static const int one = 1;
    static char address[128];

    if (fd >= MAXCLIENTS) {
        fprintf(stderr, "cannot accept() new incoming connection: file descriptor too large\n");
        return NULL;
    }
    
    memset(&clients[fd], 0, sizeof(client_t));
    client_t *client = &clients[fd];

    if (peer) {
        sprintf(address, "%s:%d", inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
    } else if (fd == STDIN_FILENO) {
        sprintf(address, "local console");
    } else {
        sprintf(address, "demo dumper");
        client->is_demo_dumper = 1;
    }

    lua_pushliteral(L, "on_new_client");/* funcname */
    lua_rawget(L, LUA_GLOBALSINDEX);    /* func     */
    lua_pushstring(L, address);         /* addr     */

    if (lua_pcall(L, 1, 1, 0) != 0) {
        fprintf(stderr, "error calling on_new_client: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return NULL;
    }

    if (!lua_toboolean(L, -1)) {
        fprintf(stderr, "rejected client %s\n", address);
        lua_pop(L, 1);
        return NULL;
    }

    lua_pop(L, 1);

    /* Non Blocking setzen */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        fprintf(stderr, "cannot set accept()ed socket nonblocking: %s\n", strerror(errno));
        return NULL;
    }

    if (peer) { 
        /* TCP_NODELAY setzen. Dadurch werden Daten frühestmöglich versendet */
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0) {
            fprintf(stderr, "cannot enable TCP_NODELAY: %s\n", strerror(errno));
            return NULL;
        }

        /* SO_LINGER setzen. Falls sich noch Daten in der Sendqueue der Verbindung
           befinden, werden diese verworfen. */
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1) {
            fprintf(stderr, "cannot set SO_LINGER: %s\n", strerror(errno));
            return NULL;
        }
    }

    // Aktivieren
    event_set(&client->rd_event, fd, EV_READ,  server_readable, &client->rd_event);
    event_set(&client->wr_event, fd, EV_WRITE, server_writable, &client->wr_event);
    
    client->in_buf    = evbuffer_new();
    client->out_buf   = evbuffer_new();

    client->compress = 0;

    client->kill_me  = NULL;
    client->player   = NULL;

    client->next = NULL;
    client->prev = NULL;

    client->is_gui_client = 0;
    client->next_gui = NULL;
    client->prev_gui = NULL;

    lua_pushliteral(L, "on_client_accepted");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, fd);
    lua_pushstring(L, address);

    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling on_client_accepted: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    if (!client->is_demo_dumper)
        event_add(&client->rd_event, NULL);

    return client;
}

static void server_readable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;
    client_t *client = &clients[fd];

    // Der Client wurde 'extern' gekickt, allerdings noch
    // nicht entfernt. Dann wird dieser Readcallback aufgerufen,
    // sollte allerdings nichts mehr machen.
    if (client->kill_me)
        return;

    int ret = evbuffer_read(client->in_buf, fd, 128);
    if (ret < 0) {
        server_destroy(client, strerror(errno));
    } else if (ret == 0) {
        server_destroy(client, "eof reached");
    } else if (EVBUFFER_LENGTH(client->in_buf) > 8192) {
        server_destroy(client, "line too long. go away.");
    } else {
        char *line;
        while ((line = evbuffer_readline(client->in_buf))) {
            lua_pushliteral(L, "on_client_input");   
            lua_rawget(L, LUA_GLOBALSINDEX);      
            lua_pushnumber(L, fd);               
            lua_pushstring(L, line);
            free(line);
                
            if (lua_pcall(L, 2, 0, 0) != 0) {
                fprintf(stderr, "error calling on_client_input: %s\n", lua_tostring(L, -1));
                server_writeto(client, lua_tostring(L, -1), lua_strlen(L, -1));
                lua_pop(L, 1);
            }

            // Kill Me Flag waehrend Aufruf von on_client_input 
            // gesetzt? Direkt rausschmeissen!
            if (client->kill_me) {
                server_destroy(client, client->kill_me);
                return;
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

    // Kompressionsrest flushen
    if (client->compress) {
        char buf[1024];
        client->strm.next_in  = NULL;
        client->strm.avail_in = 0;
        do {
            client->strm.next_out  = buf;
            client->strm.avail_out = sizeof(buf);
            if (deflate(&client->strm, Z_SYNC_FLUSH) != Z_OK) {
                fprintf(stderr, "urgh. deflate (Z_SYNC_FLUSH) didn't return Z_OK");
                // XXX: handle
            }
            evbuffer_add(client->out_buf, buf, sizeof(buf) - client->strm.avail_out);
        } while (client->strm.avail_out == 0);
    }

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

void server_start_compression(client_t *client) {
    if (client->compress)
        return;

    client->strm.zalloc = Z_NULL;
    client->strm.zfree  = Z_NULL;
    client->strm.opaque = NULL;
    if (deflateInit(&client->strm, 9) != Z_OK) {
        fprintf(stderr, "cannot alloc new zstream\n");
        return;
    }

    packet_t packet;
    packet_init(&packet, PACKET_START_COMPRESS);
    server_send_packet(&packet, client);
    client->compress  = 1;
}

void server_writeto(client_t *client, const void *data, size_t size) {
    if (size == 0) 
        return;
    if (client->is_demo_dumper) {
        write(client_num(client), data, size);
        return;
    }
    if (EVBUFFER_LENGTH(client->out_buf) > 1024*1024)
        return;
    if (client->compress) {
        char buf[1024];
        client->strm.next_in  = (void*)data; // not const?
        client->strm.avail_in = size;
        while (client->strm.avail_in > 0) {
            client->strm.next_out  = buf;
            client->strm.avail_out = sizeof(buf);
            int ret = deflate(&client->strm, 0);
            if (ret != Z_OK) {
                fprintf(stderr, "urgh. deflate didn't return Z_OK: %d\n", ret);
                // XXX: handle
            }
            evbuffer_add(client->out_buf, buf, sizeof(buf) - client->strm.avail_out);
        }
        // printf("%d => %d\n", client->strm.total_in, client->strm.total_out);
    } else {
        evbuffer_add(client->out_buf, (void*)data, size);
    }
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
        server_writeto_all_gui_clients(packet, PACKET_HEADER_SIZE + packet->len);
    else
        server_writeto(client, packet, PACKET_HEADER_SIZE + packet->len);
}

void server_destroy(client_t *client, const char *reason) {
    int fd = client_num(client);

    lua_pushliteral(L, "on_client_close");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, fd);
    lua_pushstring(L, reason);
    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling on_client_close: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Versuchen, den Rest rauszuschreiben.
    if (client->is_gui_client) {
        packet_t packet;
        packet_init(&packet, PACKET_QUIT_MSG);
        packet_writeXX(&packet, reason, strlen(reason));
        server_send_packet(&packet, client);
        // TODO: funktioniert momentan nicht, da kein 
        // FULL_FLUSH vor evbuffer_write
    } else {
        evbuffer_add(client->out_buf, "connection terminating: ", 25);
        evbuffer_add(client->out_buf, (char*)reason, strlen(reason));
        evbuffer_add(client->out_buf, "\r\n", 2);
    }

    if (!client->is_demo_dumper) 
        evbuffer_write(client->out_buf, fd);

    evbuffer_free(client->in_buf);
    evbuffer_free(client->out_buf);

    if (client->kill_me)
        free(client->kill_me);

    if (client->compress)
        deflateEnd(&client->strm);
    
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

    server_start_compression(client);

    game_send_initial_update(client);
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

client_t *client_get_checked_lua(lua_State *L, int idx) {
    int clientno = luaL_checklong(L, idx);
    if (clientno < 0 || clientno >= MAXCLIENTS) 
        luaL_error(L, "client number %d out of range", clientno);
    client_t *client = &clients[clientno];
    if (!CLIENT_USED(client)) 
        luaL_error(L, "client %d not in use", clientno);
    return client;
}

static int luaClientWrite(lua_State *L) {
    size_t msglen; const char *msg = luaL_checklstring(L, 2, &msglen);
    client_t *client = client_get_checked_lua(L, 1);
    // Nur schreiben, falls der Client kein GUI Client
    if (!client->is_gui_client)
        server_writeto(client, msg, msglen);
    return 0;
}

static int luaClientAttachToPlayer(lua_State *L) {
    lua_pushboolean(L, player_attach_client(client_get_checked_lua(L, 1), 
                                            player_get_checked_lua(L, 2), 
                                            luaL_checkstring(L, 3)));
    return 1;
}

static int luaClientDetachFromPlayer(lua_State *L) {
    lua_pushboolean(L, player_detach_client(client_get_checked_lua(L, 1), 
                                            player_get_checked_lua(L, 2))); 
    return 1;
}

static int luaClientMakeGuiClient(lua_State *L) {
    client_t *client = client_get_checked_lua(L, 1);
    client_turn_into_gui_client(client);
    return 0;
}

static int luaClientIsGuiClient(lua_State *L) {
    client_t *client = client_get_checked_lua(L, 1);
    lua_pushboolean(L, client->is_gui_client);
    return 1;
}

static int luaClientExecute(lua_State *L) {
    size_t codelen; const char *code = luaL_checklstring(L, 2, &codelen);
    client_t *client = client_get_checked_lua(L, 1);
    int client_local_output = lua_toboolean(L, 3);
    if (!client->player) 
        luaL_error(L, "client %d has no player", client_num(client));
    char buf[128];
    snprintf(buf, sizeof(buf), "input from client %d", client_num(client));
    player_execute_client_lua(client_local_output ? client : NULL, client->player, code, codelen, buf);
    return 0;
}

static int luaClientPlayerNumber(lua_State *L) {
    client_t *client = client_get_checked_lua(L, 1);
    
    if (!client->player) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, player_num(client->player));
    }
    return 1;
}

static int luaClientDisconnect(lua_State *L) {
    client_t *client = client_get_checked_lua(L, 1);
    const char *reason = luaL_checkstring(L, 2);
    if (client->kill_me) 
        free(client->kill_me);
    client->kill_me = strdup(reason);
    return 0;
}

void server_tick() {
    lua_set_cycles(L, 0xFFFFFF);
    
    lua_pushliteral(L, "server_tick");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling server_tick: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    event_loop(EVLOOP_NONBLOCK);

    // Ungefaehr jede Sekunde alle zu kickenden Clients entfernen.
    static int kicktick = 0;
    if (++kicktick % 10 == 0) {
        int clientno;
        for (clientno = 0; clientno < MAXCLIENTS; clientno++) {
            if (!CLIENT_USED(&clients[clientno])) 
                continue;
            if (clients[clientno].kill_me)
                server_destroy(&clients[clientno], clients[clientno].kill_me);
        }
    }
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

    memset(clients, 0, sizeof(clients));
    lua_register(L, "client_write",             luaClientWrite);
    lua_register(L, "client_attach_to_player",  luaClientAttachToPlayer);
    lua_register(L, "client_detach_from_player",luaClientDetachFromPlayer);
    lua_register(L, "client_execute",           luaClientExecute);
    lua_register(L, "client_make_guiclient",    luaClientMakeGuiClient);
    lua_register(L, "client_is_gui_client",     luaClientIsGuiClient);
    lua_register(L, "client_player_number",     luaClientPlayerNumber);
    lua_register(L, "client_disconnect",        luaClientDisconnect);

    // XXX: HACK: stdin client starten
    server_accept(STDIN_FILENO, NULL); 
}

client_t *server_start_demo_writer(const char *demoname) {
    int server_demo_fd = open(demoname, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR);
    if (server_demo_fd < 0)
        return NULL;
    client_t *demowriter = server_accept(server_demo_fd, NULL);
    client_turn_into_gui_client(demowriter);
    return demowriter;
}

void server_round_start() {
    lua_pushliteral(L, "server_new_round");
    lua_rawget(L, LUA_GLOBALSINDEX);
    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "error calling server_new_round: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
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

