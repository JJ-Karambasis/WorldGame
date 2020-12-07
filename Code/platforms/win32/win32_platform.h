#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include <engine.h>

#define GAME_NAME_DLL AK_Cat(GAME_NAME, ".dll")

struct win32_platform : public platform
{
    ak_arena*  Arena;
    ak_window* Window;
    
    ak_string ExecutablePath;
    ak_string EngineDLLPathName;
    ak_string GameDLLPathName;
    
    HMODULE EngineLibrary;
    HMODULE GameLibrary;
};

#endif
