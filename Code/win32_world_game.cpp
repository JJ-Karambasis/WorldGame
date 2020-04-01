#include "win32_world_game.h"
#include "audio.cpp"

#if DEVELOPER_BUILD
#include "dev_world_game.cpp"
b32 Win32_DevWindowProc(HWND, UINT, WPARAM, LPARAM);
#endif

global LARGE_INTEGER Global_Frequency;

PLATFORM_CLOCK(Win32_Clock)
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

PLATFORM_ELAPSED_TIME(Win32_Elapsed)
{
    f64 Result = Win32_ToSeconds(End-Start);
    return Result;
}

internal LRESULT CALLBACK 
Win32_WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
#if DEVELOPER_BUILD
    if(Win32_DevWindowProc(Window, Message, WParam, LParam))
        return Result;
#endif    
    
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
    buffer Result = {};
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

HANDLE Win32_OpenFile(char* Path)
{
    HANDLE Result = CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(Win32_WriteEntireFile)
{
    HANDLE FileHandle = Win32_OpenFile(Path);        
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

PLATFORM_OPEN_FILE(Win32_OpenFile)
{
    DWORD DesiredAttributes = 0;
    DWORD CreationDisposition = 0;
    if(Attributes == PLATFORM_FILE_ATTRIBUTES_READ)
    {
        DesiredAttributes = GENERIC_READ;
        CreationDisposition = OPEN_EXISTING;
    }
    else if(Attributes == PLATFORM_FILE_ATTRIBUTES_WRITE)
    {
        DesiredAttributes = GENERIC_WRITE;
        CreationDisposition = CREATE_ALWAYS;
    }
    else
    {
        WRITE_ERROR("Invalid platform file attribute (ID: %d).", (u32)Attributes);        
        return NULL;
    }    
    
    HANDLE Handle = CreateFile(Path, DesiredAttributes, 0, NULL, CreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if(Handle == INVALID_HANDLE_VALUE)
    {
        WRITE_ERROR("Failed to open the file.");
        return NULL;
    }
    
    platform_file_handle* Result = (platform_file_handle*)Win32_AllocateMemory(sizeof(platform_file_handle));
    Result->Handle = Handle;
    Result->Attributes = Attributes;
    return Result;
}    

PLATFORM_READ_FILE(Win32_ReadFile)
{
    if(File->Attributes != PLATFORM_FILE_ATTRIBUTES_READ)
    {
        WRITE_ERROR("Failed to read file because the file attributes are mapped to write.");
        return false;
    }
    
    OVERLAPPED* OffsetPointer = NULL;
    OVERLAPPED Offsets = {};
    if(Offset != NO_OFFSET)
    {
        Offsets.Offset = (DWORD)(Offset & 0xFFFFFFFF);
        Offsets.OffsetHigh = (DWORD)((Offset >> 32) & 0xFFFFFFFF);
        OffsetPointer = &Offsets;
    }
    
    DWORD BytesRead;
    if(ReadFile(File->Handle, Data, ReadSize, &BytesRead, OffsetPointer) && (BytesRead == ReadSize))
        return true;
    
    WRITE_ERROR("Failed to read file. Bytes read %d - Bytes requested %d", BytesRead, ReadSize);
    return false;
}

PLATFORM_WRITE_FILE(Win32_WriteFile)
{
    if(File->Attributes != PLATFORM_FILE_ATTRIBUTES_WRITE)
    {
        WRITE_ERROR("Failed to write file because the file attributes are mapped to read.");
        return false;
    }
    
    OVERLAPPED* OffsetPointer = NULL;
    OVERLAPPED Offsets = {};
    if(Offset != NO_OFFSET)
    {
        Offsets.Offset = (DWORD)(Offset & 0xFFFFFFFF);
        Offsets.OffsetHigh = (DWORD)((Offset >> 32) & 0xFFFFFFFF);
        OffsetPointer = &Offsets;
    }
    
    DWORD BytesWritten;
    if(WriteFile(File->Handle, Data, WriteSize, &BytesWritten, OffsetPointer) && (BytesWritten == WriteSize))
        return true;
    
    WRITE_ERROR("Failed to write file. Bytes written %d - Bytes requested %d", BytesWritten, WriteSize);
    return false;
}

PLATFORM_CLOSE_FILE(Win32_CloseFile)
{
    CloseHandle(File->Handle);
    Win32_FreeMemory(File);    
}

win32_audio Win32_InitDSound(HWND Window, ptr BufferLength, audio_format AudioFormat)
{    
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
    BOOL_CHECK_AND_HANDLE(DSoundLibrary, "Failed to load the dsound.dll library.");
    
    direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    BOOL_CHECK_AND_HANDLE(DirectSoundCreate, "Failed to load the DirectSoundCreate function");
    
    IDirectSound* DirectSound;
    HRESULT_CHECK_AND_HANDLE(DirectSoundCreate(0, &DirectSound, 0), "Failed to create the DirectSound object.");    
    HRESULT_CHECK_AND_HANDLE(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY), "Failed to set the cooperation level for direct sound.");    
    
    WAVEFORMATEX WaveFormat = {};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = AudioFormat.ChannelCount;    
    WaveFormat.nSamplesPerSec = AudioFormat.SamplesPerSecond;
    WaveFormat.wBitsPerSample = AudioFormat.BytesPerSample*8;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
    
    DSBUFFERDESC BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    
    IDirectSoundBuffer* PrimaryBuffer;
    HRESULT_CHECK_AND_HANDLE(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0), "Failed to create the direct sound primary buffer.");    
    HRESULT_CHECK_AND_HANDLE(PrimaryBuffer->SetFormat(&WaveFormat), "Failed to set the direct sound wave format.");
    
    BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    BufferDescription.dwBufferBytes = (u32)GetAudioBufferSizeFromSeconds(&AudioFormat, BufferLength);
    BufferDescription.lpwfxFormat = &WaveFormat;
    
    IDirectSoundBuffer* SoundBuffer;
    HRESULT_CHECK_AND_HANDLE(DirectSound->CreateSoundBuffer(&BufferDescription, &SoundBuffer, 0), "Failed to create the direct sound buffer.");
    
    win32_audio Result;
    Result.Format = AudioFormat;
    Result.SoundBuffer = SoundBuffer;
    Result.SampleCount = BufferLength*AudioFormat.SamplesPerSecond;
    Result.Samples = Win32_AllocateMemory(BufferDescription.dwBufferBytes);
    
    return Result;
    
    handle_error:
    return {};
}

platform* Win32_GetPlatformStruct()
{
    local platform Result;
    Result.Log             = Win32_Log;
    Result.AllocateMemory  = Win32_AllocateMemory;
    Result.FreeMemory      = Win32_FreeMemory;   
    Result.ReadEntireFile  = Win32_ReadEntireFile;
    Result.WriteEntireFile = Win32_WriteEntireFile;    
    Result.OpenFile        = Win32_OpenFile;
    Result.ReadFile        = Win32_ReadFile;
    Result.WriteFile       = Win32_WriteFile;
    Result.CloseFile       = Win32_CloseFile;
    Result.Clock           = Win32_Clock;
    Result.ElapsedTime     = Win32_Elapsed;
    return &Result;
}

DWORD WINAPI 
AudioThread(void* Paramter)
{
    
#if 0 
    
    win32_audio* Audio = (win32_audio*)Paramter;        
    audio TestAudio = LoadWAVFile("Test.wav");    
    
#define TARGET_AUDIO_HZ 20
    
    IDirectSoundBuffer* SoundBuffer = Audio->SoundBuffer;
    u32 SafetyBytes = (((f32)Audio->Format.SamplesPerSecond*(f32)Audio->Format.BytesPerSample / TARGET_AUDIO_HZ)/3.0f);
    
    SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
    
    for(;;)
    {
        platform_time AudioClock = Global_Frequency->Clock();
        f64 FromBeginToAudioSeconds = Global_Frequency->ElapsedTime(FlipClock, AudioClock);
        
        DWORD PlayCursor;
        DWORD WriteCursor;
        if(SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
        {
        }
    }
#endif
    
    return 0;
}

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{ 
    QueryPerformanceFrequency(&Global_Frequency);    
    Global_Platform = Win32_GetPlatformStruct();
    
    error_stream ErrorStream = CreateErrorStream();
    SetGlobalErrorStream(&ErrorStream);
    
    arena DefaultArena = CreateArena(MEGABYTE(256));    
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
    
    u32 SoundBufferSeconds = 2;
    win32_audio Audio = Win32_InitDSound(Window, SoundBufferSeconds, CreateAudioFormat(2, 2, 48000)); 
    if(!Audio.SoundBuffer)
        WRITE_AND_HANDLE_ERROR("Failed to initialize direct sound.");
    
    CloseHandle(CreateThread(NULL, 0, AudioThread, &Audio, 0, NULL));
    
#if DEVELOPER_BUILD    
    development_input Input = {};
    development_game Game = {};
    
    Game.DevArena = CreateArena(MEGABYTE(32));
    frame_recording* FrameRecording = &Game.FrameRecordings;        
    FrameRecording->MaxFrameCount = 1000000;    
    FrameRecording->FrameOffsets = PushArray(&Game.DevArena, FrameRecording->MaxFrameCount, ptr, Clear, 0);
    
    for(u32 RecordingIndex = 0; RecordingIndex < ARRAYCOUNT(FrameRecording->WalkingSystemRecording.EventRecordings); RecordingIndex++)
    {
        walking_event_recording* Recordings = FrameRecording->WalkingSystemRecording.EventRecordings + RecordingIndex;
        Recordings->Events = PushArray(&Game.DevArena, MAX_EVENTS, walking_event, Clear, 0);
    }
    
    IMGUI_CHECKVERSION();
    ImGuiContext* Context = ImGui::CreateContext();
    ImGui::StyleColorsDark();
    
    ImGuiIO& IO = ImGui::GetIO();
    IO.BackendFlags |= (ImGuiBackendFlags_HasMouseCursors|ImGuiBackendFlags_HasSetMousePos);
    IO.BackendPlatformName = "world_game_win32_platform";
    IO.ImeWindowHandle = Window;
    IO.KeyMap[ImGuiKey_Tab] = VK_TAB;
    IO.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    IO.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    IO.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    IO.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    IO.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    IO.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    IO.KeyMap[ImGuiKey_Home] = VK_HOME;
    IO.KeyMap[ImGuiKey_End] = VK_END;
    IO.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    IO.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    IO.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    IO.KeyMap[ImGuiKey_Space] = VK_SPACE;
    IO.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    IO.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    IO.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    IO.KeyMap[ImGuiKey_A] = 'A';
    IO.KeyMap[ImGuiKey_C] = 'C';
    IO.KeyMap[ImGuiKey_V] = 'V';
    IO.KeyMap[ImGuiKey_X] = 'X';
    IO.KeyMap[ImGuiKey_Y] = 'Y';
    IO.KeyMap[ImGuiKey_Z] = 'Z';
    
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
    graphics* Graphics = GraphicsInit(Window, Global_Platform, Context);
    BOOL_CHECK_AND_HANDLE(Graphics, "Failed to initialize the graphics.");
    EndTemporaryMemory(&TempArena);
    
    Game.Input = &Input;
    
    win32_game_code GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);
    if(!GameCode.GameLibrary.Library)    
        WRITE_AND_HANDLE_ERROR("Failed to load the game's dll code.");
    
    Input.dt = 1.0f/60.0f; 
    u64 StartTime = Win32_Clock();
    for(;;)
    {   
        DEVELOPER_GRAPHICS(Graphics);
        
        temp_arena FrameArena = BeginTemporaryMemory();
        
        FILETIME GameDLLWriteTime = Win32_GetFileCreationTime(GameDLLPathName);
        if(CompareFileTime(&GameCode.GameLibrary.LastWriteTime, &GameDLLWriteTime) < 0)
        {
            Win32_UnloadGameCode(&GameCode, TempDLLPathName);                        
            GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);            
        }
        
        Input.MouseDelta = {};
        
#if DEVELOPER_BUILD                
        Input.Scroll = 0.0f;
        for(u32 DevButtonIndex = 0; DevButtonIndex < ARRAYCOUNT(Input.DevButtons); DevButtonIndex++)
            Input.DevButtons[DevButtonIndex].WasDown = Input.DevButtons[DevButtonIndex].IsDown;        
#endif
        
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
                            
#if DEVELOPER_BUILD
                            if(FrameRecording->RecordingState == RECORDING_STATE_NONE || FrameRecording->RecordingState == RECORDING_STATE_RECORDING)                            
    #endif
                            {
                                switch(RawKeyboard->VKey)
                                {
                                    BIND_KEY('W', Input.MoveForward);
                                    BIND_KEY('S', Input.MoveBackward);
                                    BIND_KEY('A', Input.MoveLeft);
                                    BIND_KEY('D', Input.MoveRight);                            
                                    BIND_KEY('Q', Input.SwitchWorld);                                
                                }
                            }                            
#if DEVELOPER_BUILD           
                            else
                            {
                                ClearArray(Input.Buttons, ARRAYCOUNT(input::Buttons), button); 
                            }
                            
                            switch(RawKeyboard->VKey)
                            {
                                BIND_KEY(VK_MENU, Input.Alt);
                                BIND_KEY('0', Input.RecordButton);
                                BIND_KEY('1', Input.PlaybackButton);
                                BIND_KEY('2', Input.CycleButton);
                                BIND_KEY(VK_LEFT, Input.CycleLeft);
                                BIND_KEY(VK_RIGHT, Input.CycleRight);
                            }   
                            
                            if(RawKeyboard->Flags == RI_KEY_MAKE)
                            {
                                switch(RawKeyboard->VKey)
                                {
                                    case VK_F5:
                                    {
                                        Game.InDevelopmentMode = !Game.InDevelopmentMode;
                                        
                                        if(Game.InDevelopmentMode)
                                        {
                                            Game.DevCamera.Position = Game.Camera.Position;
                                            Game.DevCamera.AngularVelocity = {};
                                            Game.DevCamera.Orientation = IdentityM3();
                                            Game.DevCamera.FocalPoint = Game.Camera.FocalPoint;
                                            Game.DevCamera.Distance = Magnitude(Game.DevCamera.FocalPoint - Game.DevCamera.Position);
                                        }
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
        
        v2i WindowDim = Win32_GetWindowDim(Window);
        
        //TODO(JJ): Probably don't want this
        if(Input.dt > 1.0f/20.0f)
            Input.dt = 1.0f/20.0f;
        
#if DEVELOPER_BUILD
        IO.DisplaySize = ImVec2((f32)WindowDim.width, (f32)WindowDim.height);
        IO.DeltaTime = Input.dt;
        IO.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        IO.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        IO.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if(IO.WantSetMousePos)
        {
            v2i Position = V2i(IO.MousePos.x, IO.MousePos.y);
            ClientToScreen(Window, (POINT*)&Position);
            SetCursorPos(Position.x, Position.y);
        }
        
        IO.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        POINT MousePosition;
        if(HWND ActiveWindow = GetForegroundWindow())
        {
            if(ActiveWindow == Window || IsChild(ActiveWindow, Window))
            {
                if(GetCursorPos(&MousePosition) && ScreenToClient(Window, &MousePosition))                
                    IO.MousePos = ImVec2((f32)MousePosition.x, (f32)MousePosition.y);                
            }
        }                
        
        DevelopmentTick(&Game);        
#endif        
        
        GameCode.Tick(&Game, Graphics, Global_Platform);                        
        if(WindowDim != 0)
        {
            if(!Graphics->RenderGame(&Game, WindowDim))
            {
                ASSERT(false); 
                //TODO(JJ): Probably should fallback to Direct3D or OpenGL to try and recover so the game doesn't 
                //just straight crash. At least we can warn the user
            }
        }
        
        Input.dt = (f32)Win32_Elapsed(Win32_Clock(), StartTime);
        //CONSOLE_LOG("dt: %f\n", Input.dt*1000.0f);
        StartTime = Win32_Clock();
        
        EndTemporaryMemory(&FrameArena);        
        CHECK_ARENA(GetDefaultArena());
    }
    
    handle_error:
    string ErrorMessage = GetString(GetGlobalErrorStream());
    Global_Platform->WriteEntireFile("Errors.log", ErrorMessage.Data, SafeU32(ErrorMessage.Length));
    MessageBox(NULL, "Error has occurred, please see Errors.log file.", NULL, MB_OK);     
    return -1;
} 

#if DEVELOPER_BUILD

b32 Win32_DevWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if(!ImGui::GetCurrentContext())
        return false;
    
    ImGuiIO& IO = ImGui::GetIO();
    
    switch(Message)
    {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (Message == WM_LBUTTONDOWN || Message == WM_LBUTTONDBLCLK) { button = 0; }
            if (Message == WM_RBUTTONDOWN || Message == WM_RBUTTONDBLCLK) { button = 1; }
            if (Message == WM_MBUTTONDOWN || Message == WM_MBUTTONDBLCLK) { button = 2; }
            if (Message == WM_XBUTTONDOWN || Message == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? 3 : 4; }
            if (!ImGui::IsAnyMouseDown() && GetCapture() == NULL)
                SetCapture(Window);
            IO.MouseDown[button] = true;                    
        } break;
        
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            i32 Button = 0;
            if (Message == WM_LBUTTONUP) { Button = 0; }
            if (Message == WM_RBUTTONUP) { Button = 1; }
            if (Message == WM_MBUTTONUP) { Button = 2; }
            if (Message == WM_XBUTTONUP) { Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? 3 : 4; }
            IO.MouseDown[Button] = false;
            if (!ImGui::IsAnyMouseDown() && GetCapture() == Window)
                ReleaseCapture();                                        
        } break;
        
        case WM_MOUSEWHEEL:
        {
            IO.MouseWheel += (f32)GET_WHEEL_DELTA_WPARAM(WParam) / (f32)WHEEL_DELTA;                    
        } break;
        
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (WParam < 256)                    
                IO.KeysDown[WParam] = 1;                    
        }break;
        
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (WParam < 256)
                IO.KeysDown[WParam] = 0;                    
        } break;                
        
        case WM_CHAR:
        {
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (WParam > 0 && WParam < 0x10000)
                IO.AddInputCharacterUTF16((unsigned short)WParam);
        } break;                    
        
        default:
        {
            return false;
        } break;
    }
    
    return true;
}
#endif