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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <event.h>

#include "misc.h"
#include "infond.h"
#include "global.h"
#include "server.h"
#include "listener.h"
#include "server.h"

static int listenfd = INVALID_SOCKET;
static struct event listener_event;

static void listener_cb(int fd, short event, void *arg) {
    struct sockaddr_in peer;
    socklen_t addrlen = sizeof(struct sockaddr_in);
        
    int clientfd;
    
    /* Neue Verbindung accept()ieren */
    clientfd = accept(listenfd, (struct sockaddr*)&peer, &addrlen);

    if (clientfd == INVALID_SOCKET) {
        /* Warning nur anzeigen, falls accept() fehlgeschlagen hat allerdings
           ohne dabei EAGAIN (was bei non-blocking sockets auftreten kann)
           zu melden. */
        if (errno != EAGAIN) 
            fprintf(stderr, "cannot accept() new incoming connection: %s\n", strerror(errno));

        goto error;
    }

#ifndef WIN32
    /* TCP_NODELAY setzen. Dadurch werden Daten fruehestmoeglich versendet */
    const int one = 1;
    if (setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0) {
        fprintf(stderr, "cannot enable TCP_NODELAY: %s\n", strerror(errno));
        goto error;
    }
#endif

    /* SO_LINGER setzen. Falls sich noch Daten in der Sendqueue der Verbindung
       befinden, werden diese verworfen. */
    const struct linger l = { 1, 0 };
#ifdef WIN32
    if (setsockopt(clientfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == SOCKET_ERROR) {
        fprintf(stderr, "cannot set SO_LINGER: %s\n", ErrorString(WSAGetLastError()));
        goto error;
    }
#else
    if (setsockopt(clientfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1) {
        fprintf(stderr, "cannot set SO_LINGER: %s\n", strerror(errno));
        goto error;
    }
#endif

    char address[128];
    sprintf(address, "ip4:%s:%d", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

    if (!server_accept(clientfd, address))
        goto error; 

    return;
error:
    if (clientfd != INVALID_SOCKET) 
#ifdef WIN32
        closesocket(clientfd);
#else
        close(clientfd);
#endif
}

void listener_shutdown() {
    if (listenfd == INVALID_SOCKET) 
        return;

    event_del(&listener_event);
#ifdef WIN32
    closesocket(listenfd);
#else
    close(listenfd);
#endif
    listenfd = INVALID_SOCKET;
}

int listener_init(const char *listenaddr, int port) {
    struct sockaddr_in addr;
    const int one = 1;

    /* Alten Listener, falls vorhanden, schliessen */
    listener_shutdown();

    /* Keinen neuen starten? */
    if (strlen(listenaddr) == 0)
        return 1;

    /* Adressstruktur füllen */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family        = AF_INET;
    addr.sin_addr.s_addr   = inet_addr(listenaddr);
    addr.sin_port          = htons(port);

    /* Socket erzeugen */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fehler beim Socket erzeugen? */
    if (listenfd == INVALID_SOCKET) {
        fprintf(stderr, "cannot open socket: %s\n", strerror(errno));
        goto error;
    }

    /* Auf nonblocking setzen */
#ifdef WIN32
    DWORD notblock = 1;
    ioctlsocket(listenfd, FIONBIO, &notblock);
#else
    if (fcntl(listenfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "cannot set socket nonblocking: %s\n", strerror(errno));
        goto error;
    }
#endif

#ifndef WIN32
    /* SO_REUSEADDR verwenden. Falls sich zuvor ein Programm unschön
       beendet hat, so ist der port normalerweise für einen bestimmten
       Zeitrahmen weiterhin belegt. SO_REUSEADDR verwendet dann den
       belegten Port weiter. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        fprintf(stderr, "cannot enable SO_REUSEADDR: %s\n", strerror(errno));
        goto error;
    }
#endif

    /* Socket bind()en */
#ifdef WIN32
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
        fprintf(stderr, "cannot bind socket: %s\n", ErrorString(WSAGetLastError()));
        goto error;
    }
#else
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "cannot bind socket: %s\n", strerror(errno));
        goto error;
    }
#endif

    /* Und listen() mit einem backlog von 128 Verbindungen. Wobei
       dies eventuell ignoriert wird, falls SYN cookies aktiviert sind */
#ifdef WIN32
    if (listen(listenfd, 128) == SOCKET_ERROR) {
        fprintf(stderr, "cannot listen() on socket: %s\n", ErrorString(WSAGetLastError()));
        goto error;
    }
#else
    if (listen(listenfd, 128) == -1) {
        fprintf(stderr, "cannot listen() on socket: %s\n", strerror(errno));
        goto error;
    }
#endif

    event_set(&listener_event, listenfd, EV_READ | EV_PERSIST, listener_cb, &listener_event);
    event_add(&listener_event, NULL);

    return 1;
error:
    if (listenfd != INVALID_SOCKET) {
#ifdef WIN32
        closesocket(listenfd);
#else
        close(listenfd);
#endif
        listenfd = INVALID_SOCKET;
    }
    return 0;
}
