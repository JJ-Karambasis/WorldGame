#include "engine.h"

extern "C"
AK_EXPORT ENGINE_INITIALIZE(Engine_Initialize)
{
    engine* Engine = (engine*)AK_Allocate(sizeof(engine));
    if(!Engine)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Engine->Scratch = AK_CreateArena(AK_Megabyte(4));
    return Engine;
}

extern "C"
AK_EXPORT ENGINE_TICK(Engine_Tick)
{
    ak_temp_arena TempArena = Engine->Scratch->BeginTemp();
    Engine->Game->Tick(Engine->Game);
    Engine->Scratch->EndTemp(&TempArena);
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>