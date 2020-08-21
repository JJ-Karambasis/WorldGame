#ifndef WIN32_PROJECTWORLD_H
#define WIN32_PROJECTWORLD_H

#include <Windows.h>
#include <excpt.h>
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
    
    game_initialize* Initialize;
    game_fixed_tick* FixedTick;
    game_tick* Tick;            
    game_render* Render;
    game_output_sound_samples* OutputSoundSamples;    
};

struct win32_graphics_code
{    
    win32_hot_loaded_library GraphicsLibrary;
    execute_render_commands* ExecuteRenderCommands;
    bind_graphics_functions* BindGraphicsFunctions;    
    init_graphics* InitGraphics;
    invalidate_shaders* InvalidateShaders;    
};

struct win32_audio_output : public audio_output
{
    IDirectSoundBuffer* SoundBuffer;
    u32 RunningSampleIndex;
    samples Samples;    
};

#endif