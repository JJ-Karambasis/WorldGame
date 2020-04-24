#ifndef WIN32_PROJECTWORLD_H
#define WIN32_PROJECTWORLD_H

#include <Windows.h>
#include <DSound.h>
#include "world_game.h"

#define BIND_KEY(key, action) case key: { if((RawKeyboard->Flags == RI_KEY_MAKE) || (RawKeyboard->Message == WM_KEYDOWN) || (RawKeyboard->Message == WM_SYSKEYDOWN)) { action.IsDown = true; } else if((RawKeyboard->Flags == RI_KEY_BREAK) || (RawKeyboard->Message == WM_KEYUP) || (RawKeyboard->Message == WM_SYSKEYUP)) { action.IsDown = false; action.WasDown = true; } } break

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );

#define HRESULT_CHECK_AND_HANDLE(check, format, ...) \
do \
{ \
if(FAILED(check)) \
WRITE_AND_HANDLE_ERROR(format, __VA_ARGS__); \
} while(0)

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

struct win32_graphics_code
{
    win32_hot_loaded_library GraphicsLibrary;
    execute_render_commands* ExecuteRenderCommands;
};

struct platform_file_handle
{
    HANDLE Handle;
    platform_file_attributes Attributes;
};

struct win32_audio : public audio
{
    IDirectSoundBuffer* SoundBuffer;
    u32 RunningSampleIndex;
};

#endif