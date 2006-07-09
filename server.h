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

#include "packet.h"

#define MAXCLIENTS 1024

#define SEND_BROADCAST NULL

typedef struct client_s {
    struct event rd_event;
    struct event wr_event;
    struct evbuffer *in_buf;
    struct evbuffer *out_buf;

    // Verbindung von: 1-player <-> N-client
    struct player_s *player;
    struct client_s *next;
    struct client_s *prev;

    // Client, welcher kontinuierlich Updates erhaelt
    int is_gui_client;
    struct client_s *next_gui;
    struct client_s *prev_gui;
} client_t;

client_t clients[MAXCLIENTS];

int  client_accept(int fd, struct sockaddr_in *peer);
void client_writeto(client_t *client, const void *data, size_t size);
void client_writeto_all_gui_clients(const void *data, size_t size);
void client_destroy(client_t *client, char *reason);

void client_send_packet(packet_t *packet, client_t *client);


void server_tick();

void server_init();
void server_shutdown();

#endif
