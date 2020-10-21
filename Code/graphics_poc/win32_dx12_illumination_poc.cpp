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

global DXGI_FORMAT Global_ColorBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
global DXGI_FORMAT Global_NativeColorBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
global d3d12_serialize_root_signature* D3D12SerializeRootSignature_;
#define D3D12SerializeRootSignature D3D12SerializeRootSignature_

struct mesh
{
    ak_u32 VertexCount;
    ak_vertex_p3_n3* Vertices;
    
    ak_u32 IndexCount;
    ak_u16* Indices;
    
    ak_u32 VertexOffset;
    ak_u32 IndexOffset;
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
    
    void WaitCPU()
    {
        if(Fence->GetCompletedValue() < Value)
        {
            Fence->SetEventOnCompletion(Value, Event);
            WaitForSingleObject(Event, INFINITE);
        }
    }
    
    void WaitGPU(ID3D12CommandQueue* CommandQueue)
    {
        if(Fence->GetCompletedValue() < Value)
            CommandQueue->Wait(Fence, Value);
    }
    
    void Signal(ID3D12CommandQueue* CommandQueue)
    {
        Value++;
        CommandQueue->Signal(Fence, Value);
    }
};

struct descriptor_heap
{
    ID3D12DescriptorHeap* Heap;
    ak_u32 Count;
    ak_u32 IncrementSize;
    
    D3D12_CPU_DESCRIPTOR_HANDLE CPUIndex(ak_u32 Index)
    {
        AK_Assert(Index < Count, "Descriptor index out of bounds");
        D3D12_CPU_DESCRIPTOR_HANDLE Result = Heap->GetCPUDescriptorHandleForHeapStart();
        Result.ptr += (Index*IncrementSize);
        return Result;
    }
    
    D3D12_GPU_DESCRIPTOR_HANDLE GPUIndex(ak_u32 Index)
    {
        AK_Assert(Index < Count, "Descriptor index out of bounds");
        D3D12_GPU_DESCRIPTOR_HANDLE Result = Heap->GetGPUDescriptorHandleForHeapStart();
        Result.ptr += (Index*IncrementSize);
        return Result;
    }
};

struct root_range
{
    ak_fixed_array<D3D12_DESCRIPTOR_RANGE> Ranges;
    
    void AddRange(ak_u32 Index, D3D12_DESCRIPTOR_RANGE_TYPE Type, 
                  ak_u32 NumDescriptors, ak_u32 ShaderRegister, ak_u32 RegisterSpace, ak_u32 Offset)
    {
        AK_Assert(Index < Ranges.Count, "Root Range out of bounds");
        Ranges[Index].RangeType = Type;
        Ranges[Index].NumDescriptors = NumDescriptors;
        Ranges[Index].BaseShaderRegister = ShaderRegister;
        Ranges[Index].RegisterSpace = RegisterSpace;
        Ranges[Index].OffsetInDescriptorsFromTableStart = Offset;
    }
};

struct root_parameters
{
    ak_fixed_array<D3D12_ROOT_PARAMETER> Parameters;
    
    void AddRootConstant(ak_u32 Index, ak_u32 ShaderRegister, ak_u32 RegisterSpace, ak_u32 Num32BitValues, 
                         D3D12_SHADER_VISIBILITY ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        AK_Assert(Index < Parameters.Count, "Root Parameters out of bounds");
        Parameters[Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        Parameters[Index].Constants.ShaderRegister = ShaderRegister;
        Parameters[Index].Constants.RegisterSpace = RegisterSpace;
        Parameters[Index].Constants.Num32BitValues = Num32BitValues;
        Parameters[Index].ShaderVisibility = ShaderVisibility;
    }    
    
    void AddRootDescriptor(ak_u32 Index, D3D12_ROOT_PARAMETER_TYPE Type, ak_u32 ShaderRegister, ak_u32 RegisterSpace, 
                           D3D12_SHADER_VISIBILITY ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        AK_Assert(Index < Parameters.Count, "Root Parameters out of bounds");
        Parameters[Index].ParameterType = Type;
        Parameters[Index].Descriptor.ShaderRegister = ShaderRegister;
        Parameters[Index].Descriptor.RegisterSpace = RegisterSpace;
        Parameters[Index].ShaderVisibility = ShaderVisibility;
    }
    
    void AddRootTable(ak_u32 Index, root_range* Range, 
                      D3D12_SHADER_VISIBILITY ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        AK_Assert(Index < Parameters.Count, "Root Parameters out of bounds");
        Parameters[Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        Parameters[Index].DescriptorTable.NumDescriptorRanges = Range->Ranges.Size;
        Parameters[Index].DescriptorTable.pDescriptorRanges = Range->Ranges.Data;
        Parameters[Index].ShaderVisibility = ShaderVisibility;
    }
};

struct upload_buffer
{
    ID3D12Resource* Resource;
    ak_u8* Start;
    ak_u8* Current;
    ak_u8* End;
    
    void* Push(ak_uaddr Size, ak_i32 Alignment = 0)
    {
        AK_Assert((Current+Size) <= End, "Upload buffer ran out of memory");
        if(Alignment != 0)
            Current = (ak_u8*)AK_Internal__AlignTo((ak_uaddr)Current, Alignment);
        
        void* Result = Current;
        Current += Size;
        return Result;
    }    
    
    void Reset()
    {
        Current = Start;
    }
};

struct push_buffer
{
    ID3D12Resource* Resource;
    ak_u32 Size;
    ak_u32 Used;
    
    ak_u32 Upload(ID3D12GraphicsCommandList* CommandList, upload_buffer* UploadBuffer, void* SrcData, ak_u32 UploadSize)
    {
        AK_Assert((Used+UploadSize) <= Size, "Push buffer ran out of memory");        
        ak_u32 Offset = (ak_u32)(UploadBuffer->Current-UploadBuffer->Start);
        void* DstData = UploadBuffer->Push(UploadSize);
        AK_MemoryCopy(DstData, SrcData, UploadSize);                                        
        CommandList->CopyBufferRegion(Resource, Used, UploadBuffer->Resource, Offset, UploadSize);        
        ak_u32 Result = Used;
        Used += UploadSize;
        return Result;
    }
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(ak_uaddr Offset)
    {
        return Resource->GetGPUVirtualAddress() + Offset;
    }
};

mesh ToMesh(ak_mesh_result<ak_vertex_p3_n3> MeshResult, ID3D12GraphicsCommandList* CommandList, upload_buffer* UploadBuffer, 
            push_buffer* VertexBuffer, push_buffer* IndexBuffer)
{
    mesh Result = {};
    Result.VertexCount = MeshResult.VertexCount;
    Result.IndexCount = MeshResult.IndexCount;
    Result.Vertices = MeshResult.Vertices;
    Result.Indices = MeshResult.Indices;
    
    Result.VertexOffset = VertexBuffer->Upload(CommandList, UploadBuffer, Result.Vertices, sizeof(ak_vertex_p3_n3)*Result.VertexCount) / sizeof(ak_vertex_p3_n3);
    Result.IndexOffset = IndexBuffer->Upload(CommandList, UploadBuffer, Result.Indices, sizeof(ak_u16)*Result.IndexCount) / sizeof(ak_u16);
    
    return Result;
}

root_range BeginRootRange(ak_arena* Arena, ak_u32 Count)
{
    root_range Result = {};
    Result.Ranges.Size = Count;
    Result.Ranges.Data = Arena->PushArray<D3D12_DESCRIPTOR_RANGE>(Count);
    return Result;
}

root_parameters BeginRootParameters(ak_arena* Arena, ak_u32 Count)
{
    root_parameters Result = {};
    Result.Parameters.Size = Count;
    Result.Parameters.Data = Arena->PushArray<D3D12_ROOT_PARAMETER>(Count);
    return Result;
}

D3D12_HEAP_PROPERTIES 
GetDefaultHeapProperties()
{
    D3D12_HEAP_PROPERTIES Result = {};
    Result.Type = D3D12_HEAP_TYPE_DEFAULT;
    return Result;
}

D3D12_HEAP_PROPERTIES 
GetUploadHeapProperties()
{
    D3D12_HEAP_PROPERTIES Result = {};
    Result.Type = D3D12_HEAP_TYPE_UPLOAD;
    return Result;
}

D3D12_RESOURCE_DESC 
GetBufferDescription(ak_u64 BufferSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{    
    D3D12_RESOURCE_DESC Result = {};
    Result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Result.Width = BufferSize;
    Result.Height = 1;
    Result.DepthOrArraySize = 1;
    Result.MipLevels = 1;
    Result.Format = DXGI_FORMAT_UNKNOWN;
    Result.SampleDesc.Count = 1;
    Result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Result.Flags = Flags;
    return Result;
}

ID3D12Resource* CreateBuffer(ID3D12Device* Device, ak_u32 BufferSize, D3D12_RESOURCE_STATES InitialState, 
                             D3D12_HEAP_PROPERTIES HeapProperties, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    ID3D12Resource* Result = NULL;
    D3D12_RESOURCE_DESC Description = GetBufferDescription(BufferSize, Flags);
    if(FAILED(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &Description, InitialState, NULL, __uuidof(ID3D12Resource), (void**)&Result)))
        return NULL;    
    return Result;
}

push_buffer CreatePushBuffer(ID3D12Device* Device, ak_u32 BufferSize, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    ID3D12Resource* Resource = CreateBuffer(Device, BufferSize, InitialState, GetDefaultHeapProperties(), Flags);
    if(Resource)
    {
        push_buffer Result = {};
        Result.Resource = Resource;
        Result.Size = BufferSize;        
        return Result;
    }
    
    return {};
}

upload_buffer CreateUploadBuffer(ID3D12Device* Device, ak_u32 BufferSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    ID3D12Resource* Resource = CreateBuffer(Device, BufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, GetUploadHeapProperties(), Flags);
    if(Resource)
    {
        void* Data;
        if(FAILED(Resource->Map(0, NULL, &Data))) { COM_RELEASE(Resource); return {}; }
        
        upload_buffer Result = {};
        Result.Resource = Resource;
        Result.Start = (ak_u8*)Data;
        Result.Current = Result.Start;
        Result.End = Result.Start + BufferSize;
        
        return Result;
    }    
    return {};
}

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

descriptor_heap CreateDescriptorHeap(ID3D12Device* Device, ak_u32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
{
    D3D12_DESCRIPTOR_HEAP_DESC Description = {};
    Description.Type = Type;
    Description.Flags = Flags;
    Description.NumDescriptors = DescriptorCount;  
    
    ID3D12DescriptorHeap* Heap = NULL;
    if(FAILED(Device->CreateDescriptorHeap(&Description, __uuidof(ID3D12DescriptorHeap), (void**)&Heap))) return {};
    
    descriptor_heap Result = {};
    Result.Heap = Heap;
    Result.Count = DescriptorCount;
    Result.IncrementSize = Device->GetDescriptorHandleIncrementSize(Type);
    return Result;
}

ak_fixed_array<DXGI_MODE_DESC> GetDisplayModes(IDXGIOutput* Output, ak_arena* Arena)
{    
    ak_u32 NumModes;
    if(FAILED(Output->GetDisplayModeList(Global_NativeColorBufferFormat, 0, &NumModes, NULL))) return {};
    
    DXGI_MODE_DESC* DisplayModes = Arena->PushArray<DXGI_MODE_DESC>(NumModes);
    if(FAILED(Output->GetDisplayModeList(Global_NativeColorBufferFormat, 0, &NumModes, DisplayModes))) return {};
    
    ak_fixed_array<DXGI_MODE_DESC> Result = AK_CreateArray(DisplayModes, NumModes);
    return Result;
}

ak_fixed_array<ID3D12Resource*> GetSwapChainBuffers(IDXGISwapChain* SwapChain, ak_arena* Arena, ak_u32 Count)
{
    ID3D12Resource** Resources = Arena->PushArray<ID3D12Resource*>(Count);
    for(ak_u32 Index = 0; Index < Count; Index++)
    {
        if(FAILED(SwapChain->GetBuffer(Index, __uuidof(ID3D12Resource), (void**)&Resources[Index]))) 
            return {};
    }
    
    ak_fixed_array<ID3D12Resource*> Result = AK_CreateArray<ID3D12Resource*>(Resources, Count);
    return Result;
}

void CreateRenderTargetView(ID3D12Device* Device, ID3D12Resource* Resource, descriptor_heap* Descriptors, ak_u32 DescriptorIndex, DXGI_FORMAT Format)
{
    D3D12_RENDER_TARGET_VIEW_DESC Description = {};
    Description.Format = Format;
    Description.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    Device->CreateRenderTargetView(Resource, &Description, Descriptors->CPUIndex(DescriptorIndex));
}

D3D12_RESOURCE_BARRIER 
GetTransitionBarrier(ID3D12Resource* Resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
    D3D12_RESOURCE_BARRIER Result;
    Result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Result.Transition.pResource = Resource;
    Result.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Result.Transition.StateBefore = StateBefore;
    Result.Transition.StateAfter = StateAfter;
    return Result;
}

ID3D12RootSignature* CreateRootSignature(ID3D12Device* Device, root_parameters Parameters, 
                                         D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE, 
                                         ak_u32 NumStaticSamples = 0, D3D12_STATIC_SAMPLER_DESC* SamplersDesc = NULL)
{
    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.NumParameters = Parameters.Parameters.Size;
    RootSignatureDesc.pParameters = Parameters.Parameters.Data;
    RootSignatureDesc.NumStaticSamplers = NumStaticSamples;
    RootSignatureDesc.pStaticSamplers = SamplersDesc;
    RootSignatureDesc.Flags = Flags;
        
    ID3DBlob* DataBlob = NULL;
    ID3DBlob* ErrorBlob = NULL;  
    
    if(FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &DataBlob, &ErrorBlob)))
    {
        AK_ConsoleLog("Failed to serialize the root signature. Message: %.*s\n", ErrorBlob->GetBufferSize(), ErrorBlob->GetBufferPointer());
        return NULL;
    }
    
    ID3D12RootSignature* Result = NULL;
    if(FAILED(Device->CreateRootSignature(0, DataBlob->GetBufferPointer(), DataBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&Result))) return NULL;
    
    COM_RELEASE(DataBlob);
    COM_RELEASE(ErrorBlob);    
    return Result;
}

inline D3D12_BLEND_DESC 
GetDefaultBlendState()
{
    D3D12_BLEND_DESC DefaultBlendState = {};    
    DefaultBlendState.AlphaToCoverageEnable = FALSE;
    DefaultBlendState.IndependentBlendEnable = FALSE;
    DefaultBlendState.RenderTarget[0].BlendEnable = FALSE;
    DefaultBlendState.RenderTarget[0].LogicOpEnable = FALSE;
    DefaultBlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    DefaultBlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    DefaultBlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    DefaultBlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    DefaultBlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    DefaultBlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    DefaultBlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    DefaultBlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return DefaultBlendState;
}

inline D3D12_RASTERIZER_DESC 
GetDefaultRasterizerState()
{
    D3D12_RASTERIZER_DESC DefaultRasterizerState = {};
    DefaultRasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    DefaultRasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    DefaultRasterizerState.FrontCounterClockwise = TRUE;
    DefaultRasterizerState.DepthBias = 0;
    DefaultRasterizerState.DepthBiasClamp = 0.0f;
    DefaultRasterizerState.SlopeScaledDepthBias = 0.0f;
    DefaultRasterizerState.DepthClipEnable = TRUE;
    DefaultRasterizerState.MultisampleEnable = FALSE;
    DefaultRasterizerState.AntialiasedLineEnable = FALSE;
    DefaultRasterizerState.ForcedSampleCount = 0;
    DefaultRasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;  
    return DefaultRasterizerState;
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
    
    HMODULE DXGILib = LoadLibrary("DXGI.dll");
    if(!DXGILib) { AK_MessageBoxOk("Fatal Error", "Failed to load the DXGI Library. Qutting"); return -1; }
    
    HMODULE D3D12Lib = LoadLibrary("D3D12.dll");
    if(!D3D12Lib) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12 Library. Qutting"); return -1; }                                    
    
    create_dxgi_factory* CreateDXGIFactory = (create_dxgi_factory*)GetProcAddress(DXGILib, "CreateDXGIFactory");
    if(!CreateDXGIFactory) { AK_MessageBoxOk("Fatal Error", "Could not load the CreateDXGIFactory function from DXGI.dll. Qutting"); return -1; }
    
    d3d12_create_device* D3D12CreateDevice = (d3d12_create_device*)GetProcAddress(D3D12Lib, "D3D12CreateDevice");
    if(!D3D12CreateDevice) { AK_MessageBoxOk("Fatal Error", "Could not load the D3D12CreateDevice function from D3D12.dll. Quitting"); return -1; }
    
    D3D12SerializeRootSignature  = (d3d12_serialize_root_signature*)GetProcAddress(D3D12Lib, "D3D12SerializeRootSignature");
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
    
    ak_fixed_array<ID3D12Resource*> SwapChainBuffers = GetSwapChainBuffers(SwapChain, AppArena, SwapChainDescription.BufferCount);
    if(!SwapChainBuffers.Data) { AK_MessageBoxOk("Fatal Error", "Failed to get the swap chain buffers"); return -1; }
    
    descriptor_heap RTVDescriptors = CreateDescriptorHeap(Device, SwapChainBuffers.Size, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if(!RTVDescriptors.Heap) { AK_MessageBoxOk("Fatal Error", "Failed to create the render target view heap"); return -1; }
    
    for(ak_u32 BufferIndex = 0; BufferIndex < SwapChainBuffers.Size; BufferIndex++)
        CreateRenderTargetView(Device, SwapChainBuffers[BufferIndex], &RTVDescriptors, BufferIndex, Global_ColorBufferFormat);
    
    upload_buffer UploadBuffer = CreateUploadBuffer(Device, AK_Megabyte(8));
    if(!UploadBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the upload buffer"); return -1; }
    
    push_buffer VertexBuffer = CreatePushBuffer(Device, AK_Megabyte(32), D3D12_RESOURCE_STATE_COPY_DEST);
    if(!VertexBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the vertex buffer"); return -1; }
    
    push_buffer IndexBuffer = CreatePushBuffer(Device, AK_Megabyte(32), D3D12_RESOURCE_STATE_COPY_DEST);
    if(!IndexBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the index buffer"); return -1; }
    
    DirectCommandAllocator->Reset();
    DirectCommandList->Reset(DirectCommandAllocator, NULL);
    
    mesh BoxMesh = ToMesh(AK_GenerateTriangleBoxN(AppArena, AK_V3(1.0f, 1.0f, 1.0f)), DirectCommandList, &UploadBuffer, &VertexBuffer, &IndexBuffer);            
    
    D3D12_RESOURCE_BARRIER Barriers[2] = 
    {
        GetTransitionBarrier(VertexBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        GetTransitionBarrier(IndexBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
    };
    
    DirectCommandList->ResourceBarrier(2, Barriers);    
    
    DirectCommandList->Close();
    DirectCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&DirectCommandList);
    Fence.Signal(DirectCommandQueue);
    
    ID3D12RootSignature* RootSignature = CreateRootSignature(Device, {}, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);    
    if(!RootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the root signature"); return -1; }
        
    ak_temp_arena TempArena = AppArena->BeginTemp();
    ak_buffer VSResults = AK_ReadEntireFile("VertexShader.cso", AppArena);
    ak_buffer PSResults = AK_ReadEntireFile("PixelShader.cso", AppArena);    
    if(!VSResults.IsValid() || !PSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the hlsl shaders"); return -1; }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateDesc = {};
    
    D3D12_INPUT_ELEMENT_DESC InputElements[] = 
    {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    PipelineStateDesc.pRootSignature = RootSignature;
    PipelineStateDesc.InputLayout = {InputElements, 1};
    PipelineStateDesc.VS = {VSResults.Data, VSResults.Size};
    PipelineStateDesc.PS = {PSResults.Data, PSResults.Size};
    PipelineStateDesc.BlendState = GetDefaultBlendState();
    PipelineStateDesc.SampleMask = 0xFFFFFFFF;
    PipelineStateDesc.RasterizerState = GetDefaultRasterizerState();
    PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PipelineStateDesc.NumRenderTargets = 1;
    PipelineStateDesc.RTVFormats[0] = Global_ColorBufferFormat;
    PipelineStateDesc.SampleDesc.Count = 1;
    
    ID3D12PipelineState* PipelineState = NULL;
    if(FAILED(Device->CreateGraphicsPipelineState(&PipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&PipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the pipeline state"); return -1; }
    
    
    ak_array<object> Objects = {};            
    ak_array<light> Lights = {};    
    
    AppArena->EndTemp(&TempArena);                        
    Global_Running = true;
    
    ak_u32 FrameIndex = 0;
    while(Global_Running)
    {
        ak_high_res_clock Begin = AK_WallClock();
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        TempArena = GlobalArena->BeginTemp();
        
        Win32_ProcessMessages();        
        
        Fence.WaitCPU();
        DirectCommandAllocator->Reset();
        DirectCommandList->Reset(DirectCommandAllocator, NULL);
        
        Resolution = {};
        AK_GetWindowResolution(PlatformWindow, (ak_u16*)&Resolution.w, (ak_u16*)&Resolution.h);
        
        D3D12_RESOURCE_BARRIER Barrier = GetTransitionBarrier(SwapChainBuffers[FrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DirectCommandList->ResourceBarrier(1, &Barrier);
        
        D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle = RTVDescriptors.CPUIndex(FrameIndex);
        DirectCommandList->OMSetRenderTargets(1, &RenderTargetHandle, FALSE, NULL);
        
        ak_color4f ClearColor = AK_Blue4();
        DirectCommandList->ClearRenderTargetView(RenderTargetHandle, ClearColor.Data, 0, NULL);
        
        D3D12_VIEWPORT Viewport = {0, 0, (ak_f32)Resolution.w, (ak_f32)Resolution.h};
        D3D12_RECT Rect = {0, 0, Resolution.w, Resolution.h};
        
        DirectCommandList->RSSetViewports(1, &Viewport);
        DirectCommandList->RSSetScissorRects(1, &Rect);
        
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
        VertexBufferView.BufferLocation = VertexBuffer.GetGPUAddress(0);
        VertexBufferView.SizeInBytes = VertexBuffer.Size;
        VertexBufferView.StrideInBytes = sizeof(ak_vertex_p3_n3);
        
        D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
        IndexBufferView.BufferLocation = IndexBuffer.GetGPUAddress(0);
        IndexBufferView.SizeInBytes = IndexBuffer.Size;
        IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
        
        DirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        DirectCommandList->IASetIndexBuffer(&IndexBufferView);
        DirectCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
        
        DirectCommandList->SetPipelineState(PipelineState);
        DirectCommandList->SetGraphicsRootSignature(RootSignature);
        
        DirectCommandList->DrawIndexedInstanced(BoxMesh.IndexCount, 1, BoxMesh.IndexOffset, BoxMesh.VertexOffset, 0);
        
        Barrier = GetTransitionBarrier(SwapChainBuffers[FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        DirectCommandList->ResourceBarrier(1, &Barrier);
        
        DirectCommandList->Close();
        DirectCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&DirectCommandList);
        Fence.Signal(DirectCommandQueue);
        
        SwapChain->Present(1, 0);
        
        FrameIndex = (FrameIndex+1) % SwapChainBuffers.Size;                                                              
        
        GlobalArena->EndTemp(&TempArena);        
        
        ak_high_res_clock End = AK_WallClock();
        ak_f64 Elapsed = AK_GetElapsedTime(End, Begin);
        
        AK_ConsoleLog("Elapsed %f ms\n", Elapsed*1000.0);
    }
    
    return 0;
}