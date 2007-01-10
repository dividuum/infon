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

#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl_mdl.h"
#include "misc.h"

/* table of precalculated normals */
vec3_t anorms_table[162] = {
    { -0.525731f,  0.000000f,  0.850651f }, 
    { -0.442863f,  0.238856f,  0.864188f }, 
    { -0.295242f,  0.000000f,  0.955423f }, 
    { -0.309017f,  0.500000f,  0.809017f }, 
    { -0.162460f,  0.262866f,  0.951056f }, 
    {  0.000000f,  0.000000f,  1.000000f }, 
    {  0.000000f,  0.850651f,  0.525731f }, 
    { -0.147621f,  0.716567f,  0.681718f }, 
    {  0.147621f,  0.716567f,  0.681718f }, 
    {  0.000000f,  0.525731f,  0.850651f }, 
    {  0.309017f,  0.500000f,  0.809017f }, 
    {  0.525731f,  0.000000f,  0.850651f }, 
    {  0.295242f,  0.000000f,  0.955423f }, 
    {  0.442863f,  0.238856f,  0.864188f }, 
    {  0.162460f,  0.262866f,  0.951056f }, 
    { -0.681718f,  0.147621f,  0.716567f }, 
    { -0.809017f,  0.309017f,  0.500000f }, 
    { -0.587785f,  0.425325f,  0.688191f }, 
    { -0.850651f,  0.525731f,  0.000000f }, 
    { -0.864188f,  0.442863f,  0.238856f }, 
    { -0.716567f,  0.681718f,  0.147621f }, 
    { -0.688191f,  0.587785f,  0.425325f }, 
    { -0.500000f,  0.809017f,  0.309017f }, 
    { -0.238856f,  0.864188f,  0.442863f }, 
    { -0.425325f,  0.688191f,  0.587785f }, 
    { -0.716567f,  0.681718f, -0.147621f }, 
    { -0.500000f,  0.809017f, -0.309017f }, 
    { -0.525731f,  0.850651f,  0.000000f }, 
    {  0.000000f,  0.850651f, -0.525731f }, 
    { -0.238856f,  0.864188f, -0.442863f }, 
    {  0.000000f,  0.955423f, -0.295242f }, 
    { -0.262866f,  0.951056f, -0.162460f }, 
    {  0.000000f,  1.000000f,  0.000000f }, 
    {  0.000000f,  0.955423f,  0.295242f }, 
    { -0.262866f,  0.951056f,  0.162460f }, 
    {  0.238856f,  0.864188f,  0.442863f }, 
    {  0.262866f,  0.951056f,  0.162460f }, 
    {  0.500000f,  0.809017f,  0.309017f }, 
    {  0.238856f,  0.864188f, -0.442863f }, 
    {  0.262866f,  0.951056f, -0.162460f }, 
    {  0.500000f,  0.809017f, -0.309017f }, 
    {  0.850651f,  0.525731f,  0.000000f }, 
    {  0.716567f,  0.681718f,  0.147621f }, 
    {  0.716567f,  0.681718f, -0.147621f }, 
    {  0.525731f,  0.850651f,  0.000000f }, 
    {  0.425325f,  0.688191f,  0.587785f }, 
    {  0.864188f,  0.442863f,  0.238856f }, 
    {  0.688191f,  0.587785f,  0.425325f }, 
    {  0.809017f,  0.309017f,  0.500000f }, 
    {  0.681718f,  0.147621f,  0.716567f }, 
    {  0.587785f,  0.425325f,  0.688191f }, 
    {  0.955423f,  0.295242f,  0.000000f }, 
    {  1.000000f,  0.000000f,  0.000000f }, 
    {  0.951056f,  0.162460f,  0.262866f }, 
    {  0.850651f, -0.525731f,  0.000000f }, 
    {  0.955423f, -0.295242f,  0.000000f }, 
    {  0.864188f, -0.442863f,  0.238856f }, 
    {  0.951056f, -0.162460f,  0.262866f }, 
    {  0.809017f, -0.309017f,  0.500000f }, 
    {  0.681718f, -0.147621f,  0.716567f }, 
    {  0.850651f,  0.000000f,  0.525731f }, 
    {  0.864188f,  0.442863f, -0.238856f }, 
    {  0.809017f,  0.309017f, -0.500000f }, 
    {  0.951056f,  0.162460f, -0.262866f }, 
    {  0.525731f,  0.000000f, -0.850651f }, 
    {  0.681718f,  0.147621f, -0.716567f }, 
    {  0.681718f, -0.147621f, -0.716567f }, 
    {  0.850651f,  0.000000f, -0.525731f }, 
    {  0.809017f, -0.309017f, -0.500000f }, 
    {  0.864188f, -0.442863f, -0.238856f }, 
    {  0.951056f, -0.162460f, -0.262866f }, 
    {  0.147621f,  0.716567f, -0.681718f }, 
    {  0.309017f,  0.500000f, -0.809017f }, 
    {  0.425325f,  0.688191f, -0.587785f }, 
    {  0.442863f,  0.238856f, -0.864188f }, 
    {  0.587785f,  0.425325f, -0.688191f }, 
    {  0.688191f,  0.587785f, -0.425325f }, 
    { -0.147621f,  0.716567f, -0.681718f }, 
    { -0.309017f,  0.500000f, -0.809017f }, 
    {  0.000000f,  0.525731f, -0.850651f }, 
    { -0.525731f,  0.000000f, -0.850651f }, 
    { -0.442863f,  0.238856f, -0.864188f }, 
    { -0.295242f,  0.000000f, -0.955423f }, 
    { -0.162460f,  0.262866f, -0.951056f }, 
    {  0.000000f,  0.000000f, -1.000000f }, 
    {  0.295242f,  0.000000f, -0.955423f }, 
    {  0.162460f,  0.262866f, -0.951056f }, 
    { -0.442863f, -0.238856f, -0.864188f }, 
    { -0.309017f, -0.500000f, -0.809017f }, 
    { -0.162460f, -0.262866f, -0.951056f }, 
    {  0.000000f, -0.850651f, -0.525731f }, 
    { -0.147621f, -0.716567f, -0.681718f }, 
    {  0.147621f, -0.716567f, -0.681718f }, 
    {  0.000000f, -0.525731f, -0.850651f }, 
    {  0.309017f, -0.500000f, -0.809017f }, 
    {  0.442863f, -0.238856f, -0.864188f }, 
    {  0.162460f, -0.262866f, -0.951056f }, 
    {  0.238856f, -0.864188f, -0.442863f }, 
    {  0.500000f, -0.809017f, -0.309017f }, 
    {  0.425325f, -0.688191f, -0.587785f }, 
    {  0.716567f, -0.681718f, -0.147621f }, 
    {  0.688191f, -0.587785f, -0.425325f }, 
    {  0.587785f, -0.425325f, -0.688191f }, 
    {  0.000000f, -0.955423f, -0.295242f }, 
    {  0.000000f, -1.000000f,  0.000000f }, 
    {  0.262866f, -0.951056f, -0.162460f }, 
    {  0.000000f, -0.850651f,  0.525731f }, 
    {  0.000000f, -0.955423f,  0.295242f }, 
    {  0.238856f, -0.864188f,  0.442863f }, 
    {  0.262866f, -0.951056f,  0.162460f }, 
    {  0.500000f, -0.809017f,  0.309017f }, 
    {  0.716567f, -0.681718f,  0.147621f }, 
    {  0.525731f, -0.850651f,  0.000000f }, 
    { -0.238856f, -0.864188f, -0.442863f }, 
    { -0.500000f, -0.809017f, -0.309017f }, 
    { -0.262866f, -0.951056f, -0.162460f }, 
    { -0.850651f, -0.525731f,  0.000000f }, 
    { -0.716567f, -0.681718f, -0.147621f }, 
    { -0.716567f, -0.681718f,  0.147621f }, 
    { -0.525731f, -0.850651f,  0.000000f }, 
    { -0.500000f, -0.809017f,  0.309017f }, 
    { -0.238856f, -0.864188f,  0.442863f }, 
    { -0.262866f, -0.951056f,  0.162460f }, 
    { -0.864188f, -0.442863f,  0.238856f }, 
    { -0.809017f, -0.309017f,  0.500000f }, 
    { -0.688191f, -0.587785f,  0.425325f }, 
    { -0.681718f, -0.147621f,  0.716567f }, 
    { -0.442863f, -0.238856f,  0.864188f }, 
    { -0.587785f, -0.425325f,  0.688191f }, 
    { -0.309017f, -0.500000f,  0.809017f }, 
    { -0.147621f, -0.716567f,  0.681718f }, 
    { -0.425325f, -0.688191f,  0.587785f }, 
    { -0.162460f, -0.262866f,  0.951056f }, 
    {  0.442863f, -0.238856f,  0.864188f }, 
    {  0.162460f, -0.262866f,  0.951056f }, 
    {  0.309017f, -0.500000f,  0.809017f }, 
    {  0.147621f, -0.716567f,  0.681718f }, 
    {  0.000000f, -0.525731f,  0.850651f }, 
    {  0.425325f, -0.688191f,  0.587785f }, 
    {  0.587785f, -0.425325f,  0.688191f }, 
    {  0.688191f, -0.587785f,  0.425325f }, 
    { -0.955423f,  0.295242f,  0.000000f }, 
    { -0.951056f,  0.162460f,  0.262866f }, 
    { -1.000000f,  0.000000f,  0.000000f }, 
    { -0.850651f,  0.000000f,  0.525731f }, 
    { -0.955423f, -0.295242f,  0.000000f }, 
    { -0.951056f, -0.162460f,  0.262866f }, 
    { -0.864188f,  0.442863f, -0.238856f }, 
    { -0.951056f,  0.162460f, -0.262866f }, 
    { -0.809017f,  0.309017f, -0.500000f }, 
    { -0.864188f, -0.442863f, -0.238856f }, 
    { -0.951056f, -0.162460f, -0.262866f }, 
    { -0.809017f, -0.309017f, -0.500000f }, 
    { -0.681718f,  0.147621f, -0.716567f }, 
    { -0.681718f, -0.147621f, -0.716567f }, 
    { -0.850651f,  0.000000f, -0.525731f }, 
    { -0.688191f,  0.587785f, -0.425325f }, 
    { -0.587785f,  0.425325f, -0.688191f }, 
    { -0.425325f,  0.688191f, -0.587785f }, 
    { -0.425325f, -0.688191f, -0.587785f }, 
    { -0.587785f, -0.425325f, -0.688191f }, 
    { -0.688191f, -0.587785f, -0.425325f }
};

/* palette */
unsigned char colormap[256][3] = {
    {  0,   0,   0}, { 15,  15,  15}, { 31,  31,  31}, { 47,  47,  47}, 
    { 63,  63,  63}, { 75,  75,  75}, { 91,  91,  91}, {107, 107, 107}, 
    {123, 123, 123}, {139, 139, 139}, {155, 155, 155}, {171, 171, 171}, 
    {187, 187, 187}, {203, 203, 203}, {219, 219, 219}, {235, 235, 235}, 
    { 15,  11,   7}, { 23,  15,  11}, { 31,  23,  11}, { 39,  27,  15}, 
    { 47,  35,  19}, { 55,  43,  23}, { 63,  47,  23}, { 75,  55,  27}, 
    { 83,  59,  27}, { 91,  67,  31}, { 99,  75,  31}, {107,  83,  31}, 
    {115,  87,  31}, {123,  95,  35}, {131, 103,  35}, {143, 111,  35}, 
    { 11,  11,  15}, { 19,  19,  27}, { 27,  27,  39}, { 39,  39,  51}, 
    { 47,  47,  63}, { 55,  55,  75}, { 63,  63,  87}, { 71,  71, 103}, 
    { 79,  79, 115}, { 91,  91, 127}, { 99,  99, 139}, {107, 107, 151}, 
    {115, 115, 163}, {123, 123, 175}, {131, 131, 187}, {139, 139, 203}, 
    {  0,   0,   0}, {  7,   7,   0}, { 11,  11,   0}, { 19,  19,   0}, 
    { 27,  27,   0}, { 35,  35,   0}, { 43,  43,   7}, { 47,  47,   7}, 
    { 55,  55,   7}, { 63,  63,   7}, { 71,  71,   7}, { 75,  75,  11}, 
    { 83,  83,  11}, { 91,  91,  11}, { 99,  99,  11}, {107, 107,  15}, 
    {  7,   0,   0}, { 15,   0,   0}, { 23,   0,   0}, { 31,   0,   0}, 
    { 39,   0,   0}, { 47,   0,   0}, { 55,   0,   0}, { 63,   0,   0}, 
    { 71,   0,   0}, { 79,   0,   0}, { 87,   0,   0}, { 95,   0,   0}, 
    {103,   0,   0}, {111,   0,   0}, {119,   0,   0}, {127,   0,   0}, 
    { 19,  19,   0}, { 27,  27,   0}, { 35,  35,   0}, { 47,  43,   0}, 
    { 55,  47,   0}, { 67,  55,   0}, { 75,  59,   7}, { 87,  67,   7}, 
    { 95,  71,   7}, {107,  75,  11}, {119,  83,  15}, {131,  87,  19}, 
    {139,  91,  19}, {151,  95,  27}, {163,  99,  31}, {175, 103,  35}, 
    { 35,  19,   7}, { 47,  23,  11}, { 59,  31,  15}, { 75,  35,  19}, 
    { 87,  43,  23}, { 99,  47,  31}, {115,  55,  35}, {127,  59,  43}, 
    {143,  67,  51}, {159,  79,  51}, {175,  99,  47}, {191, 119,  47}, 
    {207, 143,  43}, {223, 171,  39}, {239, 203,  31}, {255, 243,  27}, 
    { 11,   7,   0}, { 27,  19,   0}, { 43,  35,  15}, { 55,  43,  19}, 
    { 71,  51,  27}, { 83,  55,  35}, { 99,  63,  43}, {111,  71,  51}, 
    {127,  83,  63}, {139,  95,  71}, {155, 107,  83}, {167, 123,  95}, 
    {183, 135, 107}, {195, 147, 123}, {211, 163, 139}, {227, 179, 151}, 
    {171, 139, 163}, {159, 127, 151}, {147, 115, 135}, {139, 103, 123}, 
    {127,  91, 111}, {119,  83,  99}, {107,  75,  87}, { 95,  63,  75}, 
    { 87,  55,  67}, { 75,  47,  55}, { 67,  39,  47}, { 55,  31,  35}, 
    { 43,  23,  27}, { 35,  19,  19}, { 23,  11,  11}, { 15,   7,   7}, 
    {187, 115, 159}, {175, 107, 143}, {163,  95, 131}, {151,  87, 119}, 
    {139,  79, 107}, {127,  75,  95}, {115,  67,  83}, {107,  59,  75}, 
    { 95,  51,  63}, { 83,  43,  55}, { 71,  35,  43}, { 59,  31,  35}, 
    { 47,  23,  27}, { 35,  19,  19}, { 23,  11,  11}, { 15,   7,   7}, 
    {219, 195, 187}, {203, 179, 167}, {191, 163, 155}, {175, 151, 139}, 
    {163, 135, 123}, {151, 123, 111}, {135, 111,  95}, {123,  99,  83}, 
    {107,  87,  71}, { 95,  75,  59}, { 83,  63,  51}, { 67,  51,  39}, 
    { 55,  43,  31}, { 39,  31,  23}, { 27,  19,  15}, { 15,  11,   7}, 
    {111, 131, 123}, {103, 123, 111}, { 95, 115, 103}, { 87, 107,  95}, 
    { 79,  99,  87}, { 71,  91,  79}, { 63,  83,  71}, { 55,  75,  63}, 
    { 47,  67,  55}, { 43,  59,  47}, { 35,  51,  39}, { 31,  43,  31}, 
    { 23,  35,  23}, { 15,  27,  19}, { 11,  19,  11}, {  7,  11,   7}, 
    {255, 243,  27}, {239, 223,  23}, {219, 203,  19}, {203, 183,  15}, 
    {187, 167,  15}, {171, 151,  11}, {155, 131,   7}, {139, 115,   7}, 
    {123,  99,   7}, {107,  83,   0}, { 91,  71,   0}, { 75,  55,   0}, 
    { 59,  43,   0}, { 43,  31,   0}, { 27,  15,   0}, { 11,   7,   0}, 
    {  0,   0, 255}, { 11,  11, 239}, { 19,  19, 223}, { 27,  27, 207}, 
    { 35,  35, 191}, { 43,  43, 175}, { 47,  47, 159}, { 47,  47, 143}, 
    { 47,  47, 127}, { 47,  47, 111}, { 47,  47,  95}, { 43,  43,  79}, 
    { 35,  35,  63}, { 27,  27,  47}, { 19,  19,  31}, { 11,  11,  15}, 
    { 43,   0,   0}, { 59,   0,   0}, { 75,   7,   0}, { 95,   7,   0}, 
    {111,  15,   0}, {127,  23,   7}, {147,  31,   7}, {163,  39,  11}, 
    {183,  51,  15}, {195,  75,  27}, {207,  99,  43}, {219, 127,  59}, 
    {227, 151,  79}, {231, 171,  95}, {239, 191, 119}, {247, 211, 139}, 
    {167, 123,  59}, {183, 155,  55}, {199, 195,  55}, {231, 227,  87}, 
    {127, 191, 255}, {171, 231, 255}, {215, 255, 255}, {103,   0,   0}, 
    {139,   0,   0}, {179,   0,   0}, {215,   0,   0}, {255,   0,   0}, 
    {255, 243, 147}, {255, 247, 199}, {255, 255, 255}, {159,  91,  83}, 
};

static GLuint MakeTexture(int n, mdl_model_t *mdl) {
    int i;
    GLuint id;
    GLubyte *pixels = (GLubyte *)malloc(sizeof (GLubyte) * 
                                        mdl->header.skinwidth * 
                                        mdl->header.skinheight * 3);

    /* convert indexed 8 bits texture to RGB 24 bits */
    for (i = 0; i < mdl->header.skinwidth * mdl->header.skinheight; ++i) {
        pixels[(i * 3) + 0] = colormap[mdl->skins[n].data[i]][0];
        pixels[(i * 3) + 1] = colormap[mdl->skins[n].data[i]][1];
        pixels[(i * 3) + 2] = colormap[mdl->skins[n].data[i]][2];
    }

    /* generate OpenGL texture */
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, mdl->header.skinwidth,
                    mdl->header.skinheight, GL_RGB, GL_UNSIGNED_BYTE,
                    pixels);

    /* OpenGL has its own copy of image data */
    free(pixels);
    return id;
}

int mdl_load(mdl_model_t *mdl, const char *filename) {
    FILE *fp;
    int i;

    fp = fopen(filename, "rb");
    if (!fp) {
        die("error: couldn't open '%s'", filename);
        return 0;
    }

    /* read header */
    fread (&mdl->header, 1, sizeof (mdl_header_t), fp);

    if ((mdl->header.ident != 1330660425) ||
        (mdl->header.version != 6)) {
        /* error! */
        fclose (fp);
        die("error: %s is not a mdl file", filename);
        return 0;
    }

    /* memory allocation */
    mdl->skins =     (mdl_skin_t *)    malloc(sizeof (mdl_skin_t) * mdl->header.num_skins);
    mdl->texcoords = (mdl_texCoord_t *)malloc(sizeof (mdl_texCoord_t) * mdl->header.num_verts);
    mdl->triangles = (mdl_triangle_t *)malloc(sizeof (mdl_triangle_t) * mdl->header.num_tris);
    mdl->frames =    (mdl_frame_t *)   malloc(sizeof (mdl_frame_t) * mdl->header.num_frames);
    mdl->tex_id =    (GLuint *)        malloc(sizeof (GLuint) * mdl->header.num_skins);

    mdl->iskin = 0;

    /* read texture data */
    for (i = 0; i < mdl->header.num_skins; ++i) {
        mdl->skins[i].data = (GLubyte *)malloc(sizeof (GLubyte) * 
                                               mdl->header.skinwidth * 
                                               mdl->header.skinheight);

        fread(&mdl->skins[i].group, sizeof (int), 1, fp);
        fread(mdl->skins[i].data, sizeof (GLubyte),
              mdl->header.skinwidth * mdl->header.skinheight, fp);

        mdl->tex_id[i] = MakeTexture (i, mdl);

        free (mdl->skins[i].data);
        mdl->skins[i].data = NULL;
    }

    fread (mdl->texcoords, sizeof (mdl_texCoord_t), mdl->header.num_verts, fp);
    fread (mdl->triangles, sizeof (mdl_triangle_t), mdl->header.num_tris, fp);

    /* read frames */
    for (i = 0; i < mdl->header.num_frames; ++i) {
        /* memory allocation for vertices of this frame */
        mdl->frames[i].frame.verts = (mdl_vertex_t *)
            malloc (sizeof (mdl_vertex_t) * mdl->header.num_verts);

        /* read frame data */
        fread (&mdl->frames[i].type, sizeof (long), 1, fp);
        fread (&mdl->frames[i].frame.bboxmin, sizeof (mdl_vertex_t), 1, fp);
        fread (&mdl->frames[i].frame.bboxmax, sizeof (mdl_vertex_t), 1, fp);
        fread (mdl->frames[i].frame.name, sizeof (char), 16, fp);
        // printf("%d %s\n", i, mdl->frames[i].frame.name);
        fread (mdl->frames[i].frame.verts, sizeof (mdl_vertex_t),
                mdl->header.num_verts, fp);
    }

    fclose (fp);
    return 1;
}

void mdl_free(mdl_model_t *mdl) {
    int i;

    if (mdl->skins)     free (mdl->skins);
    if (mdl->texcoords) free (mdl->texcoords);
    if (mdl->triangles) free (mdl->triangles);
    if (mdl->tex_id) {
        /* delete OpenGL textures */
        glDeleteTextures (mdl->header.num_skins, mdl->tex_id);
        free (mdl->tex_id);
    }

    if (mdl->frames) {
        for (i = 0; i < mdl->header.num_frames; ++i) 
            free(mdl->frames[i].frame.verts);
        free(mdl->frames);
    }
}

void mdl_render_frame(const mdl_model_t *mdl, int n) {
    int i, j;
    GLfloat s, t;
    vec3_t v;
    mdl_vertex_t *pvert;

    /* check if n is in a valid range */
    if ((n < 0) || (n > mdl->header.num_frames - 1))
        return;

    /* enable model's texture */
    glBindTexture (GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);

    /* draw the model */
    glBegin (GL_TRIANGLES);
    /* draw each triangle */
    for (i = 0; i < mdl->header.num_tris; ++i) {
        /* draw each vertex */
        for (j = 0; j < 3; ++j) {
            pvert = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];

            /* compute texture coordinates */
            s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
            t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

            if (!mdl->triangles[i].facesfront &&
                 mdl->texcoords[mdl->triangles[i].vertex[j]].onseam) 
                s += mdl->header.skinwidth * 0.5f; /* backface */

            /* scale s and t to range from 0.0 to 1.0 */
            s = (s + 0.5) / mdl->header.skinwidth;
            t = (t + 0.5) / mdl->header.skinheight;

            /* pass texture coordinates to OpenGL */
            glTexCoord2f (s, t);

            /* normal vector */
            glNormal3fv (anorms_table [pvert->normalIndex]);

            /* calculate real vertex position */
            v[0] = (mdl->header.scale[0] * pvert->v[0]) + mdl->header.translate[0];
            v[1] = (mdl->header.scale[1] * pvert->v[1]) + mdl->header.translate[1];
            v[2] = (mdl->header.scale[2] * pvert->v[2]) + mdl->header.translate[2];

            glVertex3fv (v);
        }
    }
    glEnd ();
}

#if 0
void RenderFrameItp (int n, float interp, mdl_model_t *mdl) {
    int i, j;
    GLfloat s, t;
    vec3_t norm, v;
    GLfloat *n_curr, *n_next;
    mdl_vertex_t *pvert1, *pvert2;

    /* check if n is in a valid range */
    if ((n < 0) || (n > mdl->header.num_frames))
        return;

    /* enable model's texture */
    glBindTexture (GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);

    /* draw the model */
    glBegin (GL_TRIANGLES);
    /* draw each triangle */
    for (i = 0; i < mdl->header.num_tris; ++i) {
        /* draw each vertex */
        for (j = 0; j < 3; ++j) {
            pvert1 = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];
            pvert2 = &mdl->frames[n + 1].frame.verts[mdl->triangles[i].vertex[j]];

            /* compute texture coordinates */
            s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
            t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

            if (!mdl->triangles[i].facesfront &&
                 mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
                s += mdl->header.skinwidth * 0.5f; /* backface */

            /* scale s and t to range from 0.0 to 1.0 */
            s = (s + 0.5) / mdl->header.skinwidth;
            t = (t + 0.5) / mdl->header.skinheight;

            /* pass texture coordinates to OpenGL */
            glTexCoord2f (s, t);

            /* interpolate normals */
            /*
               memcpy (n_curr, anorms_table[pvert1->normalIndex], sizeof (vec3_t));
               memcpy (n_next, anorms_table[pvert2->normalIndex], sizeof (vec3_t));
               */

            n_curr = anorms_table[pvert1->normalIndex];
            n_next = anorms_table[pvert2->normalIndex];

            norm[0] = n_curr[0] + interp * (n_next[0] - n_curr[0]);
            norm[1] = n_curr[1] + interp * (n_next[1] - n_curr[1]);
            norm[2] = n_curr[2] + interp * (n_next[2] - n_curr[2]);

            glNormal3fv (norm);

            /* interpolate vertices */
            v[0] = mdl->header.scale[0] * (pvert1->v[0] + interp
                    * (pvert2->v[0] - pvert1->v[0])) + mdl->header.translate[0];
            v[1] = mdl->header.scale[1] * (pvert1->v[1] + interp
                    * (pvert2->v[1] - pvert1->v[1])) + mdl->header.translate[1];
            v[2] = mdl->header.scale[2] * (pvert1->v[2] + interp
                    * (pvert2->v[2] - pvert1->v[2])) + mdl->header.translate[2];

            glVertex3fv (v);
        }
    }
    glEnd ();
}
#endif

