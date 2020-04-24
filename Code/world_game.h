//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "graphics_2.h"
#include "assets.h"
#include "audio.h"
#include "world.h"

#define CAMERA_FIELD_OF_VIEW PI*0.3f
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 100.0f

struct camera
{
    v3f Velocity;
    v3f Position;    
    v3f FocalPoint;
    m3 Orientation;
    v3f AngularVelocity;
    f32 Distance;
};

struct game
{
    b32 Initialized;
    assets* Assets;
    audio* Audio;
    input* Input;  
    camera Camera;
    
    arena GameStorage;
    
    f32 dt;
    u32 CurrentWorldIndex;
    
    world Worlds[2];        
    f32 PlayerRadius;
    f32 PlayerHeight;
};

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform, void* DevContext)
typedef GAME_TICK(game_tick);

GAME_TICK(Game_TickStub)
{
}


#include "dev_world_game.h"
