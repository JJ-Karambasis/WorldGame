#include "win32_platform.h"

#define BindKey(key, action) case key: { action.IsDown = IsDown; action.WasDown = WasDown; } break

global win32_platform Global_Platform;
global init_graphics* InitGraphics;

ak_i32 Win32_ProcessMessage(input* Input, MSG* pMessage)
{
    ak_bool GotMessage = false;
    
    DWORD SkipMessages[] = 
    {
        0x738, 0xFFFFFFFF
    };
    
    MSG Message = {};
    
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
        return 0;
    
    switch(Message.message)
    {
        case WM_QUIT:
        {                    
            return -1;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            DWORD VKCode = (DWORD)Message.wParam;
            
            ak_bool WasDown = ((Message.lParam & (1 << 30)) != 0);
            ak_bool IsDown = ((Message.lParam & (1UL << 31)) == 0);
            if(Input && WasDown != IsDown)
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
        } break;
        
        default:
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        } break;
    }
    
    if(pMessage) *pMessage = Message;
    
    return 1;
}

PLATFORM_PROCESS_MESSAGES(Win32_ProcessMessages)
{
    MSG Message = {};
    for(;;)
    {
        ak_i32 Status = Win32_ProcessMessage(Input, NULL);
        if(Status == 0) return true;
        else if(Status == -1) return false;
    }
}

PLATFORM_LOAD_GAME_CODE(Win32_LoadGameCode)
{
    Global_Platform.GameLibrary = LoadLibrary(Global_Platform.GameDLLPathName.Data);
    if(!Global_Platform.GameLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    game_startup* Startup = (game_startup*)GetProcAddress(Global_Platform.GameLibrary, "Game_Startup");
    if(!Startup)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    return Startup;
}

PLATFORM_LOAD_WORLD_CODE(Win32_LoadWorldCode)
{
    ak_temp_arena TempArena = Global_Platform.Arena->BeginTemp();
    
    ak_string WorldDLLPath = AK_FormatString(Global_Platform.Arena, "%.*s\\lib\\%.*s.dll", 
                                             WorldPath.Length, WorldPath.Data, 
                                             WorldName.Length, WorldName.Data);
    
    Global_Platform.WorldLibrary = LoadLibrary(WorldDLLPath.Data);
    if(!Global_Platform.WorldLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        Global_Platform.Arena->EndTemp(&TempArena);
        return NULL;
    }
    
    ak_string StartupCode = AK_StringConcat(WorldName, "_Startup", Global_Platform.Arena);
    world_startup* Startup = (world_startup*)GetProcAddress(Global_Platform.WorldLibrary, StartupCode.Data);
    
    if(!Startup)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        Global_Platform.Arena->EndTemp(&TempArena);
        return NULL;
    }
    
    Global_Platform.Arena->EndTemp(&TempArena);
    return Startup;
}

PLATFORM_UNLOAD_CODE(Win32_UnloadWorldCode)
{
    if(Global_Platform.WorldLibrary)
    {
        FreeLibrary(Global_Platform.WorldLibrary);
        Global_Platform.WorldLibrary = NULL;
    }
}


PLATFORM_UNLOAD_CODE(Win32_UnloadGameCode)
{
    if(Global_Platform.GameLibrary)
    {
        FreeLibrary(Global_Platform.GameLibrary);
        Global_Platform.GameLibrary = NULL;
    }
}

PLATFORM_GET_RESOLUTION(Win32_GetResolution)
{
    ak_v2i Result = {};
    AK_GetWindowResolution(Global_Platform.Window, (ak_u16*)&Result.w, (ak_u16*)&Result.h);
    return Result;
}

ak_bool Win32_InitPlatform()
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
    Global_Platform.UnloadGameCode = Win32_UnloadGameCode;
    Global_Platform.LoadWorldCode = Win32_LoadWorldCode;
    Global_Platform.UnloadWorldCode = Win32_UnloadWorldCode;
    Global_Platform.GetResolution = Win32_GetResolution;
    
    Global_Platform.ProgramPath = AK_GetExecutablePath(Global_Platform.Arena);
    Global_Platform.EngineDLLPathName = AK_StringConcat(Global_Platform.ProgramPath, "Engine.dll", Global_Platform.Arena);
    Global_Platform.GameDLLPathName = AK_StringConcat(Global_Platform.ProgramPath, GAME_NAME_DLL, Global_Platform.Arena);
    Global_Platform.AssetPath = AK_StringConcat(Global_Platform.ProgramPath, "WorldGame.assets", Global_Platform.Arena);
    
    Global_Platform.Window = AK_CreateWindow(1280, 720, GAME_NAME);
    if(!Global_Platform.Window)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    ak_string GraphicsDLLPathName = AK_StringConcat(Global_Platform.ProgramPath, "OpenGL.dll", Global_Platform.Arena);
    HMODULE GraphicsLibrary = LoadLibrary(GraphicsDLLPathName.Data);
    if(!GraphicsLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    InitGraphics = (init_graphics*)GetProcAddress(GraphicsLibrary, "InitGraphics");
    execute_render_commands* ExecuteRenderCommands = (execute_render_commands*)GetProcAddress(GraphicsLibrary, "ExecuteRenderCommands");
    if(!InitGraphics || !ExecuteRenderCommands)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Global_Platform.ExecuteRenderCommands = ExecuteRenderCommands;
    
    return true;
}

#if 0
int Win32_GameMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{
    if(!Win32_InitPlatform())
        return -1;
    
    void* PlatformData[2] = 
    {
        Global_Platform.Window,
        Instance
    };
    
    graphics* Graphics = InitGraphics(PlatformData);
    
    return EngineCode.EngineRun(&Global_Platform, Graphics);
}
#endif

#ifdef DEV_EDITOR
#include <imgui.h>

#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_demo.cpp>

global win32_dev_platform Global_DevPlatform;

DEV_PLATFORM_UPDATE(Win32_DevUpdate)
{
    ImGuiIO* IO = &ImGui::GetIO();
    
    HWND Window = (HWND)IO->ImeWindowHandle;
    
    RECT Rect;
    GetClientRect(Window, &Rect);
    
    IO->DisplaySize = ImVec2((ak_f32)(Rect.right-Rect.left), (ak_f32)(Rect.bottom-Rect.top));
    IO->DeltaTime = dt;
    IO->KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    IO->KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    IO->KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if(IO->WantSetMousePos)
    {
        ak_v2i Position = AK_V2((ak_i32)IO->MousePos.x, (ak_i32)IO->MousePos.y);
        ClientToScreen(Window, (POINT*)&Position);
        SetCursorPos(Position.x, Position.y);
    }
    
    dev_input* DevInput = &Editor->Input;
    
    IO->MousePos = ImVec2(-AK_MAX32, -AK_MAX32);
    POINT MousePosition;
    if(HWND ActiveWindow = GetForegroundWindow())
    {
        if(ActiveWindow == Window || IsChild(ActiveWindow, Window))
        {
            if(GetCursorPos(&MousePosition) && ScreenToClient(Window, &MousePosition))
            {                
                IO->MousePos = ImVec2((ak_f32)MousePosition.x, (ak_f32)MousePosition.y);                
                DevInput->MouseCoordinates = AK_V2((ak_i32)MousePosition.x, (ak_i32)MousePosition.y);
            }
        }
    }    
    
    for(;;)
    {
        MSG Message = {};
        
        input* Input = Editor->Game ? &Editor->Game->Input : NULL;
        ak_i32 Status = Win32_ProcessMessage(Input, &Message);
        if(Status == -1) return false;
        else if(Status == 0) break;
        
        if((Message.message == WM_SYSKEYDOWN) || (Message.message == WM_KEYDOWN) ||
           (Message.message == WM_SYSKEYUP) || (Message.message == WM_KEYUP))
        {
            
            DWORD VKCode = (DWORD)Message.wParam;    
            ak_bool WasDown = ((Message.lParam & (1 << 30)) != 0);
            ak_bool IsDown = ((Message.lParam & (1UL << 31)) == 0);
            
            if(WasDown != IsDown)
            {
                switch(VKCode)
                {
                    BindKey(VK_F5, DevInput->ToggleDevState);
                    BindKey(VK_MENU, DevInput->Alt);  
                    BindKey('W', DevInput->W);
                    BindKey('E', DevInput->E);
                    BindKey('R', DevInput->R);      
                    BindKey('S', DevInput->S);
                    BindKey('F', DevInput->F);
                    BindKey('Q', DevInput->Q);
                    BindKey('N', DevInput->N);
                    BindKey('D', DevInput->D);
                    BindKey('L', DevInput->L);
                    BindKey('Z', DevInput->Z);
                    BindKey('Y', DevInput->Y);
                    BindKey(VK_DELETE, DevInput->Delete);
                    BindKey(VK_CONTROL, DevInput->Ctrl);
                }
            }
        }
        
        switch(Message.message)
        {
            case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
            {
                int button = 0;
                if (Message.message == WM_LBUTTONDOWN || Message.message == WM_LBUTTONDBLCLK) { button = 0; }
                if (Message.message == WM_RBUTTONDOWN || Message.message == WM_RBUTTONDBLCLK) { button = 1; }
                if (Message.message == WM_MBUTTONDOWN || Message.message == WM_MBUTTONDBLCLK) { button = 2; }
                if (Message.message == WM_XBUTTONDOWN || Message.message == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(Message.wParam) == XBUTTON1) ? 3 : 4; }
                if (!ImGui::IsAnyMouseDown() && GetCapture() == NULL)
                    SetCapture(Message.hwnd);
                IO->MouseDown[button] = true;                    
            } break;
            
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                ak_i32 Button = 0;
                if (Message.message == WM_LBUTTONUP) { Button = 0; }
                if (Message.message == WM_RBUTTONUP) { Button = 1; }
                if (Message.message == WM_MBUTTONUP) { Button = 2; }
                if (Message.message == WM_XBUTTONUP) { Button = (GET_XBUTTON_WPARAM(Message.wParam) == XBUTTON1) ? 3 : 4; }
                IO->MouseDown[Button] = false;
                if (!ImGui::IsAnyMouseDown() && GetCapture() == Message.hwnd)
                    ReleaseCapture();                                        
            } break;
            
            case WM_MOUSEWHEEL:
            {
                ak_f32 Scroll = (ak_f32)GET_WHEEL_DELTA_WPARAM(Message.wParam) / (ak_f32)WHEEL_DELTA;
                IO->MouseWheel += Scroll;                     
                DevInput->Scroll = Scroll;
            } break;
            
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                if (Message.wParam < 256)                    
                    IO->KeysDown[Message.wParam] = 1;                    
                
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }break;
            
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                if (Message.wParam < 256)
                    IO->KeysDown[Message.wParam] = 0;                                                        
            } break;                
            
            case WM_CHAR:
            {
                // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                if (Message.wParam > 0 && Message.wParam < 0x10000)
                    IO->AddInputCharacterUTF16((unsigned short)Message.wParam);
            } break;  
        }
    }
    
#define Dev_BindMouse(key, action) do \
{ \
ak_bool IsDown = GetKeyState(key) & (1 << 15); \
if(IsDown != action.IsDown) \
{ \
if(IsDown == false) \
{ \
action.WasDown = true; \
action.IsDown = false; \
} \
else \
{ \
action.IsDown = true; \
} \
} \
} while(0)
    
    Dev_BindMouse(VK_LBUTTON, DevInput->LMB);
    Dev_BindMouse(VK_MBUTTON, DevInput->MMB);
    
    return true;
}

DEV_BUILD_WORLD(Win32_BuildWorld)
{
    ak_high_res_clock Start = AK_WallClock();
    STARTUPINFOA StartupInfo = {};
    StartupInfo.cb = sizeof(STARTUPINFOA);
    
    PROCESS_INFORMATION ProcessInfo = {};
    ak_temp_arena TempArena = Global_Platform.Arena->BeginTemp();
    
    ak_string BatFilePath = AK_StringConcat(WorldPath, "build.bat", Global_Platform.Arena);
    
    ak_char* Command = AK_FormatString(Global_Platform.Arena, "cmd.exe /C %.*s >> %s", BatFilePath.Length, BatFilePath.Data, BUILD_WORLD_LOG_FILE).Data; 
    if(!CreateProcess(NULL, Command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, WorldPath.Data, &StartupInfo, &ProcessInfo))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        Global_Platform.Arena->EndTemp(&TempArena);
        return false;
    }
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    
    DWORD ExitCode;
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
    
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
    Global_Platform.Arena->EndTemp(&TempArena);
    return ExitCode == 0;
}

DEV_SET_GAME_DEBUG_EDITOR(Win32_SetGameDebugEditor)
{
    editor** OutEditor = (editor**)GetProcAddress(Global_Platform.GameLibrary, "Internal__Editor");
    if(OutEditor)
    {
        *OutEditor = Editor;
        return true;
    }
    return false;
}

int Win32_EditorMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{
    if(!Win32_InitPlatform())
        return -1;
    
    
    void* PlatformData[2] = 
    {
        Global_Platform.Window,
        Instance
    };
    
    IMGUI_CHECKVERSION();
    
    
    graphics* Graphics = InitGraphics(PlatformData);
    
    ImGuiContext* Context = ImGui::CreateContext();
    
    ImGuiIO* IO = &ImGui::GetIO();
    IO->BackendFlags |= (ImGuiBackendFlags_RendererHasVtxOffset |
                         ImGuiBackendFlags_HasMouseCursors|ImGuiBackendFlags_HasSetMousePos);
    IO->BackendPlatformName = "win32";
    IO->ImeWindowHandle = AK_GetPlatformWindow(Global_Platform.Window);
    IO->KeyMap[ImGuiKey_Tab] = VK_TAB;
    IO->KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    IO->KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    IO->KeyMap[ImGuiKey_UpArrow] = VK_UP;
    IO->KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    IO->KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    IO->KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    IO->KeyMap[ImGuiKey_Home] = VK_HOME;
    IO->KeyMap[ImGuiKey_End] = VK_END;
    IO->KeyMap[ImGuiKey_Insert] = VK_INSERT;
    IO->KeyMap[ImGuiKey_Delete] = VK_DELETE;
    IO->KeyMap[ImGuiKey_Backspace] = VK_BACK;
    IO->KeyMap[ImGuiKey_Space] = VK_SPACE;
    IO->KeyMap[ImGuiKey_Enter] = VK_RETURN;
    IO->KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    IO->KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    IO->KeyMap[ImGuiKey_A] = 'A';
    IO->KeyMap[ImGuiKey_C] = 'C';
    IO->KeyMap[ImGuiKey_V] = 'V';
    IO->KeyMap[ImGuiKey_X] = 'X';
    IO->KeyMap[ImGuiKey_Y] = 'Y';
    IO->KeyMap[ImGuiKey_Z] = 'Z';    
    
    ak_string EditorDLLPathName = AK_StringConcat(Global_Platform.ProgramPath, "Editor.dll", Global_Platform.Arena);
    HMODULE EditorDLL = LoadLibrary(EditorDLLPathName.Data);
    if(!EditorDLL)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    editor_run* Editor_Run = (editor_run*)GetProcAddress(EditorDLL, "Editor_Run");
    if(!Editor_Run)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    Global_DevPlatform.Update = Win32_DevUpdate;
    Global_DevPlatform.BuildWorld = Win32_BuildWorld;
    Global_DevPlatform.SetGameDebugEditor = Win32_SetGameDebugEditor;
    
    return Editor_Run(Graphics, &Global_Platform, &Global_DevPlatform, Context);
}

#endif

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{            
    int Result;
    __try
    {        
#ifdef DEV_EDITOR
        Result = Win32_EditorMain(Instance, PrevInstance, CmdLineArgs, CmdLineOpts);
#else
        Result = Win32_GameMain(Instance, PrevInstance, CmdLineArgs, CmdLineOpts);
#endif
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