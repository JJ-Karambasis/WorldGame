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

struct game;

#include "input.h"
#include "graphics.h"
#include "assets/assets.h"
#include "audio.h"
#include "camera.h"
#include "animation.h"

struct jumping_quad
{
    ak_v3f CenterP;
    ak_v2f Dimensions;    
    jumping_quad* OtherQuad;
};

#include "simulation/simulation.h"
#include "entity.h"
#include "world_loader.h"

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
    entity_id* BlockEntities;                
    
    ak_bool IsComplete;
    
    void* CompleteData;
    puzzle_complete_callback* CompleteCallback;    
};

typedef ak_pool<entity> entity_storage;
typedef ak_pool<pushing_object> pushing_object_storage;

struct game
{                    
    ak_arena* GameStorage;    
    ak_arena* TempStorage;
    
    ak_u32 CurrentWorldIndex;
    player Players[2];    
    pushing_object_storage PushingObjectStorage;    
    
    //This stuff is probably going to be our level data    
    entity_storage EntityStorage[2];    
    ak_array<ak_sqtf> PrevTransforms[2];
    ak_array<ak_sqtf> CurrentTransforms[2];    
    game_camera PrevCameras[2];
    game_camera CurrentCameras[2];    
    jumping_quad JumpingQuads[2];                
    //////////////////////////////////////////////////
    
    simulation Simulations[2];
    
    ak_f32 dt;
    ak_f32 dtFixed;    
    
    assets* Assets;
    audio_output* AudioOutput;
    input* Input;              
    graphics_render_buffer* RenderBuffer;            
    
    ak_bool NoInterpolation;
};

struct graphics_object
{
    ak_m4f WorldTransform;    
    mesh_asset_id MeshID;    
    material Material;
    
    ak_u32 JointCount;
    ak_m4f* JointTransforms;        
};

struct graphics_object_list
{
    graphics_object* Objects;
    ak_u32 Count;
};

struct graphics_state
{
    graphics_object_list GraphicsObjects;
    game_camera Camera;
};

struct graphics_object_list_iter
{
    graphics_object_list* List;
    ak_u32 CurrentIndex;
    
    inline graphics_object* First()
    {
        if(List->Count == 0)
            return NULL;    
        
        graphics_object* Result = List->Objects + CurrentIndex++;        
        return Result;
    }
    
    inline graphics_object* Next()
    {
        if(CurrentIndex >= List->Count)
            return NULL;
        
        graphics_object* Result = List->Objects + CurrentIndex++;
        return Result;
    }
};

inline graphics_object_list_iter 
AK_BeginIter(graphics_object_list* List)
{
    graphics_object_list_iter Iter = {};
    Iter.List = List;
    return Iter;
}

inline simulation*
GetSimulation(game* Game, ak_u32 WorldIndex)
{
    simulation* Simulation = Game->Simulations + WorldIndex;
    return Simulation;
}

inline simulation*
GetSimulation(game* Game, entity_id ID)
{
    simulation* Simulation = GetSimulation(Game, ID.WorldIndex);
    return Simulation;
}

#if DEVELOPER_BUILD
#define GAME_INITIALIZE(name) game* name(ak_arena* TempStorage, input* Input, audio_output* AudioOutput, ak_string AssetPath, void* DevContext)
#define GAME_FIXED_TICK(name) void name(game* Game, void* DevContext)
#define GAME_TICK(name) void name(game* Game, void* DevContext)
#else
#define GAME_INITIALIZE(name) game* name(input* Input, audio_output* AudioOutput, ak_string AssetPath, error_stream* ErrorStream)
#define GAME_FIXED_TICK(name) void name(game* Game)
#define GAME_TICK(name) void name(game* Game)
#endif
typedef GAME_INITIALIZE(game_initialize);
typedef GAME_FIXED_TICK(game_fixed_tick);
typedef GAME_TICK(game_tick);

#define GAME_RENDER(name) void name(game* Game, graphics* Graphics, graphics_state* GraphicsState)
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

#include "dev_world_game.h"
#endif