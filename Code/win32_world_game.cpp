#include "win32_world_game.h"

global LARGE_INTEGER Global_Frequency;

inline u64 
Win32_Clock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result.QuadPart;
}

inline f64
Win32_ToSeconds(u64 Time)
{
    f64 Result = ((f64)Time/(f64)Global_Frequency.QuadPart);
    return Result;
}

inline f64
Win32_Elapsed(u64 End, u64 Begin)
{
    f64 Result = Win32_ToSeconds(End-Begin);
    return Result;
}

internal LRESULT CALLBACK 
Win32_WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {        
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;                
    }
    
    return Result;
}

HWND Win32_CreateWindow(WNDCLASSEX* WindowClass, char* WindowName,  
                        v2i WindowDim)
{    
    DWORD ExStyle = 0;
    DWORD Style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    
    RECT WindowRect = {0, 0, (LONG)WindowDim.width, (LONG)WindowDim.height};
    AdjustWindowRectEx(&WindowRect, Style, FALSE, ExStyle);
    
    HWND Window = CreateWindowEx(ExStyle, WindowClass->lpszClassName, "AKEngine", Style, 
                                 CW_USEDEFAULT, CW_USEDEFAULT, 
                                 WindowRect.right-WindowRect.left,
                                 WindowRect.bottom-WindowRect.top,
                                 0, 0, WindowClass->hInstance, NULL);
    if(!Window)            
        return {};        
        
    return Window;
}

inline v2i 
Win32_GetWindowDim(HWND Window)
{
    v2i Result = {};    
    RECT Rect;
    if(GetClientRect(Window, &Rect))    
        Result = V2i(Rect.right-Rect.left, Rect.bottom-Rect.top);            
    return Result;
}

string Win32_GetExePathWithName()
{
    //TODO(JJ): We need to test and handle the case when our file path is larger than MAX_PATH
    char EXEPathWithName[MAX_PATH+1];
    ptr Size = GetModuleFileName(NULL, EXEPathWithName, MAX_PATH+1);
    string Result = PushLiteralString(EXEPathWithName, Size);
    return Result;
}

FILETIME Win32_GetFileCreationTime(string FilePath)
{    
    WIN32_FILE_ATTRIBUTE_DATA FindData;
    GetFileAttributesExA(FilePath.Data, GetFileExInfoStandard, &FindData);    
    return FindData.ftLastWriteTime;
}

win32_game_code Win32_DefaultGameCode()
{
    win32_game_code Result = {};    
    Result.Tick = Game_TickStub;    
    return Result;
}

win32_game_code Win32_LoadGameCode(string DLLPath, string TempDLLPath)
{
    win32_game_code Result = {};    
    BOOL CopyResult = CopyFile(DLLPath.Data, TempDLLPath.Data, false);
    if(!CopyResult)
        return Win32_DefaultGameCode();    
    
    Result.GameLibrary.Library = LoadLibrary(TempDLLPath.Data);
    Result.Tick = (game_tick*)GetProcAddress(Result.GameLibrary.Library, "Tick");
    if(!Result.GameLibrary.Library || !Result.Tick)
        return Win32_DefaultGameCode();
    
    Result.GameLibrary.LastWriteTime = Win32_GetFileCreationTime(DLLPath);
    return Result;
}

void Win32_UnloadGameCode(win32_game_code* GameCode, string TempDLLPath)
{
    if(GameCode->GameLibrary.Library)
    {
        ASSERT(FreeLibrary(GameCode->GameLibrary.Library));
        GameCode->GameLibrary.Library = NULL;       
        DeleteFile(TempDLLPath.Data);
    }
}

ALLOCATE_MEMORY(Win32_AllocateMemory)
{
    void* Result = VirtualAlloc(0, Capacity, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    return Result;
}

FREE_MEMORY(Win32_FreeMemory)
{
    if(Memory)    
        VirtualFree(Memory, 0, MEM_RELEASE);    
}

#if DEVELOPER_BUILD
PLATFORM_LOG(Win32_Log)
{   
    char String[1024];
    
    va_list Args;
    va_start(Args, Format);
    vsprintf(String, Format, Args);
    va_end(Args);
    
    OutputDebugStringA(String);    
}
#else
inline PLATFORM_LOG(Win32_Log)
{
}
#endif

PLATFORM_READ_ENTIRE_FILE(Win32_ReadEntireFile)
{
    file_results Result = {};
    HANDLE FileHandle = CreateFile(Path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
    if(FileHandle == INVALID_HANDLE_VALUE) 
    {
        WRITE_ERROR("Failed to open the file for reading");
        return Result;
    }
    
    DWORD HighWord = 0;
    u32 FileSize = GetFileSize(FileHandle, &HighWord);
    if(HighWord != 0) 
    { 
        CloseHandle(FileHandle); 
        WRITE_ERROR("File size is too large for reading."); 
        return Result;
    }
    
    void* Data = PushSize(FileSize, Clear, 0);
    DWORD BytesRead;
    if(!ReadFile(FileHandle, Data, FileSize, &BytesRead, NULL) || (BytesRead != FileSize)) 
    { 
        CloseHandle(FileHandle); 
        WRITE_ERROR("Was unable to read the entire file.");
        return Result; 
    }
    
    CloseHandle(FileHandle);    
    
    Result.Data = (u8*)Data;
    Result.Size = FileSize;
    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(Win32_WriteEntireFile)
{
    HANDLE FileHandle = CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
    
    if(FileHandle == INVALID_HANDLE_VALUE) 
    {        
        WRITE_ERROR("Failed to open the file for writing.");        
        return;
    }
    
    DWORD BytesWritten;
    if(!WriteFile(FileHandle, Data, Length, &BytesWritten, NULL) ||
       (BytesWritten != Length))        
        WRITE_ERROR("Was unable to write the entire file.");           
    
    CloseHandle(FileHandle);    
}

platform* Win32_GetPlatformStruct()
{
    local platform Result;
    Result.Log = Win32_Log;
    Result.AllocateMemory = Win32_AllocateMemory;
    Result.FreeMemory = Win32_FreeMemory;   
    Result.ReadEntireFile = Win32_ReadEntireFile;
    Result.WriteEntireFile = Win32_WriteEntireFile;    
    return &Result;
}

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{ 
    QueryPerformanceFrequency(&Global_Frequency);    
    Global_Platform = Win32_GetPlatformStruct();
    
    error_stream ErrorStream = CreateErrorStream();
    SetGlobalErrorStream(&ErrorStream);
    
    arena DefaultArena = CreateArena(MEGABYTE(1));    
    InitMemory(&DefaultArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);    
    Global_Platform->TempArena = &DefaultArena;
    Global_Platform->ErrorStream = &ErrorStream;
    
    string EXEFilePathName = Win32_GetExePathWithName();
    string EXEFilePath = GetFilePath(EXEFilePathName);    
    string GameDLLPathName = Concat(EXEFilePath, "World_Game.dll");
    string TempDLLPathName = Concat(EXEFilePath, "World_Game_Temp.dll");    
    string VulkanGraphicsDLLPathName = Concat(EXEFilePath, "Vulkan_Graphics.dll");
    
    u16 KeyboardUsage = 6;
    u16 MouseUsage = 2;
    u16 UsagePage = 1;
    
    RAWINPUTDEVICE RawInputDevices[] = 
    {
        {UsagePage, KeyboardUsage,  0, NULL},
        {UsagePage, MouseUsage, 0, NULL}
    };
    
    RegisterRawInputDevices(RawInputDevices, ARRAYCOUNT(RawInputDevices), sizeof(RAWINPUTDEVICE));
    
    WNDCLASSEX WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32_WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = GAME_NAME;    
    if(!RegisterClassEx(&WindowClass))    
        WRITE_AND_HANDLE_ERROR("Failed to register window class.");                    
    
    HWND Window = Win32_CreateWindow(&WindowClass, GAME_NAME, V2i(1280, 720));
    if(!Window)    
        WRITE_AND_HANDLE_ERROR("Failed to create game window."); 
    
#if DEVELOPER_BUILD
    development_input Input = {};
    development_game Game = {};
#else
    input Input = {};
    game Game = {};
#endif
    
    HMODULE GraphicsLib = LoadLibrary(VulkanGraphicsDLLPathName.Data);
    if(!GraphicsLib)
        WRITE_AND_HANDLE_ERROR("Failed to load the graphics dll %s.", VulkanGraphicsDLLPathName.Data);
    
    win32_graphics_init* GraphicsInit = (win32_graphics_init*)GetProcAddress(GraphicsLib, "GraphicsInit");
    if(!GraphicsInit)
        WRITE_AND_HANDLE_ERROR("Failed to load the graphics initialize routine.");
    
    temp_arena TempArena = BeginTemporaryMemory();
    graphics* Graphics = GraphicsInit(Window, Global_Platform);
    EndTemporaryMemory(&TempArena);
    
    Game.Input = &Input;
        
    win32_game_code GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);
    if(!GameCode.GameLibrary.Library)    
        WRITE_AND_HANDLE_ERROR("Failed to load the game's dll code.");
    
    Input.dt = 1.0f/60.0f;
    u64 StartTime = Win32_Clock();
    for(;;)
    {                
        temp_arena FrameArena = BeginTemporaryMemory();
        
        FILETIME GameDLLWriteTime = Win32_GetFileCreationTime(GameDLLPathName);
        if(CompareFileTime(&GameCode.GameLibrary.LastWriteTime, &GameDLLWriteTime) < 0)
        {
            Win32_UnloadGameCode(&GameCode, TempDLLPathName);                        
            GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);            
        }
        
        for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input.Buttons); ButtonIndex++)        
            Input.Buttons[ButtonIndex].WasDown = Input.Buttons[ButtonIndex].IsDown; 
        MSG Message;
        while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {                    
                    return 0;
                } break;
                
                case WM_INPUT:
                {
                    UINT Size;
                    UINT Result = GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER));
                    
                    void* InputData = PushSize(Size, NoClear, 8);                    
                    Result = GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, InputData, &Size, sizeof(RAWINPUTHEADER));                    
                    
                    RAWINPUT* RawInput = (RAWINPUT*)InputData;
                    RAWINPUTHEADER* RawInputHeader = &RawInput->header;
                    
                    switch(RawInputHeader->dwType)
                    {
                        case RIM_TYPEMOUSE:
                        {
                            RAWMOUSE* RawMouse = &RawInput->data.mouse;                            
                            Input.MouseDelta = V2i(RawMouse->lLastX, RawMouse->lLastY);   
                            
#if DEVELOPER_BUILD                            
                            switch(RawMouse->usButtonFlags)
                            {
                                case RI_MOUSE_LEFT_BUTTON_DOWN:
                                {
                                    Input.LMB.IsDown = true;                                    
                                } break;
                                
                                case RI_MOUSE_LEFT_BUTTON_UP:
                                {
                                    Input.LMB.IsDown = false;
                                    Input.LMB.WasDown = true;
                                } break;
                                
                                case RI_MOUSE_MIDDLE_BUTTON_DOWN:
                                {
                                    Input.MMB.IsDown = true;
                                } break;
                                
                                case RI_MOUSE_MIDDLE_BUTTON_UP:
                                {
                                    Input.MMB.IsDown = false;
                                    Input.MMB.WasDown = true;
                                } break;
                                
                                case RI_MOUSE_WHEEL:
                                {
                                    Input.Scroll = (f32)(i16)RawMouse->usButtonData / (f32)WHEEL_DELTA;
                                } break;
                            }              
#endif
                        } break;
                        
                        case RIM_TYPEKEYBOARD:
                        {
                            RAWKEYBOARD* RawKeyboard = &RawInput->data.keyboard;                            
                            b32 Quit = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) && (RawKeyboard->VKey == VK_F4) && (RawKeyboard->Flags == RI_KEY_MAKE);
                            if(Quit)
                                PostQuitMessage(0);
                            
                            Quit = (RawKeyboard->VKey == VK_ESCAPE) && (RawKeyboard->Flags == RI_KEY_MAKE);
                            if(Quit)
                                PostQuitMessage(0);
                            
                            switch(RawKeyboard->VKey)
                            {
                                BIND_KEY('W', Input.MoveForward);
                                BIND_KEY('S', Input.MoveBackward);
                                BIND_KEY('A', Input.MoveLeft);
                                BIND_KEY('D', Input.MoveRight);                            
                                BIND_KEY('Q', Input.SwitchWorld);                                
                            }             
                            
#if DEVELOPER_BUILD                            
                            switch(RawKeyboard->VKey)
                            {
                                BIND_KEY(VK_MENU, Input.Alt);
                            }   
                            
                            if(RawKeyboard->Flags == RI_KEY_MAKE)
                            {
                                switch(RawKeyboard->VKey)
                                {
                                    case VK_F5:
                                    {
                                        Game.InDevelopmentMode = !Game.InDevelopmentMode;
                                        
#if 0 
                                        Game.DebugCamera.Position = Graphics->Camera.Position;
                                        Game.DebugCamera.AngularVelocity = {};
                                        Game.DebugCamera.Orientation = Identity3();
                                        Game.DebugCamera.FocalPoint = Graphics->Camera.FocalPoint;
                                        Game.DebugCamera.Distance = Len(Game.DebugCamera.FocalPoint - Game.DebugCamera.Position);
#endif
                                    } break;                                    
                                }
                            }
#endif
                        } break;
                    }
                } break;
                
                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }
        
        //TODO(JJ): Probably don't want this
        if(Input.dt > 1.0f/20.0f)
            Input.dt = 1.0f/20.0f;
        
        v2i WindowDim = Win32_GetWindowDim(Window);
        
        GameCode.Tick(&Game, Global_Platform);        
        if(!Graphics->RenderGame(&Game, WindowDim))
        {
            ASSERT(false); 
            //TODO(JJ): Probably should fallback to Direct3D or OpenGL to try and recover so the game doesn't 
            //just straight crash. At least we can warn the user
        }
        
        Input.dt = (f32)Win32_Elapsed(Win32_Clock(), StartTime);
        StartTime = Win32_Clock();
        
        Input.MouseDelta = {};
        
#if DEVELOPER_BUILD                
        Input.Scroll = 0.0f;
        Input.Alt.WasDown = Input.Alt.IsDown;
        Input.LMB.WasDown = Input.LMB.IsDown;
        Input.MMB.WasDown = Input.MMB.IsDown;
#endif
        
        EndTemporaryMemory(&FrameArena);        
        CHECK_ARENA(GetDefaultArena());
    }
    
handle_error:
    string ErrorMessage = GetString(GetGlobalErrorStream());
    Global_Platform->WriteEntireFile("Errors.log", ErrorMessage.Data, SafeU32(ErrorMessage.Length));
    MessageBox(NULL, "Error has occurred, please see Errors.log file.", NULL, MB_OK);     
    return -1;
}