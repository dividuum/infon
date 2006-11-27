/*
 * mdl.c -- mdl model loader
 * last modification: dec. 19, 2005
 *
 * Copyright (c) 2005 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef GL_MDL_H
#define GL_MDL_H

#include <GL/gl.h>

/* vector */
typedef float vec3_t[3];

/* mdl header */
typedef struct {
    int ident;            /* magic number: "IDPO" */
    int version;          /* version: 6 */

    vec3_t scale;         /* scale factor */
    vec3_t translate;     /* translation vector */
    float boundingradius;
    vec3_t eyeposition;   /* eyes' position */

    int num_skins;        /* number of textures */
    int skinwidth;        /* texture width */
    int skinheight;       /* texture height */

    int num_verts;        /* number of vertices */
    int num_tris;         /* number of triangles */
    int num_frames;       /* number of frames */

    int synctype;         /* 0 = synchron, 1 = random */
    int flags;            /* state flag */
    float size;
} mdl_header_t;

/* skin */
typedef struct {
    int group;      /* 0 = single, 1 = group */
    GLubyte *data;  /* texture data */
} mdl_skin_t;

/* texture coords */
typedef struct {
    int onseam;
    int s;
    int t;
} mdl_texCoord_t;

/* triangle info */
typedef struct {
    int facesfront;  /* 0 = backface, 1 = frontface */
    int vertex[3];   /* vertex indices */
} mdl_triangle_t;

/* compressed vertex */
typedef struct {
    unsigned char v[3];
    unsigned char normalIndex;
} mdl_vertex_t;

/* simple frame */
typedef struct {
    mdl_vertex_t bboxmin;	/* bouding box min */
    mdl_vertex_t bboxmax;	/* bouding box max */
    char name[16];
    mdl_vertex_t *verts;    /* vertex list of the frame */
} mdl_simpleframe_t;

/* model frame */
typedef struct {
    int type;                 /* 0 = simple, !0 = group */
    mdl_simpleframe_t frame;  /* this program can't read models
                                 composed of group frames! */
} mdl_frame_t;

/* mdl model structure */
typedef struct {
    mdl_header_t     header;
    mdl_skin_t      *skins;
    mdl_texCoord_t  *texcoords;
    mdl_triangle_t  *triangles;
    mdl_frame_t     *frames;
    GLuint          *tex_id;
    int              iskin;
} mdl_model_t;

int  mdl_load        (mdl_model_t *mdl, const char *filename);
void mdl_free        (mdl_model_t *mdl);

void mdl_render_frame(const mdl_model_t *mdl, int n);

#endif
