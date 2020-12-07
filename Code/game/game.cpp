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

extern "C" 
AK_EXPORT GAME_UPDATE(Game_Update)
{
    world_game* WorldGame = (world_game*)Game;
}

extern "C"
AK_EXPORT GAME_GET_GRAPHICS_STATE(Game_GetGraphicsState)
{
    world_game* WorldGame = (world_game*)Game;
    
    graphics_state GraphicsState = {};
    return GraphicsState;
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>