#ifndef GRAPHICS_H
#define GRAPHICS_H

#if DEVELOPER_BUILD
#ifndef AK_DEVELOPER_BUILD
#define AK_DEVELOPER_BUILD
#endif
#endif

#include <ak_common.h>

#define MAX_JOINT_COUNT 256
#define MAX_DIRECTIONAL_LIGHT_COUNT 1
#define MAX_POINT_LIGHT_COUNT 9

#define SHADOW_MAP_WIDTH (512*2)
#define SHADOW_MAP_HEIGHT (512*2)

#define INVALID_GRAPHICS_MESH_ID ((graphics_mesh_id)-1)
#define INVALID_GRAPHICS_TEXTURE_ID ((graphics_texture_id)-1)

typedef ak_u64 graphics_texture_id;
typedef ak_u64 graphics_mesh_id;

struct view_settings
{
    ak_v3f Position;
    ak_m3f Orientation;
    ak_f32 FieldOfView;
    ak_f32 ZNear;
    ak_f32 ZFar;
};

enum graphics_vertex_format
{
    GRAPHICS_VERTEX_FORMAT_UNKNOWN,
    GRAPHICS_VERTEX_FORMAT_P2_UV_C,
    GRAPHICS_VERTEX_FORMAT_P3,
    GRAPHICS_VERTEX_FORMAT_P3_N3,
    GRAPHICS_VERTEX_FORMAT_P3_N3_UV,
    GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS,
    GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS        
};

enum graphics_index_format
{
    GRAPHICS_INDEX_FORMAT_UNKNOWN,
    GRAPHICS_INDEX_FORMAT_16_BIT,
    GRAPHICS_INDEX_FORMAT_32_BIT
};

enum graphics_texture_format
{   
    GRAPHICS_TEXTURE_FORMAT_R8,    
    GRAPHICS_TEXTURE_FORMAT_R8G8B8,
    GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB,
    GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8, 
    GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8
};

struct graphics_diffuse_material_slot
{
    ak_bool IsTexture;
    union
    {
        graphics_texture_id DiffuseID;
        ak_v3f Diffuse;
    };
};

struct graphics_specular_material_slot
{
    ak_bool InUse;    
    ak_bool IsTexture;
    union
    {
        graphics_texture_id SpecularID;
        ak_f32 Specular;
    };
    ak_i32 Shininess;
};

struct graphics_normal_material_slot
{
    ak_bool InUse;
    graphics_texture_id NormalID;
};

struct graphics_material
{   
    graphics_diffuse_material_slot  Diffuse;
    graphics_specular_material_slot Specular;
    graphics_normal_material_slot   Normal;
};

inline graphics_texture_format 
ConvertToSRGBFormat(graphics_texture_format Format)
{
    switch(Format)
    {
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8:
        {
            return GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB;
        }
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8:
        {
            return GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8;
        }
                        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (graphics_texture_format)-1;
}

enum graphics_filter
{
    GRAPHICS_FILTER_UNKNOWN,
    GRAPHICS_FILTER_LINEAR
};

enum graphics_cull_mode
{
    GRAPHICS_CULL_MODE_NONE,
    GRAPHICS_CULL_MODE_FRONT,
    GRAPHICS_CULL_MODE_BACK
};

struct graphics_draw_info
{
    ak_u32 IndexCount;
    ak_u32 IndexOffset;
    ak_u32 VertexOffset;
};

struct graphics_sampler_info
{
    graphics_filter MinFilter;
    graphics_filter MagFilter;
};

struct graphics_directional_light
{    
    ak_v3f Position; 
    ak_v3f Direction;    
    ak_color3f Color;    
    ak_f32 Intensity;        
    ak_m4f ViewProjection;
};

struct graphics_point_light
{        
    ak_color3f Color;  
    ak_f32 Intensity;
    ak_v3f Position;
    ak_f32 Radius;
};

struct graphics_light_buffer
{
    ak_u32 DirectionalLightCount;
    ak_u32 PointLightCount;
    graphics_directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];    
    graphics_point_light PointLights[MAX_POINT_LIGHT_COUNT];
};

enum graphics_blend
{
    GRAPHICS_BLEND_UNKNOWN,
    GRAPHICS_BLEND_SRC_ALPHA,
    GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA
};

struct graphics_render_buffer
{
    ak_v2i Resolution;
};

#include "graphics_push_commands.h"

struct graphics;

#define ALLOCATE_TEXTURE(name) graphics_texture_id name(graphics* Graphics, void* Data, ak_u32 Width, ak_u32 Height, graphics_texture_format TextureFormat, graphics_sampler_info* SamplerInfo)
typedef ALLOCATE_TEXTURE(allocate_texture);

#define ALLOCATE_MESH(name) graphics_mesh_id name(graphics* Graphics, void* VertexData, ak_u32 VertexDataSize, graphics_vertex_format VertexFormat, void* IndexData, ak_u32 IndexDataSize, graphics_index_format IndexFormat)
typedef ALLOCATE_MESH(allocate_mesh);

#define ALLOCATE_DYNAMIC_MESH(name) graphics_mesh_id name(graphics* Graphics, graphics_vertex_format VertexFormat, graphics_index_format IndexFormat)
typedef ALLOCATE_DYNAMIC_MESH(allocate_dynamic_mesh);

#define ALLOCATE_RENDER_BUFFER(name) graphics_render_buffer* name(graphics* Graphics, ak_v2i Resolution)
typedef ALLOCATE_RENDER_BUFFER(allocate_render_buffer);

#define FREE_RENDER_BUFFER(name) void name(graphics* Graphics, graphics_render_buffer* RenderBuffer)
typedef FREE_RENDER_BUFFER(free_render_buffer);

#define STREAM_MESH_DATA(name) void name(graphics* Graphics, graphics_mesh_id MeshID, void* VertexData, ak_u32 VertexSize, void* IndexData, ak_u32 IndexSize)
typedef STREAM_MESH_DATA(stream_mesh_data);

#define INIT_GRAPHICS(name) graphics* name(ak_arena* TempStorage, void** PlatformData)
typedef INIT_GRAPHICS(init_graphics);

#define BIND_GRAPHICS_FUNCTIONS(name) ak_bool name(graphics* Graphics)
typedef BIND_GRAPHICS_FUNCTIONS(bind_graphics_functions);

#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics, void* DevContext)
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

#define INVALIDATE_SHADERS(name) void name(graphics* Graphics)
typedef INVALIDATE_SHADERS(invalidate_shaders);

struct graphics
{       
    ak_v2i RenderDim;
    push_command_list CommandList;    
    void** PlatformData;                                        
    
    allocate_texture* AllocateTexture;
    allocate_mesh* AllocateMesh;
    allocate_dynamic_mesh* AllocateDynamicMesh;
    allocate_render_buffer* AllocateRenderBuffer;
    free_render_buffer* FreeRenderBuffer;
    stream_mesh_data* StreamMeshData;        
};

STREAM_MESH_DATA(Graphics_StreamMeshDataStub)
{
}

INIT_GRAPHICS(Graphics_InitGraphicsStub)
{
    return NULL;
}

BIND_GRAPHICS_FUNCTIONS(Graphics_BindGraphicsFunctionsStub)
{
    Graphics->StreamMeshData = Graphics_StreamMeshDataStub;
    return false;
}

EXECUTE_RENDER_COMMANDS(Graphics_ExecuteRenderCommandsStub)
{    
    Graphics->CommandList.Count = 0;
}

INVALIDATE_SHADERS(Graphics_InvalidateShadersStub)
{    
}

inline ak_u32 
GetVertexStride(graphics_vertex_format Format)
{
    ak_u32 Result = 0;
    
    switch(Format)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C: { Result = sizeof(ak_vertex_p2_uv_c); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3: { Result = sizeof(ak_vertex_p3); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3_N3: { Result = sizeof(ak_vertex_p3_n3); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV: { Result = sizeof(ak_vertex_p3_n3_uv); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS: { Result = sizeof(ak_vertex_p3_n3_w); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS: { Result = sizeof(ak_vertex_p3_n3_uv_w); } break;                
        AK_INVALID_DEFAULT_CASE;
    }
    
    return Result;
}

inline ak_u32
GetVertexBufferSize(graphics_vertex_format Format, ak_u32 VertexCount)
{
    ak_u32 Result = GetVertexStride(Format)*VertexCount;
    return Result;
}

inline ak_u32
GetIndexSize(graphics_index_format Format)
{
    AK_Assert(Format != GRAPHICS_INDEX_FORMAT_UNKNOWN, "Unknown graphics index format");    
    ak_u32 Result = (Format == GRAPHICS_INDEX_FORMAT_16_BIT) ? sizeof(ak_u16) : sizeof(ak_u32);
    return Result;
}

inline ak_u32
GetIndexBufferSize(graphics_index_format Format, ak_u32 IndexCount)
{    
    ak_u32 Result = GetIndexSize(Format) * IndexCount;
    return Result;
}

inline graphics_directional_light
CreateDirectionalLight(ak_v3f Position, ak_color3f Color, ak_f32 Intensity, ak_v3f Direction, 
                       ak_f32 MinX, ak_f32 MaxX, ak_f32 MinY, ak_f32 MaxY, ak_f32 MinZ, ak_f32 MaxZ)
{
    graphics_directional_light Result;
    Result.Position = Position;
    Result.Color = Color;
    Result.Intensity = Intensity;
    Result.Direction = Direction;    
    Result.ViewProjection = AK_LookAt(Position, Position+Direction)*AK_Orthographic(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);    
    return Result;
}

inline graphics_point_light 
CreatePointLight(ak_color3f Color, ak_f32 Intensity, ak_v3f Position, ak_f32 Radius)
{
    graphics_point_light Result;    
    Result.Color = Color;
    Result.Intensity = Intensity;    
    Result.Position = Position;
    Result.Radius = Radius;
    return Result;
}

inline graphics_diffuse_material_slot 
CreateDiffuseMaterialSlot(ak_color3f Diffuse)
{
    graphics_diffuse_material_slot Result = {};
    Result.Diffuse = Diffuse;
    return Result;
}

inline graphics_diffuse_material_slot 
CreateDiffuseMaterialSlot(graphics_texture_id DiffuseID)
{
    graphics_diffuse_material_slot Result = {};
    Result.IsTexture = true;
    Result.DiffuseID = DiffuseID;
    return Result;
}

inline graphics_material InvalidGraphicsMaterial()
{
    graphics_material Material = {};
    Material.Diffuse.IsTexture = -1;
    return Material;
}

ak_bool IsInvalidGraphicsMaterial(graphics_material Material)
{
    ak_bool Result = Material.Diffuse.IsTexture == -1;
    return Result;
}

inline ak_bool 
DiffuseSlotsEqual(graphics_diffuse_material_slot A, graphics_diffuse_material_slot B)
{
    if(A.IsTexture == -1 || B.IsTexture == -1) return false;    
    if(A.IsTexture != B.IsTexture) return false;
    
    if(A.IsTexture) return A.DiffuseID == B.DiffuseID;            
    else return A.Diffuse == B.Diffuse;    
}

inline ak_bool
NormalSlotsEqual(graphics_normal_material_slot A, graphics_normal_material_slot B)
{
    if(A.InUse != B.InUse) return false;    
    if(A.InUse) return A.NormalID == B.NormalID;
    else return true;    
}

inline ak_bool
SpecularSlotsEqual(graphics_specular_material_slot A, graphics_specular_material_slot B)
{
    if(A.InUse != B.InUse) return false;
    
    if(A.InUse)
    {
        if(A.IsTexture != B.IsTexture) return false;
        if(A.Shininess != B.Shininess) return false;
        
        if(A.IsTexture) return A.SpecularID == B.SpecularID;
        else return A.Specular == B.Specular;
    }
    else return true;
}

inline ak_bool 
AreSameMaterials(graphics_material A, graphics_material B)
{
    ak_bool Result = (DiffuseSlotsEqual(A.Diffuse, B.Diffuse) && 
                      NormalSlotsEqual(A.Normal, B.Normal) && 
                      SpecularSlotsEqual(A.Specular, B.Specular));
    return Result;
}

inline ak_bool
ShouldUpdateMaterial(graphics_material Current, graphics_material Prev)
{
    ak_bool Result = !IsInvalidGraphicsMaterial(Current) && !AreSameMaterials(Current, Prev);
    return Result;
}

#endif