#include <stdio.h>

#include "packet.h"
#include "player.h"
#include "world.h"
#include "scroller.h"
#include "creature.h"

void packet_reset(packet_t *packet) {
    packet->offset = 0;
}

void packet_send(int type, packet_t *packet) {
    packet->len  = packet->offset;
    packet->type = type;
    client_writeto_all_gui_clients(packet, 1 + 1 + packet->len);
}

int packet_read08(packet_t *packet, uint8_t *data) {
    if (packet->len - packet->offset < 1) {
        printf("reading beyond end\n");
        return 0;
    }
    *data = packet->data[packet->offset];
    packet->offset += 1;
    return 1;
}

int  packet_read16(packet_t *packet, uint16_t *data) {
    if (packet->len - packet->offset < 2) {
        printf("reading beyond end\n");
        return 0;
    }
    *data = ntohs(*(uint16_t*)&packet->data[packet->offset]);
    packet->offset += 2;
    return 1;
}

int  packet_read32(packet_t *packet, uint32_t *data) {
    if (packet->len - packet->offset < 4) {
        printf("reading beyond end\n");
        return 0;
    }
    *data = ntohl(*(uint32_t*)&packet->data[packet->offset]);
    packet->offset += 4;
    return 1;
}

int  packet_readXX(packet_t *packet, void *data, int len) {
    if (packet->len - packet->offset < len) {
        printf("reading beyond end\n");
        return 0;
    }
    memcpy(data, &packet->data[packet->offset], len);
    packet->offset += len;
    return 1;
}

int packet_write08(packet_t *packet, uint8_t data) {
    if (sizeof(packet->data) - packet->offset <= 1) {
        printf("packet too full\n");
        return 0;
    }
    *((uint8_t*)&packet->data[packet->offset]) = data;
    packet->offset += 1;
    return 1;
}

int packet_write16(packet_t *packet, uint16_t data) {
    if (sizeof(packet->data) - packet->offset <= 2) {
        printf("packet too full\n");
        return 0;
    }
    *((uint16_t*)&packet->data[packet->offset]) = htons(data);
    packet->offset += 2;
    return 1;
}

int packet_write32(packet_t *packet, uint32_t data) {
    if (sizeof(packet->data) - packet->offset <= 4) {
        printf("packet too full\n");
        return 0;
    }
    *((uint16_t*)&packet->data[packet->offset]) = htonl(data);
    packet->offset += 4;
    return 1;
}

int packet_writeXX(packet_t *packet, const void *data, int len) {
    if (sizeof(packet->data) - packet->offset <= len) {
        printf("packet too full\n");
        return 0;
    }
    memcpy(&packet->data[packet->offset], data, len);
    packet->offset += len;
    return 1;
}


void packet_handle(struct evbuffer *packet_buffer) {
    /*
    while (EVBUFFER_LENGTH(packet_buffer) >= 1) {
        switch (EVBUFFER_DATA(packet_buffer)[0]) {
            case PACKET_PLAYER_STATIC:    handle_player_update_static);
            case PACKET_PLAYER_ROUND:     handle_player_update_round);
            case PACKET_PLAYER_JOINLEAVE: handle_player_join_leave);
            case PACKET_PLAYER_KING:      handle_player_update_king);
            case PACKET_WORLD_SPRITE:     handle_world_sprite);
            case PACKET_WORLD_FOOD:       handle_world_food);
            case PACKET_SCROLLER_MSG:     handle_scroller_msg);
            case PACKET_CREATURE_SPAWNDIE:handle_creature_spawndie);
        }
    }
    */
}
