#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>
#include <dxgi.h>
#include <d3d12.h>
#include "shader_config.h"

#define BindKey(key, action) case key: { action.IsDown = IsDown; action.WasDown = WasDown; } break

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
global DXGI_FORMAT Global_DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;
global d3d12_serialize_root_signature* D3D12SerializeRootSignature_;
#define D3D12SerializeRootSignature D3D12SerializeRootSignature_

struct mesh_p3_n3
{
    ak_u32 VertexCount;
    ak_vertex_p3_n3* Vertices;
    
    ak_u32 IndexCount;
    ak_u16* Indices;
    
    ak_u32 VertexOffset;
    ak_u32 IndexOffset;
};

struct mesh_p3
{
    ak_u32 VertexCount;
    ak_vertex_p3* Vertices;
    
    ak_u32 IndexCount;
    ak_u16* Indices;
    
    ak_u32 VertexOffset;
    ak_u32 IndexOffset;
};

struct box
{
    union
    {
        struct
        {
            ak_v3f Min;
            ak_v3f Max;
        };
        
        ak_v3f Bounds[2];
    };
};

struct object
{
    ak_m4f      Transform;
    ak_color4f  Color;
    mesh_p3_n3* Mesh;
    
    box OriginalBox;
    box CurrentBox;    
};

struct camera
{
    ak_v3f Target;
    ak_v3f SphericalCoordinates;    
};

struct view_settings
{
    ak_v3f Position;
    ak_m3f Orientation;
};

struct light
{
    ak_v3f Position;
    ak_f32 Radius;
    ak_color3f Color;
    ak_f32 Intensity;
};

struct button
{
    ak_bool WasDown;
    ak_bool IsDown;
};

struct input
{
    union
    {
        button Buttons[3];
        struct
        {            
            button LMB;
            button MMB;                                                                                                            
            button Alt;
        };
    };
    
    ak_v2i LastMouseCoordinates;
    ak_v2i MouseCoordinates;
    ak_f32 Scroll;
};

struct ray_trace_probe
{
    ak_u32 ProbeIndex;
    ak_i32 RaysPerProbe;
    ak_array<object> Objects;
    ak_v3f  RayOrigin;
    ak_v4f* RayDirections;
    ak_u32* HitAlbedo;
    ak_v4f* HitPositions;
    ak_v4f* HitNormals;    
    ak_f32* RayDepths;    
};

inline ak_bool IsDown(button Button)
{
    ak_bool Result = Button.IsDown;
    return Result;
}

inline ak_bool IsPressed(button Button)
{
    ak_bool Result = IsDown(Button) && !Button.WasDown;
    return Result;
}

inline ak_bool IsReleased(button Button)
{
    ak_bool Result = !IsDown(Button) && Button.WasDown;
    return Result;
}

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
            Current = (ak_u8*)AK_AlignTo((ak_uaddr)Current, Alignment);
        
        void* Result = Current;
        Current += Size;
        return Result;
    }    
    
    void Reset()
    {
        Current = Start;
    }
};

struct normal_buffer
{
    ID3D12Resource* Resource;
    ak_u32          Size;
    
    void Upload(ID3D12GraphicsCommandList* CommandList, upload_buffer* UploadBuffer, void* SrcData, ak_u32 UploadSize, ak_u32 BufferOffset)
    {
        AK_Assert((BufferOffset+UploadSize) <= Size, "Buffer ran out of memory");
        ak_u32 UploadOffset = (ak_u32)(UploadBuffer->Current-UploadBuffer->Start);
        void* DstData = UploadBuffer->Push(UploadSize);
        AK_MemoryCopy(DstData, SrcData, UploadSize);
        CommandList->CopyBufferRegion(Resource, BufferOffset, UploadBuffer->Resource, UploadOffset, UploadSize);        
    }
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(ak_uaddr Offset)
    {
        return Resource->GetGPUVirtualAddress() + Offset;
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

view_settings GetViewSettings(camera* Camera)
{    
    view_settings ViewSettings = {};
    ak_v3f Up = AK_ZAxis();        
    ak_f32 Degree = AK_ToDegree(Camera->SphericalCoordinates.inclination);                    
    if(Degree > 0)
        Up = -AK_ZAxis();    
    if(Degree < -180.0f)
        Up = -AK_YAxis();
    if(AK_EqualZeroEps(Degree))
        Up = AK_YAxis(); 
    if(AK_EqualEps(AK_Abs(Degree), 180.0f))
        Up = -AK_YAxis();        
    
    ViewSettings.Position = Camera->Target + AK_SphericalToCartesian(Camera->SphericalCoordinates);            
    ViewSettings.Orientation = AK_OrientAt(ViewSettings.Position, Camera->Target, Up);           
    return ViewSettings;
}

mesh_p3_n3 ToMesh(ak_mesh_result<ak_vertex_p3_n3> MeshResult, ID3D12GraphicsCommandList* CommandList, upload_buffer* UploadBuffer, 
                  push_buffer* VertexBuffer, push_buffer* IndexBuffer)
{
    mesh_p3_n3 Result = {};
    Result.VertexCount = MeshResult.VertexCount;
    Result.IndexCount = MeshResult.IndexCount;
    Result.Vertices = MeshResult.Vertices;
    Result.Indices = MeshResult.Indices;
    
    Result.VertexOffset = VertexBuffer->Upload(CommandList, UploadBuffer, Result.Vertices, sizeof(ak_vertex_p3_n3)*Result.VertexCount) / sizeof(ak_vertex_p3_n3);
    Result.IndexOffset = IndexBuffer->Upload(CommandList, UploadBuffer, Result.Indices, sizeof(ak_u16)*Result.IndexCount) / sizeof(ak_u16);
    
    return Result;
}

mesh_p3 ToDebugMesh(ak_mesh_result<ak_vertex_p3> MeshResult, ID3D12GraphicsCommandList* CommandList, upload_buffer* UploadBuffer, 
                    push_buffer* VertexBuffer, push_buffer* IndexBuffer, ak_u32 DebugOffset)
{
    mesh_p3 Result = {};
    Result.VertexCount = MeshResult.VertexCount;
    Result.IndexCount = MeshResult.IndexCount;
    Result.Vertices = MeshResult.Vertices;
    Result.Indices = MeshResult.Indices;
    
    ak_u32 Diff = VertexBuffer->Upload(CommandList, UploadBuffer, Result.Vertices, sizeof(ak_vertex_p3)*Result.VertexCount) - DebugOffset;        
    Result.VertexOffset = Diff / sizeof(ak_vertex_p3);
    Result.IndexOffset = IndexBuffer->Upload(CommandList, UploadBuffer, Result.Indices, sizeof(ak_u16)*Result.IndexCount) / sizeof(ak_u16);
    
    return Result;
}

void UploadTexture(ID3D12GraphicsCommandList* CommandList, ID3D12Resource* DstTexture, upload_buffer* UploadBuffer, 
                   void* SrcTexture, DXGI_FORMAT Format, ak_u32 Width, ak_u32 Height, ak_u32 Pitch)
{    
    D3D12_SUBRESOURCE_FOOTPRINT Footprint = {};
    Footprint.Format = Format;
    Footprint.Width = Width;
    Footprint.Height = Height;
    Footprint.Depth = 1;
    Footprint.RowPitch = (ak_u32)AK_AlignTo(Pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);    
    
    ak_u8* UploadData = (ak_u8*)UploadBuffer->Push(Footprint.Height*Footprint.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);    
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedTexture2D = {};
    PlacedTexture2D.Offset = (ak_uaddr)UploadData - (ak_uaddr)UploadBuffer->Start;
    PlacedTexture2D.Footprint = Footprint;
    
    ak_u8* Src = (ak_u8*)SrcTexture;            
    ak_u8* Dst = (ak_u8*)UploadData;
    for(ak_u32 YIndex = 0; YIndex < Footprint.Height; YIndex++)
    {        
        AK_MemoryCopy(Dst, Src, Pitch);        
        Dst += Footprint.RowPitch;
        Src += Pitch;
    }
    
    D3D12_TEXTURE_COPY_LOCATION DstCopyLocation = {};
    DstCopyLocation.pResource = DstTexture;
    DstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DstCopyLocation.SubresourceIndex = 0;        
    
    D3D12_TEXTURE_COPY_LOCATION SrcCopyLocation = {};
    SrcCopyLocation.pResource = UploadBuffer->Resource;
    SrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    SrcCopyLocation.PlacedFootprint = PlacedTexture2D;      
    
    CommandList->CopyTextureRegion(&DstCopyLocation, 0, 0, 0, &SrcCopyLocation, NULL);                
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

ID3D12Resource* CreateTexture1D(ID3D12Device* Device, ak_u32 Width, DXGI_FORMAT Format, D3D12_RESOURCE_STATES InitialState, 
                                D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    D3D12_RESOURCE_DESC Description = {};
    Description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    Description.Width = Width;
    Description.Height = 1;
    Description.DepthOrArraySize = 1;
    Description.MipLevels = 1;
    Description.Format = Format;
    Description.SampleDesc.Count = 1;
    Description.Flags = Flags;
    
    D3D12_HEAP_PROPERTIES DefaultHeap = GetDefaultHeapProperties();
    
    ID3D12Resource* Result = NULL;
    if(FAILED(Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &Description, InitialState, NULL, __uuidof(ID3D12Resource), (void**)&Result))) return NULL;
    return Result;
}

ID3D12Resource* CreateTexture2D(ID3D12Device* Device, ak_u32 Width, ak_u32 Height, DXGI_FORMAT Format, D3D12_RESOURCE_STATES InitialState, 
                                D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    D3D12_RESOURCE_DESC Description = {};
    Description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    Description.Width = Width;
    Description.Height = Height;
    Description.DepthOrArraySize = 1;
    Description.MipLevels = 1;
    Description.Format = Format;
    Description.SampleDesc.Count = 1;
    Description.Flags = Flags;
    
    D3D12_HEAP_PROPERTIES DefaultHeap = GetDefaultHeapProperties();
    
    ID3D12Resource* Result = NULL;
    if(FAILED(Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &Description, InitialState, NULL, __uuidof(ID3D12Resource), (void**)&Result))) return NULL;
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

normal_buffer CreateNormalBuffer(ID3D12Device* Device, ak_u32 BufferSize, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    ID3D12Resource* Resource = CreateBuffer(Device, BufferSize, InitialState, GetDefaultHeapProperties(), Flags);
    if(Resource)
    {
        normal_buffer Result = {};
        Result.Resource = Resource;
        Result.Size = BufferSize;
        return Result;
    }
    
    return {};
}

ID3D12Resource* CreateDepthBuffer(ID3D12Device* Device, ak_v2i Resolution, ak_u16 DepthSize=1)
{
    D3D12_RESOURCE_DESC DepthStencilDesc = {};
    DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Alignment = 0;
    DepthStencilDesc.Width = Resolution.w;
    DepthStencilDesc.Height = Resolution.h;
    DepthStencilDesc.DepthOrArraySize = DepthSize;
    DepthStencilDesc.MipLevels = 1;
    DepthStencilDesc.Format = Global_DepthBufferFormat;
    DepthStencilDesc.SampleDesc.Count = 1;
    DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE DepthClear =  { Global_DepthBufferFormat };
    DepthClear.DepthStencil.Depth = 1.0f;
    DepthClear.DepthStencil.Stencil = 0;
    
    ID3D12Resource* Result = NULL;
    D3D12_HEAP_PROPERTIES DefaultHeapProperties = GetDefaultHeapProperties();
    if(FAILED(Device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, 
                                              D3D12_RESOURCE_STATE_DEPTH_WRITE, &DepthClear, __uuidof(ID3D12Resource), 
                                              (void**)&Result))) return NULL;
    
    return Result;
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

ak_u32 FindSupport(mesh_p3_n3* Mesh, ak_v3f Direction)
{
    ak_u32 BestVertexIndex = (ak_u32)-1;        
    ak_f32 BestDot = -AK_MAX32;
    for(ak_u32 VertexIndex = 0; VertexIndex < Mesh->VertexCount; VertexIndex++)
    {
        ak_f32 TempDot = AK_Dot(Direction, Mesh->Vertices[VertexIndex].P);
        if(TempDot > BestDot)
        {
            BestVertexIndex = VertexIndex;
            BestDot = TempDot;
        }
    }
    
    return BestVertexIndex;
}

box CreateBox(mesh_p3_n3* Mesh)
{
    box Result = {};
    ak_u32 Indices[] =
    {
        FindSupport(Mesh, -AK_XAxis()), 
        FindSupport(Mesh,  AK_XAxis()),
        FindSupport(Mesh, -AK_YAxis()),
        FindSupport(Mesh,  AK_YAxis()),
        FindSupport(Mesh, -AK_ZAxis()),
        FindSupport(Mesh,  AK_ZAxis())
    };    
    
    Result.Min = Mesh->Vertices[Indices[0]].P;
    Result.Max = Mesh->Vertices[Indices[0]].P;
    for(ak_u32 Index = 1; Index < 6; Index++)
    {
        ak_v3f P = Mesh->Vertices[Indices[Index]].P;
        
        Result.Min.x = AK_Min(Result.Min.x, P.x);
        Result.Min.y = AK_Min(Result.Min.y, P.y);
        Result.Min.z = AK_Min(Result.Min.z, P.z);
        
        Result.Max.x = AK_Max(Result.Max.x, P.x);
        Result.Max.y = AK_Max(Result.Max.y, P.y);
        Result.Max.z = AK_Max(Result.Max.z, P.z);                
    }
    
    return Result;
}

box UpdateBox(box Box, ak_m4f Transform)
{
    box Result;
    for(ak_u32 I = 0; I < 3; I++)
    {
        Result.Min[I] = Result.Max[I] = Transform.Translation[I];        
        for(ak_u32 J = 0; J < 3; J++)
        {
            ak_u32 Index = (I*4)+J;
            ak_f32 e = Transform[Index]*Box.Min[J];
            ak_f32 f = Transform[Index]*Box.Max[J];
            if(e < f)
            {
                Result.Min[I] += e;
                Result.Max[I] += f;
            }
            else
            {
                Result.Min[I] += f;
                Result.Max[I] += e;
            }
        }
    }    
    return Result;
}

ak_bool FastRayBoxIntersection(ak_v3f Origin, ak_v3f InvDir, ak_bool* Sign, box Box)
{
    float tMin, tMax, tMinY, tMaxY, tMinZ, tMaxZ; 
    
    tMin = (Box.Bounds[Sign[0]].x - Origin.x) * InvDir.x; 
    tMax = (Box.Bounds[1-Sign[0]].x - Origin.x) * InvDir.x; 
    tMinY = (Box.Bounds[Sign[1]].y - Origin.y) * InvDir.y; 
    tMaxY = (Box.Bounds[1-Sign[1]].y - Origin.y) * InvDir.y; 
    
    if ((tMin > tMaxY) || (tMinY > tMax)) 
        return false; 
    if (tMinY > tMin) 
        tMin = tMinY; 
    if (tMaxY < tMax) 
        tMax = tMaxY; 
    
    tMinZ = (Box.Bounds[Sign[2]].z - Origin.z) * InvDir.z; 
    tMaxZ = (Box.Bounds[1-Sign[2]].z - Origin.z) * InvDir.z; 
    
    if ((tMin > tMaxZ) || (tMinZ > tMax)) 
        return false; 
    if (tMinZ > tMin) 
        tMin = tMinZ; 
    if (tMaxZ < tMax) 
        tMax = tMaxZ; 
    
    return true; 
}

object CreateObject(ak_v3f Position, ak_quatf Orientation, ak_v3f Scale, mesh_p3_n3* Mesh, ak_color4f Color)
{
    object Result;
    Result.Transform = AK_TransformM4(Position, Orientation, Scale);
    Result.Color = Color;
    Result.Mesh = Mesh;
    
    ak_u32 Indices[] =
    {
        FindSupport(Mesh, -AK_XAxis()), 
        FindSupport(Mesh,  AK_XAxis()),
        FindSupport(Mesh, -AK_YAxis()),
        FindSupport(Mesh,  AK_YAxis()),
        FindSupport(Mesh, -AK_ZAxis()),
        FindSupport(Mesh,  AK_ZAxis())
    };    
    
    Result.OriginalBox = CreateBox(Mesh);
    Result.CurrentBox = UpdateBox(Result.OriginalBox, Result.Transform);
    
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

void Win32_ProcessMessages(input* Input)
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
                        BindKey(VK_MENU, Input->Alt);                          
                    }
                }
                
                if(((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) && (VKCode == VK_F4) && IsDown)
                    PostQuitMessage(0);                                
            } break;
            
            case WM_MOUSEWHEEL:
            {
                ak_f32 Scroll = (ak_f32)GET_WHEEL_DELTA_WPARAM(Message.wParam) / (ak_f32)WHEEL_DELTA;                
                Input->Scroll = Scroll;
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }                
}

ak_bool RayTriangleIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* t, ak_f32* u, ak_f32* v)
{    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(RayDirection, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))
        return false;
    
    ak_v3f TVec = RayOrigin - P0;
    
    *u = AK_Dot(TVec, PVec);
    if(*u < 0.0f || *u > Det)
        return false;
    
    ak_v3f QVec = AK_Cross(TVec, Edge1);
    
    *v = AK_Dot(RayDirection, QVec);
    if(*v < 0.0f || *u + *v > Det)
        return false;
    
    *t = AK_Dot(Edge2, QVec);
    
    ak_f32 InvDet = 1.0f / Det;
    *t *= InvDet;
    *u *= InvDet;
    *v *= InvDet;
    
    return true;
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

void CreateDepthStencilView(ID3D12Device* Device, ID3D12Resource* Resource, descriptor_heap* Descriptors, ak_u32 DescriptorIndex)
{
    Device->CreateDepthStencilView(Resource, NULL, Descriptors->CPUIndex(DescriptorIndex));
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

D3D12_RESOURCE_BARRIER GetUAVBarrier(ID3D12Resource* Resource)
{
    D3D12_RESOURCE_BARRIER Result;
    Result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Result.UAV.pResource = Resource;
    return Result;
}

ak_v3f GenerateSphericalFibonacciDir(ak_f32 Index, ak_f32 NumSamples)
{
    ak_f32 b = (AK_Sqrt(5.0f)*0.5f + 0.5f) - 1.0f;
    ak_f32 phi = 2*AK_PI*AK_Frac(Index*b);
    ak_f32 cos = 1.0f - (2.0f*Index + 1.0f) * (1.0f/NumSamples);
    ak_f32 sin = AK_Sqrt(AK_Saturate(1.0f - (cos*cos)));    
    ak_v3f Result = AK_V3((AK_Cos(phi) * sin), (AK_Sin(phi)*sin), cos);
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

inline D3D12_DEPTH_STENCIL_DESC GetDepthOnState()
{
    D3D12_DEPTH_STENCIL_DESC DepthOnState = {};
    DepthOnState.DepthEnable = TRUE;
    DepthOnState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    DepthOnState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    return DepthOnState;
}

AK_ASYNC_TASK_CALLBACK(RayTraceProbeCallback)
{
    ray_trace_probe* RayTraceProbe = (ray_trace_probe*)UserData;    
    ak_i32 RaysPerProbe = RayTraceProbe->RaysPerProbe;
    
    ak_array<object> Objects = RayTraceProbe->Objects;
    ak_v3f RayOrigin = RayTraceProbe->RayOrigin;
    
    for(ak_i32 RayIndex = 0; RayIndex < RaysPerProbe; RayIndex++)
    {        
        ak_v3f RayDirection = RayTraceProbe->RayDirections[RayIndex].xyz;
        ak_v3f InvRayDirection = 1.0f/RayDirection;
        ak_bool Sign[3] = {InvRayDirection.x < 0, InvRayDirection.y < 0, InvRayDirection.z < 0};
        
        ak_f32 tMin = INFINITY, uMin = INFINITY, vMin = INFINITY;
        ak_u32 BestTriangleIndex = 0;
        object* HitObject = NULL;
        AK_ForEach(Object, &Objects)
        {                    
            if(FastRayBoxIntersection(RayOrigin, InvRayDirection, Sign, Object->CurrentBox))
            {
                mesh_p3_n3* Mesh = Object->Mesh;
                ak_u32 TriangleCount = Mesh->IndexCount/3;
                for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
                {
                    ak_v3f Triangle[3] = 
                    {
                        Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+0]].P, 
                        Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+1]].P,
                        Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+2]].P
                    };
                    
                    Triangle[0] = AK_TransformPoint(Triangle[0], Object->Transform);
                    Triangle[1] = AK_TransformPoint(Triangle[1], Object->Transform);
                    Triangle[2] = AK_TransformPoint(Triangle[2], Object->Transform);
                    
                    ak_f32 t, u, v;
                    if(RayTriangleIntersection(RayOrigin, RayDirection, Triangle[0], Triangle[1], Triangle[2], &t, &u, &v))
                    {
                        if((t < tMin) && (t > 0.001f))
                        {
                            HitObject = Object;
                            tMin = t;
                            uMin = u;
                            vMin = v;
                            BestTriangleIndex = TriangleIndex;
                        }
                    }
                }
            }
        }
        
        
        if(HitObject)
        {            
            ak_m3f NormalTransform = AK_Transpose(AK_InvTransformM3(AK_M3(HitObject->Transform)));
            
            mesh_p3_n3* Mesh = HitObject->Mesh;                    
            ak_v3f VertexN[3] = 
            { 
                Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+0]].N*NormalTransform, 
                Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+1]].N*NormalTransform, 
                Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+2]].N*NormalTransform
            };
            
            RayTraceProbe->HitAlbedo[RayIndex] = AK_RGBA_U32(HitObject->Color);
            RayTraceProbe->HitPositions[RayIndex] = AK_V4(RayOrigin + RayDirection*tMin, 1.0f);
            RayTraceProbe->HitNormals[RayIndex] = AK_V4(VertexN[0]*uMin + VertexN[1]*vMin + VertexN[2]*(1-uMin-vMin), 0.0f);
            RayTraceProbe->RayDepths[RayIndex] = tMin;
        }                        
    }    
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
    
    LARGE_INTEGER Seed;
    QueryPerformanceCounter(&Seed);
    AK_SetRandomSeed64(Seed.QuadPart);
    AK_SetRandomSeed32(Seed.LowPart);
    
    ak_async_task_queue* TaskQueue = AK_CreateAsyncTaskQueue(11);
    
    irradiance_field IrradianceField = {};
    IrradianceField.BottomLeft = AK_V3(-5.9f, -5.9f, -2.5f);
    IrradianceField.Spacing = AK_V3(1.7f, 1.7f, 1.7f);
    IrradianceField.Count = AK_V3(8.0f, 8.0f, 8.0f);        
    //IrradianceField.ProbeEncodingGamma = 5.0f;
    
    ak_u32 TotalProbeCount = (ak_u32)IrradianceField.Count.x*(ak_u32)IrradianceField.Count.y*(ak_u32)IrradianceField.Count.z;
    
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
    
    ID3D12Resource* DepthStencilBuffer = CreateDepthBuffer(Device, Resolution);
    if(!DepthStencilBuffer) { AK_MessageBoxOk("Fatal Error", "Failed to create the depth stencil buffer"); return -1; }
    
    ak_v2i ShadowMapResolution = AK_V2(2048, 2048);
    ak_u16 ShadowMapCount = 1;
    ID3D12Resource* ShadowMapTextures = CreateDepthBuffer(Device, ShadowMapResolution, ShadowMapCount*6);
    if(!ShadowMapTextures) { AK_MessageBoxOk("Fatal Error", "Failed to create the shadow map textures"); return -1; }
    
    ID3D12Resource* HitAlbedoTexture = CreateTexture2D(Device, TotalProbeCount, RAYS_PER_PROBE, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_COMMON);
    if(!HitAlbedoTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the albedo texture"); return -1; }
    
    ID3D12Resource* HitPositionTexture = CreateTexture2D(Device, TotalProbeCount, RAYS_PER_PROBE, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_STATE_COMMON);
    if(!HitPositionTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the position texture"); return -1; }
    
    ID3D12Resource* HitNormalTexture = CreateTexture2D(Device, TotalProbeCount, RAYS_PER_PROBE, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_STATE_COMMON);
    if(!HitNormalTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the normal texture"); return -1; }
    
    ID3D12Resource* HitDepthTexture = CreateTexture2D(Device, TotalProbeCount, RAYS_PER_PROBE, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_COMMON);
    if(!HitDepthTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the depth texture"); return -1; }
    
    ID3D12Resource* ProbeRadianceTexture = CreateTexture2D(Device, TotalProbeCount, RAYS_PER_PROBE, DXGI_FORMAT_R32G32B32A32_FLOAT, 
                                                           D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(!ProbeRadianceTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the probe radiance texture"); return -1; }
    
    ak_i32 IrradianceTexelCountPlusPadding = IRRADIANCE_TEXEL_COUNT+2;
    ak_i32 DistanceTexelCountPlusPadding = DISTANCE_TEXEL_COUNT+2;    
    ak_i32 IrradianceFieldPitch = (ak_i32)IrradianceField.Count.x*(ak_i32)IrradianceField.Count.z;
    
    ak_v2i IrradianceDim = AK_V2(IrradianceTexelCountPlusPadding*IrradianceFieldPitch, IrradianceTexelCountPlusPadding*(ak_i32)IrradianceField.Count.y);
    ak_v2i DistanceDim = AK_V2(DistanceTexelCountPlusPadding*IrradianceFieldPitch, DistanceTexelCountPlusPadding*(ak_i32)IrradianceField.Count.y);
    
    ID3D12Resource* IrradianceTexture = CreateTexture2D(Device, IrradianceDim.w, IrradianceDim.h, 
                                                        DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(!IrradianceTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the irradiance texture"); return -1; }
    
    ID3D12Resource* DistanceTexture = CreateTexture2D(Device, DistanceDim.x, DistanceDim.h, 
                                                      DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(!DistanceTexture) { AK_MessageBoxOk("Fatal Error", "Failed to create the distance texture"); return -1; } 
    
    descriptor_heap RTVDescriptors = CreateDescriptorHeap(Device, SwapChainBuffers.Size, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if(!RTVDescriptors.Heap) { AK_MessageBoxOk("Fatal Error", "Failed to create the render target view heap"); return -1; }
    
    descriptor_heap DSVDescriptors = CreateDescriptorHeap(Device, 1+ShadowMapCount*6, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if(!DSVDescriptors.Heap) { AK_MessageBoxOk("Fatal Error", "Failed to create the depth stencil view heap"); return -1; }
    
    descriptor_heap SRVDescriptors = CreateDescriptorHeap(Device, 11, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if(!SRVDescriptors.Heap) { AK_MessageBoxOk("Fatal Error", "Failed to create the SRV view heap"); return -1; }
    
    for(ak_u32 BufferIndex = 0; BufferIndex < SwapChainBuffers.Size; BufferIndex++)
        CreateRenderTargetView(Device, SwapChainBuffers[BufferIndex], &RTVDescriptors, BufferIndex, Global_ColorBufferFormat);
    
    CreateDepthStencilView(Device, DepthStencilBuffer, &DSVDescriptors, 0);
    
    for(ak_u32 ShadowMapIndex = 0; ShadowMapIndex < ShadowMapCount; ShadowMapIndex++)
    {        
        for(ak_u32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
            DepthStencilViewDesc.Format = Global_DepthBufferFormat;
            DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            DepthStencilViewDesc.Texture2DArray.FirstArraySlice = (ShadowMapIndex*6)+FaceIndex;
            DepthStencilViewDesc.Texture2DArray.ArraySize = 1;            
            Device->CreateDepthStencilView(ShadowMapTextures, &DepthStencilViewDesc, DSVDescriptors.CPUIndex(1+(ShadowMapIndex*6)+FaceIndex));
        }
    }
    
    D3D12_SHADER_RESOURCE_VIEW_DESC ShadowMapViewDesc = {};
    ShadowMapViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
    ShadowMapViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    ShadowMapViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ShadowMapViewDesc.Texture2DArray.MipLevels = (UINT)-1;
    ShadowMapViewDesc.Texture2DArray.ArraySize = 6;
    
    Device->CreateShaderResourceView(ShadowMapTextures, &ShadowMapViewDesc, SRVDescriptors.CPUIndex(0));
    Device->CreateShaderResourceView(HitAlbedoTexture, NULL, SRVDescriptors.CPUIndex(1));
    Device->CreateShaderResourceView(HitPositionTexture, NULL, SRVDescriptors.CPUIndex(2));
    Device->CreateShaderResourceView(HitNormalTexture, NULL, SRVDescriptors.CPUIndex(3));
    Device->CreateShaderResourceView(HitDepthTexture, NULL, SRVDescriptors.CPUIndex(4));        
    Device->CreateShaderResourceView(IrradianceTexture, NULL, SRVDescriptors.CPUIndex(5));
    Device->CreateShaderResourceView(DistanceTexture, NULL, SRVDescriptors.CPUIndex(6));
    Device->CreateUnorderedAccessView(ProbeRadianceTexture, NULL, NULL, SRVDescriptors.CPUIndex(7));
    
    Device->CreateShaderResourceView(ProbeRadianceTexture, NULL, SRVDescriptors.CPUIndex(8));
    Device->CreateUnorderedAccessView(IrradianceTexture, NULL, NULL, SRVDescriptors.CPUIndex(9));
    Device->CreateUnorderedAccessView(DistanceTexture, NULL, NULL, SRVDescriptors.CPUIndex(10));
    
    upload_buffer UploadBuffer = CreateUploadBuffer(Device, AK_Megabyte(8));
    if(!UploadBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the upload buffer"); return -1; }
    
    push_buffer VertexBuffer = CreatePushBuffer(Device, AK_Megabyte(32), D3D12_RESOURCE_STATE_COPY_DEST);
    if(!VertexBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the vertex buffer"); return -1; }
    
    push_buffer IndexBuffer = CreatePushBuffer(Device, AK_Megabyte(32), D3D12_RESOURCE_STATE_COPY_DEST);
    if(!IndexBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the index buffer"); return -1; }
    
    ak_u32 ViewProjSize = sizeof(ak_m4f)*2;
    
    ak_u32 MatrixAlignedSize = (ak_u32)AK_AlignTo(sizeof(ak_m4f), 256);    
    normal_buffer ViewProjBuffer = CreateNormalBuffer(Device, MatrixAlignedSize*(1+ShadowMapCount*6), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    if(!ViewProjBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the view projection buffer"); return -1; }
    
    normal_buffer LightBuffer = CreateNormalBuffer(Device, sizeof(light)*MAX_LIGHT_COUNT + sizeof(ak_u32), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    if(!LightBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the light buffer"); return -1; }
    
    normal_buffer IrradianceFieldBuffer = CreateNormalBuffer(Device, (ak_u32)AK_AlignTo(sizeof(irradiance_field), 256), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    if(!IrradianceFieldBuffer.Resource) { AK_MessageBoxOk("Fatal Error", "Failed to create the irradiance field buffer"); return -1; }
    
    DirectCommandAllocator->Reset();
    DirectCommandList->Reset(DirectCommandAllocator, NULL);
    
    mesh_p3_n3 BoxMesh = ToMesh(AK_GenerateTriangleBoxN(AppArena, AK_V3(1.0f, 1.0f, 1.0f)), DirectCommandList, &UploadBuffer, &VertexBuffer, &IndexBuffer);            
    
    ak_u32 DebugPrimitiveOffset = VertexBuffer.Used;
    mesh_p3 DebugSphereMesh = ToDebugMesh(AK_GenerateTriangleSphere(AppArena, 1.0f, 5), DirectCommandList, &UploadBuffer, &VertexBuffer, &IndexBuffer, DebugPrimitiveOffset);    
    mesh_p3 DebugBoxMesh = ToDebugMesh(AK_GenerateTriangleBox(AppArena, AK_V3(1.0f, 1.0f, 1.0f)), DirectCommandList, &UploadBuffer, &VertexBuffer, &IndexBuffer, DebugPrimitiveOffset);
    
    D3D12_RESOURCE_BARRIER Barriers[] = 
    {
        GetTransitionBarrier(VertexBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        GetTransitionBarrier(IndexBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)        
    };
    
    DirectCommandList->ResourceBarrier(AK_Count(Barriers), Barriers);    
    
    DirectCommandList->Close();
    DirectCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&DirectCommandList);
    Fence.Signal(DirectCommandQueue);
    
    ak_temp_arena TempArena = AppArena->BeginTemp();
    
    D3D12_STATIC_SAMPLER_DESC LightShadingSamplerDesc[2] = {};
    LightShadingSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    LightShadingSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[0].MaxAnisotropy = 1;
    LightShadingSamplerDesc[0].ShaderRegister = 0;
    LightShadingSamplerDesc[0].RegisterSpace = 1;
    LightShadingSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    LightShadingSamplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    LightShadingSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    LightShadingSamplerDesc[1].MaxAnisotropy = 1;
    LightShadingSamplerDesc[1].ShaderRegister = 1;
    LightShadingSamplerDesc[1].RegisterSpace = 1;
    LightShadingSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;            
    
    root_range ObjectRootRange = BeginRootRange(AppArena, 2);
    ObjectRootRange.AddRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, 0);
    ObjectRootRange.AddRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 1, 5);
    
    root_parameters ObjectRootParameters = BeginRootParameters(AppArena, 6);
    ObjectRootParameters.AddRootConstant(0, 0, 0, 16, D3D12_SHADER_VISIBILITY_VERTEX);
    ObjectRootParameters.AddRootConstant(1, 0, 1, 8, D3D12_SHADER_VISIBILITY_PIXEL);    
    ObjectRootParameters.AddRootDescriptor(2, D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);    
    ObjectRootParameters.AddRootDescriptor(3, D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    ObjectRootParameters.AddRootDescriptor(4, D3D12_ROOT_PARAMETER_TYPE_CBV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    ObjectRootParameters.AddRootTable(5, &ObjectRootRange, D3D12_SHADER_VISIBILITY_PIXEL);
    
    ID3D12RootSignature* ObjectRootSignature = CreateRootSignature(Device, ObjectRootParameters, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, AK_Count(LightShadingSamplerDesc), LightShadingSamplerDesc);    
    if(!ObjectRootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the root signature"); return -1; }
    
    ak_buffer ObjectVSResults = AK_ReadEntireFile("ObjectVertexShader.cso", AppArena);
    ak_buffer ObjectPSResults = AK_ReadEntireFile("ObjectPixelShader.cso", AppArena);    
    if(!ObjectVSResults.IsValid() || !ObjectPSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the hlsl shaders"); return -1; }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ObjectPipelineStateDesc = {};
    
    D3D12_INPUT_ELEMENT_DESC ObjectInputElements[] = 
    {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    ObjectPipelineStateDesc.pRootSignature = ObjectRootSignature;
    ObjectPipelineStateDesc.InputLayout = {ObjectInputElements, AK_Count(ObjectInputElements)};
    ObjectPipelineStateDesc.VS = {ObjectVSResults.Data, ObjectVSResults.Size};
    ObjectPipelineStateDesc.PS = {ObjectPSResults.Data, ObjectPSResults.Size};
    ObjectPipelineStateDesc.BlendState = GetDefaultBlendState();
    ObjectPipelineStateDesc.SampleMask = 0xFFFFFFFF;
    ObjectPipelineStateDesc.DepthStencilState = GetDepthOnState();
    ObjectPipelineStateDesc.RasterizerState = GetDefaultRasterizerState();
    ObjectPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ObjectPipelineStateDesc.NumRenderTargets = 1;
    ObjectPipelineStateDesc.RTVFormats[0] = Global_ColorBufferFormat;
    ObjectPipelineStateDesc.DSVFormat = Global_DepthBufferFormat;
    ObjectPipelineStateDesc.SampleDesc.Count = 1;
    
    ID3D12PipelineState* ObjectPipelineState = NULL;
    if(FAILED(Device->CreateGraphicsPipelineState(&ObjectPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&ObjectPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the pipeline state"); return -1; }
    
    root_parameters ShadowMapRootParameters = BeginRootParameters(AppArena, 3);
    ShadowMapRootParameters.AddRootConstant(0, 0, 0, 16, D3D12_SHADER_VISIBILITY_VERTEX);
    ShadowMapRootParameters.AddRootConstant(1, 0, 1, 4, D3D12_SHADER_VISIBILITY_PIXEL);
    ShadowMapRootParameters.AddRootDescriptor(2, D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);    
    
    ID3D12RootSignature* ShadowMapRootSignature = CreateRootSignature(Device, ShadowMapRootParameters, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    if(!ShadowMapRootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the shadow map root signature"); return -1; }
    
    ak_buffer ShadowMapVSResults = AK_ReadEntireFile("ShadowMapVertexShader.cso", AppArena);
    ak_buffer ShadowMapPSResults = AK_ReadEntireFile("ShadowMapPixelShader.cso", AppArena);
    if(!ShadowMapVSResults.IsValid() || !ShadowMapPSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the shadow map hlsl shaders"); return -1; }
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ShadowMapPipelineStateDesc = {};
    
    D3D12_INPUT_ELEMENT_DESC ShadowMapInputElements[] = 
    {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}        
    };
    
    ShadowMapPipelineStateDesc.pRootSignature = ShadowMapRootSignature;
    ShadowMapPipelineStateDesc.InputLayout = {ShadowMapInputElements, AK_Count(ShadowMapInputElements)};
    ShadowMapPipelineStateDesc.VS = {ShadowMapVSResults.Data, ShadowMapVSResults.Size};
    ShadowMapPipelineStateDesc.PS = {ShadowMapPSResults.Data, ShadowMapPSResults.Size};
    ShadowMapPipelineStateDesc.BlendState = GetDefaultBlendState();
    ShadowMapPipelineStateDesc.SampleMask = 0xFFFFFFFF;
    ShadowMapPipelineStateDesc.DepthStencilState = GetDepthOnState();
    ShadowMapPipelineStateDesc.RasterizerState = GetDefaultRasterizerState();    
    ShadowMapPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ShadowMapPipelineStateDesc.DSVFormat = Global_DepthBufferFormat;
    ShadowMapPipelineStateDesc.SampleDesc.Count = 1;
    
    ID3D12PipelineState* ShadowMapPipelineState = NULL;
    if(FAILED(Device->CreateGraphicsPipelineState(&ShadowMapPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&ShadowMapPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the shadow map pipeline state"); return -1; }
    
    ak_buffer DebugPrimitiveVSResults = AK_ReadEntireFile("DebugPrimitiveVertexShader.cso", AppArena);
    ak_buffer DebugPrimitivePSResults = AK_ReadEntireFile("DebugPrimitivePixelShader.cso", AppArena);
    if(!DebugPrimitiveVSResults.IsValid() || !DebugPrimitivePSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the debug primitive hlsl shaders"); return -1; }
    
    D3D12_INPUT_ELEMENT_DESC DebugPrimitiveInputElements[] = 
    {
        {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC DebugPrimitivePipelineStateDesc = {};
    DebugPrimitivePipelineStateDesc.pRootSignature = ObjectRootSignature;
    DebugPrimitivePipelineStateDesc.InputLayout = {DebugPrimitiveInputElements, AK_Count(DebugPrimitiveInputElements)};
    DebugPrimitivePipelineStateDesc.VS = {DebugPrimitiveVSResults.Data, DebugPrimitiveVSResults.Size};
    DebugPrimitivePipelineStateDesc.PS = {DebugPrimitivePSResults.Data, DebugPrimitivePSResults.Size};
    DebugPrimitivePipelineStateDesc.BlendState = GetDefaultBlendState();
    DebugPrimitivePipelineStateDesc.SampleMask = 0xFFFFFFFF;
    DebugPrimitivePipelineStateDesc.DepthStencilState = GetDepthOnState();
    DebugPrimitivePipelineStateDesc.RasterizerState = GetDefaultRasterizerState();
    DebugPrimitivePipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    DebugPrimitivePipelineStateDesc.DSVFormat = Global_DepthBufferFormat;
    DebugPrimitivePipelineStateDesc.NumRenderTargets = 1;
    DebugPrimitivePipelineStateDesc.RTVFormats[0] = Global_ColorBufferFormat;
    DebugPrimitivePipelineStateDesc.SampleDesc.Count = 1;
    
    ID3D12PipelineState* DebugPrimitivePipelineState = NULL;
    if(FAILED(Device->CreateGraphicsPipelineState(&DebugPrimitivePipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&DebugPrimitivePipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the debug primitive pipeline state"); return -1; }
    
    LightShadingSamplerDesc[0].RegisterSpace = 0;
    LightShadingSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    
    LightShadingSamplerDesc[1].RegisterSpace = 0;
    LightShadingSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    
    root_range ShadeSurfelsRootRange = BeginRootRange(AppArena, 2);    
    ShadeSurfelsRootRange.AddRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0, 0, 0);        
    ShadeSurfelsRootRange.AddRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);    
    
    root_parameters ShadeSurfelsRootParameters = BeginRootParameters(AppArena, 3);
    ShadeSurfelsRootParameters.AddRootDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0);
    ShadeSurfelsRootParameters.AddRootDescriptor(1, D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0);
    ShadeSurfelsRootParameters.AddRootTable(2, &ShadeSurfelsRootRange);
    
    ID3D12RootSignature* ShadeSurfelsRootSignature = CreateRootSignature(Device, ShadeSurfelsRootParameters, D3D12_ROOT_SIGNATURE_FLAG_NONE, 
                                                                         AK_Count(LightShadingSamplerDesc), LightShadingSamplerDesc);
    if(!ShadeSurfelsRootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the shade surfels root signature"); return -1; }    
    
    ak_buffer ShadeSurfelsCSResults = AK_ReadEntireFile("ShadeSurfelsComputeShader.cso", AppArena);
    if(!ShadeSurfelsCSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the shade surfels hlsl shader"); return -1; }
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC ShadeSurfelsPipelineStateDesc = {};
    ShadeSurfelsPipelineStateDesc.pRootSignature = ShadeSurfelsRootSignature;
    ShadeSurfelsPipelineStateDesc.CS = {ShadeSurfelsCSResults.Data, ShadeSurfelsCSResults.Size};
    
    ID3D12PipelineState* ShadeSurfelsPipelineState = NULL;
    if(FAILED(Device->CreateComputePipelineState(&ShadeSurfelsPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&ShadeSurfelsPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the shade surfels pipeline state"); return -1; }        
    
    root_range ProbeBlendingRootRange = BeginRootRange(AppArena, 2);    
    ProbeBlendingRootRange.AddRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);
    ProbeBlendingRootRange.AddRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
    
    root_parameters ProbeBlendingRootParameters = BeginRootParameters(AppArena, 2);
    ProbeBlendingRootParameters.AddRootDescriptor(0, D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0);
    ProbeBlendingRootParameters.AddRootTable(1, &ProbeBlendingRootRange);
    
    ID3D12RootSignature* ProbeBlendingRootSignature = CreateRootSignature(Device, ProbeBlendingRootParameters);
    if(!ProbeBlendingRootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the probe blending root signature"); return -1; }
    
    ak_buffer IrradianceProbeBlendingCSResults = AK_ReadEntireFile("IrradianceProbeBlendingComputeShader.cso", AppArena);
    ak_buffer DistanceProbeBlendingCSResults = AK_ReadEntireFile("DistanceProbeBlendingComputeShader.cso", AppArena);
    
    if(!IrradianceProbeBlendingCSResults.IsValid() || !DistanceProbeBlendingCSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the probe blending hlsl shaders"); return -1; }
    
    ID3D12PipelineState* IrradianceProbeBlendingPipelineState = NULL;
    ID3D12PipelineState* DistanceProbeBlendingPipelineState = NULL;
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC ProbeBlendingPipelineStateDesc = {};
    ProbeBlendingPipelineStateDesc.pRootSignature = ProbeBlendingRootSignature;
    ProbeBlendingPipelineStateDesc.CS = {IrradianceProbeBlendingCSResults.Data, IrradianceProbeBlendingCSResults.Size};
    if(FAILED(Device->CreateComputePipelineState(&ProbeBlendingPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&IrradianceProbeBlendingPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the irradiance probe blending pipeline state"); return -1; }
    
    ProbeBlendingPipelineStateDesc.CS = {DistanceProbeBlendingCSResults.Data, DistanceProbeBlendingCSResults.Size};
    if(FAILED(Device->CreateComputePipelineState(&ProbeBlendingPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&DistanceProbeBlendingPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the distance probe blending pipeline state"); return -1; }
    
    root_range ProbeBorderUpdateRootRange = BeginRootRange(AppArena, 1);
    ProbeBorderUpdateRootRange.AddRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, 0);
    
    root_parameters ProbeBorderUpdateRootParameters = BeginRootParameters(AppArena, 3);
    ProbeBorderUpdateRootParameters.AddRootConstant(0, 0, 0, 2);
    ProbeBorderUpdateRootParameters.AddRootDescriptor(1, D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0);
    ProbeBorderUpdateRootParameters.AddRootTable(2, &ProbeBorderUpdateRootRange);
    
    ID3D12RootSignature* ProbeBorderUpdateRootSignature = CreateRootSignature(Device, ProbeBorderUpdateRootParameters);
    if(!ProbeBorderUpdateRootSignature) { AK_MessageBoxOk("Fatal Error", "Failed to create the probe border update root signature"); }        
    
    ak_buffer ProbeBorderUpdateRowCSResults = AK_ReadEntireFile("ProbeBorderUpdateRowComputeShader.cso", AppArena);
    ak_buffer ProbeBorderUpdateColumnCSResults = AK_ReadEntireFile("ProbeBorderUpdateColumnComputeShader.cso", AppArena);
    
    if(!ProbeBorderUpdateRowCSResults.IsValid() || !ProbeBorderUpdateColumnCSResults.IsValid()) { AK_MessageBoxOk("Fatal Error", "Failed to load the probe border update hlsl shaders"); return -1; }        
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC ProbeBorderUpdateStateDesc = {};    
    ProbeBorderUpdateStateDesc.pRootSignature = ProbeBorderUpdateRootSignature;
    ProbeBorderUpdateStateDesc.CS = {ProbeBorderUpdateRowCSResults.Data, ProbeBorderUpdateRowCSResults.Size};
    
    ID3D12PipelineState* ProbeBorderUpdateRowPipelineState = NULL;
    ID3D12PipelineState* ProbeBorderUpdateColumnPipelineState = NULL;
    if(FAILED(Device->CreateComputePipelineState(&ProbeBorderUpdateStateDesc, __uuidof(ID3D12PipelineState), (void**)&ProbeBorderUpdateRowPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the probe border update row pipleine state"); return -1; }
    
    ProbeBorderUpdateStateDesc.CS = {ProbeBorderUpdateColumnCSResults.Data, ProbeBorderUpdateColumnCSResults.Size};
    if(FAILED(Device->CreateComputePipelineState(&ProbeBorderUpdateStateDesc, __uuidof(ID3D12PipelineState), (void**)&ProbeBorderUpdateColumnPipelineState))) { AK_MessageBoxOk("Fatal Error", "Failed to create the probe border update column pipeline state"); return -1; }
    
    input Input = {};
    
    camera Camera = {};
    Camera.Target = AK_V3(0.0f, 0.0f, -1.0f);
    Camera.SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    ak_array<object> Objects = {};            
    Objects.Add(CreateObject(AK_V3( -2.0f,  0.0f, -1.0f), AK_IdentityQuat<ak_f32>(), AK_V3(0.5f, 0.5f, 1.0f)*2.0f, &BoxMesh, AK_White4()));
    Objects.Add(CreateObject(AK_V3(  2.0f,  0.0f, -1.0f), AK_IdentityQuat<ak_f32>(), AK_V3(0.25f, 0.25f, 3.0f)*2.0f, &BoxMesh, AK_White4()));
    Objects.Add(CreateObject(AK_V3(  0.0f,  2.0f, -1.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 1.0f, 1.0f)*2.0f, &BoxMesh, AK_White4()));
    Objects.Add(CreateObject(AK_V3(  0.0f, -2.0f, -1.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 1.0f, 1.0f)*2.0f, &BoxMesh, AK_White4()));
    Objects.Add(CreateObject(AK_V3(  0.0f,  0.0f, -2.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 10.0f, 1.0f), &BoxMesh, AK_Red4()*1.0f));
    Objects.Add(CreateObject(AK_V3( -5.0f,  0.0f, -2.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 10.0f, 10.0f), &BoxMesh, AK_Red4()*1.0f));
    Objects.Add(CreateObject(AK_V3(  5.0f,  0.0f, -2.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 10.0f, 10.0f), &BoxMesh, AK_Red4()*1.0f));
    Objects.Add(CreateObject(AK_V3(  0.0f,  5.0f, -2.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 1.0f, 10.0f), &BoxMesh, AK_Red4()*1.0f));
    Objects.Add(CreateObject(AK_V3(  0.0f,  0.0f, 7.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 10.0f, 1.0f), &BoxMesh, AK_Red4()*1.0f));
    Objects.Add(CreateObject(AK_V3(  0.0f, -5.0f, -2.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 1.0f, 10.0f), &BoxMesh, AK_Red4()*1.0f));
    
    ak_array<light> Lights = {};    
    Lights.Add(CreateLight(AK_V3(0.0f, 0.0f, 5.0f), 20.0f, AK_White3(), 10.0f));    
    
    AppArena->EndTemp(&TempArena);                        
    
    Global_Running = true;
    
    ak_u32 FrameIndex = 0;
    while(Global_Running)
    {        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        TempArena = GlobalArena->BeginTemp();
        
#if 0 
        local ak_bool Left = false;
        
        Objects[0].Transform.Translation.x += Left ? -0.01f : 0.01f;        
        if(Left && Objects[0].Transform.Translation.x < -2.0f)
            Left = !Left;
        
        if(!Left && Objects[0].Transform.Translation.x > 2.0f)
            Left = !Left;
#endif
        
        Win32_ProcessMessages(&Input);    
        POINT MousePosition;
        if(HWND ActiveWindow = GetForegroundWindow())
        {
            if(ActiveWindow == (HWND)PlatformWindow || IsChild(ActiveWindow, (HWND)PlatformWindow))
            {
                if(GetCursorPos(&MousePosition) && ScreenToClient((HWND)PlatformWindow, &MousePosition))
                {                                    
                    Input.MouseCoordinates = AK_V2((ak_i32)MousePosition.x, (ak_i32)MousePosition.y);
                }
            }
        }
        
        Dev_BindMouse(VK_LBUTTON, Input.LMB);
        Dev_BindMouse(VK_MBUTTON, Input.MMB);
        
        ak_v2i MouseDelta = Input.MouseCoordinates - Input.LastMouseCoordinates;    
        ak_v3f* SphericalCoordinates = &Camera.SphericalCoordinates;                
        
        ak_f32 Roll = 0;
        ak_f32 Pitch = 0;        
        
        ak_v2f PanDelta = AK_V2<ak_f32>();
        ak_f32 Scroll = 0;
        
        if(IsDown(Input.Alt))
        {
            if(IsDown(Input.LMB))
            {
                SphericalCoordinates->inclination += MouseDelta.y*1e-3f;
                SphericalCoordinates->azimuth += MouseDelta.x*1e-3f;
                
                ak_f32 InclindationDegree = AK_ToDegree(SphericalCoordinates->inclination);
                if(InclindationDegree < -180.0f)
                {
                    ak_f32 Diff = InclindationDegree + 180.0f;
                    InclindationDegree = 180.0f - Diff;
                    InclindationDegree = AK_Min(180.0f, InclindationDegree);
                    SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
                }
                else if(InclindationDegree > 180.0f)
                {
                    ak_f32 Diff = InclindationDegree - 180.0f;
                    InclindationDegree = -180.0f + Diff;
                    InclindationDegree = AK_Min(-180.0f, InclindationDegree);
                    SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
                }            
            }
            
            if(IsDown(Input.MMB))        
                PanDelta += AK_V2f(MouseDelta)*1e-3f;        
            
            if(AK_Abs(Input.Scroll) > 0)        
            {
                SphericalCoordinates->radius -= Input.Scroll*0.5f;                    
                if(SphericalCoordinates->radius < 0.001f)
                    SphericalCoordinates->radius = 0.001f;
            }
        }    
        
        view_settings ViewSettings = GetViewSettings(&Camera);    
        Camera.Target += (ViewSettings.Orientation.XAxis*PanDelta.x - ViewSettings.Orientation.YAxis*PanDelta.y);        
        
        Resolution = {};
        AK_GetWindowResolution(PlatformWindow, (ak_u16*)&Resolution.w, (ak_u16*)&Resolution.h);
        
        ak_high_res_clock Begin = AK_WallClock();
        
        ak_v4f* RayDirections = GlobalArena->PushArray<ak_v4f>(RAYS_PER_PROBE);        
        for(ak_i32 RayIndex = 0; RayIndex < RAYS_PER_PROBE; RayIndex++)                    
            RayDirections[RayIndex] = AK_V4(GenerateSphericalFibonacciDir((ak_f32)RayIndex, (ak_f32)RAYS_PER_PROBE), 0.0f);
        
        ak_v3f* ProbePositions = GlobalArena->PushArray<ak_v3f>(TotalProbeCount);
        ak_f32* RayDepths = GlobalArena->PushArray<ak_f32>(TotalProbeCount*RAYS_PER_PROBE);
        
        ak_v3f* ProbePositionsAt = ProbePositions;
        for(ak_i32 ProbeZ = 0; ProbeZ < IrradianceField.Count.z; ProbeZ++)        
        {            
            for(ak_i32 ProbeY = 0; ProbeY < IrradianceField.Count.y; ProbeY++)
            {
                for(ak_i32 ProbeX = 0; ProbeX < IrradianceField.Count.x; ProbeX++)
                {
                    *ProbePositionsAt++ = IrradianceField.BottomLeft + AK_V3f(ProbeX, ProbeY, ProbeZ)*IrradianceField.Spacing;                                    
                }
            }
        }     
        
        ak_u32* HitAlbedo = GlobalArena->PushArray<ak_u32>(TotalProbeCount*RAYS_PER_PROBE);
        ak_v4f* HitPositions = GlobalArena->PushArray<ak_v4f>(TotalProbeCount*RAYS_PER_PROBE);
        ak_v4f* HitNormals = GlobalArena->PushArray<ak_v4f>(TotalProbeCount*RAYS_PER_PROBE);
        
        for(ak_u32 ProbeIndex = 0; ProbeIndex < TotalProbeCount; ProbeIndex++)
        {
            ray_trace_probe* RayTraceProbe = GlobalArena->Push<ray_trace_probe>();
            RayTraceProbe->ProbeIndex = ProbeIndex;
            RayTraceProbe->RaysPerProbe = RAYS_PER_PROBE;
            RayTraceProbe->Objects = Objects;
            RayTraceProbe->RayOrigin = ProbePositions[ProbeIndex];
            RayTraceProbe->RayDirections = RayDirections;
            RayTraceProbe->HitAlbedo = HitAlbedo + (ProbeIndex*RAYS_PER_PROBE);
            RayTraceProbe->HitNormals = HitNormals + (ProbeIndex*RAYS_PER_PROBE);
            RayTraceProbe->HitPositions = HitPositions + (ProbeIndex*RAYS_PER_PROBE);  
            RayTraceProbe->RayDepths = RayDepths + (ProbeIndex*RAYS_PER_PROBE);
            TaskQueue->AddTask(RayTraceProbeCallback, RayTraceProbe);
        }
        
        TaskQueue->CompleteAllTasks(GlobalArena);        
        
        ak_high_res_clock End = AK_WallClock();
        ak_f64 Elapsed = AK_GetElapsedTime(End, Begin);        
        AK_ConsoleLog("Elapsed %f ms\n", Elapsed*1000.0);
        
        Fence.WaitCPU();
        DirectCommandAllocator->Reset();
        DirectCommandList->Reset(DirectCommandAllocator, NULL);
        
        ak_m4f ViewProj[2] = 
        {
            AK_InvTransformM4(ViewSettings.Position, ViewSettings.Orientation),
            AK_Perspective(AK_ToRadians(35.0f), AK_SafeRatio(Resolution.w, Resolution.h), 0.001f, 100.0f)
        };
        
        D3D12_RESOURCE_BARRIER PreCopyBarriers[] = 
        {            
            GetTransitionBarrier(ViewProjBuffer.Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST),
            GetTransitionBarrier(LightBuffer.Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST), 
            GetTransitionBarrier(IrradianceFieldBuffer.Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST), 
            GetTransitionBarrier(HitAlbedoTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST), 
            GetTransitionBarrier(HitPositionTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST),
            GetTransitionBarrier(HitNormalTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST), 
            GetTransitionBarrier(HitDepthTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST),             
        };         
        DirectCommandList->ResourceBarrier(AK_Count(PreCopyBarriers), PreCopyBarriers);
        
        ViewProjBuffer.Upload(DirectCommandList, &UploadBuffer, ViewProj, sizeof(ViewProj), 0);        
        LightBuffer.Upload(DirectCommandList, &UploadBuffer, &Lights[0], sizeof(light)*Lights.Size, 0);
        LightBuffer.Upload(DirectCommandList, &UploadBuffer, &Lights.Size, sizeof(ak_u32), MAX_LIGHT_COUNT*sizeof(light));
        
        IrradianceField.RotationTransform = AK_IdentityM4<ak_f32>();        
        
        float u1 = 2.0f*AK_PI * AK_RandomF32();
        float cos1 = AK_Cos(u1);
        float sin1 = AK_Sin(u1);
        
        float u2 = 2.0f*AK_PI * AK_RandomF32();
        float cos2 = AK_Cos(u2);
        float sin2 = AK_Sin(u2);
        
        float u3 = (ak_f32)AK_RandomF32();
        float sq3 = 2.f * AK_Sqrt(u3 * (1.f - u3));
        
        float s2 = 2.f * u3 * sin2 * sin2 - 1.f; 
        float c2 = 2.f * u3 * cos2 * cos2 - 1.f; 
        float sc = 2.f * u3 * sin2 * cos2;
        
        // Create the random rotation matrix
        float _11 = cos1 * c2 - sin1 * sc;
        float _12 = sin1 * c2 + cos1 * sc;
        float _13 = sq3 * cos2;
        
        float _21 = cos1 * sc - sin1 * s2;
        float _22 = sin1 * sc + cos1 * s2;
        float _23 = sq3 * sin2;
        
        float _31 = cos1 * (sq3 * cos2) - sin1 * (sq3 * sin2);
        float _32 = sin1 * (sq3 * cos2) + cos1 * (sq3 * sin2);
        float _33 = 1.f - 2.f * u3;
        
        IrradianceField.RotationTransform.XAxis = AK_V4(_11, _12, _13, 1.0f);
        IrradianceField.RotationTransform.YAxis = AK_V4(_21, _22, _23, 1.0f);
        IrradianceField.RotationTransform.ZAxis = AK_V4(_31, _32, _33, 1.0f);
        IrradianceField.RotationTransform.Translation = AK_V4(0.0f, 0.0f, 0.0f, 1.0f);
        
        IrradianceFieldBuffer.Upload(DirectCommandList, &UploadBuffer, &IrradianceField, sizeof(irradiance_field), 0);        
        
        for(ak_u32 LightIndex = 0; LightIndex < Lights.Size; LightIndex++)
        {
            light Light = Lights[LightIndex];
            ak_m4f LightPerspective = AK_Perspective(AK_PI*0.5f, AK_SafeRatio(ShadowMapResolution.w, ShadowMapResolution.h), 0.01f, Light.Radius);                    
            ak_m4f LightViewProj[6][4] = 
            {
                {AK_LookAt(Light.Position, Light.Position + AK_XAxis(), AK_YAxis())*LightPerspective},
                {AK_LookAt(Light.Position, Light.Position - AK_XAxis(), AK_YAxis())*LightPerspective},
                {AK_LookAt(Light.Position, Light.Position + AK_YAxis(), AK_ZAxis())*LightPerspective},
                {AK_LookAt(Light.Position, Light.Position - AK_YAxis(), AK_ZAxis())*LightPerspective},
                {AK_LookAt(Light.Position, Light.Position + AK_ZAxis(), AK_YAxis())*LightPerspective},
                {AK_LookAt(Light.Position, Light.Position - AK_ZAxis(), AK_YAxis())*LightPerspective}
            };                
            
            ViewProjBuffer.Upload(DirectCommandList, &UploadBuffer, LightViewProj, sizeof(LightViewProj), MatrixAlignedSize*(1+LightIndex*6));
        }        
        
        UploadTexture(DirectCommandList, HitAlbedoTexture, &UploadBuffer, HitAlbedo, DXGI_FORMAT_R8G8B8A8_UNORM, 
                      TotalProbeCount, RAYS_PER_PROBE, TotalProbeCount*sizeof(ak_u32));                
        UploadTexture(DirectCommandList, HitPositionTexture, &UploadBuffer, HitPositions, DXGI_FORMAT_R32G32B32A32_FLOAT, 
                      TotalProbeCount, RAYS_PER_PROBE, TotalProbeCount*sizeof(ak_v4f));
        UploadTexture(DirectCommandList, HitNormalTexture, &UploadBuffer, HitNormals, DXGI_FORMAT_R32G32B32A32_FLOAT, 
                      TotalProbeCount, RAYS_PER_PROBE, TotalProbeCount*sizeof(ak_v4f));
        UploadTexture(DirectCommandList, HitDepthTexture, &UploadBuffer, RayDepths, DXGI_FORMAT_R32_FLOAT, 
                      TotalProbeCount, RAYS_PER_PROBE, TotalProbeCount*sizeof(ak_f32));
        
        D3D12_RESOURCE_BARRIER PostCopyBarriers[] = 
        {
            GetTransitionBarrier(ViewProjBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            GetTransitionBarrier(LightBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            GetTransitionBarrier(IrradianceFieldBuffer.Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER), 
            GetTransitionBarrier(SwapChainBuffers[FrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
            GetTransitionBarrier(HitAlbedoTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON), 
            GetTransitionBarrier(HitPositionTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON),
            GetTransitionBarrier(HitNormalTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON), 
            GetTransitionBarrier(HitDepthTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON),            
        };
        DirectCommandList->ResourceBarrier(AK_Count(PostCopyBarriers), PostCopyBarriers);
        
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
        
        if(Lights.Size)
        {            
            D3D12_VIEWPORT Viewport = {0, 0, (ak_f32)ShadowMapResolution.w, (ak_f32)ShadowMapResolution.h, 0.0f, 1.0f};
            D3D12_RECT Rect = {0, 0, ShadowMapResolution.w, ShadowMapResolution.h};
            
            DirectCommandList->RSSetViewports(1, &Viewport);
            DirectCommandList->RSSetScissorRects(1, &Rect);
            
            DirectCommandList->SetPipelineState(ShadowMapPipelineState);
            DirectCommandList->SetGraphicsRootSignature(ShadowMapRootSignature);            
            
            for(ak_u32 LightIndex = 0; LightIndex < Lights.Size; LightIndex++)
            {       
                light Light = Lights[LightIndex];
                for(ak_u32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = DSVDescriptors.CPUIndex(1+(LightIndex*6 + FaceIndex));
                    DirectCommandList->OMSetRenderTargets(0, NULL, FALSE, &DSVHandle);
                    DirectCommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
                    
                    DirectCommandList->SetGraphicsRootConstantBufferView(2, ViewProjBuffer.GetGPUAddress(MatrixAlignedSize*(1+(LightIndex*6+FaceIndex))));
                    
                    ak_v4f ConstantData = AK_V4(Light.Position, Light.Radius);
                    DirectCommandList->SetGraphicsRoot32BitConstants(1, 4, &ConstantData, 0);
                    AK_ForEach(Object, &Objects)
                    {                        
                        mesh_p3_n3* Mesh = Object->Mesh;
                        
                        DirectCommandList->SetGraphicsRoot32BitConstants(0, 16, &Object->Transform, 0);
                        DirectCommandList->DrawIndexedInstanced(Mesh->IndexCount, 1, Mesh->IndexOffset, Mesh->VertexOffset, 0);
                    }
                }
            }
        }                
        D3D12_RESOURCE_BARRIER PreComputeBarriers[] =
        {
            GetTransitionBarrier(IrradianceTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON), 
            GetTransitionBarrier(DistanceTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),    
            GetTransitionBarrier(ProbeRadianceTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        
        DirectCommandList->ResourceBarrier(AK_Count(PreComputeBarriers), PreComputeBarriers);
        
        DirectCommandList->SetDescriptorHeaps(1, &SRVDescriptors.Heap);
        
        DirectCommandList->SetPipelineState(ShadeSurfelsPipelineState);
        DirectCommandList->SetComputeRootSignature(ShadeSurfelsRootSignature);
        DirectCommandList->SetComputeRootConstantBufferView(0, LightBuffer.GetGPUAddress(0));
        DirectCommandList->SetComputeRootConstantBufferView(1, IrradianceFieldBuffer.GetGPUAddress(0));
        DirectCommandList->SetComputeRootDescriptorTable(2, SRVDescriptors.GPUIndex(0));
        
        DirectCommandList->Dispatch(AK_Ceil(TotalProbeCount/8.0f), AK_Ceil(RAYS_PER_PROBE/4.0f), 1);
        
        D3D12_RESOURCE_BARRIER MidComputeBarriers[] = 
        {
            GetTransitionBarrier(IrradianceTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS), 
            GetTransitionBarrier(DistanceTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS), 
            GetTransitionBarrier(ProbeRadianceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON)
        };
        
        DirectCommandList->ResourceBarrier(AK_Count(MidComputeBarriers), MidComputeBarriers);
        
        DirectCommandList->SetComputeRootSignature(ProbeBlendingRootSignature);
        DirectCommandList->SetComputeRootConstantBufferView(0, IrradianceFieldBuffer.GetGPUAddress(0));
        DirectCommandList->SetComputeRootDescriptorTable(1, SRVDescriptors.GPUIndex(8));        
        
        DirectCommandList->SetPipelineState(IrradianceProbeBlendingPipelineState);        
        DirectCommandList->Dispatch(IrradianceFieldPitch, (ak_u32)IrradianceField.Count.y, 1);
        
        DirectCommandList->SetPipelineState(DistanceProbeBlendingPipelineState);
        DirectCommandList->Dispatch(IrradianceFieldPitch, (ak_u32)IrradianceField.Count.y, 1);
        
        D3D12_RESOURCE_BARRIER UAVBarriers[2] = 
        {
            GetUAVBarrier(IrradianceTexture),
            GetUAVBarrier(DistanceTexture)
        };
        
        DirectCommandList->ResourceBarrier(AK_Count(UAVBarriers), UAVBarriers);
        
        DirectCommandList->SetComputeRootSignature(ProbeBorderUpdateRootSignature);
        DirectCommandList->SetComputeRootConstantBufferView(1, IrradianceFieldBuffer.GetGPUAddress(0));
        DirectCommandList->SetComputeRootDescriptorTable(2, SRVDescriptors.GPUIndex(9));
        
        ak_u32 ProbeUpdateBorderConstants[2] = {};            
        DirectCommandList->SetPipelineState(ProbeBorderUpdateRowPipelineState);
        ProbeUpdateBorderConstants[0] = IRRADIANCE_TEXEL_COUNT;
        ProbeUpdateBorderConstants[1] = 0;
        DirectCommandList->SetComputeRoot32BitConstants(0, 2, ProbeUpdateBorderConstants, 0);                        
        DirectCommandList->Dispatch(TotalProbeCount, (ak_u32)IrradianceField.Count.y, 1);
        
        ProbeUpdateBorderConstants[0] = DISTANCE_TEXEL_COUNT;
        ProbeUpdateBorderConstants[1] = 1;
        DirectCommandList->SetComputeRoot32BitConstants(0, 2, ProbeUpdateBorderConstants, 0);
        DirectCommandList->Dispatch(TotalProbeCount, (ak_u32)IrradianceField.Count.y, 1);
        
        DirectCommandList->ResourceBarrier(AK_Count(UAVBarriers), UAVBarriers);
                
        DirectCommandList->SetPipelineState(ProbeBorderUpdateColumnPipelineState);
        ProbeUpdateBorderConstants[0] = IRRADIANCE_TEXEL_COUNT;
        ProbeUpdateBorderConstants[1] = 0;
        DirectCommandList->SetComputeRoot32BitConstants(0, 2, ProbeUpdateBorderConstants, 0);
        DirectCommandList->Dispatch((ak_u32)IrradianceField.Count.y, TotalProbeCount, 1);
        
        ProbeUpdateBorderConstants[0] = DISTANCE_TEXEL_COUNT;
        ProbeUpdateBorderConstants[1] = 1;
        DirectCommandList->SetComputeRoot32BitConstants(0, 2, ProbeUpdateBorderConstants, 0);
        DirectCommandList->Dispatch((ak_u32)IrradianceField.Count.y, TotalProbeCount, 1);
        
        D3D12_RESOURCE_BARRIER PostComputeBarriers[] = 
        {
            GetTransitionBarrier(IrradianceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), 
            GetTransitionBarrier(DistanceTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };
        
        DirectCommandList->ResourceBarrier(AK_Count(PostComputeBarriers), PostComputeBarriers);
        
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = RTVDescriptors.CPUIndex(FrameIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = DSVDescriptors.CPUIndex(0);
        DirectCommandList->OMSetRenderTargets(1, &RTVHandle, FALSE, &DSVHandle);
        
        ak_color4f ClearColor = AK_Black4();
        DirectCommandList->ClearRenderTargetView(RTVHandle, ClearColor.Data, 0, NULL);
        DirectCommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
        
        D3D12_VIEWPORT Viewport = {0, 0, (ak_f32)Resolution.w, (ak_f32)Resolution.h, 0.0f, 1.0f};
        D3D12_RECT Rect = {0, 0, Resolution.w, Resolution.h};
        
        DirectCommandList->RSSetViewports(1, &Viewport);
        DirectCommandList->RSSetScissorRects(1, &Rect);
        
        DirectCommandList->SetPipelineState(ObjectPipelineState);
        DirectCommandList->SetGraphicsRootSignature(ObjectRootSignature);
        DirectCommandList->SetGraphicsRootConstantBufferView(2, ViewProjBuffer.GetGPUAddress(0));
        DirectCommandList->SetGraphicsRootConstantBufferView(3, LightBuffer.GetGPUAddress(0));
        DirectCommandList->SetGraphicsRootConstantBufferView(4, IrradianceFieldBuffer.GetGPUAddress(0));
        DirectCommandList->SetGraphicsRootDescriptorTable(5, SRVDescriptors.GPUIndex(0));
        
        AK_ForEach(Object, &Objects)
        {            
            mesh_p3_n3* Mesh = Object->Mesh;
            
            ak_v4f ConstantData[2] = 
            {
                Object->Color, 
                AK_V4(ViewSettings.Position, 1.0f)
            };
            
            DirectCommandList->SetGraphicsRoot32BitConstants(0, 16, &Object->Transform, 0);        
            DirectCommandList->SetGraphicsRoot32BitConstants(1, 8, ConstantData, 0);
            DirectCommandList->DrawIndexedInstanced(Mesh->IndexCount, 1, Mesh->IndexOffset, Mesh->VertexOffset, 0);
        }
        
        VertexBufferView = {};
        VertexBufferView.BufferLocation = VertexBuffer.GetGPUAddress(DebugPrimitiveOffset);
        VertexBufferView.SizeInBytes = VertexBuffer.Size-DebugPrimitiveOffset;
        VertexBufferView.StrideInBytes = sizeof(ak_vertex_p3);
        
        DirectCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);        
        DirectCommandList->SetPipelineState(DebugPrimitivePipelineState);
        
        AK_ForEach(Light, &Lights)
        {
            ak_m4f Transform = AK_TransformM4(Light->Position, AK_V3(0.25f, 0.25f, 0.25f));
            ak_color4f Color = AK_Yellow4();
            
            DirectCommandList->SetGraphicsRoot32BitConstants(0, 16, &Transform, 0);
            DirectCommandList->SetGraphicsRoot32BitConstants(1, 4, &Color, 0);
            DirectCommandList->DrawIndexedInstanced(DebugSphereMesh.IndexCount, 1, DebugSphereMesh.IndexOffset, DebugSphereMesh.VertexOffset, 0);
        }
        
        for(ak_i32 ProbeZ = 0; ProbeZ < IrradianceField.Count.z; ProbeZ++)        
        {            
            for(ak_i32 ProbeY = 0; ProbeY < IrradianceField.Count.y; ProbeY++)
            {
                for(ak_i32 ProbeX = 0; ProbeX < IrradianceField.Count.x; ProbeX++)
                {
                    ak_u32 ProbeIndex = ProbeZ*(ak_u32)IrradianceField.Count.z*(ak_u32)IrradianceField.Count.y + ProbeY*(ak_u32)IrradianceField.Count.y + ProbeX;
                    ak_v3f Position = IrradianceField.BottomLeft + AK_V3f(ProbeX, ProbeY, ProbeZ)*IrradianceField.Spacing;
                    
                    ak_m4f Transform = AK_TransformM4(Position, AK_V3(0.15f, 0.15f, 0.15f));
                    ak_color4f Color = AK_Orange4()*0.4f;
                    
                    DirectCommandList->SetGraphicsRoot32BitConstants(0, 16, &Transform, 0);
                    DirectCommandList->SetGraphicsRoot32BitConstants(1, 4, &Color, 0);
                    DirectCommandList->DrawIndexedInstanced(DebugSphereMesh.IndexCount, 1, DebugSphereMesh.IndexOffset, DebugSphereMesh.VertexOffset, 0);
                    
#if 0
                    for(ak_i32 RayIndex = 0; RayIndex < RAYS_PER_PROBE; RayIndex++)
                    {
                        ak_v3f RayEnd = Position+RayDirections[RayIndex]*100.0f;
                        ak_v3f ZAxis = RayEnd-Position;
                        ak_f32 ZLength = AK_Magnitude(ZAxis);
                        ZAxis /= ZLength;
                        
                        ak_m3f Basis = AK_Basis(ZAxis);                        
                        ak_m4f Model = AK_TransformM4(Position, Basis, AK_V3(0.003f, 0.003f, ZLength));
                        
                        ak_color4f RayColor = AK_Yellow4();
                        if(AK_SqrMagnitude(HitNormals[(ProbeIndex*RAYS_PER_PROBE)+RayIndex]) > 0.0f)
                            RayColor = AK_Red4();
                        
                        DirectCommandList->SetGraphicsRoot32BitConstants(0, 16, &Model, 0);
                        DirectCommandList->SetGraphicsRoot32BitConstants(1, 4, &RayColor, 0);
                        DirectCommandList->DrawIndexedInstanced(DebugBoxMesh.IndexCount, 1, DebugBoxMesh.IndexOffset, DebugBoxMesh.VertexOffset, 0);
                    }
#endif
                }
            }
        }
        
        
        
        D3D12_RESOURCE_BARRIER PrePresentBarriers[] = 
        {
            GetTransitionBarrier(SwapChainBuffers[FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)
        };
        
        DirectCommandList->ResourceBarrier(AK_Count(PrePresentBarriers), PrePresentBarriers);
        
        DirectCommandList->Close();
        DirectCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&DirectCommandList);
        Fence.Signal(DirectCommandQueue);
        
        SwapChain->Present(1, 0);
        
        FrameIndex = (FrameIndex+1) % SwapChainBuffers.Size;                                                              
        
        UploadBuffer.Reset();
        
        GlobalArena->EndTemp(&TempArena);        
        
        Input.LastMouseCoordinates = Input.MouseCoordinates;
        Input.Scroll = 0;
        
    }
    
    return 0;
}