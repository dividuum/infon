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

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lauxlib.h>
#include <event.h>
#include <zlib.h>

#include "infond.h"
#include "server.h"
#include "packet.h"
#include "global.h"
#include "player.h"
#include "world.h"
#include "listener.h"
#include "misc.h"
#include "game.h"
#include "creature.h"
#include "pinger.h"

#define CLIENT_USED(client) ((client)->in_buf)

static client_t *guiclients = NULL;

static client_t  clients[MAXCLIENTS];
static int       num_clients = 0;

static client_t *output_client = NULL;

static int       traffic       = 0;

static void server_readable(int fd, short event, void *arg);
static void server_writable(int fd, short event, void *arg);

int client_num(client_t *client) {
    return client - clients;
}

client_t *server_accept(int fd, const char *address) {
    client_t *client = clients;

    for (int i = 0; i < MAXCLIENTS; i++, client++) {
        if (!CLIENT_USED(client))
            goto found;
    }

    // write(fd, "no free slot\r\n", 14);
    fprintf(stderr, "cannot accept() new incoming connection: no free slot\n");
    return NULL;

found:
    memset(client, 0, sizeof(client_t));
    client->fd = fd;

    // File Writer wird leicht unterschiedlich behandelt
    client->is_file_writer = strstr(address, "special:file") == address;

    // Non Blocking setzen 
#ifdef WIN32
    DWORD notblock = 1;
    ioctlsocket(client->fd, FIONBIO, &notblock);
#else
    if (fcntl(client->fd, F_SETFL, O_NONBLOCK) < 0) {
        fprintf(stderr, "cannot set accept()ed socket nonblocking: %s\n", strerror(errno));
        return NULL;
    }
#endif

    // Soll Verbindung angenommen werden?
    lua_pushliteral(L, "on_new_client");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushstring(L, address);

    if (lua_pcall(L, 1, 2, 0) != 0) {
        fprintf(stderr, "error calling on_new_client: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return NULL;
    }

    if (!lua_toboolean(L, -2)) {
        size_t len; const char *msg = lua_tolstring(L, -1, &len);
        write(client->fd, msg, len);
        lua_pop(L, 2);
        return NULL;
    }

    lua_pop(L, 2);

    // Libevent aktivieren
    event_set(&client->rd_event, client->fd, EV_READ | EV_PERSIST, server_readable, client);
    event_set(&client->wr_event, client->fd, EV_WRITE            , server_writable, client);
    
    client->in_buf  = evbuffer_new();
    client->out_buf = evbuffer_new();

    client->compress = 0;

    client->kill_me = NULL;
    client->player  = NULL;

    client->next = NULL;
    client->prev = NULL;

    client->is_gui_client = 0;
    client->next_gui = NULL;
    client->prev_gui = NULL;

    num_clients++;

    // Annehmen
    lua_pushliteral(L, "on_client_accepted");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, client_num(client));
    lua_pushstring(L, address);

    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling on_client_accepted: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    if (!client->is_file_writer)
        event_add(&client->rd_event, NULL);

    return client;
}

static void server_readable(int fd, short event, void *arg) {
    client_t *client = (client_t*)arg;

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
            lua_pushnumber(L, client_num(client));
            lua_pushstring(L, line);
            free(line);

            // Cycles fuer die Verarbeitung hochsetzen
            lua_set_cycles(L, 0xFFFFFF);
                
            // Input verarbeiten
            output_client = client;
            if (lua_pcall(L, 2, 0, 0) != 0) {
                fprintf(stderr, "error calling on_client_input: %s\n", lua_tostring(L, -1));
                server_writeto(client, lua_tostring(L, -1), lua_strlen(L, -1));
                lua_pop(L, 1);
            }
            output_client = NULL;

            // Kill Me Flag waehrend Aufruf von on_client_input 
            // gesetzt? Direkt rausschmeissen!
            if (client->kill_me) {
                server_destroy(client, client->kill_me);
                return;
            }
        }
    }
}

static void server_flush_compression(client_t *client) {
    if (!client->compress)
        return;

    char buf[1024];
    client->strm.next_in  = NULL;
    client->strm.avail_in = 0;
    do {
        client->strm.next_out  = (unsigned char*)buf;
        client->strm.avail_out = sizeof(buf);
        if (deflate(&client->strm, Z_SYNC_FLUSH) != Z_OK) {
            fprintf(stderr, "urgh. deflate (Z_SYNC_FLUSH) didn't return Z_OK");
            // XXX: handle
        }
        evbuffer_add(client->out_buf, buf, sizeof(buf) - client->strm.avail_out);
    } while (client->strm.avail_out == 0);
}

static void server_writable(int fd, short event, void *arg) {
    client_t *client = (client_t*)arg;

#ifndef NO_CONSOLE_CLIENT    
    // HACK um die Ausgabe des Consolenclients an
    // stdout statt stdin zu schicken.
    if (fd == STDIN_FILENO) fd = STDOUT_FILENO; 
#endif

    // Kompressionsrest flushen
    server_flush_compression(client);

    // Schreiben
    int ret = evbuffer_write(client->out_buf, fd);
    if (ret < 0) {
        server_destroy(client, strerror(errno));
    } else if (ret == 0) {
        server_destroy(client, "null write?");
    } else {
        if (EVBUFFER_LENGTH(client->out_buf) > 0) 
            event_add(&client->wr_event, NULL);
    }
}

void server_start_compression(client_t *client) {
    // Kompression bereits aktiviert?
    if (client->compress)
        return;

    // Demos sind immer unkomprimiert. Eine Kompression ueber 
    // die erstellte Datei macht wesentlich mehr Sinn.
    if (client->is_file_writer)
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
    traffic += size;

    // Fileclients werden direkt ueber write abgewickelt.
    if (client->is_file_writer) {
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
            client->strm.next_out  = (unsigned char*)buf;
            client->strm.avail_out = sizeof(buf);
            int ret = deflate(&client->strm, 0);
            if (ret != Z_OK) {
                fprintf(stderr, "urgh. deflate didn't return Z_OK: %d\n", ret);
                // XXX: handle
            }
            evbuffer_add(client->out_buf, buf, sizeof(buf) - client->strm.avail_out);
        }
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
    lua_pushliteral(L, "on_client_close");
    lua_rawget(L, LUA_GLOBALSINDEX);
    lua_pushnumber(L, client_num(client));
    lua_pushstring(L, reason);
    if (lua_pcall(L, 2, 0, 0) != 0) {
        fprintf(stderr, "error calling on_client_close: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Quitmeldung senden
    if (client->is_gui_client) {
        packet_t packet;
        packet_init(&packet, PACKET_QUIT_MSG);
        packet_writeXX(&packet, reason, strlen(reason));
        server_send_packet(&packet, client);
    } else {
        server_writeto(client, "connection terminated: ", 23);
        server_writeto(client, reason, strlen(reason));
        server_writeto(client, "\r\n", 2);
    }

    // Kompressionsrest flushen
    server_flush_compression(client);

    // Rest rausschreiben (hier keine Fehlerbehandlung mehr, da eh egal).
    // Bei Filewritern muss nichts geschrieben werden, da deren Daten
    // immer direkt rausgeschrieben werden.
    if (!client->is_file_writer) 
        evbuffer_write(client->out_buf, client->fd);

    evbuffer_free(client->in_buf);
    evbuffer_free(client->out_buf);

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

    num_clients--;
    
#ifndef NO_CONSOLE_CLIENT    
    if (client->fd != STDIN_FILENO)
#endif
#ifdef WIN32
        if (client->is_file_writer) {
            close(client->fd);
        } else {
            closesocket(client->fd);
        }
#else
        close(client->fd);
#endif
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

client_t *server_start_file_writer(const char *filename) {
    int fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0644);
    if (fd < 0)
        return NULL;
    char address[512];
    snprintf(address, sizeof(address), "special:file:%s", filename);
    return server_accept(fd, address);
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

static int luaStartFileWriter(lua_State *L) {
    const char *file     = luaL_checkstring(L, 1);
    int         one_game = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : 1;
    int    is_gui_client = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : 1;
    client_t *filewriter = server_start_file_writer(file);
    if (!filewriter) 
        luaL_error(L, "cannot start file %s", file);
    if (is_gui_client) 
        client_turn_into_gui_client(filewriter);
    if (one_game)      
        filewriter->kick_at_end_of_game = one_game;
    lua_pushnumber(L, client_num(filewriter));
    return 1;
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
    client_turn_into_gui_client(client_get_checked_lua(L, 1));
    return 0;
}

static int luaClientIsGuiClient(lua_State *L) {
    lua_pushboolean(L, client_get_checked_lua(L, 1)->is_gui_client);
    return 1;
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
    free(client->kill_me);
    client->kill_me = strdup(reason);
    return 0;
}

static int luaClientPrint(lua_State *L) {
    // No output client set? => Discard
    if (!output_client || !CLIENT_USED(output_client)) 
        return 0;

    int n=lua_gettop(L);
    for (int i=1; i <= n; i++) {
        if (i > 1) server_writeto(output_client, "\t", 1);
        if (lua_isstring(L,i))
            server_writeto(output_client, lua_tostring(L,i), lua_strlen(L, i));
        else if (lua_isnil(L,i))
            server_writeto(output_client, "nil", 3);
        else if (lua_isboolean(L,i))
            lua_toboolean(L,i) ? server_writeto(output_client, "true",  4): 
                                 server_writeto(output_client, "false", 5);
        else {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s:%p", lua_typename(L,lua_type(L,i)), lua_topointer(L,i));
            server_writeto(output_client, buffer, strlen(buffer));
        }
    }
    server_writeto(output_client, "\r\n", 2);
    return 0;
}

static int luaGetTraffic(lua_State *L) {
    lua_pushnumber(L, traffic);
    return 1;
}

static int luaSetupListener(lua_State *L) {
    lua_pushboolean(L, listener_init(luaL_checkstring(L, 1), luaL_checklong(L, 2)));
    return 1;
}

static int luaPingMaster(lua_State *L) {
    ping_master(luaL_checkstring(L, 1), luaL_checklong(L, 2),
                luaL_checkstring(L, 3), 
                server_num_clients(), 
                player_num_players(),
                creature_num_creatures());
    return 0;
}

static int luaServerInfo(lua_State *L) {
    lua_pushnumber(L, server_num_clients());
    lua_pushnumber(L, player_num_players());
    lua_pushnumber(L, creature_num_creatures());
    return 3;
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

int server_num_clients() {
    return num_clients;
}

void server_init() {
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) 
        die("WSAStartup failed");
#endif

    event_init();

    memset(clients, 0, sizeof(clients));
    lua_register(L, "client_write",             luaClientWrite);
    lua_register(L, "client_attach_to_player",  luaClientAttachToPlayer);
    lua_register(L, "client_detach_from_player",luaClientDetachFromPlayer);
    lua_register(L, "client_make_guiclient",    luaClientMakeGuiClient);
    lua_register(L, "client_is_gui_client",     luaClientIsGuiClient);
    lua_register(L, "client_player_number",     luaClientPlayerNumber);
    lua_register(L, "client_disconnect",        luaClientDisconnect);
    lua_register(L, "client_print",             luaClientPrint);
    lua_register(L, "cprint",                   luaClientPrint);
    lua_register(L, "server_start_writer",      luaStartFileWriter);
    lua_register(L, "server_get_traffic",       luaGetTraffic);
    lua_register(L, "server_ping_master",       luaPingMaster);
    lua_register(L, "server_info",              luaServerInfo);
    lua_register(L, "setup_listener",           luaSetupListener);
}

void server_game_end() {
    int clientno;
    for (clientno = 0; clientno < MAXCLIENTS; clientno++) {
        if (CLIENT_USED(&clients[clientno]) && clients[clientno].kick_at_end_of_game) 
            server_destroy(&clients[clientno], "game ended");
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

