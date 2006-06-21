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

#ifndef PACKET_H
#define PACKET_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <event.h>

/*

typedef struct {
    uint8_t type;
    uint8_t playerno;
    uint8_t join_or_leave;
} __attribute__((packed)) packet_player_join_leave_t;

typedef struct {
    uint8_t  type;
    uint8_t  playerno;
    uint8_t  updatemask; 

#define      PLAYER_UPDATE_ALL        0x0F
    uint8_t  data[256];
} __attribute__((packed)) packet_player_update_static_t;

typedef struct {
    uint8_t  type;
    uint8_t  playerno;
    uint8_t  cpu_usage;
    uint16_t score;
} __attribute__((packed)) packet_player_update_round_t;

typedef struct {
    uint8_t type;
    uint8_t king_player;
} __attribute__((packed)) packet_player_update_king_t;

typedef struct {
    uint8_t type;
    uint8_t x;
    uint8_t y;
    uint8_t sprite;
} __attribute__((packed)) packet_world_sprite_t;

typedef struct {
    uint8_t type;
    uint8_t x;
    uint8_t y;
    uint16_t food;
} __attribute__((packed)) packet_world_food_t;

typedef struct {
    uint8_t type;
	uint8_t msglen;
	uint8_t msg[257];
} __attribute__((packed)) packet_scroller_msg_t;

typedef struct {
    uint8_t type;
    uint16_t creatureno;
    uint8_t spawn_or_die;
} __attribute__((packed)) packet_creature_spawndie_t;

typedef struct {
    uint8_t  type;
    uint16_t creatureno;
    uint8_t  msg[8];
    uint8_t  creaturetype;
    uint8_t  player;
    uint16_t  target;
} __attribute__((packed)) packet_creature_update_static_t;

*/

typedef struct {
// Wire Data    
    uint8_t  len;
    uint8_t  type;
#define PACKET_PLAYER_UPDATE     0
#define PACKET_WORLD_UPDATE      1
#define PACKET_SCROLLER_MSG      2
    uint8_t  data[256];
// Mgmt Data    
    uint8_t  offset;
} packet_t __attribute__((packed));

void packet_reset(packet_t *packet);
void packet_send(int type, packet_t *packet);

int   packet_read08(packet_t *packet, uint8_t  *data);
int   packet_read16(packet_t *packet, uint16_t *data);
int   packet_read32(packet_t *packet, uint32_t *data);
int   packet_readXX(packet_t *packet, void *data, int len);
int  packet_write08(packet_t *packet, uint8_t  data);
int  packet_write16(packet_t *packet, uint16_t data);
int  packet_write32(packet_t *packet, uint32_t data);
int  packet_writeXX(packet_t *packet, const void *data, int len);

void packet_handle(struct evbuffer *packet_buffer);

#endif
