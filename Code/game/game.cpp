#include "game.h"

#include <assets.cpp>
#include <src/graphics_state.cpp>

extern "C"
AK_EXPORT GAME_STARTUP(Game_Startup)
{
    game* Game = (game*)AK_Allocate(sizeof(game));
    if(!Game)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Game->Scratch = AK_CreateArena(AK_Megabyte(4));
    Game->Graphics = Graphics;
    Game->Assets = Assets;
    Game->dtFixed = dtFixed;
    
    Game->Update = Game_Update;
    Game->Shutdown = Game_Shutdown;
    
    return Game;
}

extern "C"
AK_EXPORT GAME_UPDATE(Game_Update)
{
    Game->World->Update(Game);
}

extern "C"
AK_EXPORT GAME_SHUTDOWN(Game_Shutdown)
{
    Game->World->Shutdown(Game);
    AK_DeleteArena(Game->Scratch);
    AK_Free(Game);
}

#include "game_common_source.cpp"

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>