#include "win32_world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "simulation/simulation.cpp"
#include "graphics_state.cpp"
#include "entity.cpp"

#define DEV_TOOLS_IMPLEMENTATION
#include "dev_tools/dev_tools.h"

#if DEVELOPER_BUILD
PLATFORM_INIT_IMGUI(Platform_InitImGui);
PLATFORM_DEVELOPMENT_UPDATE(Platform_DevUpdate);

ak_bool Win32_DevWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
void Win32_HandleDevKeyboard(dev_context* DevContext, RAWKEYBOARD* Keyboard);
void Win32_HandleDevMouse(dev_context* DevContext, RAWMOUSE* Mouse);
#define Dev_Initialize(Game, Graphics, PlatformWindow) DevContext_Initialize(Game, Graphics, PlatformWindow, Platform_InitImGui, Platform_DevUpdate)
#define Dev_Tick() DevContext_Tick()
#define Dev_Render() DevContext_Render()
#define Dev_WindowProc(Window, Message, WParam, LParam) Win32_DevWindowProc(Window, Message, WParam, LParam)
#define Dev_HandleMouse(RawMouse) Win32_HandleDevMouse(Dev_GetDeveloperContext(), RawMouse)
#define Dev_HandleKeyboard(RawKeyboard) Win32_HandleDevKeyboard(Dev_GetDeveloperContext(), RawKeyboard)
#define Dev_RecordFrame() DevelopmentRecord(Dev_GetDeveloperContext())
#else
#define Dev_Initialize(Game, Graphics, PlatformData)
#define Dev_Tick()
#define Dev_Render() 
#define Dev_WindowProc(Window, Message, WParam, LParam) false
#define Dev_HandleMouse(RawMouse) 
#define Dev_HandleKeyboard(RawKeyboard) 
#define Dev_RecordFrame() 
#endif

#define BindKey(key, action) case key: { if((RawKeyboard->Flags == RI_KEY_MAKE) || (RawKeyboard->Message == WM_KEYDOWN) || (RawKeyboard->Message == WM_SYSKEYDOWN)) { action.IsDown = true; } else if((RawKeyboard->Flags == RI_KEY_BREAK) || (RawKeyboard->Message == WM_KEYUP) || (RawKeyboard->Message == WM_SYSKEYUP)) { action.IsDown = false; action.WasDown = true; } } break
#define HR_Release(iunknown) \
do \
{ \
    if(iunknown) \
    { \
        iunknown->Release(); \
        iunknown = NULL; \
    } \
} while(0)

global ak_string Global_EXEFilePath;
global ak_arena* Global_PlatformArena;
global win32_game_code Global_GameCode;
global ak_bool Global_Running;

//TODO(JJ): This lock can probably be moved out into some developer code macros since it is only used for hot reloading (for the audio thread)
global ak_lock Global_Lock;

internal LRESULT CALLBACK 
Win32_WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    if(Dev_WindowProc(Window, Message, WParam, LParam))
        return Result;
    
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
                        ak_v2i WindowDim)
{    
    DWORD ExStyle = 0;
    DWORD Style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    
    RECT WindowRect = {0, 0, (LONG)WindowDim.w, (LONG)WindowDim.h};
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

inline ak_v2i 
Win32_GetWindowDim(HWND Window)
{
    ak_v2i Result = {};    
    RECT Rect;
    if(GetClientRect(Window, &Rect))    
        Result = AK_V2((ak_i32)(Rect.right-Rect.left), (ak_i32)(Rect.bottom-Rect.top));            
    return Result;
}

FILETIME Win32_GetFileCreationTime(ak_string FilePath)
{    
    WIN32_FILE_ATTRIBUTE_DATA FindData;
    GetFileAttributesExA(FilePath.Data, GetFileExInfoStandard, &FindData);    
    return FindData.ftLastWriteTime;
}

win32_game_code Win32_DefaultGameCode()
{
    win32_game_code Result = {};    
    Result.Initialize = Game_InitializeStub;
    Result.FixedTick = Game_FixedTickStub;
    Result.Tick = Game_TickStub;    
    Result.Render = Game_RenderStub;
    Result.OutputSoundSamples = Game_OutputSoundSamplesStub;
    return Result;
}

win32_game_code 
Win32_LoadGameCode(ak_string DLLPath, ak_string TempDLLPath)
{
    win32_game_code Result = {};    
    BOOL CopyResult = CopyFile(DLLPath.Data, TempDLLPath.Data, false);
    if(!CopyResult)
        return Win32_DefaultGameCode();    
    
    Result.GameLibrary.Library = LoadLibrary(TempDLLPath.Data);
    Result.Initialize = (game_initialize*)GetProcAddress(Result.GameLibrary.Library, "Initialize");
    Result.FixedTick = (game_fixed_tick*)GetProcAddress(Result.GameLibrary.Library, "FixedTick");
    Result.Render = (game_render*)GetProcAddress(Result.GameLibrary.Library, "Render");
    Result.Tick = (game_tick*)GetProcAddress(Result.GameLibrary.Library, "Tick");
    Result.OutputSoundSamples = (game_output_sound_samples*)GetProcAddress(Result.GameLibrary.Library, "OutputSoundSamples");
    if(!Result.GameLibrary.Library || !Result.Initialize || !Result.FixedTick || !Result.Tick || !Result.Render || !Result.OutputSoundSamples)
        return Win32_DefaultGameCode();
    
    Result.GameLibrary.LastWriteTime = Win32_GetFileCreationTime(DLLPath);
    return Result;
}

void 
Win32_UnloadGameCode(win32_game_code* GameCode, ak_string TempDLLPath)
{
    if(GameCode->GameLibrary.Library)
    {
        FreeLibrary(GameCode->GameLibrary.Library);
        GameCode->GameLibrary.Library = NULL;       
        DeleteFile(TempDLLPath.Data);
    }
}

inline win32_graphics_code 
Win32_DefaultGraphicsCode()
{
    win32_graphics_code Result = {};
    Result.InitGraphics = Graphics_InitGraphicsStub;
    Result.BindGraphicsFunctions = Graphics_BindGraphicsFunctionsStub;
    Result.ExecuteRenderCommands = Graphics_ExecuteRenderCommandsStub;            
    Result.InvalidateShaders = Graphics_InvalidateShadersStub;
    return Result;
}

win32_graphics_code
Win32_LoadGraphicsCode(ak_string DLLPath, ak_string TempDLLPath)
{
    win32_graphics_code Result = {};
    BOOL CopyResult = CopyFile(DLLPath.Data, TempDLLPath.Data, false);
    if(!CopyResult)
        return Win32_DefaultGraphicsCode();
    
    Result.GraphicsLibrary.Library = LoadLibrary(TempDLLPath.Data);
    if(!Result.GraphicsLibrary.Library)
        return Win32_DefaultGraphicsCode();
    
    Result.InitGraphics = (init_graphics*)GetProcAddress(Result.GraphicsLibrary.Library, "InitGraphics");
    if(!Result.InitGraphics)
        return Win32_DefaultGraphicsCode();
    
    Result.BindGraphicsFunctions = (bind_graphics_functions*)GetProcAddress(Result.GraphicsLibrary.Library, "BindGraphicsFunctions");
    if(!Result.BindGraphicsFunctions)
        return Win32_DefaultGraphicsCode();
    
    Result.ExecuteRenderCommands = (execute_render_commands*)GetProcAddress(Result.GraphicsLibrary.Library, "ExecuteRenderCommands");
    if(!Result.ExecuteRenderCommands)
        return Win32_DefaultGraphicsCode();
    
    Result.InvalidateShaders = (invalidate_shaders*)GetProcAddress(Result.GraphicsLibrary.Library, "InvalidateShaders");
    if(!Result.InvalidateShaders)
        return Win32_DefaultGraphicsCode();
    
    Result.GraphicsLibrary.LastWriteTime = Win32_GetFileCreationTime(DLLPath);
    return Result;
}

void 
Win32_UnloadGraphicsCode(win32_graphics_code* GraphicsCode, ak_string TempDLLPath)
{
    if(GraphicsCode->GraphicsLibrary.Library)
    {
        FreeLibrary(GraphicsCode->GraphicsLibrary.Library);
        GraphicsCode->GraphicsLibrary.Library = NULL;
        DeleteFile(TempDLLPath.Data);
    }
}

win32_audio_output Win32_InitDSound(HWND Window, ak_uaddr BufferLength)
{    
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
    if(!DSoundLibrary)
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }    
    
    direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    if(!DirectSoundCreate)
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }    
    
    IDirectSound* DirectSound;
    if(FAILED(DirectSoundCreate(0, &DirectSound, 0)))
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }
    
    if(FAILED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
    {
        //TODO(JJ): Diagnostic and error logging 
        return {};
    }
    
    WAVEFORMATEX WaveFormat = {};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = AUDIO_OUTPUT_CHANNEL_COUNT;    
    WaveFormat.nSamplesPerSec = AUDIO_OUTPUT_SAMPLES_PER_SECOND;
    WaveFormat.wBitsPerSample = sizeof(ak_i16)*8;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;    
    
    DSBUFFERDESC BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    
    IDirectSoundBuffer* PrimaryBuffer;
    if(FAILED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }
    
    if(FAILED(PrimaryBuffer->SetFormat(&WaveFormat)))
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }
    
    BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    BufferDescription.dwBufferBytes = (ak_u32)GetAudioBufferSizeFromSeconds(AUDIO_OUTPUT_CHANNEL_COUNT, BufferLength);
    BufferDescription.lpwfxFormat = &WaveFormat;
    
    IDirectSoundBuffer* SoundBuffer;
    if(FAILED(DirectSound->CreateSoundBuffer(&BufferDescription, &SoundBuffer, 0)))
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }
    
    win32_audio_output Result = {};        
    Result.SoundBuffer = SoundBuffer;    
    Result.Samples.Count = BufferLength*AUDIO_OUTPUT_SAMPLES_PER_SECOND;
    Result.Samples.Data = (ak_i16*)AK_Allocate(BufferDescription.dwBufferBytes);
    Result.Mute = true;
    Result.AudioThreadArena = AK_CreateArena(AK_Megabyte(1));
    Result.PlayingAudioStorage = AK_CreatePool<playing_audio>(16);
    
    return Result;    
}

void Win32_ClearSoundBuffer(win32_audio_output* Output)
{
    IDirectSoundBuffer* SoundBuffer = Output->SoundBuffer;    
    
    VOID* Region1;
    VOID* Region2;
    DWORD Region1Size;            
    DWORD Region2Size;
    
    if(SUCCEEDED(SoundBuffer->Lock(0, 0, &Region1, &Region1Size, &Region2, &Region2Size, DSBLOCK_ENTIREBUFFER)))
    {   
        AK_MemoryClear(Region1, Region1Size);
        AK_MemoryClear(Region2, Region2Size);        
        SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

DWORD WINAPI 
Win32_AudioThread(void* Paramter)
{    
    game* Game = (game*)Paramter;
    win32_audio_output* AudioOutput = (win32_audio_output*)Game->AudioOutput;
    
#define TARGET_AUDIO_HZ 20
    
    IDirectSoundBuffer* SoundBuffer = AudioOutput->SoundBuffer;    
    Win32_ClearSoundBuffer(AudioOutput);
    SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
    
    ak_high_res_clock FlipWallClock = AK_WallClock();    
    ak_bool SoundIsValid = false;
    
    ak_u16 BytesPerSample = sizeof(ak_u16)*AUDIO_OUTPUT_CHANNEL_COUNT;
    ak_u32 SoundBufferSize = AK_SafeU32(AudioOutput->Samples.Count*BytesPerSample);    
    ak_f32 TargetSecondsPerFrame = AK_SafeInverse(TARGET_AUDIO_HZ);
    
    ak_i32 SafetyBytes = (ak_i32)(((ak_f32)AUDIO_OUTPUT_CHANNEL_COUNT*(ak_f32)BytesPerSample / TARGET_AUDIO_HZ)/3.0f);
    
    for(;;)
    {                
        ak_temp_arena TempArena = AudioOutput->AudioThreadArena->BeginTemp();
        
        ak_high_res_clock AudioWallClock = AK_WallClock();
        ak_f32 FromBeginToAudioSeconds = (ak_f32)AK_GetElapsedTime(AudioWallClock, FlipWallClock);
        
        DWORD PlayCursor;
        DWORD WriteCursor;
        if(SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
        {            
            if(!SoundIsValid)
            {
                AudioOutput->RunningSampleIndex = WriteCursor / BytesPerSample;
                SoundIsValid = true;
            }
            
            DWORD ByteToLock = ((AudioOutput->RunningSampleIndex*BytesPerSample) % SoundBufferSize);
            
            DWORD ExpectedSoundBytesPerFrame =
                (ak_i32)((ak_f32)(AUDIO_OUTPUT_SAMPLES_PER_SECOND*BytesPerSample) / TARGET_AUDIO_HZ);
            ak_f32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(ak_f32)ExpectedSoundBytesPerFrame);
            
            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
            
            DWORD SafeWriteCursor = WriteCursor;
            if(SafeWriteCursor < PlayCursor)
            {
                SafeWriteCursor += SoundBufferSize;
            }
            AK_Assert(SafeWriteCursor >= PlayCursor, "Write cursor needs to be ahead of the play cursor");
            SafeWriteCursor += SafetyBytes;
            
            ak_bool AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);
            
            DWORD TargetCursor = 0;
            if(AudioCardIsLowLatency)
            {
                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
            }
            else
            {
                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SafetyBytes);
            }
            TargetCursor = (TargetCursor % SoundBufferSize);
            
            DWORD BytesToWrite = 0;
            if(ByteToLock > TargetCursor)
            {
                BytesToWrite = (SoundBufferSize - ByteToLock);
                BytesToWrite += TargetCursor;
            }
            else
            {
                BytesToWrite = TargetCursor - ByteToLock;
            }
            
            DWORD SamplesToWrite = BytesToWrite / BytesPerSample;
            if(!Global_Lock.IsLocked())
            {   
                AK_Assert(SamplesToWrite < AudioOutput->Samples.Count, "Samples to write must be smaller the samples allocated in the audio buffer");
                samples Samples = {SamplesToWrite, AudioOutput->Samples.Data};
                
                Global_GameCode.OutputSoundSamples(Game, &Samples);
                
                VOID* Region1;
                VOID* Region2;
                DWORD Region1Size;            
                DWORD Region2Size;
                if(SUCCEEDED(SoundBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
                {   
                    ak_i16* SrcSamples = AudioOutput->Samples.Data;                    
                    DWORD Region1SampleCount = Region1Size/BytesPerSample;
                    
                    ak_i16* DstSample  = (ak_i16*)Region1;                 
                    for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
                    {                            
                        *DstSample++ = *SrcSamples++;
                        *DstSample++ = *SrcSamples++;                                                    
                        AudioOutput->RunningSampleIndex++;                        
                    }
                    
                    DWORD Region2SampleCount = Region2Size/BytesPerSample;
                    DstSample  = (ak_i16*)Region2;                 
                    for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
                    {                            
                        *DstSample++ = *SrcSamples++;
                        *DstSample++ = *SrcSamples++;                                                    
                        AudioOutput->RunningSampleIndex++;                        
                    }
                    
                    SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);                
                }                       
            }
        }
        else
        {
            SoundIsValid = false;
        }                  
        
        
        ak_f32 Delta = (ak_f32)AK_GetElapsedTime(AK_WallClock(), FlipWallClock);
        while(Delta < TargetSecondsPerFrame)        
            Delta = (ak_f32)AK_GetElapsedTime(AK_WallClock(), FlipWallClock);        
        
        FlipWallClock = AK_WallClock();        
        
        AudioOutput->AudioThreadArena->EndTemp(&TempArena);
        AK_Assert(AudioOutput->AudioThreadArena->TemporaryArenas == 0, "Still have some temporary arenas at the end of the audio frame in use");        
    }    
}

void Win32_ProcessMessages(input* Input)
{
    
    MSG Message;
    while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {                    
                Global_Running = false;
            } break;
            
            case WM_INPUT:
            {
                UINT Size;
                UINT Result = GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER));
                
                ak_arena* GlobalArena = AK_GetGlobalArena();
                ak_temp_arena TempArena = GlobalArena->BeginTemp();
                
                void* InputData = GlobalArena->Push(Size, AK_ARENA_CLEAR_FLAGS_NOCLEAR, 8);                    
                Result = GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, InputData, &Size, sizeof(RAWINPUTHEADER));                    
                
                RAWINPUT* RawInput = (RAWINPUT*)InputData;
                RAWINPUTHEADER* RawInputHeader = &RawInput->header;
                
                switch(RawInputHeader->dwType)
                {
                    case RIM_TYPEMOUSE:
                    {
                        RAWMOUSE* RawMouse = &RawInput->data.mouse;                                                        
                        Dev_HandleMouse(RawMouse);                            
                    } break;
                    
                    case RIM_TYPEKEYBOARD:
                    {
                        RAWKEYBOARD* RawKeyboard = &RawInput->data.keyboard;                            
                        ak_bool Quit = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) && (RawKeyboard->VKey == VK_F4) && (RawKeyboard->Flags == RI_KEY_MAKE);
                        if(Quit)
                            PostQuitMessage(0);
                        
                        Quit = (RawKeyboard->VKey == VK_ESCAPE) && (RawKeyboard->Flags == RI_KEY_MAKE);
                        if(Quit)
                            PostQuitMessage(0);
                        
                        switch(RawKeyboard->VKey)
                        {
                            BindKey('W', Input->MoveForward);
                            BindKey('S', Input->MoveBackward);
                            BindKey('A', Input->MoveLeft);
                            BindKey('D', Input->MoveRight);                            
                            BindKey('Q', Input->SwitchWorld);                                
                            BindKey(VK_SPACE, Input->Action);
                        }
                        
                        Dev_HandleKeyboard(RawKeyboard);                            
                    } break;
                }
                
                GlobalArena->EndTemp(&TempArena);
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }    
}

int Win32_GameMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{   
    AK_SetGlobalArena(AK_CreateArena(AK_Megabyte(32)));        
    Global_PlatformArena = AK_CreateArena(AK_Megabyte(1)); 
    
    LARGE_INTEGER Seed;
    QueryPerformanceCounter(&Seed);
    AK_SetRandomSeed64(Seed.QuadPart);
    AK_SetRandomSeed32(Seed.LowPart);
    
    Global_EXEFilePath = AK_GetExecutablePath(Global_PlatformArena);
    ak_string GameDLLPathName = AK_StringConcat(Global_EXEFilePath, "World_Game.dll", Global_PlatformArena);
    ak_string TempDLLPathName = AK_StringConcat(Global_EXEFilePath, "World_Game_Temp.dll", Global_PlatformArena);    
    ak_string OpenGLGraphicsDLLPathName = AK_StringConcat(Global_EXEFilePath, "OpenGL.dll", Global_PlatformArena);
    ak_string OpenGLGraphicsTempDLLPathName = AK_StringConcat(Global_EXEFilePath, "OpenGL_Temp.dll", Global_PlatformArena);        
    ak_string AssetFilePath = AK_StringConcat(Global_EXEFilePath, "WorldGame.assets", Global_PlatformArena);
    
    ak_u16 KeyboardUsage = 6;
    ak_u16 MouseUsage = 2;
    ak_u16 UsagePage = 1;
    
    RAWINPUTDEVICE RawInputDevices[] = 
    {
        {UsagePage, KeyboardUsage,  0, NULL},
        {UsagePage, MouseUsage, 0, NULL}
    };
    
    RegisterRawInputDevices(RawInputDevices, AK_Count(RawInputDevices), sizeof(RAWINPUTDEVICE));
    
    WNDCLASSEX WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32_WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = GAME_NAME;    
    if(!RegisterClassEx(&WindowClass))    
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to register window class");
        return -1;
    }
    
    //HWND Window = Win32_CreateWindow(&WindowClass, GAME_NAME, V2i(1280, 720));
    HWND Window = Win32_CreateWindow(&WindowClass, GAME_NAME, AK_V2(1920, 1080));
    if(!Window)   
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to create the game window");
        return -1;
    }
    
    ak_u32 SoundBufferSeconds = 2;
    win32_audio_output AudioOutput = Win32_InitDSound(Window, SoundBufferSeconds); 
    if(!AudioOutput.SoundBuffer)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to initialize direct sound");
        return -1;
    }
    
    input Input = {};
    
    Global_GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);
    if(!Global_GameCode.GameLibrary.Library)    
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to load the game's dll code");
        return -1;
    }
    
    win32_graphics_code GraphicsCode = Win32_LoadGraphicsCode(OpenGLGraphicsDLLPathName, OpenGLGraphicsTempDLLPathName);
    if(!GraphicsCode.GraphicsLibrary.Library)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to load the graphics's dll code");
        return -1;
    }
    
    void* PlatformData[2] = 
    {
        Window,
        Instance
    };
    
    game* Game = Global_GameCode.Initialize(AK_GetGlobalArena(), &Input, &AudioOutput, AssetFilePath);                                                
    CloseHandle(CreateThread(NULL, 0, Win32_AudioThread, Game, 0, NULL));    
    
    graphics* Graphics = GraphicsCode.InitGraphics(Game->TempStorage, PlatformData);
    if(!Graphics)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to initialize the graphics");
        return-1;
    }
    
    Dev_Initialize(Game, Graphics, PlatformData[0]);    
    
    Global_Running = true;
    
    Game->dtFixed = 1.0f/60.0f;        
    ak_f32 Accumulator = 0.0f;        
    
    ak_high_res_clock CurrentTime = AK_WallClock();
    while(Global_Running)
    {           
        ak_arena* GlobalArena = AK_GetGlobalArena();
        ak_temp_arena TempArena = GlobalArena->BeginTemp();
        
        ak_high_res_clock NewTime = AK_WallClock();
        ak_f32 FrameTime = (ak_f32)AK_GetElapsedTime(NewTime, CurrentTime);
        
        if(FrameTime > 0.05f)
            FrameTime = 0.05f;
        
        CurrentTime = NewTime;
        
        Accumulator += FrameTime;
        
        FILETIME GameDLLWriteTime = Win32_GetFileCreationTime(GameDLLPathName);
        if(CompareFileTime(&Global_GameCode.GameLibrary.LastWriteTime, &GameDLLWriteTime) < 0)
        {
            Global_Lock.Begin();
            {
                Win32_UnloadGameCode(&Global_GameCode, TempDLLPathName);                        
                Global_GameCode = Win32_LoadGameCode(GameDLLPathName, TempDLLPathName);            
            }
            Global_Lock.End();
        }
        
        FILETIME GraphicsDLLWriteTime = Win32_GetFileCreationTime(OpenGLGraphicsDLLPathName);
        if(CompareFileTime(&GraphicsCode.GraphicsLibrary.LastWriteTime, &GraphicsDLLWriteTime) < 0)
        {
            GraphicsCode.InvalidateShaders(Graphics);
            Win32_UnloadGraphicsCode(&GraphicsCode, OpenGLGraphicsTempDLLPathName);            
            GraphicsCode = Win32_LoadGraphicsCode(OpenGLGraphicsDLLPathName, OpenGLGraphicsTempDLLPathName);
            GraphicsCode.BindGraphicsFunctions(Graphics);
        }
        
        //DEVELOPMENT_RECORD_FRAME(Game);
        //DEVELOPMENT_PLAY_FRAME(Game);
        
        while(Accumulator >= Game->dtFixed)
        {   
            AK_Assert(Game->World.OldTransforms[0].Size == Game->World.NewTransforms[0].Size, "Old Entries and New Entries size do not match for World A");
            AK_Assert(Game->World.OldTransforms[1].Size == Game->World.NewTransforms[1].Size, "Old Entries and New Entries size do not match for World B");
            
            AK_CopyArray(Game->World.OldTransforms[0].Entries, Game->World.NewTransforms[0].Entries, Game->World.NewTransforms[0].Size);
            AK_CopyArray(Game->World.OldTransforms[1].Entries, Game->World.NewTransforms[1].Entries, Game->World.NewTransforms[1].Size);
            Game->World.OldCameras[0] = Game->World.NewCameras[0];
            Game->World.OldCameras[1] = Game->World.NewCameras[1];
            
            if(Dev_ShouldPlayGame())
            {                
                //Dev_RecordFrame();
                Global_GameCode.FixedTick(Game, Dev_GetDeveloperContext());                                
            }
            
            Accumulator -= Game->dtFixed;
        }
        
        for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(Input.Buttons); ButtonIndex++)        
            Input.Buttons[ButtonIndex].WasDown = Input.Buttons[ButtonIndex].IsDown; 
        
        Win32_ProcessMessages(&Input);
        
        if(Dev_ShouldPlayGame())
            Global_GameCode.Tick(Game, Dev_GetDeveloperContext());
        
        Game->dt = FrameTime;                
        Game->Resolution = Win32_GetWindowDim(Window);
        
        Dev_Tick();
        
        if(Dev_ShouldPlayGame())
        {
            ak_f32 tInterpolated = Accumulator / Game->dtFixed;
            Global_GameCode.Render(Game, Graphics, tInterpolated);
        }
        
        Dev_Render();
        
        //DEVELOPMENT_TICK(Game, Graphics, &GraphicsState, tRenderInterpolate);                                
        
        GraphicsCode.ExecuteRenderCommands(Graphics, Dev_GetDeveloperContext());
        
        GlobalArena->EndTemp(&TempArena);
        AK_Assert(GlobalArena->TemporaryArenas == 0, "There are still temporary arena's being used in the frame");
    }
    
    return 0;
} 

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{            
    int Result;
    __try
    {        
        Result = Win32_GameMain(Instance, PrevInstance, CmdLineArgs, CmdLineOpts);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {        
        //TODO(JJ): When errors occur (like failed assertions or any unhandled exceptions occur) lets output the last set of frames (about 30 seconds worth) 
        //to a file so we can playback later
        Result = -1;
    }
    
    return Result;
}

#if DEVELOPER_BUILD
PLATFORM_INIT_IMGUI(Platform_InitImGui)
{        
    IO->BackendFlags |= (ImGuiBackendFlags_HasMouseCursors|ImGuiBackendFlags_HasSetMousePos);
    IO->BackendPlatformName = "world_game_win32_platform";
    IO->ImeWindowHandle = (HWND)PlatformWindow;
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
}

PLATFORM_DEVELOPMENT_UPDATE(Platform_DevUpdate)
{
    HWND Window = (HWND)IO->ImeWindowHandle;    
    IO->DisplaySize = ImVec2((ak_f32)RenderDim.w, (ak_f32)RenderDim.h);
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
}

const global ak_char Global_FrameRecordingPrefix[] = "FrameRecording_";
global ak_char* Global_DataPath;
global ak_i32 Global_FrameRecordingIndex = 1;
ak_string Platform_FindNewFrameRecordingPath()
{
    ak_string Result = AK_CreateEmptyString();
    
    if(!Global_DataPath)
    {
        Global_DataPath = Global_PlatformArena->PushArray<ak_char>(Global_EXEFilePath.Length*2);
        DWORD ReadSize = GetCurrentDirectory((DWORD)Global_EXEFilePath.Length*2, Global_DataPath);
        AK_Assert(ReadSize <= Global_EXEFilePath.Length*2, "Directory size for frame recording path is much larger than the executable path");
    }
    
    WIN32_FIND_DATAA FindData;
    HANDLE FileHandle = FindFirstFile("frame_recordings\\*", &FindData);        
    
    while(FileHandle != INVALID_HANDLE_VALUE)
    {                
        if(AK_StringBeginsWith(FindData.cFileName, Global_FrameRecordingPrefix))
        {            
            ak_i32 IndexValue = AK_ToInt(&FindData.cFileName[sizeof(Global_FrameRecordingPrefix)-1]);
            if(IndexValue >= Global_FrameRecordingIndex)
                Global_FrameRecordingIndex = IndexValue+1;
        }
        
        if(!FindNextFile(FileHandle, &FindData))
            break;
    }        
    
    Result = AK_FormatString(AK_GetGlobalArena(), "%s\\frame_recordings\\FrameRecording_%d.recording", Global_DataPath, Global_FrameRecordingIndex);    
    return Result;
}

ak_bool Win32_DevWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if(!GetCurrentContext())
        return false;
    
    ImGuiIO& IO = GetIO();
    
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
            if (!IsAnyMouseDown() && GetCapture() == NULL)
                SetCapture(Window);
            IO.MouseDown[button] = true;                    
        } break;
        
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            ak_i32 Button = 0;
            if (Message == WM_LBUTTONUP) { Button = 0; }
            if (Message == WM_RBUTTONUP) { Button = 1; }
            if (Message == WM_MBUTTONUP) { Button = 2; }
            if (Message == WM_XBUTTONUP) { Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? 3 : 4; }
            IO.MouseDown[Button] = false;
            if (!IsAnyMouseDown() && GetCapture() == Window)
                ReleaseCapture();                                        
        } break;
        
        case WM_MOUSEWHEEL:
        {
            IO.MouseWheel += (ak_f32)GET_WHEEL_DELTA_WPARAM(WParam) / (ak_f32)WHEEL_DELTA;                    
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

void Win32_HandleDevKeyboard(dev_context* DevContext, RAWKEYBOARD* RawKeyboard)
{   
    dev_input* Input = &DevContext->DevInput;
    
    switch(RawKeyboard->VKey)
    {
        BindKey(VK_F5, Input->ToggleDevState);
        BindKey(VK_MENU, Input->Alt);  
        BindKey('W', Input->W);
        BindKey('E', Input->E);
        BindKey('R', Input->R);      
        BindKey('S', Input->S);
        BindKey(VK_DELETE, Input->Delete);
        BindKey(VK_CONTROL, Input->Ctrl);
    }       
}

void Win32_HandleDevMouse(dev_context* DevContext, RAWMOUSE* RawMouse)
{
    dev_input* Input = &DevContext->DevInput;        
    
    switch(RawMouse->usButtonFlags)
    {
        case RI_MOUSE_LEFT_BUTTON_DOWN:
        {
            Input->LMB.IsDown = true;                                    
        } break;
        
        case RI_MOUSE_LEFT_BUTTON_UP:
        {
            Input->LMB.IsDown = false;
            Input->LMB.WasDown = true;
        } break;
        
        case RI_MOUSE_MIDDLE_BUTTON_DOWN:
        {
            Input->MMB.IsDown = true;
        } break;
        
        case RI_MOUSE_MIDDLE_BUTTON_UP:
        {
            Input->MMB.IsDown = false;
            Input->MMB.WasDown = true;
        } break;
        
        case RI_MOUSE_WHEEL:
        {
            Input->Scroll = (ak_f32)(ak_i16)RawMouse->usButtonData / (ak_f32)WHEEL_DELTA;
        } break;
    }           
}

#endif

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>
