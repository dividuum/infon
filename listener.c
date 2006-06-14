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
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <event.h>

#include "global.h"
#include "server.h"
#include "listener.h"
#include "server.h"

int          listenfd = -1;
struct event listener_event;

static void listener_cb(int fd, short event, void *arg) {
    struct event *cb_event = arg;

    struct sockaddr_in peer;
    socklen_t addrlen = sizeof(struct sockaddr_in);
        
    int clientfd;
    
    /* Neue Verbindung accept()ieren */
    clientfd = accept(listenfd, (struct sockaddr*)&peer, &addrlen);

    if (clientfd == -1) {
        /* Warning nur anzeigen, falls accept() fehlgeschlagen hat allerdings
           ohne dabei EAGAIN (was bei non-blocking sockets auftreten kann)
           zu melden. */
        if (errno != EAGAIN) 
            fprintf(stderr, "cannot accept() new incoming connection: %s\n", strerror(errno));

        goto error;
    }

    if (!client_accept(clientfd, &peer))
        goto error; 

    event_add(cb_event, NULL);
    return;
error:
    if (clientfd != -1)
        close(clientfd);
    event_add(cb_event, NULL);
}

int listener_init() {
    struct sockaddr_in addr;
    static const int one = 1;

    lua_pushliteral(L, "listenport");  
    lua_rawget(L, LUA_GLOBALSINDEX);
    
    if (!lua_isnumber(L, -1)) {
        fprintf(stdout, "listenport nicht definiert\n");
        lua_pop(L, 1);
        goto error;
    }

    lua_pushliteral(L, "listenaddr");  
    lua_rawget(L, LUA_GLOBALSINDEX);

    if (!lua_isstring(L, -1)) {
        fprintf(stdout, "listaddr nicht definiert\n");
        lua_pop(L, 2);
        goto error;
    }

    /* Adressstruktur füllen */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family        = AF_INET;
    addr.sin_addr.s_addr   = inet_addr(lua_tostring(L, -1));
    addr.sin_port          = htons((int)lua_tonumber(L, -2));

    lua_pop(L, 2); 

    /* Socket erzeugen */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Fehler beim Socket erzeugen? */
    if (listenfd == -1) {
        fprintf(stderr, "cannot open socket: %s\n", strerror(errno));
        goto error;
    }

    /* Auf nonblocking setzen */
    if (fcntl(listenfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "cannot set socket nonblocking: %s\n", strerror(errno));
        goto error;
    }

    /* SO_REUSEADDR verwenden. Falls sich zuvor ein Programm unschön
       beendet hat, so ist der port normalerweise für einen bestimmten
       Zeitrahmen weiterhin belegt. SO_REUSEADDR verwendet dann den
       belegten Port weiter. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        fprintf(stderr, "cannot enable SO_REUSEADDR: %s\n", strerror(errno));
        goto error;
    }

    /* Socket bind()en */
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "cannot bind socket: %s\n", strerror(errno));
        goto error;
    }

    /* Und listen() mit einem backlog von 128 Verbindungen. Wobei
       dies eventuell ignoriert wird, falls SYN cookies aktiviert sind */
    if (listen(listenfd, 128) == -1) {
        fprintf(stderr, "cannot listen() on socket: %s\n", strerror(errno));
        goto error;
    }

    event_set(&listener_event, listenfd, EV_READ, listener_cb, &listener_event);
    event_add(&listener_event, NULL);

    return 1;
error:
    if (listenfd != -1) {
        close(listenfd);
        listenfd = -1;
    }
    fprintf(stderr, "cannot setup listen socket\n");
    return 0;
}

void listener_shutdown() {
    if (listenfd != -1) {
        event_del(&listener_event);
        close(listenfd);
        listenfd = -1;
    }
}

