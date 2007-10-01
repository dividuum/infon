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

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "misc.h"
#include "global.h"
#include "gl_video.h"
#include "map.h"

static SDL_Surface *screen;
static Uint32       flags = SDL_OPENGL | SDL_RESIZABLE;

void video_setup_opengl(int width, int height) {
    float ratio = (float) width / (float) height;

    /* Our shading model--Gouraud (smooth). */
    glShadeModel(GL_SMOOTH);

    /* Culling. */
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    glClearColor(0, 0, 0, 0);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //             fov   aspect minz maxz
    gluPerspective(60.0, ratio, 1.0, TILE_SCALE * 64);

    //static GLfloat ModelAmb[] = { 0.1, 0.1, 0.1, 1.0 };
    //glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ModelAmb);
    //glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0);

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    // FOG
    glEnable(GL_FOG);
    float fogColor[] = {0, 0, 0, 1};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 1.0);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glFogf(GL_FOG_START, TILE_SCALE * 30);
    glFogf(GL_FOG_END, TILE_SCALE * 60);

    GLint depth;
    glGetIntegerv(GL_DEPTH_BITS, &depth);
    printf("Zbuffer depth = %d\n", depth);
}

void video_init(int w, int h, int fs) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
        die("Couldn't initialize SDL: %s", SDL_GetError());

    if (fs) 
        flags |= SDL_FULLSCREEN;

    screen = SDL_SetVideoMode(w, h, 0, flags);

    if (!screen)
        die("Couldn't set display mode: %s", SDL_GetError());

    video_set_title(GAME_NAME);
    SDL_ShowCursor(1);
    SDL_EnableUNICODE(1);

    video_setup_opengl(w, h);
}

void video_set_title(const char *title) {
    SDL_WM_SetCaption(title, "infon");
}

void video_shutdown() {
    // SDL_FreeSurface(screen);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void video_fullscreen_toggle() {
#ifdef WIN32
    flags ^= SDL_FULLSCREEN;
    screen = SDL_SetVideoMode(0, 0, 0, flags);
    if (!screen)
        die("couldn't toggle fullscreen. sorry");
#else
    SDL_WM_ToggleFullScreen(screen);
#endif
    video_setup_opengl(screen->w, screen->h);
}

void video_resize(int w, int h) {
    if (w < 320 || h < 200)
        return;
    screen = SDL_SetVideoMode(w, h, 0, flags);
    if (!screen)
        die("couldn't change resolution. sorry");
    video_setup_opengl(screen->w, screen->h);
}

void video_flip() {
    SDL_GL_SwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int  video_width() {
    return screen->w;
} 

int  video_height() {
    return screen->h;
}

