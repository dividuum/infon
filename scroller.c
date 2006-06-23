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

#include <SDL.h>
#include <assert.h>

#include "video.h"
#include "scroller.h"
#include "packet.h"

static struct evbuffer *scrollbuffer;

static int    last_time;

static void append(const char *msg) {
    evbuffer_add(scrollbuffer, (char*)msg, strlen(msg));
    evbuffer_add(scrollbuffer, "  -  ", 5);
}

void add_to_scroller(const char* msg) {
    // Zu lange Einzelnachricht? 
    if (strlen(msg) > 255) 
        return;

    // Zuviel insgesamt?
    if (EVBUFFER_LENGTH(scrollbuffer) > 10000)
        return;

    append(msg);
    
    // Network Sync
    packet_t packet;
    packet_reset(&packet);
    packet_writeXX(&packet, msg, strlen(msg));
    packet_send(PACKET_SCROLLER_MSG, &packet, PACKET_BROADCAST);
}

#ifdef SERVER_GUI
static float  x = 0;
static float  curspeed = 0;

void scroller_draw() {
    int chars_per_screen = video_width() / 6 + 1;

    Uint32 now = SDL_GetTicks();

    float speed = ((float)now- last_time) * 
                   (float)(EVBUFFER_LENGTH(scrollbuffer) - chars_per_screen - 3) / 200.0;

    if (curspeed < speed) curspeed += 0.1; //(speed - curspeed)/30.0;
    if (curspeed > speed) curspeed = speed;
    if (curspeed < 0.1)   curspeed = 0;

    x        -= curspeed;
    last_time = now;

    while (x < -6) {
        evbuffer_drain(scrollbuffer, 1);
        x += 6;
    }

    while (EVBUFFER_LENGTH(scrollbuffer) <= chars_per_screen + 1) 
        evbuffer_add(scrollbuffer, " ", 1);

    video_rect(0, video_height() - 15, video_width(), video_height(), 0, 0, 0, 0);

    assert(chars_per_screen < EVBUFFER_LENGTH(scrollbuffer));
    char *end = &EVBUFFER_DATA(scrollbuffer)[chars_per_screen];
    char saved = *end; *end = '\0';
    video_write(x, video_height() - 15, EVBUFFER_DATA(scrollbuffer));
    *end = saved;
    
    // Bitte drinlassen. Danke
    if (now / 1000 % 60 < 5) 
        video_draw(video_width() - 190, 20, sprite_get(SPRITE_LOGO));
}
#else
void scroller_draw() {
    evbuffer_drain(scrollbuffer, EVBUFFER_LENGTH(scrollbuffer));
}
#endif

void scroller_from_network(packet_t *packet) {
    char buf[257];
    memcpy(buf, &packet->data, packet->len);
    buf[packet->len] = '\0';
    append(buf);
}

void scroller_init() {
    scrollbuffer = evbuffer_new();
    last_time = SDL_GetTicks();
}

void scroller_shutdown() {
    evbuffer_free(scrollbuffer);
}
