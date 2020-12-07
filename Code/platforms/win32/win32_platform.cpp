#include "win32_platform.h"

ak_bool
Win32_InitPlatform(win32_platform* Platform)
{
    Platform->Arena = AK_CreateArena(AK_Megabyte(1));
    if(!Platform->Arena)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Platform->ExecutablePath = AK_GetExecutablePath(Platform->Arena);
    Platform->EngineDLLPathName = AK_StringConcat(Platform->ExecutablePath, "Engine.dll", Platform->Arena);
    
    Platform->EngineLibrary = LoadLibrary(Platform->EngineDLLPathName.Data);
    if(!Platform->EngineLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Platform->Engine_Initialize = (engine_initialize*)GetProcAddress(Platform->EngineLibrary, "Engine_Initialize");
    Platform->Engine_Tick = (engine_tick*)GetProcAddress(Platform->EngineLibrary, "Engine_Tick");
    
    if(!Platform->Engine_Initialize || !Platform->Engine_Tick)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    return true;
}

int Win32_Main(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{
    win32_platform Platform = {};
    if(!Win32_InitPlatform(&Platform))
        return -1;
    
    ak_window* PlatformWindow = AK_CreateWindow(1920, 1080, AK_Stringify(GAME_NAME));
    if(!PlatformWindow)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    for(;;)
    {
        MSG Message;
        while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
                    return 0;
                } break;
                
                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }
    }
    
    //return -1;
}


int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{            
    int Result;
    __try
    {        
        Result = Win32_Main(Instance, PrevInstance, CmdLineArgs, CmdLineOpts);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {        
        //TODO(JJ): When errors occur (like failed assertions or any unhandled exceptions occur) lets output the last set of frames (about 30 seconds worth) 
        //to a file so we can playback later
        Result = -1;
    }
    
    return Result;
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>