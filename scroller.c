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

#include <assert.h>
#include <string.h>

#include "scroller.h"
#include "packet.h"
#include "server.h"

void add_to_scroller(const char* msg) {
    // Network Sync
    packet_t packet;
    packet_init(&packet, PACKET_SCROLLER_MSG);
    packet_writeXX(&packet, msg, strlen(msg));
    server_send_packet(&packet, SEND_BROADCAST);
}
