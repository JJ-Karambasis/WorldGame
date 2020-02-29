#ifndef WIN32_PROJECTWORLD_H
#define WIN32_PROJECTWORLD_H

#include <Windows.h>
#include "world_game.h"

#define BIND_KEY(key, action) case key: { if((RawKeyboard->Flags == RI_KEY_MAKE) || (RawKeyboard->Message == WM_KEYDOWN) || (RawKeyboard->Message == WM_SYSKEYDOWN)) { action.IsDown = true; } else if((RawKeyboard->Flags == RI_KEY_BREAK) || (RawKeyboard->Message == WM_KEYUP) || (RawKeyboard->Message == WM_SYSKEYUP)) { action.IsDown = false; action.WasDown = true; } } break

#define RELEASE(iunknown) \
do \
{ \
if(iunknown) \
{ \
iunknown->Release(); \
iunknown = NULL; \
} \
} while(0)

struct win32_hot_loaded_library
{
    HMODULE Library;
    FILETIME LastWriteTime;
};

struct win32_game_code
{
    win32_hot_loaded_library GameLibrary;
    game_tick* Tick;        
};

struct platform_file_handle
{
    HANDLE Handle;
    platform_file_attributes Attributes;
};

#endif