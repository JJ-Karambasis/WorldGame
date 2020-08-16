//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include <ak_common.h>

#include "platform.h"
#include "input.h"
#include "graphics.h"
#include "assets/assets.h"
#include "collision_detection.h"
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
    assets Assets;
        
    audio_output* AudioOutput;
    input* Input;  
    graphics_render_buffer* RenderBuffer;    
    
    arena GameStorage;
    
    f32 dt;
    u32 CurrentWorldIndex;
    
    free_list<collision_volume> CollisionVolumeStorage;
    
    world Worlds[2];                
    block_puzzle TestPuzzle;
};

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform, void* DevContext)
typedef GAME_TICK(game_tick);

#define GAME_OUTPUT_SOUND_SAMPLES(name) void name(game* Game, platform* Platform, samples* OutputSamples, arena* TempArena)
typedef GAME_OUTPUT_SOUND_SAMPLES(game_output_sound_samples);

GAME_TICK(Game_TickStub)
{
}

GAME_OUTPUT_SOUND_SAMPLES(Game_OutputSoundSamplesStub)
{    
}

#include "dev_world_game.h"
