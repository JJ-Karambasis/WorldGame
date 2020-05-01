//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "AKCommon/common.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "graphics.h"
#include "assets.h"
#include "audio.h"
#include "camera.h"
#include "world.h"

struct block_puzzle_goal
{
    
};

struct block_puzzle
{
    u32 GoalRectCount;
    u32 BlockEntityCount;
    rect3D_center_dim* GoalRects;
    world_entity_id* BlockEntities;
};

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
    
    block_puzzle TestPuzzle;
};

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform, void* DevContext)
typedef GAME_TICK(game_tick);

GAME_TICK(Game_TickStub)
{
}


#include "dev_world_game.h"
