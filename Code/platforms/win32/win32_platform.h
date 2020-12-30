#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include <game.h>
#define GAME_NAME_DLL AK_Cat(GAME_NAME, ".dll")
#define GAME_NAME_TEMP_ADLL AK_Cat(AK_Cat(GAME_NAME, "_tmp"), ".dll")

struct win32_hot_reloaded_library
{
    HMODULE Library;
    FILETIME LastWriteTime;
};

struct win32_platform : public platform
{
    ak_arena*  Arena;
    ak_window* Window;
    
    ak_string GameDLLPathName;
    ak_string GameTempDLLPathName;
    
    win32_hot_reloaded_library GameLibrary;
    win32_hot_reloaded_library WorldLibrary;
};

#ifdef DEV_EDITOR

#include <editor.h>
#define Dev_WindowProc(Message) Win32_DevWindowProc(Message) 

struct win32_dev_platform : public dev_platform
{
    
};
#else
#define Dev_WindowProc(Message)
#endif

#endif
