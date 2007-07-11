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
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <string.h>
#include <unistd.h>

#include "packet.h"

void ping_master(const char *master_addr, int port, const char *servername, 
                 int clients, int players, int creatures) 
{
    struct sockaddr_in master;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    size_t namelen = strlen(servername);
    if (namelen > 32) namelen = 32;

    memset(&master, 0, sizeof(struct sockaddr_in));
    master.sin_family        = AF_INET;
    master.sin_addr.s_addr   = inet_addr(master_addr);
    master.sin_port          = htons(port);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) 
        return;

    packet_t packet;
    packet_init(&packet, PACKET_MASTER_PING);
    packet_write08(&packet, clients);
    packet_write08(&packet, players);
    packet_write08(&packet, creatures);
    packet_writeXX(&packet, servername, namelen);
    packet.len  = packet.offset;
    sendto(sock, &packet, PACKET_HEADER_SIZE + packet.len, 0,
           (struct sockaddr *)&master, addrlen);
    close(sock);
}
