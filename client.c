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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <event.h>
#include <zlib.h>

#include "packet.h"
#include "global.h"
#include "client.h"
#include "misc.h"
#include "renderer.h"
#include "client_player.h"
#include "client_world.h"
#include "client_creature.h"
#include "client_game.h"

static int              clientfd = INVALID_SOCKET;
static struct event     rd_event;
static struct event     wr_event;
static struct evbuffer *in_buf;
static struct evbuffer *packet_buf;
static struct evbuffer *out_buf;

static int              is_file_source;
static int              next_packet_countdown = 0;
static int              traffic               = 0;

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

    next_packet_countdown += delta;
}

static void client_scroller_from_network(packet_t *packet) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%.*s", packet->len, packet->data); 
    renderer_scroll_message(buf);
}

static void client_start_compression() {
    if (compression)
        PROTOCOL_ERROR();

    if (getenv("UNCOMPRESSED_DEMO_COMPATIBILITY") && is_file_source)
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
            client_player_from_network(packet);
            break;
        case PACKET_WORLD_UPDATE:  
            client_world_from_network(packet);
            break;
        case PACKET_SCROLLER_MSG:   
            client_scroller_from_network(packet);
            break;
        case PACKET_CREATURE_UPDATE: 
            client_creature_from_network(packet);
            break;
        case PACKET_QUIT_MSG:
            if (!is_file_source)
                infomsg("Server wants us to disconnect:\n%.*s",
                        packet->len, packet->data);
            client_destroy("done");
            break;
        case PACKET_KOTH_UPDATE:
            client_player_king_from_network(packet);
            break;
        case PACKET_WORLD_INFO:
            client_world_info_from_network(packet);
            break;
        case PACKET_CREATURE_SMILE:
            client_creature_smile_from_network(packet);
            break;
        case PACKET_GAME_INFO:
            break;
        case PACKET_ROUND:
            client_round_info_from_network(packet);
            break;
        case PACKET_INTERMISSION:
            client_game_intermission_from_network(packet);
            break;
        case PACKET_WELCOME_MSG:    
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

// returns 0 on error
// returns 1 if more data is needed
// returns 2 if more time is needed
static int client_parse_in_buf() {
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
                return 0;
            }

            evbuffer_add(packet_buf, buf, sizeof(buf) - strm.avail_out);
            if (EVBUFFER_LENGTH(packet_buf) > 1024 * 1024) {
                client_destroy("too much to decompress. funny data?");
                return 0;
            }
        } while (strm.avail_out == 0);
        evbuffer_drain(in_buf, EVBUFFER_LENGTH(in_buf) - strm.avail_in);
    } else {
        evbuffer_add_buffer(packet_buf, in_buf);
        evbuffer_drain(in_buf, EVBUFFER_LENGTH(in_buf));
    }

    while (EVBUFFER_LENGTH(packet_buf) >= PACKET_HEADER_SIZE) {
        int packet_len = PACKET_HEADER_SIZE + EVBUFFER_DATA(packet_buf)[0];

        // Nicht genuegend Daten da?
        if (EVBUFFER_LENGTH(packet_buf) < packet_len)
            return 1;

        // Alten Kompressionszustand merken
        int old_compression = compression;

        // Packet rauspopeln...
        static packet_t packet;
        memcpy(&packet, EVBUFFER_DATA(packet_buf), packet_len);
        packet_rewind(&packet);
        client_handle_packet(&packet);

        // Disconnect Packet?
        if (!client_is_connected()) 
            return 0;

        // Weg damit
        evbuffer_drain(packet_buf, packet_len);

        // Wurde Kompression aktiviert? Dann Rest des packet_buf zurueck 
        // in den in_buf, da es sich dabei bereits um komprimierte Daten 
        // handelt und diese oben erst dekomprimiert werden muessen.
        if (compression != old_compression) {
            evbuffer_add_buffer(in_buf, packet_buf);
            evbuffer_drain(packet_buf, EVBUFFER_LENGTH(packet_buf));
            goto restart;
        }

        // Beim Demo abspielen muss erst pausiert werden
        if (is_file_source && next_packet_countdown > 0)
            return 2;
    }
    
    return 1;
}

static void client_readable(int fd, short event, void *arg) {
    int ret = evbuffer_read(in_buf, fd, 8192);
    if (ret < 0) {
        client_destroy(strerror(errno));
    } else if (ret == 0) {
        client_destroy("eof reached");
    } else {
        traffic += ret;
        client_parse_in_buf();
    }
}

static void client_writable(int fd, short event, void *arg) {
    int ret = evbuffer_write(out_buf, fd);
    if (ret < 0) {
        client_destroy(strerror(errno));
    } else if (ret == 0) {
        client_destroy("null write?");
    } else if (EVBUFFER_LENGTH(out_buf) > 0) {
        event_add(&wr_event, NULL);
    }
}

void client_writeto(const void *data, size_t size) {
    if (is_file_source)
        return;
    if (size == 0) 
        return;
    if (EVBUFFER_LENGTH(in_buf) > 1024*1024)
        return;
    evbuffer_add(out_buf, (void*)data, size);
    event_add(&wr_event, NULL);
}

int client_is_file_source() {
    return is_file_source;
}

void client_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    client_writeto(buf, strlen(buf));
}

int  client_is_connected() {
    return clientfd != INVALID_SOCKET;
}

void file_loop(int delta) {
    next_packet_countdown -= delta;
    
    while (next_packet_countdown <= 0) {
        int status = client_parse_in_buf();

        if (status == 0 || // error -> disconnected
            status == 2)   // call again later
            return;
            
        char buf[64];
        int  ret = read(clientfd, buf, sizeof(buf));

        if (ret < 0) {
            client_destroy(strerror(errno));
            return;
        } else if (ret == 0) { 
            client_destroy("eof");
            return;
        }

        evbuffer_add(in_buf, buf, ret);
        traffic += ret;
    }
}

int  client_traffic() {
    return traffic;
}

void client_tick(int delta) {
    if (is_file_source) {
        file_loop(delta);
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
        die("gethostbyname failed: %s", ErrorString(WSAGetLastError()));
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
    if (fd == INVALID_SOCKET)
#ifdef WIN32
        die("cannot open socket: %s", ErrorString(WSAGetLastError()));
#else
        die("cannot open socket: %s", strerror(errno));
#endif

#ifdef WIN32
    if (connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        die("cannot connect socket: %s", ErrorString(WSAGetLastError()));
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

void client_init(char *source) {
    struct stat stat_buf;

    if (!strcmp(source, "-"))
        source = "/dev/stdin";

    if (stat(source, &stat_buf) < 0) {
#ifdef WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) 
            die("WSAStartup failed");
#endif
        clientfd       = client_open_socket(source);
        is_file_source = 0;

        event_init();

        event_set(&rd_event, clientfd, EV_READ | EV_PERSIST,  client_readable, &rd_event);
        event_set(&wr_event, clientfd, EV_WRITE,              client_writable, &wr_event);

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
    assert(clientfd != INVALID_SOCKET);
    fprintf(stderr, "datasource destroyed: %s\n", reason);

    evbuffer_free(in_buf);
    evbuffer_free(out_buf);
    evbuffer_free(packet_buf);
    
    if (compression) 
        inflateEnd(&strm);

#ifdef WIN32
    if (is_file_source) {
        close(clientfd);
    } else {
        closesocket(clientfd);
    }
#else
    close(clientfd);
#endif
    clientfd = INVALID_SOCKET;
    
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

