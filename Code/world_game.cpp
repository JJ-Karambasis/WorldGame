#include "world_game.h"

extern "C"
EXPORT GAME_TICK(Tick)
{           
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    if(!Game->Initialized)
    {
        Game->Initialized = true;
    }        
}