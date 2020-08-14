#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <ak_common.h>
#include "platform.h"

#define MAX_JOINT_COUNT 256
#define MAX_DIRECTIONAL_LIGHT_COUNT 1
#define MAX_POINT_LIGHT_COUNT 9

#define SHADOW_MAP_WIDTH (512*2)
#define SHADOW_MAP_HEIGHT (512*2)

#define INVALID_GRAPHICS_MESH_ID ((graphics_mesh_id)-1)
#define INVALID_GRAPHICS_TEXTURE_ID ((graphics_texture_id)-1)

typedef i64 graphics_texture_id;
typedef i64 graphics_mesh_id;

struct view_settings
{
    v3f Position;
    m3 Orientation;
    f32 FieldOfView;
    f32 ZNear;
    f32 ZFar;
};

enum graphics_vertex_format
{
    GRAPHICS_VERTEX_FORMAT_UNKNOWN,
    GRAPHICS_VERTEX_FORMAT_P2_UV_C,
    GRAPHICS_VERTEX_FORMAT_P3,
    GRAPHICS_VERTEX_FORMAT_P3_N3,
    GRAPHICS_VERTEX_FORMAT_P3_N3_UV,
    GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS,
    GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS,
    GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV,
    GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV_WEIGHTS
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
    b32 IsTexture;
    union
    {
        graphics_texture_id DiffuseID;
        v3f Diffuse;
    };
};

struct graphics_specular_material_slot
{
    b32 InUse;    
    b32 IsTexture;
    union
    {
        graphics_texture_id SpecularID;
        f32 Specular;
    };
    i32 Shininess;
};

struct graphics_normal_material_slot
{
    b32 InUse;
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
        
        INVALID_DEFAULT_CASE;
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
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct graphics_sampler_info
{
    graphics_filter MinFilter;
    graphics_filter MagFilter;
};

struct graphics_directional_light
{    
    v3f Position; 
    v3f Direction;    
    c3 Color;    
    f32 Intensity;        
    m4 ViewProjection;
};

struct graphics_point_light
{        
    c3 Color;  
    f32 Intensity;
    v3f Position;
    f32 Radius;
};

struct graphics_light_buffer
{
    u32 DirectionalLightCount;
    u32 PointLightCount;
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
    v2i Resolution;
};

#include "graphics_push_commands.h"

struct graphics;

#define ALLOCATE_TEXTURE(name) graphics_texture_id name(graphics* Graphics, void* Data, v2i Dimensions, graphics_texture_format TextureFormat, graphics_sampler_info* SamplerInfo)
typedef ALLOCATE_TEXTURE(allocate_texture);

#define ALLOCATE_MESH(name) graphics_mesh_id name(graphics* Graphics, void* VertexData, ptr VertexDataSize, graphics_vertex_format VertexFormat, void* IndexData, ptr IndexDataSize, graphics_index_format IndexFormat)
typedef ALLOCATE_MESH(allocate_mesh);

#define ALLOCATE_DYNAMIC_MESH(name) graphics_mesh_id name(graphics* Graphics, graphics_vertex_format VertexFormat, graphics_index_format IndexFormat)
typedef ALLOCATE_DYNAMIC_MESH(allocate_dynamic_mesh);

#define ALLOCATE_RENDER_BUFFER(name) graphics_render_buffer* name(graphics* Graphics, v2i Resolution)
typedef ALLOCATE_RENDER_BUFFER(allocate_render_buffer);

#define STREAM_MESH_DATA(name) void name(graphics* Graphics, graphics_mesh_id MeshID, void* VertexData, ptr VertexSize, void* IndexData, ptr IndexSize)
typedef STREAM_MESH_DATA(stream_mesh_data);

#define INIT_GRAPHICS(name) graphics* name(platform* Platform, void** PlatformData)
typedef INIT_GRAPHICS(init_graphics);

#define BIND_GRAPHICS_FUNCTIONS(name) b32 name(graphics* Graphics)
typedef BIND_GRAPHICS_FUNCTIONS(bind_graphics_functions);

#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics, platform* Platform, void* DevContext)
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

#define INVALIDATE_SHADERS(name) void name(graphics* Graphics)
typedef INVALIDATE_SHADERS(invalidate_shaders);

struct graphics
{       
    v2i RenderDim;
    push_command_list CommandList;    
    void** PlatformData;                                
    
    allocate_texture* AllocateTexture;
    allocate_mesh* AllocateMesh;
    allocate_dynamic_mesh* AllocateDynamicMesh;
    allocate_render_buffer* AllocateRenderBuffer;
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

inline ptr 
GetVertexStride(graphics_vertex_format Format)
{
    ptr Result = 0;
    
    switch(Format)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C: { Result = sizeof(vertex_p2_uv_c); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3: { Result = sizeof(vertex_p3); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3_N3: { Result = sizeof(vertex_p3_n3); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV: { Result = sizeof(vertex_p3_n3_uv); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS: { Result = sizeof(vertex_p3_n3_weights); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS: { Result = sizeof(vertex_p3_n3_uv_weights); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV: { Result = sizeof(vertex_p3_n3_t4_uv); } break;
        case GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV_WEIGHTS: { Result = sizeof(vertex_p3_n3_t4_uv_weights); } break;
        INVALID_DEFAULT_CASE;
    }
    
    return Result;
}

inline ptr
GetVertexBufferSize(graphics_vertex_format Format, u32 VertexCount)
{
    ptr Result = GetVertexStride(Format)*VertexCount;
    return Result;
}

inline ptr
GetIndexSize(graphics_index_format Format)
{
    ASSERT(Format != GRAPHICS_INDEX_FORMAT_UNKNOWN);    
    ptr Result = (Format == GRAPHICS_INDEX_FORMAT_16_BIT) ? sizeof(u16) : sizeof(u32);
    return Result;
}

inline ptr
GetIndexBufferSize(graphics_index_format Format, u32 IndexCount)
{    
    ptr Result = GetIndexSize(Format) * IndexCount;
    return Result;
}

inline graphics_directional_light
CreateDirectionalLight(v3f Position, c3 Color, f32 Intensity, v3f Direction, 
                       f32 MinX, f32 MaxX, f32 MinY, f32 MaxY, f32 MinZ, f32 MaxZ)
{
    graphics_directional_light Result;
    Result.Position = Position;
    Result.Color = Color;
    Result.Intensity = Intensity;
    Result.Direction = Direction;    
    Result.ViewProjection = LookAt(Position, Position+Direction)*OrthographicM4(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);    
    return Result;
}

inline graphics_point_light 
CreatePointLight(c3 Color, f32 Intensity, v3f Position, f32 Radius)
{
    graphics_point_light Result;    
    Result.Color = Color;
    Result.Intensity = Intensity;    
    Result.Position = Position;
    Result.Radius = Radius;
    return Result;
}

inline graphics_diffuse_material_slot 
CreateDiffuseMaterialSlot(c3 Diffuse)
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

inline graphics_specular_material_slot
CreateSpecularMaterialSlot(f32 Specular, i32 Shininess)
{
    graphics_specular_material_slot Result = {};
    Result.InUse = true;    
    Result.Specular = Specular;
    Result.Shininess = Shininess;
    return Result;
}

inline graphics_specular_material_slot 
CreateSpecularMaterialSlot(graphics_texture_id SpecularID, i32 Shininess)
{
    graphics_specular_material_slot Result = {};
    Result.InUse = true;
    Result.IsTexture = true;
    Result.SpecularID = SpecularID;    
    Result.Shininess = Shininess;
    return Result;
}

inline graphics_normal_material_slot 
CreateNormalMaterialSlot(graphics_texture_id NormalID)
{
    graphics_normal_material_slot Result;
    Result.InUse = true;
    Result.NormalID = NormalID;
    return Result;
}

#endif