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
#else
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "packet.h"

void packet_rewind(packet_t *packet) {
    packet->offset = 0;
}

void packet_init(packet_t *packet, int type) {
    packet->type = type;
    packet_rewind(packet);
}

int packet_read08(packet_t *packet, uint8_t *data) {
    if (packet->len - packet->offset < 1) {
        fprintf(stderr, "reading beyond end\n");
        return 0;
    }
    *data = packet->data[packet->offset];
    packet->offset += 1;
    return 1;
}

int packet_read16(packet_t *packet, uint16_t *data) {
    uint8_t byte;
    if (!packet_read08(packet, &byte))     return 0;
    *data = byte & 0x7F;
    if (byte & 0x80) {
        if (!packet_read08(packet, &byte)) return 0;
        *data |= byte << 7;
    }
    return 1;
}

int packet_read32(packet_t *packet, uint32_t *data) {
    if (packet->len - packet->offset < 4) {
        fprintf(stderr, "reading beyond end\n");
        return 0;
    }
    *data = ntohl(*(uint32_t*)&packet->data[packet->offset]);
    packet->offset += 4;
    return 1;
}

int packet_readXX(packet_t *packet, void *data, int len) {
    if (packet->len - packet->offset < len) {
        fprintf(stderr, "reading beyond end\n");
        return 0;
    }
    memcpy(data, &packet->data[packet->offset], len);
    packet->offset += len;
    return 1;
}

int packet_write08(packet_t *packet, uint8_t data) {
    if (sizeof(packet->data) - packet->offset <= 1) {
        fprintf(stderr, "packet too full\n");
        return 0;
    }
    *((uint8_t*)&packet->data[packet->offset]) = data;
    packet->offset += 1;
    return 1;
}

int packet_write16(packet_t *packet, uint16_t data) {
    assert(data <= 0x7FFF);
    if (data <= 0x7F) 
        return packet_write08(packet, data);
    else 
        return packet_write08(packet, (data & 0x7F) | 0x80) &&
               packet_write08(packet,  data >> 7);
}

int packet_write32(packet_t *packet, uint32_t data) {
    if (sizeof(packet->data) - packet->offset <= 4) {
        fprintf(stderr, "packet too full\n");
        return 0;
    }
    *((uint32_t*)&packet->data[packet->offset]) = htonl(data);
    packet->offset += 4;
    return 1;
}

int packet_writeXX(packet_t *packet, const void *data, int len) {
    if (sizeof(packet->data) - packet->offset <= len) {
        fprintf(stderr, "packet too full\n");
        return 0;
    }
    memcpy(&packet->data[packet->offset], data, len);
    packet->offset += len;
    return 1;
}
