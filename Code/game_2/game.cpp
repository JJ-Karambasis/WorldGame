#include "game.h"

GAME_UPDATE(Game_Tick);

extern "C"
AK_EXPORT GAME_STARTUP(Game_Startup)
{
    world_game* Game = (world_game*)AK_Allocate(sizeof(world_game));
    if(!Game)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Game->Engine = Engine;
    Engine->Game = Game;
    
    return true;
}

void ResizeWorld(world* World)
{
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<entity>* EntityStorage = World->EntityStorage + WorldIndex;
        
        if(World->OldTransforms[WorldIndex].Size < EntityStorage->Capacity)
            World->OldTransforms[WorldIndex].Resize(EntityStorage->Capacity);
        
        if(World->PhysicObjects[WorldIndex].Size < EntityStorage->Capacity)
            World->PhysicObjects[WorldIndex].Resize(EntityStorage->Capacity);
    }
}

extern "C" 
AK_EXPORT GAME_UPDATE(Game_Update)
{
    world_game* WorldGame = (world_game*)Game;
    ResizeWorld(WorldGame->World);
    
    Game->UpdateWorld(Game);
}

extern "C"
AK_EXPORT GAME_GET_GRAPHICS_STATE(Game_GetGraphicsState)
{
    world_game* WorldGame = (world_game*)Game;
    
    graphics_state GraphicsState = {};
    return GraphicsState;
}

AK_EXPORT GAME_SHUTDOWN(Game_Shutdown)
{
    engine* Engine = Game->Engine;
    AK_Free(Game);
    Engine->Game = NULL;
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>

#include "worlds/world_generated.cpp"