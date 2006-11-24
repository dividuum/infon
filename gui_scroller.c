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

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#endif

#include <event.h>

#include "global.h"
#include "video.h"
#include "scroller.h"
#include "packet.h"

static struct evbuffer *scrollbuffer;
static int    last_time;
static float  x        = 0;
static float  curspeed = 0;

static void append(const char *msg) {
    // Zuviel insgesamt?
    if (EVBUFFER_LENGTH(scrollbuffer) > 10000)
        return;

    evbuffer_add(scrollbuffer, (char*)msg, strlen(msg));
    evbuffer_add(scrollbuffer, "  -  ", 5);
}

void gui_scroller_draw() {
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

    video_rect(0, video_height() - 16, video_width(), video_height(), 0, 0, 0, 0);

    assert(chars_per_screen < EVBUFFER_LENGTH(scrollbuffer));
    unsigned char *end = &EVBUFFER_DATA(scrollbuffer)[chars_per_screen];
    char saved = *end; *end = '\0';
    video_write(x, video_height() - 15, (char*)EVBUFFER_DATA(scrollbuffer));
    *end = saved;
}

void gui_scroller_from_network(packet_t *packet) {
    char buf[257];
    memcpy(buf, &packet->data, packet->len);
    buf[packet->len] = '\0';
    append(buf);
}

void gui_scroller_init() {
    last_time = SDL_GetTicks();

    scrollbuffer = evbuffer_new();
    for (int i = 0; i < 20; i++)
        evbuffer_add(scrollbuffer, "          ", 10);

    append(GAME_NAME);
}

void gui_scroller_shutdown() {
    evbuffer_free(scrollbuffer);
}
