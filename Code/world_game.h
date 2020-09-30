#ifndef WORLD_GAME_H
#define WORLD_GAME_H
//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#if DEVELOPER_BUILD
#define AK_DEVELOPER_BUILD
#endif


#include <ak_common.h>

#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 1.3f
#define PLAYER_MASS 65.0f

struct game;

#include "input.h"
#include "graphics.h"
#include <assets.h>
#include "audio.h"
#include "animation.h"

struct jumping_quad
{
    ak_v3f CenterP;
    ak_v2f Dimensions;    
    jumping_quad* OtherQuad;
};

#include "simulation/simulation.h"
#include "graphics_state.h"
#include "entity.h"
#include "world_file.h"

struct goal_rect
{
    ak_u32 WorldIndex;
    ak_rect3f Rect;
    ak_bool GoalIsMet;
};

#define PUZZLE_COMPLETE_CALLBACK(name) void name(game* Game, void* UserData)
typedef PUZZLE_COMPLETE_CALLBACK(puzzle_complete_callback);

struct block_puzzle
{
    ak_u32 GoalRectCount;
    ak_u32 BlockEntityCount;
    goal_rect* GoalRects;    
    world_id* BlockEntities;                
    
    ak_bool IsComplete;
    
    void* CompleteData;
    puzzle_complete_callback* CompleteCallback;    
};

typedef ak_pool<entity> entity_storage;
typedef ak_pool<pushing_object> pushing_object_storage;

struct world
{
    pushing_object_storage PushingObjectStorage;  
    entity_storage EntityStorage[2];    
    ak_array<ak_sqtf> OldTransforms[2];
    ak_array<ak_sqtf> NewTransforms[2];    
    camera OldCameras[2];    
    camera NewCameras[2];        
    simulation Simulations[2];
    graphics_state GraphicsStates[2];    
};

struct game
{                    
    ak_arena* GameStorage;    
    ak_arena* TempStorage;
    
    ak_u32 CurrentWorldIndex;
    player Players[2];          
    
    world World;
    
    //This stuff is probably going to be our level data        
    jumping_quad JumpingQuads[2];                
    //////////////////////////////////////////////////
    
    ak_v2i Resolution;
    ak_f32 dt;
    ak_f32 dtFixed;    
    
    assets* Assets;
    audio_output* AudioOutput;
    input* Input;                          
};


#define GAME_INITIALIZE(name) game* name(ak_arena* TempStorage, input* Input, audio_output* AudioOutput, ak_string AssetPath)
#define GAME_FIXED_TICK(name) void name(game* Game, void* DevContext)
#define GAME_TICK(name) void name(game* Game, void* DevContext)
#define GAME_RENDER(name) void name(game* Game, graphics* Graphics, ak_f32 tInterpolate, ak_u32 WorldIndex)

typedef GAME_INITIALIZE(game_initialize);
typedef GAME_FIXED_TICK(game_fixed_tick);
typedef GAME_TICK(game_tick);
typedef GAME_RENDER(game_render);

#define GAME_OUTPUT_SOUND_SAMPLES(name) void name(game* Game, samples* Samples)
typedef GAME_OUTPUT_SOUND_SAMPLES(game_output_sound_samples);

GAME_INITIALIZE(Game_InitializeStub)
{
    return NULL;
}

GAME_FIXED_TICK(Game_FixedTickStub)
{    
}

GAME_TICK(Game_TickStub)
{
}

GAME_RENDER(Game_RenderStub)
{
}

GAME_OUTPUT_SOUND_SAMPLES(Game_OutputSoundSamplesStub)
{    
}

#include "dev_tools/dev_tools.h"
#endif