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

#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <event.h>
#include <zlib.h>

#include "packet.h"
#include "infond.h"
#include "global.h"

#define MAXCLIENTS 1024

#define SEND_BROADCAST NULL

typedef struct client_s {
    int              fd;

    struct event     rd_event;
    struct event     wr_event;
    struct evbuffer *in_buf;
    struct evbuffer *out_buf;

    int              is_file_writer;
    int              compress;
    z_stream         strm;

    char            *kill_me;
    int              kick_at_end_of_game;

    // Verbindung von: 1-player <-> N-client
    struct player_s *player;
    struct client_s *next;
    struct client_s *prev;

    // Client, welcher kontinuierlich Updates erhaelt
    int is_gui_client;
    struct client_s *next_gui;
    struct client_s *prev_gui;
} client_t;

client_t *server_accept(int fd, const char *address);
void      server_writeto(client_t *client, const void *data, size_t size);
void      server_writeto_all_gui_clients(const void *data, size_t size);
void      server_destroy(client_t *client, const char *reason);

client_t *client_get_checked_lua(lua_State *L, int idx);

void      server_send_packet(packet_t *packet, client_t *client);

client_t *server_start_demo_writer(const char *demoname, int one_game);

void      server_tick();
int       server_num_clients();

void      server_init();
void      server_game_end();
void      server_shutdown();

#endif
