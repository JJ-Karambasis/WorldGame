//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include <ak_common.h>

#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 1.3f

struct game;

#include "platform.h"
#include "input.h"
#include "graphics.h"
#include "assets/assets.h"
#include "audio.h"
#include "camera.h"
#include "animation.h"

struct jumping_quad
{
    v3f CenterP;
    v2f Dimensions;    
    jumping_quad* OtherQuad;
};

//#include "world.h"
#include "entity.h"

#include "simulation/simulation.h"

struct goal_rect
{
    u32 WorldIndex;
    rect3D Rect;
    b32 GoalIsMet;
};

#define PUZZLE_COMPLETE_CALLBACK(name) void name(game* Game, void* UserData)
typedef PUZZLE_COMPLETE_CALLBACK(puzzle_complete_callback);

struct block_puzzle
{
    u32 GoalRectCount;
    u32 BlockEntityCount;
    goal_rect* GoalRects;    
    entity_id* BlockEntities;                
    
    b32 IsComplete;
    
    void* CompleteData;
    puzzle_complete_callback* CompleteCallback;    
};

struct game
{            
    arena _Internal_GameStorage_;                        
    arena* GameStorage;    
    u32 CurrentWorldIndex;
    
    //This stuff is probably going to be our level data
    collision_volume_storage CollisionVolumeStorage[2];
    entity_storage EntityStorage[2];    
    sqt* PrevTransforms[2];    
    sqt* CurrentTransforms[2];
    game_camera PrevCameras[2];
    game_camera CurrentCameras[2];
    sim_state* SimStates[2];            
    jumping_quad JumpingQuads[2];                
    //////////////////////////////////////////////////
    
    contact_storage ContactStorage;
    manifold_storage ManifoldStorage;
    
    f32 dt;
    f32 dtFixed;    
    
    assets* Assets;
    audio_output* AudioOutput;
    input* Input;              
    graphics_render_buffer* RenderBuffer;    
};

struct graphics_object
{
    m4 WorldTransform;    
    mesh_asset_id MeshID;    
    material Material;
    
    u32 JointCount;
    m4* JointTransforms;        
};

struct graphics_object_list
{
    graphics_object* Objects;
    u32 Count;
};

struct graphics_state
{
    graphics_object_list GraphicsObjects;
    game_camera Camera;
};

struct graphics_object_list_iter
{
    graphics_object_list* List;
    u32 CurrentIndex;
};

inline graphics_object_list_iter 
BeginIter(graphics_object_list* List)
{
    graphics_object_list_iter Iter = {};
    Iter.List = List;
    return Iter;
}

inline graphics_object* 
GetFirst(graphics_object_list_iter* Iter)
{    
    if(Iter->List->Count == 0)
        return NULL;    
    
    graphics_object* Result = Iter->List->Objects + Iter->CurrentIndex++;        
    return Result;
}

inline graphics_object*
GetNext(graphics_object_list_iter* Iter)
{    
    if(Iter->CurrentIndex >= Iter->List->Count)
        return NULL;
    
    graphics_object* Result = Iter->List->Objects + Iter->CurrentIndex++;
    return Result;
}

#define GAME_INITIALIZE(name) game* name(input* Input, audio_output* AudioOutput, platform* Platform, void* DevContext)
typedef GAME_INITIALIZE(game_initialize);

#define GAME_FIXED_TICK(name) void name(game* Game)
typedef GAME_FIXED_TICK(game_fixed_tick);

#define GAME_TICK(name) void name(game* Game)
typedef GAME_TICK(game_tick);

#define GAME_RENDER(name) void name(game* Game, graphics* Graphics, graphics_state* GraphicsState)
typedef GAME_RENDER(game_render);

#define GAME_OUTPUT_SOUND_SAMPLES(name) void name(game* Game, platform* Platform, samples* OutputSamples, arena* TempArena)
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
