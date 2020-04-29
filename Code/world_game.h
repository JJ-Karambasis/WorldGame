//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "graphics.h"
#include "assets.h"
#include "audio.h"
#include "camera.h"
#include "world.h"

struct game
{
    b32 Initialized;
    assets* Assets;
    audio* Audio;
    input* Input;  
    
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
