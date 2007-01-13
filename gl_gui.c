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

#include <GL/glu.h>
#include <SDL.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <event.h>
#include <assert.h>
#include <math.h>

#include "renderer.h"
#include "map.h"
#include "misc.h"

#include "gl_video.h"
#include "gl_mdl.h"

static Uint32 real_time;

static const float colors[16][3] = {
    {  1.0,  0.0,  0.0},
    {  0.0,  1.0,  0.0},
    {  0.0,  0.0,  1.0},
    {  1.0,  1.0,  0.0},
    {  0.0,  1.0,  1.0},
    {  1.0,  0.0,  1.0},
    {  1.0,  1.0,  1.0},
    {  0.0,  0.0,  0.0},

    {  1.0,  0.5,  0.5},
    {  0.5,  1.0,  0.5},
    {  0.5,  0.5,  1.0},
    {  1.0,  1.0,  0.5},
    {  0.5,  1.0,  1.0},
    {  1.0,  0.5,  1.0},
    {  0.5,  0.5,  0.5},
    {  0.2,  0.8,  1.0},
};


typedef struct {
    float x;
    float y;
    float z;
} Vec;

static float direction;
static Vec   lookat;
static Vec   eye;

static Vec   lookat_dest;
static Vec   eye_dest;

static float vec_interpolate(float val, float dest, float divisor) {
    if (val - dest > -1 && val - dest < 1)
        return dest;
    return val + (dest - val) / divisor;
}

void vec_moveto(Vec *vec, Vec *dest, float ox, float oy, float oz) {
    vec->x = vec_interpolate(vec->x, dest->x + ox, 30);
    vec->y = vec_interpolate(vec->y, dest->y + oy, 30);
    vec->z = vec_interpolate(vec->z, dest->z + oz, 30);
}

#define PIdiv180 M_PI/180.0

static void cam_update_eye_dest() {
    if (eye_dest.z == 100) {
        eye_dest.x = lookat_dest.x + sin(direction * PIdiv180) * 200;
        eye_dest.y = lookat_dest.y - cos(direction * PIdiv180) * 200;
    } else if (eye_dest.z > 6000) {
        eye_dest.x = lookat_dest.x + sin(direction * PIdiv180) * 1500;
        eye_dest.y = lookat_dest.y - cos(direction * PIdiv180) * 1500;
    } else {
        eye_dest.x = lookat_dest.x + sin(direction * PIdiv180) * (eye_dest.z+ 200);
        eye_dest.y = lookat_dest.y - cos(direction * PIdiv180) * (eye_dest.z + 200);
    }
}

static void cam_tick(int msec) {
    cam_update_eye_dest();

    vec_moveto(&eye,    &eye_dest,    0, 0, 0);
    vec_moveto(&lookat, &lookat_dest, 0, 0, 0);

    gluLookAt(eye.x, eye.y, eye.z, lookat.x, lookat.y, lookat.z, 0.0, 0.0, -1.0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT2);

    static GLfloat l1amb[] = { 0, 0, 0, 1.0 };
    glLightfv(GL_LIGHT2, GL_AMBIENT, l1amb);
    GLfloat l1diffuse[] = {1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT2, GL_DIFFUSE, l1diffuse);
    GLfloat l1specular [] = {1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT2, GL_SPECULAR, l1specular);

    GLfloat l1pos[] = { eye.x, eye.y, eye.z, 1.0 };
    glLightfv(GL_LIGHT2, GL_POSITION, l1pos);

    GLfloat l1dir[] = { lookat.x - eye.x, lookat.y - eye.y, lookat.z - eye.z };
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, l1dir);

    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 0.1);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 120.0);

    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
}

/*
static void cam_screen_to_plane(int x, int y, float *zx, float *zy, int *inFront) {
    double modelMatrix[16], projMatrix[16];
    int    viewport[4];
    double xNear, yNear, zNear, xFar, yFar, zFar;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    gluUnProject(x, viewport[3] - y, 0.0f, modelMatrix, projMatrix, viewport, &xNear, &yNear, &zNear);
    gluUnProject(x, viewport[3] - y, 1.0f, modelMatrix, projMatrix, viewport, &xFar,  &yFar,  &zFar);

    float dx = (float)(xNear - xFar);
    float dy = (float)(yNear - yFar);
    float dz = (float)(zNear - zFar);

    *zx = eye.x - dx * (eye.z / dz);
    *zy = eye.y - dy * (eye.z / dz);

    *inFront = dz > 0;
}

static float cam_direction() {
    float w = 180 * atan2(eye.x - lookat.x, lookat.y - eye.y) / M_PI;
    if (w < 0) w += 360;
    return w;
}
*/

static void cam_set_height(float height) {
    eye_dest.z = height;
    if (eye_dest.z < 100)
        eye_dest.z = 100;

    if (eye_dest.z == 100) {
        lookat_dest.z = 100;
    } else {
        lookat_dest.z = 0;
    }
}

static void cam_change_height(float dz) {
    cam_set_height(eye_dest.z + dz);
}

static void cam_set_direction(float dir) {
    direction = dir;
}

static void cam_change_direction(float delta) {
    assert(delta > -360 && delta < 360);
    direction += delta;
    if (direction >= 360)
        direction -= 360;
    if (direction < 0)
        direction += 360;
    assert(direction >= 0 && direction < 360);
}

static void cam_set_pos(float x, float y) {
    lookat_dest.x = x;
    lookat_dest.y = y;
}


static void cam_change_pos(float dx, float dy) {
    cam_set_pos(lookat_dest.x + dx, lookat_dest.y + dy);
}

static void cam_change_pos_rel(float dx, float dy) {
    cam_change_pos(dy * sin(direction * PIdiv180) -dx * cos(direction * PIdiv180),
                  -dy * cos(direction * PIdiv180) -dx * sin(direction * PIdiv180));
}

static void cam_recenter() {
    const client_world_info_t *info = infon->get_world_info();
    if (!info) return;
    cam_set_pos(info->width  * 256 / 2, info->height * 256 / 2);
    cam_set_height(7000);
    cam_set_direction(180);
    cam_update_eye_dest();
}

static void cam_init() {
    eye.x = 3000;
    eye.y = 3000;
    eye.z = 3000;
    cam_set_pos(0, 0);
    cam_set_height(3000);
    cam_set_direction(90);

    cam_update_eye_dest();
}

static void cam_free() {
}

/* Particle */

typedef struct {
    int used;
    GLfloat orig[3];
    GLfloat pos[3];
    GLfloat time;
    GLfloat vel[2];
    GLfloat dir[2];
    GLfloat color[4];
    int disappear;
} Particle;

#define MAXPARTICLE 2000

Particle particles[MAXPARTICLE] = {{0}};
static int next = 0;

static void particle_add_fountain(int x, int y, int z) {
    float angle = (float)(rand() % 30 + 30) * M_PI / 180.0; 
    float dir   = (float)(rand() % 360) * M_PI / 180.0; 
    particles[next].orig[0] = x;
    particles[next].orig[1] = y;
    particles[next].orig[2] = z;
    particles[next].color[0] = 1.0;
    particles[next].color[1] = 1;
    particles[next].color[2] = 1;
    particles[next].color[3] = 1;
    particles[next].time = real_time;
    particles[next].dir[0] = cos(dir);
    particles[next].dir[1] = sin(dir);
    particles[next].vel[0] = cos(angle) * 10;
    particles[next].vel[1] = sin(angle) * 100;
    particles[next].used  = 1;
    particles[next].disappear = 0;
    if (++next >= MAXPARTICLE) next = 0;
}

static void particle_add_blood(int x, int y, int z) {
    float angle = (float)(rand() % 30 + 30) * M_PI / 180.0; 
    float dir   = (float)(rand() % 360) * M_PI / 180.0; 
    particles[next].orig[0] = x;
    particles[next].orig[1] = y;
    particles[next].orig[2] = z;
    particles[next].color[0] = 1.0;
    particles[next].color[1] = 1;
    particles[next].color[2] = 1;
    particles[next].color[3] = 1;
    particles[next].time = real_time;
    particles[next].dir[0] = cos(dir);
    particles[next].dir[1] = sin(dir);
    particles[next].vel[0] = cos(angle) * (rand() % 40);
    particles[next].vel[1] = sin(angle) * 40;
    particles[next].used  = 1;
    particles[next].disappear = 1;
    if (++next >= MAXPARTICLE) next = 0;
}

static void particle_move(int delta) {
    for (int i = 0; i < MAXPARTICLE; i++) {
        if (!particles[i].used) continue;
        float lifetime = (real_time - particles[i].time) / 100.0;
        float distance = particles[i].vel[0] * lifetime;

        /* X and Z */
        particles[i].pos[0] = particles[i].orig[0] + particles[i].dir[0] * distance;
        particles[i].pos[1] = particles[i].orig[1] + particles[i].dir[1] * distance;

        /* Z */
        particles[i].pos[2] = particles[i].orig[2] + (particles[i].vel[1]  - 9.81 * lifetime) * lifetime;
        
        if (particles[i].pos[2] < 5) switch (particles[i].disappear) {
            case 0:
                particles[i].vel[1] *= 0.6;
                if (particles[i].vel[1] < 0.5) {
                    particles[i].used = 0;
                } else {
                    particles[i].pos[2]  = 5;
                    particles[i].orig[0] = particles[i].pos[0];
                    particles[i].orig[1] = particles[i].pos[1];
                    particles[i].orig[2] = particles[i].pos[2];
                    particles[i].time    = real_time;
                }
                break;
            case 1:
                particles[i].pos[2]  = 5;
                particles[i].orig[0] = particles[i].pos[0];
                particles[i].orig[1] = particles[i].pos[1];
                particles[i].orig[2] = particles[i].pos[2];
                particles[i].vel[0] = particles[i].vel[1] = 0;
                break;
        }
    }
}

static void particle_draw() {
    glDisable(GL_LIGHTING);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < MAXPARTICLE; i++) {
        if (!particles[i].used) continue;
        glColor3fv(particles[i].color);
        glVertex3f(particles[i].pos[0] + 14, particles[i].pos[1] + 14, particles[i].pos[2]); 
        glVertex3fv(particles[i].pos); 
        glVertex3f(particles[i].pos[0], particles[i].pos[1] + 14, particles[i].pos[2]);
    }
    glEnd();
}

/* Texture */

typedef struct {
    GLuint  textureID; 
    GLsizei height;
    GLsizei width;  
    int     dimension;
} Texture;

static void texture_load(Texture *texture, const char *filename, Uint32 key) {
    int has_transparent_key = key != 0xFFFFFFFF;

    fprintf(stderr, "loading %s\n", filename);

    SDL_Surface *imageSurface = SDL_LoadBMP(filename);
    if (!imageSurface) 
        die("cannot load %s: %s", filename, SDL_GetError());

    /* Generate an OpenGL texture */
    glGenTextures(1, &texture->textureID);

    /* Set up some parameters for the format of the OpenGL texture */
    glBindTexture(GL_TEXTURE_2D, texture->textureID ); /* Bind Our Texture */
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* Linear Filtered */
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* Linear Filtered */

    texture->height = imageSurface->h;
    texture->width  = imageSurface->w;

    texture->dimension = 1;
    while (texture->dimension < texture->width && texture->dimension < texture->height)
        texture->dimension *= 2;

#define _LITTLE_ENDIAN
    SDL_Surface *tempSurface;
    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
    if (has_transparent_key) {
#ifdef _LITTLE_ENDIAN
        tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->dimension, texture->dimension, 32,
                                           0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);
#else
        tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->dimension, texture->dimension, 32,
                                           0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000);
#endif
    } else {
#ifdef _LITTLE_ENDIAN
        tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->dimension, texture->dimension, 24,
                                           0x0000FF, 0x00FF00, 0xFF0000, 0x000000);
#else
        tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->dimension, texture->dimension, 24,
                                           0xFF0000, 0x00FF00, 0x0000FF, 0x000000);
#endif
    }

    SDL_BlitSurface(imageSurface, NULL, tempSurface, NULL);

    /* Fix the alpha values? */
    if (has_transparent_key) {
        SDL_LockSurface(tempSurface);
        Uint32 *pixels = (Uint32*)tempSurface->pixels;
        //printf("%08x %08x\n", *pixels, key);
        for (int y = 0; y < texture->dimension; y++) {
            for (int x = 0; x < texture->dimension; x++) {
                if (pixels[y * texture->dimension + x] != key) {
#ifdef _LITTLE_ENDIAN
                    pixels[y * texture->dimension + x] |= 0xFF000000;
#else
                    pixels[y * texture->dimension + x] |= 0x000000FF;
#endif
                }
            }
        }
        SDL_UnlockSurface(tempSurface);
    }

    /* actually create the OpenGL textures */
    glTexImage2D(GL_TEXTURE_2D, 0,
                 has_transparent_key ? GL_RGBA : GL_RGB,
                 texture->dimension, texture->dimension,
                 0,
                 has_transparent_key ? GL_RGBA : GL_RGB,
                 GL_UNSIGNED_BYTE,
                 tempSurface->pixels);

    SDL_FreeSurface(tempSurface);
    SDL_FreeSurface(imageSurface);
}

static void texture_activate(Texture *texture) {
    glBindTexture(GL_TEXTURE_2D, texture->textureID);
}

static void texture_free(Texture *texture) {
    glDeleteTextures(1, &texture->textureID);
}

/* WALL */

#define WALL_HEIGHT 30
static GLint   wall_top;
static Texture wall_top_texture;
static GLint   wall_side;
static Texture wall_side_texture;

static void wall_init() {
    texture_load(&wall_top_texture,  "gfx/solid.bmp", 0xFFFFFFFF);
    texture_load(&wall_side_texture, "gfx/solid.bmp", 0xFFFFFFFF);
    wall_top = glGenLists(1);
    wall_side= glGenLists(1);

    glFrontFace(GL_CCW);
    glNewList(wall_side, GL_COMPILE);
        glBegin(GL_QUADS);
            glNormal3f(0, -1, 0);
            glTexCoord2f(0.0, 0.0); glVertex3f(0,          0,           0);
            glTexCoord2f(0.0, 1.0); glVertex3f(0,          0,           WALL_HEIGHT);
            glTexCoord2f(1.0, 1.0); glVertex3f(TILE_WIDTH, 0,           WALL_HEIGHT);
            glTexCoord2f(1.0, 0.0); glVertex3f(TILE_WIDTH, 0,           0);

            glNormal3f(1, 0, 0);
            glTexCoord2f(0.0, 0.0); glVertex3f(TILE_WIDTH, 0,           0);
            glTexCoord2f(0.0, 1.0); glVertex3f(TILE_WIDTH, 0,           WALL_HEIGHT);
            glTexCoord2f(1.0, 1.0); glVertex3f(TILE_WIDTH, TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 0.0); glVertex3f(TILE_WIDTH, TILE_HEIGHT, 0);
            
            glNormal3f(0, 1, 0);
            glTexCoord2f(0.0, 0.0); glVertex3f(TILE_WIDTH, TILE_HEIGHT, 0);
            glTexCoord2f(0.0, 1.0); glVertex3f(TILE_WIDTH, TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 1.0); glVertex3f(0,          TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 0.0); glVertex3f(0,          TILE_HEIGHT, 0);
     
            glNormal3f(-1, 0, 0);
            glTexCoord2f(0.0, 0.0); glVertex3f(0,          TILE_HEIGHT, 0);
            glTexCoord2f(0.0, 1.0); glVertex3f(0,          TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 1.0); glVertex3f(0,          0,           WALL_HEIGHT);
            glTexCoord2f(1.0, 0.0); glVertex3f(0,          0,           0);
        glEnd();
    glEndList();

    glNewList(wall_top, GL_COMPILE);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glTexCoord2f(0.0, 0.0); glVertex3f(0,          0,           WALL_HEIGHT);
            glTexCoord2f(0.0, 1.0); glVertex3f(0,          TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 1.0); glVertex3f(TILE_WIDTH, TILE_HEIGHT, WALL_HEIGHT);
            glTexCoord2f(1.0, 0.0); glVertex3f(TILE_WIDTH, 0,           WALL_HEIGHT);
        glEnd();
    glEndList();
}

static void wall_draw() {   
    texture_activate(&wall_side_texture);
    glCallList(wall_side);
    texture_activate(&wall_top_texture);
    glCallList(wall_top);
}

static void wall_free() {
    texture_free(&wall_side_texture);
    texture_free(&wall_top_texture);
}

/* Plain */

static Texture plain_texture;
static GLint   plain;

static void plain_init() {
    texture_load(&plain_texture,  "gfx/plain.bmp",  0xFFFFFFFF);
    plain = glGenLists(1);
    glFrontFace(GL_CCW);
    glNewList(plain, GL_COMPILE);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glTexCoord2f(0.0, 0.0); glVertex3f(0,           0,           0);
            glTexCoord2f(0.0, 1.0); glVertex3f(0,           TILE_HEIGHT, 0);
            glTexCoord2f(1.0, 1.0); glVertex3f(TILE_WIDTH,  TILE_HEIGHT, 0);
            glTexCoord2f(1.0, 0.0); glVertex3f(TILE_WIDTH,  0,           0);
        glEnd();
    glEndList();
}

static void plain_draw() {
    texture_activate(&plain_texture);
    glCallList(plain);
}

static void plain_free() {
    texture_free(&plain_texture);
}

/* Koth */

static Texture koth_texture;
static GLint   koth;

static void koth_init() {
    texture_load(&koth_texture,  "gfx/koth.bmp",  0xFFFFFFFF);
    koth = glGenLists(1);
    glFrontFace(GL_CCW);
    glNewList(koth, GL_COMPILE);
        glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            glTexCoord2f(0.0, 0.0); glVertex3f(0,           0,           0);
            glTexCoord2f(0.0, 1.0); glVertex3f(0,           TILE_HEIGHT, 0);
            glTexCoord2f(1.0, 1.0); glVertex3f(TILE_WIDTH,  TILE_HEIGHT, 0);
            glTexCoord2f(1.0, 0.0); glVertex3f(TILE_WIDTH,  0,           0);
        glEnd();
    glEndList();
}

static void koth_draw() {
    texture_activate(&koth_texture);
    glCallList(koth);
}

static void koth_free() {
    texture_free(&koth_texture);
}

/* World */

static void  gl_world_info_changed(const client_world_info_t *info) {
    cam_recenter();
}

static void world_draw() {
    const client_world_info_t *info = infon->get_world_info();
    if (!info) return;

    const client_maptile_t *world = infon->get_world();
    for (int y = 0; y < info->height; y++) {
        const client_maptile_t *tile = &world[y * info->width];
        for (int x = 0; x < info->width; x++) {
            glPushMatrix();
                glTranslatef(TILE_X1(x), TILE_Y1(y), 0.0);
                if (tile->food != 0) {
                    GLfloat diffuse[] = {0.0, (float)tile->food / 10.0, 0.0, 0};
                    glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuse);
                    GLfloat amb[] = {0.0, (float)tile->food / 10.0, 0.0, 0};
                    glLightfv(GL_LIGHT3, GL_AMBIENT, amb);
                    glEnable(GL_LIGHT3);
                }
                switch (tile->type) {
                    case TILE_SOLID:
                        wall_draw();
                        break;
                    case TILE_PLAIN:
                        if (tile->gfx == TILE_GFX_KOTH)
                            koth_draw();
                        else
                            plain_draw();
                        break;
                    default:
                        break;
                }
                if (tile->food != 0) {
                    static const GLfloat zero[] = { 0, 0, 0, 0 };
                    glLightfv(GL_LIGHT3, GL_DIFFUSE, zero);
                    glLightfv(GL_LIGHT3, GL_AMBIENT, zero);
                    glDisable(GL_LIGHT3);
                }
            glPopMatrix();
            tile++;
        }
    }
}

/* Creatures */

typedef struct {
    float dir;
    int   animoffset;
} CreatureInfo;

mdl_model_t GLCreature[4];

static void creature_init() {
    // XXX: 3 mal das gleich nehmen, solange 
    // nichts besseres vorhanden ist.
    mdl_load(&GLCreature[0], "gfx/creature.mdl");
    mdl_load(&GLCreature[1], "gfx/creature.mdl");
    mdl_load(&GLCreature[2], "gfx/creature.mdl");
}

static void creature_free() {
    mdl_free(&GLCreature[0]);
    mdl_free(&GLCreature[1]);
    mdl_free(&GLCreature[2]);
}

static void creature_update_info(const client_creature_t *creature) {
    CreatureInfo *info = (CreatureInfo*)creature->userdata;
    float newdir = creature->dir * 360.0 / 32.0 - 90;
    float dw = info->dir - newdir;
    if (dw < -180) dw += 360;
    if (dw >  180) dw -= 360;

    if (dw < -1) {
        info->dir += dw < -5 ? 2 : 1;
    } else if (dw > 1) {
        info->dir -= dw >  5 ? 2 : 1; 
    } else {
        info->dir = newdir;
    }
}


static void *creature_spawned(const client_creature_t *creature) {
    CreatureInfo *info = malloc(sizeof(CreatureInfo));
    info->dir = 0.0;
    info->animoffset = rand() % 1000;
    return info;
}

static void creature_died(const client_creature_t *creature) {
    free(creature->userdata);
}

static void creature_draw(const client_creature_t *creature, void *opaque) {
    creature_update_info(creature);
    CreatureInfo *info            = (CreatureInfo*)creature->userdata;
    const client_player_t *player = infon->get_player(creature->player);
    const mdl_model_t     *model  = &GLCreature[creature->type];
    
    int hi = (player->color & 0xF0) >> 4;
    int lo = (player->color & 0x0F);

    //particle_add_fountain(creature->x, creature->y, 100);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    GLfloat l1[] = { colors[hi][0] * 40 - 20, colors[hi][1] * 40 - 20, colors[hi][2] * 40 - 20, 1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, l1);

    GLfloat l2[] = { colors[lo][0] * 40 - 20, colors[lo][1] * 40 - 20, colors[lo][2] * 40 - 20, 1.0};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l2);

    glPushMatrix();
        int scale1[] = {  3,   6,   1 };
        int scale2[] = {  3,   6,   3 };
        int      z[] = { 80, 160, 180 };
        int  animb[] = { 69,  48,  0, 69,  0,  0,  0, 60 };
        int  animl[] = {  9,  12,  1,  3,  7,  1,  1,  9 };
        int  anims[] = {100,  50, 60, 70, 60, 60, 60, 50 };
        glTranslatef(creature->x, creature->y, z[creature->type]);
        // particle_add_blood(creature->x, creature->y, z[creature->type]);
        glRotatef(info->dir, 0, 0, 1);
        glScalef (scale1[creature->type], scale2[creature->type], scale1[creature->type]);
        mdl_render_frame(model, ((real_time + info->animoffset) / anims[creature->state]) % animl[creature->state] + animb[creature->state]);
        
        //printf("%d\n",(real_time / 300) % 100);
        //mdl_render_frame(model, (real_time / 300) % 100);
    glPopMatrix();
    
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
}

/* Init / Shutdown */

static void gl_init_resources() {
    wall_init();
    plain_init();
    koth_init();
    creature_init();
}

static void gl_free_resources() {
    creature_free();
    koth_free();
    plain_free();
    wall_free();
}

static int gl_open(int w, int h, int fs) {
    video_init(w, h, fs);
    cam_init();
    gl_init_resources();
    return 1;
}

static void gl_close() {
    gl_free_resources();
    cam_free();
    video_shutdown();
}

static void gl_resize(int w, int h) {
    video_resize(w, h);
#ifdef WIN32
    /* 
     * Der GL Context geht unter Windows beim resizen verloren.
     * Daher muessen Texturen und Displaylisten neu gebaut werden.
     * 
     * http://www.libsdl.org/pipermail/sdl/2003-March/052962.html
     */
    gl_free_resources();
    gl_init_resources();
#endif
}

/* Event */

static void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RETURN:
                        if (event.key.keysym.mod & KMOD_ALT)
                            video_fullscreen_toggle();
                        break;
                    case SDLK_1: gl_resize( 640,  480); break;
                    case SDLK_2: gl_resize( 800,  600); break;
                    case SDLK_3: gl_resize(1024,  768); break;
                    case SDLK_4: gl_resize(1280, 1024); break;
                    case SDLK_5: gl_resize(1600, 1200); break;
                    case SDLK_c:
                        cam_recenter();
                        break;
                    case SDLK_ESCAPE:
                        infon->shutdown();
                        break;
                    default: ;
                }
                break;
           case SDL_MOUSEMOTION: 
                if (event.motion.state & 1) {
                    cam_change_pos_rel(event.motion.xrel * 10, event.motion.yrel * 10);
                }
                if (event.motion.state & 2) {
                    cam_change_height(event.motion.yrel * 10);
                }
                if (event.motion.state & 4) {
                    cam_change_direction((float)event.motion.xrel / 4);
                }
                break;
           case SDL_VIDEORESIZE:
                gl_resize(event.resize.w, event.resize.h);
                break;
           case SDL_QUIT:
                infon->shutdown();
                break;
        }
    }
}

static void gl_tick(int gt, int delta) {
    real_time = SDL_GetTicks();

    handle_events();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef (1., -1., 1.);
    glEnable(GL_DEPTH_TEST);

    cam_tick(delta);
    //cam_screen_to_plane(m_mousex, m_mousey, m_mouseworldx, m_mouseworldy, m_mouseinfront);
    // particle_move(delta);

    world_draw();
    infon->each_creature(creature_draw, NULL);
    // particle_draw();

    video_flip();

#ifdef WIN32
    Sleep(20);
#else
    usleep(4000);
#endif
}

static const renderer_api_t gl_api = {
    .version             = RENDERER_API_VERSION,
    .open                = gl_open,
    .close               = gl_close,
    .tick                = gl_tick,
    .world_info_changed  = gl_world_info_changed,
    .world_changed       = NULL,
    .player_joined       = NULL,
    .player_changed      = NULL,
    .player_left         = NULL,
    .creature_spawned    = creature_spawned,
    .creature_changed    = NULL,
    .creature_died       = creature_died,
    .scroll_message      = NULL,
};

RENDERER_EXPORT(gl_api)
