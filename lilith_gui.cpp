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

#include "lilith/lilith/lilith3d.h"
#include "lilith/lilith/import/md2.h"

#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "renderer.h"

using namespace grinliz;
using namespace lilith3d;

static SDL_Surface    *surface = 0; 
static TextureManager *texman  = 0;
static MeshResManager *meshman = 0;
static Lilith3D       *lilith  = 0;

static Uint32          real_time;

class LCreature : public Mob {
public:
    LCreature(const client_creature_t *creature) : m_rand(real_time), m_toggle(0) {
        MeshResManager *meshman  = MeshResManager::Instance();
        m_mesh = new AnimatedMesh(meshman->GetAnimatedRes("Marine"));
        m_lastdir = 0;
        update(creature);
        for (int i = 0; i < 10; i++) 
            smoke();
    }

    ~LCreature() {
        for (int i = 0; i < 10; i++) 
            smoke();
        delete m_mesh;
    }

    void Update( TimeClock* tc ) {
    }

    void KillAnimation() {   
        m_mesh->SetAction( AnimatedMesh::DEATH );
    }
    
    void smoke() {
        Vector3F smoke_vel= { m_rand.FRand(1.0) + -0.5 , m_rand.FRand(1.0) + -0.5 , m_rand.FRand(1.0) + 0.5 };
        float r = m_rand.FRand(0.3) + 0.7;
        Color3F smoke_col = { r, r, r };
        lilith->GetGravParticles()->Create(m_mesh->Pos(), 
                                           smoke_vel,
                                           smoke_col,
                                           2,
                                           GravParticles::FLAME0);
    }

    void update(const client_creature_t *creature) {
        float x = (float)creature->x / TILE_WIDTH  + 1.5;
        float y = (float)creature->y / TILE_HEIGHT + 1.5;
        int action;
        switch (creature->state) {
            case CREATURE_IDLE:
            case CREATURE_HEAL:
            case CREATURE_CONVERT:
            case CREATURE_SPAWN:
            case CREATURE_FEED:
            case CREATURE_EAT:
                if (creature->health == 0)
                    action = AnimatedMesh::DEATH;
                else
                    action = AnimatedMesh::STAND;
                break;
            case CREATURE_WALK:
                action = AnimatedMesh::RUN;
                break;
            case CREATURE_ATTACK:
                action = AnimatedMesh::ATTACK;
                break;
        }

        if (action != m_mesh->GetAction()) 
            m_mesh->SetAction(action);

        float dir = creature->dir * 360.0 / 32.0 - 90;
        /*
        float dw = m_lastdir - dir;

        if (dw < -180) dw += 180;
        if (dw >  180) dw -= 180;

        if (dw < -6) {
            m_lastdir += 6;
        } else if (dw > 6) {
            m_lastdir -= 6;
        } else {
            m_lastdir = dir; 
        }
        */

        m_lastdir = dir; 

        if (creature->type == 2) {
            float flow = creature->type == 2 ? sin(creature->num + (float)real_time / 1000.0) : 0;
            Vector3F at = { x - 0.5 + flow, y - 0.5 + flow, 3.0f + flow };
            m_mesh->SetPos( at, m_lastdir );
            m_mesh->AttachToTerrain(false);
        } else {
            Vector3F at = { x, y, 0 };
            m_mesh->SetPos( at, m_lastdir );
            m_mesh->AttachToTerrain(true);
        }

        static const Color3F colors[16] = {
            { 1.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 },
            { 1.0, 1.0, 0.0 },
            { 0.0, 1.0, 1.0 },
            { 1.0, 0.0, 1.0 },
            { 1.0, 1.0, 1.0 },
            { 0.0, 0.0, 0.0 },

            { 1.0, 0.5, 0.5 },
            { 0.5, 1.0, 0.5 },
            { 0.5, 0.5, 1.0 },
            { 1.0, 1.0, 0.5 },
            { 0.5, 1.0, 1.0 },
            { 1.0, 0.5, 1.0 },
            { 0.5, 0.5, 0.5 },
            { 0.3, 0.7, 1.0 },
        };


        Vector3F velocity = { 0, 0, m_rand.FRand(0.5) + 0.5 };

        const client_player_t *player = infon->get_player(creature->player);
        int hi = (player->color & 0xF0) >> 4;
        int lo = (player->color & 0x0F);

        lilith->GetGravParticles()->Create(m_mesh->Pos(), 
                                           velocity, 
                                           colors[m_toggle ? hi : lo],
                                           2,
                                           GravParticles::SMALL );
        m_toggle ^= 1;

        switch (creature->state) {
            case CREATURE_CONVERT:
            case CREATURE_SPAWN:
                smoke();
                break;
            default:
                break;
        }
    }

private:
    float         m_lastdir;
    Random        m_rand;
    int           m_toggle;
    AnimatedMesh *m_mesh;
};

static int lilith_open(int w, int h, int fs) {
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER ) < 0 ) 
        die("SDL initialization failed: %s\n", SDL_GetError( ));

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    int videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
        videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */

    videoFlags |= SDL_RESIZABLE;
    //int bpp = SDL_VideoModeOK(w, h, 32, videoFlags );
    //GLASSERT( bpp );

    surface = SDL_SetVideoMode(w, h, 32, videoFlags );
    GLASSERT( surface );

    SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
    InitOpenGL();

    // Tutorial 1: Textures and Models
    texman  = new TextureManager();
    meshman = new MeshResManager();

    texman->SetImportPath ("lilith/lilith/graphics/");
    meshman->SetImportPath("lilith/lilith/graphics/");

    texman->LoadRequiredTextures();
    meshman->LoadRequiredModels();

    texman->SetImportPath ("gfx/");
    meshman->SetImportPath("gfx/");

    MD2Resource* marine = meshman->LoadMD2Res("bug.md2", "Marine");
    marine->SetTexture( texman->LoadTexture("bug.pcx", "Marine"),
                        MD2Resource::SPRITE );

    lilith = new Lilith3D();
    lilith->SetRenderMode( Lilith3D::RENDER_SHADER );

    return 1;
}

static void creature_update(const client_creature_t *creature, void *opaque) {
    LCreature *lcreature = (LCreature*)creature->userdata;
    lcreature->update(creature);
}

static void lilith_tick(int gt, int delta) {
    real_time = SDL_GetTicks();

    static bool ignoreJump = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RETURN:
                        //if (event.key.keysym.mod & KMOD_ALT)
                        //    video_fullscreen_toggle();
                        break;
                    case SDLK_ESCAPE:
                        infon->shutdown();
                        break;
                    case SDLK_F1:
                        lilith->SetRenderMode( Lilith3D::RENDER_SHADER );
                        break;
                    case SDLK_F2:
                        lilith->SetRenderMode( Lilith3D::RENDER_FLAT );
                        break;
                    case SDLK_F3:
                        lilith->SetRenderMode( Lilith3D::RENDER_LOD );
                        break;
                    case SDLK_F4:
                        lilith->SetRenderMode( Lilith3D::RENDER_PATH );
                        break;
                    case SDLK_F5:
                        TimeClock::Instance()->ToggleSun();
                        break;
                    case SDLK_F6:
                        lilith->SetLineMode( !lilith->LineMode() );
                        break;
                    default: ;
                }
                break;
           case SDL_MOUSEBUTTONDOWN:
                if ( event.button.button == SDL_BUTTON_RIGHT ) {
                    SDL_WM_GrabInput( SDL_GRAB_ON );
                    SDL_ShowCursor( SDL_DISABLE );
                    ignoreJump = true;
                }
                break;
           case SDL_MOUSEBUTTONUP:
                if ( event.button.button == SDL_BUTTON_RIGHT ) {
                    SDL_WM_GrabInput( SDL_GRAB_OFF );
                    SDL_ShowCursor( SDL_ENABLE );
                }
                break;
           case SDL_MOUSEMOTION:
                if ( event.motion.state & SDL_BUTTON (SDL_BUTTON_RIGHT) ) {   
                    if ( !ignoreJump ) {
                        Camera* camera = lilith->GetCamera();
                        float xVel = (float)event.motion.xrel * 20.0f;
                        float yVel = (float)event.motion.yrel * 20.0f;
                        camera->MoveCameraRotation( Camera::YAW, -xVel );
                        camera->MoveCameraRotation( Camera::PITCH, yVel );
                    }
                    ignoreJump = false;
                }
                break; 
           case SDL_VIDEORESIZE:
                break;
           case SDL_QUIT:
                infon->shutdown();
                break;
        }
    }

    if ( SDL_GetMouseState(0, 0) == (SDL_BUTTON( SDL_BUTTON_LEFT ) | SDL_BUTTON( SDL_BUTTON_RIGHT ) ) )
        lilith->GetCamera()->MoveCameraAxial3D( 20.0f );

    if ( SDL_GetMouseState(0, 0) == (SDL_BUTTON( SDL_BUTTON_MIDDLE) | SDL_BUTTON( SDL_BUTTON_RIGHT ) ) )
        lilith->GetCamera()->MoveCameraAxial3D(-20.0f );
    
    infon->each_creature(creature_update, 0);

    lilith->BeginDraw();
    lilith->Draw();
    lilith->EndDraw();
}

static void lilith_world_info_changed(const client_world_info_t *info) {
    assert(info->width  < MAPSIZE);
    assert(info->height < MAPSIZE);

    TerrainMesh *tmesh  = lilith->GetTerrainMesh();
    tmesh->StartHeightChange();
    for(int i=0; i<(int)VERTEXSIZE; ++i ) {
        for(int j=0; j<(int)VERTEXSIZE; ++j ) {
            tmesh->SetHeight(i, j, -0.3);  
        }
    }
    tmesh->EndHeightChange();
}

static void lilith_world_changed(int x, int y) {
    TerrainMesh *tmesh           = lilith->GetTerrainMesh();
    const client_maptile_t *tile = infon->get_world_tile(x, y);
       
    tmesh->StartHeightChange();
    switch (tile->map) {
        case TILE_SOLID:
            tmesh->SetHeight(x + 2, y + 2, 1+sin(x+y)/10);
            break;
        case TILE_PLAIN:
            tmesh->SetHeight(x + 2, y + 2, 0.5+sin(x+y)/10);
            break;
        default:
            tmesh->SetHeight(x + 2, y + 2, -0.3);
            break;
    }
    tmesh->EndHeightChange();
}

static void *lilith_creature_spawned(const client_creature_t *creature) {
    return new LCreature(creature);
}

static void lilith_creature_changed(const client_creature_t *creature, int changed) {
}

static void lilith_creature_died(const client_creature_t *creature) {
    LCreature *lcreature = (LCreature*)creature->userdata;
    delete lcreature;
}

static void lilith_close() {
    delete lilith;
    delete meshman;
    delete texman;
    SDL_Quit();
}
    
static const renderer_api_t lilith_api = {
    RENDERER_API_VERSION,
    lilith_open,
    lilith_close,
    lilith_tick,
    lilith_world_info_changed,
    lilith_world_changed,
    NULL,
    NULL,
    NULL,
    lilith_creature_spawned,
    lilith_creature_changed,
    lilith_creature_died,
    NULL
};

RENDERER_EXPORT(lilith_api)
