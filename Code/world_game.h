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
#include "animation.h"
#include "world.h"

struct goal_rect
{
    u32 WorldIndex;
    rect3D Rect;
    b32 GoalIsMet;
};

#define PUZZLE_COMPLETE_CALLBACK(name) void name(struct game* Game, void* UserData)
typedef PUZZLE_COMPLETE_CALLBACK(puzzle_complete_callback);

struct block_puzzle
{
    u32 GoalRectCount;
    u32 BlockEntityCount;
    goal_rect* GoalRects;    
    world_entity_id* BlockEntities;                
    
    b32 IsComplete;
    
    void* CompleteData;
    puzzle_complete_callback* CompleteCallback;    
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
    block_puzzle TestPuzzle;
};

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform, void* DevContext)
typedef GAME_TICK(game_tick);

GAME_TICK(Game_TickStub)
{
}


#include "dev_world_game.h"
