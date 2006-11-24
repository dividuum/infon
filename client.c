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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <SDL.h>
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
#include "gui_game.h"

static int              clientfd;
static struct event     rd_event;
static struct event     wr_event;
static struct evbuffer *in_buf;
static struct evbuffer *packet_buf;
static struct evbuffer *out_buf;

static int              is_file_source;
static Uint32           next_demo_read = 0;
static int              traffic = 0;

// static int           demo_write_fd;

static int              compression;
static z_stream         strm;

void client_destroy(const char *reason);
void client_writeto(const void *data, size_t size);

static void client_handshake_from_network(packet_t *packet) {
    uint8_t serverprotocol;

    if (!packet_read08(packet, &serverprotocol)) 
        PROTOCOL_ERROR();

    if (serverprotocol != PROTOCOL_VERSION) {
        die("%s has %s protocol version %d. I have %d.\n"
            "visit the infon homepage for more information.",
               is_file_source ? "demo" : "server",
               serverprotocol < PROTOCOL_VERSION ? "older" : "newer",
               serverprotocol,  PROTOCOL_VERSION);
    }
}

static void client_round_info_from_network(packet_t *packet) {
    uint8_t delta;

    if (!packet_read08(packet, &delta)) 
        PROTOCOL_ERROR();

    next_demo_read = SDL_GetTicks() + delta;
}

static void client_start_compression() {
    if (compression)
        PROTOCOL_ERROR();

    // Bei Demofiles Kompressionstart ignorieren,
    // da diese unkomprimiert abgelegt werden.
    if (is_file_source)
        return;

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
            fprintf(stderr, "server wants us to disconnect: %.*s\n",
                    packet->len, packet->data);
            client_destroy("done");
            break;
        case PACKET_KOTH_UPDATE:
            gui_player_king_from_network(packet);
            break;
        case PACKET_WORLD_INFO:
            gui_world_info_from_network(packet);
            break;
        case PACKET_CREATURE_SMILE:
            gui_creature_smile_from_network(packet);
            break;
        case PACKET_GAME_INFO:
            break;
        case PACKET_ROUND:
            client_round_info_from_network(packet);
            break;
        case PACKET_INTERMISSION:
            gui_game_intermission_from_network(packet);
            break;
        case PACKET_WELCOME_MSG:    
            if (!is_file_source)
                client_writeto("guiclient\n", 10);
            break;
        case PACKET_START_COMPRESS:
            client_start_compression();
            break;
        case PACKET_HANDSHAKE:
            client_handshake_from_network(packet);
            break;
        default:
            fprintf(stderr, "packet->type %d unknown\n", packet->type);
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
    
    traffic += ret;

restart:
    if (compression) {
        char buf[8192];
        strm.next_in   = EVBUFFER_DATA(in_buf);
        strm.avail_in  = EVBUFFER_LENGTH(in_buf);
        do {
            strm.next_out  = (unsigned char*)buf;
            strm.avail_out = sizeof(buf);

            int ret = inflate(&strm, Z_SYNC_FLUSH);
            if (ret != Z_OK && ret != Z_BUF_ERROR) {
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

    while (EVBUFFER_LENGTH(packet_buf) >= PACKET_HEADER_SIZE) {
        int packet_len = PACKET_HEADER_SIZE + EVBUFFER_DATA(packet_buf)[0];

        // Genuegend Daten da?
        if (EVBUFFER_LENGTH(packet_buf) < packet_len)
            break;

        // Alten Kompressionszustand merken
        int old_compression = compression;

        // Packet rauspopeln...
        static packet_t packet;
        memcpy(&packet, EVBUFFER_DATA(packet_buf), packet_len);
        packet_rewind(&packet);
        client_handle_packet(&packet);
        evbuffer_drain(packet_buf, packet_len);

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

int  client_is_connected() {
    return clientfd != -1;
}

void file_loop() {
    static packet_t packet;
    static char errbuf[128];
    int ret;
    while (SDL_GetTicks() >= next_demo_read) {
        ret = read(clientfd, &packet, PACKET_HEADER_SIZE);
        if (ret != PACKET_HEADER_SIZE) {
            snprintf(errbuf, sizeof(errbuf), "eof, reading header: want %d, got %d", PACKET_HEADER_SIZE, ret);
            client_destroy(errbuf);
            return;
        }

        traffic += ret;
        
        ret = read(clientfd, &packet.data, packet.len);
        if (ret != packet.len) {
            snprintf(errbuf, sizeof(errbuf), "eof, reading packet: want %d, got %d", packet.len, ret);
            client_destroy(errbuf);
            return;
        }

        traffic += ret;
        
        packet_rewind(&packet);
        client_handle_packet(&packet);

        if (!client_is_connected()) 
            return;
    }
}

int  client_traffic() {
    return traffic;
}

void client_tick() {
    if (is_file_source) {
        file_loop();
    } else {
        event_loop(EVLOOP_NONBLOCK);
    }
}

int client_open_socket(char *addr) {
    int    port = 1234;                                             
    struct hostent *host;

    char *colon = strchr(addr, ':');
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
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fehler beim Socket erzeugen? */
#ifdef WIN32
    if (fd == INVALID_SOCKET)
        die("cannot open socket: Error %d", WSAGetLastError());
#else
    if (fd == -1) 
        die("cannot open socket: %s", strerror(errno));
#endif

#ifdef WIN32
    if (connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        die("cannot connect socket: Error %d", WSAGetLastError());
#else
    if (connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        die("cannot connect socket: %s", strerror(errno));
#endif

    fprintf(stderr, "connected!\n");

    /* Non Blocking setzen */
#ifdef WIN32
    DWORD notblock = 1;
    ioctlsocket(fd, FIONBIO, &notblock);
#else
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) 
        die("cannot set socket nonblocking: %s", strerror(errno));
#endif

    return fd;
}

int client_open_file(char *filename) {
#ifdef WIN32
    int fd = open(filename, O_RDONLY | O_BINARY);
#else
    int fd = open(filename, O_RDONLY);
#endif
    if (fd < 0)
        die("cannot open file %s: %s", filename, strerror(errno));
    return fd;
}

void client_init(char *source, char *demo_save_name) {
    struct stat stat_buf;
    if (stat(source, &stat_buf) < 0) {
#ifdef WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) 
            die("WSAStartup failed");
#endif
        clientfd       = client_open_socket(source);
        is_file_source = 0;

        event_init();

        event_set(&rd_event, clientfd, EV_READ,  client_readable, &rd_event);
        event_set(&wr_event, clientfd, EV_WRITE, client_writable, &wr_event);

        event_add(&rd_event, NULL);
    } else {
        clientfd       = client_open_file(source);
        is_file_source = 1;
    }

    in_buf      = evbuffer_new();
    out_buf     = evbuffer_new();
    packet_buf  = evbuffer_new();
}

void client_destroy(const char *reason) {
    assert(clientfd != -1);
    fprintf(stderr, "datasource destroyed: %s\n", reason);

    evbuffer_free(in_buf);
    evbuffer_free(out_buf);
    evbuffer_free(packet_buf);
    
    if (compression) 
        inflateEnd(&strm);

    close(clientfd);
    clientfd = -1;
    
    if (!is_file_source) {
        event_del(&rd_event);
        event_del(&wr_event);
#ifdef WIN32
        WSACleanup();
#endif
    }
}


void client_shutdown() {
    if (client_is_connected())
        client_destroy("shutdown");
}

