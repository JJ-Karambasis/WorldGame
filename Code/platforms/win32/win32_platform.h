#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include <engine.h>


struct win32_platform
{
    ak_arena* Arena;
    
    ak_string ExecutablePath;
    ak_string EngineDLLPathName;
    
    HMODULE EngineLibrary;
    engine_initialize* Engine_Initialize;
    engine_tick* Engine_Tick;
};

#endif
