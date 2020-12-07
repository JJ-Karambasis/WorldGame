#include "win32_platform.h"

#define BindKey(key, action) case key: { action.IsDown = IsDown; action.WasDown = WasDown; } break

global win32_platform Global_Platform;

PLATFORM_PROCESS_MESSAGES(Win32_ProcessMessages)
{
    MSG Message = {};
    for(;;)
    {
        ak_bool GotMessage = false;
        
        DWORD SkipMessages[] = 
        {
            0x738, 0xFFFFFFFF
        };
        
        DWORD LastMessage = 0;
        for(ak_u32 SkipIndex = 0;
            SkipIndex < AK_Count(SkipMessages);
            ++SkipIndex)
        {
            
            DWORD Skip = SkipMessages[SkipIndex];
            GotMessage = PeekMessage(&Message, 0, LastMessage, Skip - 1, PM_REMOVE);
            if(GotMessage)
            {
                break;
            }
            
            LastMessage = Skip + 1;
        }
        
        if(!GotMessage)
            return true;
        
        //Dev_WindowProc(Message);
        
        switch(Message.message)
        {
            case WM_QUIT:
            {                    
                return false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                DWORD VKCode = (DWORD)Message.wParam;
                
                ak_bool WasDown = ((Message.lParam & (1 << 30)) != 0);
                ak_bool IsDown = ((Message.lParam & (1UL << 31)) == 0);
                if(WasDown != IsDown)
                {
                    switch(VKCode)
                    {
                        BindKey('W', Input->MoveForward);
                        BindKey('S', Input->MoveBackward);
                        BindKey('A', Input->MoveLeft);
                        BindKey('D', Input->MoveRight);                            
                        BindKey('Q', Input->SwitchWorld);                                
                        BindKey(VK_SPACE, Input->Action);
                    }
                }
                
                if(((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) && (VKCode == VK_F4) && IsDown)
                    PostQuitMessage(0);
                
                //Dev_HandleKeyboard(Message);                            
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

PLATFORM_LOAD_GAME_CODE(Win32_LoadGameCode)
{
    Global_Platform.GameLibrary = LoadLibrary(Global_Platform.GameDLLPathName.Data);
    if(!Global_Platform.GameLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    GameCode->Startup = (game_startup*)GetProcAddress(Global_Platform.GameLibrary, "Game_Startup");
    GameCode->Update = (game_update*)GetProcAddress(Global_Platform.GameLibrary, "Game_Update");
    GameCode->GetGraphicsState = (game_get_graphics_state*)GetProcAddress(Global_Platform.GameLibrary, "Game_GetGraphicsState");
    
    if(!GameCode->Startup || !GameCode->Update)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    return true;
}

PLATFORM_GET_RESOLUTION(Win32_GetResolution)
{
    ak_v2i Result = {};
    AK_GetWindowResolution(Global_Platform.Window, (ak_u16*)&Result.w, (ak_u16*)&Result.h);
    return Result;
}

int Win32_Main(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{
    Global_Platform.Arena = AK_CreateArena(AK_Megabyte(1));
    if(!Global_Platform.Arena)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Global_Platform.ProcessMessages = Win32_ProcessMessages;
    Global_Platform.LoadGameCode = Win32_LoadGameCode;
    Global_Platform.GetResolution = Win32_GetResolution;
    
    Global_Platform.ExecutablePath = AK_GetExecutablePath(Global_Platform.Arena);
    Global_Platform.EngineDLLPathName = AK_StringConcat(Global_Platform.ExecutablePath, "Engine.dll", Global_Platform.Arena);
    Global_Platform.GameDLLPathName = AK_StringConcat(Global_Platform.ExecutablePath, GAME_NAME_DLL, Global_Platform.Arena);
    Global_Platform.AssetPath = AK_StringConcat(Global_Platform.ExecutablePath, "WorldGame.assets", Global_Platform.Arena);
    
    HMODULE EngineLibrary = LoadLibrary(Global_Platform.EngineDLLPathName.Data);
    if(!EngineLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    engine_run* Engine_Run = (engine_run*)GetProcAddress(EngineLibrary, "Engine_Run");
    if(!Engine_Run)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    
    Global_Platform.Window = AK_CreateWindow(1920, 1080, AK_Stringify(GAME_NAME));
    if(!Global_Platform.Window)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    ak_string GraphicsDLLPathName = AK_StringConcat(Global_Platform.ExecutablePath, "OpenGL.dll", Global_Platform.Arena);
    HMODULE GraphicsLibrary = LoadLibrary(GraphicsDLLPathName.Data);
    if(!GraphicsLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    init_graphics* InitGraphics = (init_graphics*)GetProcAddress(GraphicsLibrary, "InitGraphics");
    execute_render_commands* ExecuteRenderCommands = (execute_render_commands*)GetProcAddress(GraphicsLibrary, "ExecuteRenderCommands");
    if(!InitGraphics || !ExecuteRenderCommands)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    void* PlatformData[2] = 
    {
        AK_GetPlatformWindow(Global_Platform.Window),
        Instance
    };
    
    graphics* Graphics = InitGraphics(PlatformData);
    if(!Graphics)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Global_Platform.ExecuteRenderCommands = ExecuteRenderCommands;
    return Engine_Run(&Global_Platform, Graphics);
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