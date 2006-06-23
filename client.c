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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <event.h>

#include "packet.h"
#include "global.h"
#include "player.h"
#include "server.h"
#include "world.h"
#include "misc.h"
#include "scroller.h"
#include "creature.h"

static int serverfd;
static struct event rd_event;
static struct event wr_event;
static struct evbuffer *in_buf;
static struct evbuffer *out_buf;

void server_destroy(char *reason);
void server_writeto(const void *data, size_t size);
int  client_is_connected();

static void server_handle_packet(packet_t *packet) {
    //printf("ptype=%d\n", packet->type);
    switch (packet->type) {
        case PACKET_PLAYER_UPDATE:  
            player_from_network(packet);
            break;
        case PACKET_WORLD_UPDATE:  
            world_from_network(packet);
            break;
        //case PACKET_CREATURE_UPDATE: creature_f                                    
        case PACKET_SCROLLER_MSG:   
            scroller_from_network(packet);
            break;
        case PACKET_WELCOME_MSG:    
            server_writeto("guiclient\n", 10);
            break;
        case PACKET_QUIT_MSG:
            printf("server wants us to disconnect: %.*s\n",
                   packet->len, packet->data);
            server_destroy("done");
            break;
        default:
            printf("packet->type %d unknown\n", packet->type);
            break;
    }
}

static void server_readable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;

    int ret = evbuffer_read(in_buf, fd, 260);
    if (ret < 0) {
        server_destroy(strerror(errno));
    } else if (ret == 0) {
        server_destroy("eof reached");
    } else if (EVBUFFER_LENGTH(in_buf) > 8192) {
        server_destroy("line too long. strange server.");
    } else {
        while (EVBUFFER_LENGTH(in_buf) >= (int)EVBUFFER_DATA(in_buf)[0] + 2) {
            packet_t packet;
            memcpy(&packet,EVBUFFER_DATA(in_buf), (int)EVBUFFER_DATA(in_buf)[0] + 2);
            int len = packet.len + 2;
            packet_reset(&packet);
            server_handle_packet(&packet);
            if (!client_is_connected()) 
                return;
            evbuffer_drain(in_buf, len);
        }
        event_add(cb_event, NULL);
    }
}

static void server_writable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;

    int ret = evbuffer_write(out_buf, fd);
    if (ret < 0) {
        server_destroy(strerror(errno));
    } else if (ret == 0) {
        server_destroy("null write?");
    } else {
        if (EVBUFFER_LENGTH(out_buf) > 0) 
            event_add(cb_event, NULL);
    }
}

void server_writeto(const void *data, size_t size) {
    if (size == 0) 
        return;
    if (EVBUFFER_LENGTH(in_buf) > 1024*1024)
        return;
    evbuffer_add(out_buf, (void*)data, size);
    event_add(&wr_event, NULL);
}

void server_destroy(char *reason) {
    assert(serverfd != -1);
    printf("disconnected from server: %s\n", reason);
    evbuffer_free(in_buf);
    evbuffer_free(out_buf);
    event_del(&rd_event);
    event_del(&wr_event);
    close(serverfd);
    serverfd = -1;
}

int  client_is_connected() {
    return serverfd != -1;
}

void client_tick() {
    event_loop(EVLOOP_NONBLOCK);
}

void client_init(char *addr) {
    event_init();

    int    port = 1234;                                             
    struct hostent *host;

    char *colon = index(addr, ':');
    if (colon) {                                       
        *colon = '\0';                                            
        port = atoi(colon + 1);                                  
    }                                                          

    struct sockaddr_in serveraddr;                                   
    memset(&serveraddr, 0, sizeof(serveraddr)); 
                                                                   
    host = gethostbyname(addr);
    if (!host)
        die("gethostbyname failed: %s", strerror(errno));

    if (host->h_length != 4) 
        die("h_length != 4");

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(port);     
    memcpy(&serveraddr.sin_addr, host->h_addr_list[0], host->h_length);

    printf("connecting to %s:%d (%s:%d)\n", addr, port, inet_ntoa(serveraddr.sin_addr), port);

    /* Socket erzeugen */
    serverfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fehler beim Socket erzeugen? */
    if (serverfd == -1) 
        die("cannot open socket: %s", strerror(errno));

    if (connect(serverfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        die("cannot connect socket: %s", strerror(errno));

    /* Non Blocking setzen */
    if (fcntl(serverfd, F_SETFL, O_NONBLOCK) < 0) 
        die("cannot set socket nonblocking: %s", strerror(errno));

    event_set(&rd_event, serverfd, EV_READ,  server_readable, &rd_event);
    event_set(&wr_event, serverfd, EV_WRITE, server_writable, &wr_event);
    in_buf  = evbuffer_new();
    out_buf = evbuffer_new();

    event_add(&rd_event, NULL);
}

void client_shutdown() {
    if (client_is_connected())
        server_destroy("shutdown");
}

