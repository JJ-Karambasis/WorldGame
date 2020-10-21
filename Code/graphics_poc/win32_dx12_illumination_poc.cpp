#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>
#include <dxgi.h>
#include <d3d12.h>

#define COM_RELEASE(com) \
if(com) \
com->Release(); \
com = NULL

typedef HRESULT create_dxgi_factory(REFIID riid, void** ppFactory);
typedef HRESULT d3d12_get_debug_interface(REFIID riid, void **ppvDebug);
typedef HRESULT d3d12_create_device(IUnknown *pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void **ppDevice);
typedef HRESULT d3d12_serialize_root_signature(const D3D12_ROOT_SIGNATURE_DESC *pRootSignature, D3D_ROOT_SIGNATURE_VERSION Version, ID3DBlob **ppBlob, ID3DBlob **ppErrorBlob);

struct mesh
{
    ak_u32 VertexCount;
    ak_vertex_p3_n3* Vertices;
    
    ak_u32 IndexCount;
    ak_u16* Indices;
};

struct object
{
    ak_sqtf     Transform;
    ak_color4f  Color;
    mesh*       Mesh;
};

struct camera
{
    ak_v3f Position;
    ak_v3f Direction;    
};

struct light
{
    ak_v3f Position;
    ak_f32 Radius;
    ak_color3f Color;
    ak_f32 Intensity;
};

struct command_fence
{
    ID3D12Fence* Fence;
    HANDLE Event;
    ak_u32 Value;
    
    void CPUWait()
    {
        if(Fence->GetCompletedValue() < Value)
        {
            Fence->SetEventOnCompletion(Value, Event);
            WaitForSingleObject(Event, INFINITE);
        }
    }
    
    void Signal(ID3D12CommandQueue* CommandQueue)
    {
        Value++;
        CommandQueue->Signal(Fence, Value);
    }
};

object CreateObject(ak_v3f Position, ak_quatf Orientation, ak_v3f Scale, mesh* Mesh, ak_color4f Color)
{
    object Result;
    Result.Transform = AK_SQT(Position, Orientation, Scale);
    Result.Color = Color;
    Result.Mesh = Mesh;
    return Result;
}

light CreateLight(ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    light Light;
    Light.Position = Position;
    Light.Radius = Radius;
    Light.Color = Color;
    Light.Intensity = Intensity;
    return Light;
}

global ak_bool Global_Running;
void Win32_ProcessMessages()
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
            return;
        
        switch(Message.message)
        {
            case WM_QUIT:
            {                    
                Global_Running = false;
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }        
}

ak_fixed_array<IDXGIAdapter*> GetAdapters(ak_arena* Arena, IDXGIFactory* Factory)
{
    ak_u32 Count = 0;
    for(;;)
    {
        IDXGIAdapter* Adapter;
        if(Factory->EnumAdapters(Count, &Adapter) == DXGI_ERROR_NOT_FOUND)
            break;
        
        COM_RELEASE(Adapter);
        Count++;
    }
    
    IDXGIAdapter** Adapters = Arena->PushArray<IDXGIAdapter*>(Count);
    for(ak_u32 AdapterIndex = 0; AdapterIndex < Count; AdapterIndex++)
        Factory->EnumAdapters(AdapterIndex, &Adapters[AdapterIndex]);
    
    return AK_CreateArray(Adapters, Count);
}

IDXGIAdapter* GetFirstCompatibleAdapter(ak_fixed_array<IDXGIAdapter*> Adapters, d3d12_create_device* D3D12CreateDevice)
{
    AK_ForEach(Adapter, &Adapters)
    {
        if(SUCCEEDED(D3D12CreateDevice(*Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), NULL)))
            return *Adapter;
    }    
    return NULL;
}

ID3D12CommandQueue* CreateCommandQueue(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
{
    ID3D12CommandQueue* CommandQueue = NULL;
    
    D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
    CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    CommandQueueDesc.Type = Type;
    if(FAILED(Device->CreateCommandQueue(&CommandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&CommandQueue))) return NULL;    
    return CommandQueue;
}

ID3D12CommandAllocator* 
CreateCommandAllocator(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
{            
    ID3D12CommandAllocator* CommandAllocator;
    if(FAILED(Device->CreateCommandAllocator(Type, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocator))) return NULL;    
    return CommandAllocator;
}

ID3D12GraphicsCommandList* 
CreateCommandList(ID3D12Device* Device, ID3D12CommandAllocator* Allocator, D3D12_COMMAND_LIST_TYPE Type)
{
    ID3D12GraphicsCommandList* CommandList;
    if(FAILED(Device->CreateCommandList(0, Type, Allocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&CommandList))) return NULL;
    CommandList->Close();
    return CommandList;
}

command_fence CreateCommandFence(ID3D12Device* Device)
{
    command_fence Result = {};    
    if(FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Result.Fence))) return {};
    Result.Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!Result.Event) return {};    
    return Result;
}

ak_fixed_array<DXGI_MODE_DESC> GetDisplayModes(IDXGIOutput* Output, ak_arena* Arena)
{    
    ak_u32 NumModes;
    if(FAILED(Output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, NULL))) return {};
    
    DXGI_MODE_DESC* DisplayModes = Arena->PushArray<DXGI_MODE_DESC>(NumModes);
    if(FAILED(Output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, DisplayModes))) return {};
    
    ak_fixed_array<DXGI_MODE_DESC> Result = AK_CreateArray(DisplayModes, NumModes);
    return Result;
}

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{   
    AK_SetGlobalArena(AK_CreateArena(AK_Megabyte(32)));
    
    ak_arena* AppArena = AK_CreateArena(AK_Megabyte(1));
    
    ak_window* PlatformWindow = AK_CreateWindow(1920, 1080, "Graphics_POC");
    if(!PlatformWindow)
    {
        AK_MessageBoxOk("Fatal Error", "Failed to create the platform window. Quitting");
        return -1;
    }
    
    ak_array<object> Objects = {};            
    ak_array<light> Lights = {};
    
    HMODULE DXGILib = LoadLibrary("DXGI.dll");
    if(!DXGILib) { AK_MessageBoxOk("Fatal Error", "Failed to load the DXGI Library. Qutting"); return -1; }
    
    HMODULE D3D12Lib = LoadLibrary("D3D12.dll");
    if(!D3D12Lib) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12 Library. Qutting"); return -1; }                                    
    
    create_dxgi_factory* CreateDXGIFactory = (create_dxgi_factory*)GetProcAddress(DXGILib, "CreateDXGIFactory");
    if(!CreateDXGIFactory) { AK_MessageBoxOk("Fatal Error", "Could not load the CreateDXGIFactory function from DXGI.dll. Qutting"); return -1; }
    
    d3d12_create_device* D3D12CreateDevice = (d3d12_create_device*)GetProcAddress(D3D12Lib, "D3D12CreateDevice");
    if(!D3D12CreateDevice) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12CreateDevice function from D3D12.dll. Quitting"); return -1; }
    
    d3d12_serialize_root_signature* D3D12SerializeRootSignature  = (d3d12_serialize_root_signature*)GetProcAddress(D3D12Lib, "D3D12SerializeRootSignature");
    if(!D3D12SerializeRootSignature) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12SerializeRootSignature function form D3D12.dll. Qutting"); return -1; }
    
#if DEVELOPER_BUILD
    d3d12_get_debug_interface* D3D12GetDebugInterface = (d3d12_get_debug_interface*)GetProcAddress(D3D12Lib, "D3D12GetDebugInterface");
    if(!D3D12GetDebugInterface) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12GetDebugInterface function from D3D12.dll. Qutting"); return -1; }
    
    ID3D12Debug* DebugInterface = NULL;
    if(SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&DebugInterface)))    
        DebugInterface->EnableDebugLayer();    
#endif
    
    IDXGIFactory* Factory;
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory))) { AK_MessageBoxOk("Fatal Error", "Could not create the dxgi factory from DXGI.dll. Qutting"); return -1; }
    
    ak_fixed_array<IDXGIAdapter*> Adapters = GetAdapters(AppArena, Factory);
    IDXGIAdapter* Adapter = GetFirstCompatibleAdapter(Adapters, D3D12CreateDevice);
    if(!Adapter) { AK_MessageBoxOk("Fatal Error", "Could not find a valid DXGI Adapter for D3D12 usage. Qutting"); return -1; }
    
    ID3D12Device* Device;
    if(FAILED(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Device))) { AK_MessageBoxOk("Fatal Error", "Failed to create the D3D12 Device. Qutting"); return -1; }    
    
    ID3D12CommandQueue* DirectCommandQueue = CreateCommandQueue(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if(!DirectCommandQueue) {AK_MessageBoxOk("Fatal Error", "Could not create the D3D12 direct command queue"); return -1; }
    
    ID3D12CommandAllocator* DirectCommandAllocator = CreateCommandAllocator(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if(!DirectCommandAllocator) { AK_MessageBoxOk("Fatal Error", "Could not create the D3D12 direct command allocator"); return -1; }
    
    ID3D12GraphicsCommandList* DirectCommandList = CreateCommandList(Device, DirectCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if(!DirectCommandList) { AK_MessageBoxOk("Fatal Error", "Could not create the D3D12 direct command list"); return -1; }
    
    command_fence Fence = CreateCommandFence(Device);
    if(!Fence.Fence) { AK_MessageBoxOk("Fatal Error", "Create not create the D3D12 command fence"); return -1; }
    
    IDXGIOutput* Output;
    if(FAILED(Adapter->EnumOutputs(0, &Output))) { AK_MessageBoxOk("Fatal Error", "Could not find the display output for the graphics adapter"); return -1; }
    
    ak_fixed_array<DXGI_MODE_DESC> DisplayModes = GetDisplayModes(Output, AppArena);
    if(!DisplayModes.Data)  { AK_MessageBoxOk("Fatal Error", "Could not receive the display modes"); return -1; }
    
    DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
    SwapChainDescription.BufferCount = 2;    
    SwapChainDescription.SampleDesc.Count = 1;
    SwapChainDescription.SampleDesc.Quality = 0;
    SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDescription.OutputWindow = AK_GetPlatformWindow(PlatformWindow);
    SwapChainDescription.Windowed = TRUE;
    SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    
    
    ak_v2i Resolution = {};
    AK_GetWindowResolution(PlatformWindow, (ak_u16*)&Resolution.w, (ak_u16*)&Resolution.h);
    
    AK_ForEach(Mode, &DisplayModes)
    {
        if((Mode->Width == (ak_u32)Resolution.w) && (Mode->Height == (ak_u32)Resolution.h))
        {
            SwapChainDescription.BufferDesc = *Mode;
            break;
        }
    }
    
    IDXGISwapChain* SwapChain;
    if(FAILED(Factory->CreateSwapChain(DirectCommandQueue, &SwapChainDescription, &SwapChain))) { AK_MessageBoxOk("Fatal Error", "Failed to create the swap chain"); return -1; }
    
    
                
    Global_Running = true;
    while(Global_Running)
    {
        Win32_ProcessMessages();        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        ak_temp_arena TempArena = GlobalArena->BeginTemp();
        
        GlobalArena->EndTemp(&TempArena);
    }
    
    return 0;
}