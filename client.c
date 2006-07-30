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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <event.h>
#include <zlib.h>

#include "packet.h"
#include "global.h"
#include "client.h"
#include "misc.h"
#include "gui_player.h"
#include "gui_world.h"
#include "gui_scroller.h"
#include "gui_creature.h"

static int              clientfd;
static struct event     rd_event;
static struct event     wr_event;
static struct evbuffer *in_buf;
static struct evbuffer *packet_buf;
static struct evbuffer *out_buf;

static int              compression;
static z_stream         strm;

void client_destroy(char *reason);
void client_writeto(const void *data, size_t size);
int  client_is_connected();

static void client_read_handshake(packet_t *packet) {
    uint8_t serverprotocol;
    if (!packet_read08(packet, &serverprotocol)) PROTOCOL_ERROR();
    if (serverprotocol != PROTOCOL_VERSION) {
        die("server has %s protocol version %d. I have %d.\n"
            "visit the infon homepage for more information.",
               serverprotocol < PROTOCOL_VERSION ? "older" : "newer",
               serverprotocol,  PROTOCOL_VERSION);
    }
}

static void client_start_compression() {
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = NULL;
    if (inflateInit(&strm) != Z_OK)
        die("cannot initialize decompression");
    compression = 1;
}

static void client_handle_packet(packet_t *packet) {
    switch (packet->type) {
        case PACKET_PLAYER_UPDATE:  
            gui_player_from_network(packet);
            break;
        case PACKET_WORLD_UPDATE:  
            gui_world_from_network(packet);
            break;
        case PACKET_SCROLLER_MSG:   
            gui_scroller_from_network(packet);
            break;
        case PACKET_CREATURE_UPDATE: 
            gui_creature_from_network(packet);
            break;
        case PACKET_QUIT_MSG:
            printf("server wants us to disconnect: %.*s\n",
                   packet->len, packet->data);
            client_destroy("done");
            break;
        case PACKET_KOTH_UPDATE:
            gui_player_king_from_network(packet);
            break;
        case PACKET_WORLD_INFO:
            gui_world_info_from_network(packet);
            break;
        case PACKET_WELCOME_MSG:    
            client_writeto("guiclient\n", 10);
            break;
        case PACKET_START_COMPRESS:
            client_start_compression();
            break;
        case PACKET_HANDSHAKE:
            client_read_handshake(packet);
            break;
        default:
            printf("packet->type %d unknown\n", packet->type);
            break;
    }
}

static void client_readable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;

    int ret = evbuffer_read(in_buf, fd, 8192);
    if (ret < 0) {
        client_destroy(strerror(errno));
        return;
    } else if (ret == 0) {
        client_destroy("eof reached");
        return;
    }

restart:
    if (compression) {
        char buf[8192];
        strm.next_in   = EVBUFFER_DATA(in_buf);
        strm.avail_in  = EVBUFFER_LENGTH(in_buf);
        do {
            strm.next_out  = buf;
            strm.avail_out = sizeof(buf);
            if (inflate(&strm, Z_SYNC_FLUSH) != Z_OK) {
                client_destroy("decompression error");
                return;
            }
            evbuffer_add(packet_buf, buf, sizeof(buf) - strm.avail_out);
            if (EVBUFFER_LENGTH(packet_buf) > 1024 * 1024) {
                client_destroy("too much to decompress. funny server?");
                return;
            }
        } while (strm.avail_out == 0);
        evbuffer_drain(in_buf, EVBUFFER_LENGTH(in_buf) - strm.avail_in);
    } else {
        evbuffer_add_buffer(packet_buf, in_buf);
        evbuffer_drain(in_buf, EVBUFFER_LENGTH(in_buf));
    }

    int old_compression = compression;

    while (EVBUFFER_LENGTH(packet_buf) >= (int)EVBUFFER_DATA(packet_buf)[0] + 2) {
        // Packet rauspopeln...
        packet_t packet;
        memcpy(&packet,EVBUFFER_DATA(packet_buf), (int)EVBUFFER_DATA(packet_buf)[0] + 2);
        int len = packet.len + 2;
        packet_rewind(&packet);
        client_handle_packet(&packet);
        evbuffer_drain(packet_buf, len);

        // Disconnect Packet?
        if (!client_is_connected()) 
            return;

        // Wurde Kompression aktiviert? Dann Rest des packet_buf zurueck 
        // in den in_buf, da es sich dabei bereits um komprimierte Daten 
        // handelt und diese oben erst dekomprimiert werden muessen.
        if (compression != old_compression) {
            evbuffer_add_buffer(in_buf, packet_buf);
            evbuffer_drain(packet_buf, EVBUFFER_LENGTH(packet_buf));
            goto restart;
        }
    }

    event_add(cb_event, NULL);
}

static void client_writable(int fd, short event, void *arg) {
    struct event  *cb_event = arg;

    int ret = evbuffer_write(out_buf, fd);
    if (ret < 0) {
        client_destroy(strerror(errno));
    } else if (ret == 0) {
        client_destroy("null write?");
    } else {
        if (EVBUFFER_LENGTH(out_buf) > 0) 
            event_add(cb_event, NULL);
    }
}

void client_writeto(const void *data, size_t size) {
    if (size == 0) 
        return;
    if (EVBUFFER_LENGTH(in_buf) > 1024*1024)
        return;
    evbuffer_add(out_buf, (void*)data, size);
    event_add(&wr_event, NULL);
}

void client_destroy(char *reason) {
    assert(clientfd != -1);
    printf("disconnected from server: %s\n", reason);
    evbuffer_free(in_buf);
    evbuffer_free(out_buf);
    evbuffer_free(packet_buf);
    if (compression) {
        inflateEnd(&strm);
    }
    event_del(&rd_event);
    event_del(&wr_event);
    close(clientfd);
    clientfd = -1;
}

int  client_is_connected() {
    return clientfd != -1;
}

void client_tick() {
    event_loop(EVLOOP_NONBLOCK);
}

void client_init(char *addr) {
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) 
        die("WSAStartup failed");
#endif

    event_init();

    int    port = 1234;                                             
    struct hostent *host;

#ifdef WIN32
    char *colon = addr;
    while (*colon && *colon != ':') 
        colon++;
    if (!*colon) colon = NULL;
#else
    char *colon = index(addr, ':');
#endif
    if (colon) {                                       
        *colon = '\0';                                            
        port = atoi(colon + 1);                                  
    }                                                          

    struct sockaddr_in serveraddr;                                   
    memset(&serveraddr, 0, sizeof(serveraddr)); 
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(port);     
                                                                   
    fprintf(stderr, "resolving %s\n", addr);

    host = gethostbyname(addr);
    if (!host)
#ifdef WIN32
        die("gethostbyname failed: %d", WSAGetLastError());
#else
        die("gethostbyname failed: %s", hstrerror(h_errno));
#endif

    if (host->h_length != 4) 
        die("h_length != 4");

    memcpy(&serveraddr.sin_addr, host->h_addr_list[0], host->h_length);

    fprintf(stderr, "connecting to %s:%d (%s:%d)\n", addr, port, inet_ntoa(serveraddr.sin_addr), port);

    /* Socket erzeugen */
    clientfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fehler beim Socket erzeugen? */
#ifdef WIN32
    if (clientfd == INVALID_SOCKET)
        die("cannot open socket: Error %d", WSAGetLastError());
#else
    if (clientfd == -1) 
        die("cannot open socket: %s", strerror(errno));
#endif

#ifdef WIN32
    if (connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        die("cannot connect socket: Error %d", WSAGetLastError());
#else
    if (connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        die("cannot connect socket: %s", strerror(errno));
#endif

    fprintf(stderr, "connected!\n");

    /* Non Blocking setzen */
#ifdef WIN32
    DWORD notblock = 1;
    ioctlsocket(clientfd, FIONBIO, &notblock);
#else
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0) 
        die("cannot set socket nonblocking: %s", strerror(errno));
#endif
    
    event_set(&rd_event, clientfd, EV_READ,  client_readable, &rd_event);
    event_set(&wr_event, clientfd, EV_WRITE, client_writable, &wr_event);

    in_buf      = evbuffer_new();
    out_buf     = evbuffer_new();
    packet_buf  = evbuffer_new();

    event_add(&rd_event, NULL);
}

void client_shutdown() {
    if (client_is_connected())
        client_destroy("shutdown");
#ifdef WIN32
    WSACleanup();
#endif
}

