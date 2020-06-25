#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "AKCommon/common.h"
#include "platform.h"

#define MAX_JOINT_COUNT 256
#define MAX_DIRECTIONAL_LIGHT_COUNT 1
#define MAX_POINT_LIGHT_COUNT 8

#define SHADOW_MAP_WIDTH 1024
#define SHADOW_MAP_HEIGHT 1024

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

struct shadow_map;

struct graphics_directional_light
{    
    v3f Position; 
    v3f Direction;    
    c3 Color;    
    f32 Intensity;    
    shadow_map* ShadowMap;
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

enum push_command_type
{
    PUSH_COMMAND_UNKNOWN,
    PUSH_COMMAND_CLEAR_COLOR,
    PUSH_COMMAND_CLEAR_DEPTH,
    PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH,
    PUSH_COMMAND_DEPTH,
    PUSH_COMMAND_CULL,
    PUSH_COMMAND_WIREFRAME,
    PUSH_COMMAND_BLEND,
    PUSH_COMMAND_SCISSOR,    
    PUSH_COMMAND_VIEWPORT,
    PUSH_COMMAND_PROJECTION,
    PUSH_COMMAND_VIEW_TRANSFORM,    
    PUSH_COMMAND_SUBMIT_LIGHT_BUFFER,
    PUSH_COMMAND_DRAW_COLORED_LINE_MESH,
    PUSH_COMMAND_DRAW_COLORED_MESH,
    PUSH_COMMAND_DRAW_TEXTURED_MESH,
    PUSH_COMMAND_DRAW_COLORED_SKINNING_MESH,
    PUSH_COMMAND_DRAW_TEXTURED_SKINNING_MESH,
    PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_MESH,
    PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_MESH,
    PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_SKINNING_MESH,
    PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_SKINNING_MESH,
    PUSH_COMMAND_DRAW_PHONG_COLORED_MESH,
    PUSH_COMMAND_DRAW_PHONG_TEXTURED_MESH,
    PUSH_COMMAND_DRAW_PHONG_COLORED_SKINNING_MESH,    
    PUSH_COMMAND_DRAW_PHONG_TEXTURED_SKINNING_MESH,    
    PUSH_COMMAND_DRAW_IMGUI_UI,    
    PUSH_COMMAND_LIGHT_FOR_SHADOW_MAP,
    PUSH_COMMAND_DRAW_SHADOWED_MESH
};

struct push_command
{
    push_command_type Type;
};

struct push_command_clear_color : public push_command
{
    f32 R, G, B, A;
};

struct push_command_clear_depth : public push_command
{
    f32 Depth;
};

struct push_command_clear_color_and_depth : public push_command
{
    f32 R, G, B, A;
    f32 Depth;
};

struct push_command_depth : public push_command
{
    b32 Enable;
};

struct push_command_cull : public push_command
{
    graphics_cull_mode CullMode;
};

struct push_command_wireframe : public push_command
{
    b32 Enable;
};

enum graphics_blend
{
    GRAPHICS_BLEND_UNKNOWN,
    GRAPHICS_BLEND_SRC_ALPHA,
    GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA
};

struct push_command_blend : public push_command
{
    b32 Enable;
    graphics_blend SrcGraphicsBlend;
    graphics_blend DstGraphicsBlend;
};

struct push_command_rect : public push_command
{ 
    i32 X, Y;
    i32 Width, Height;
};

struct push_command_4x4_matrix : public push_command
{
    m4 Matrix;
};

struct push_command_view_transform : public push_command
{
    v3f Position;
    m3  Orientation;
};

struct push_command_submit_light_buffer : public push_command
{
    graphics_light_buffer LightBuffer;
};

struct push_command_draw_colored_mesh : public push_command
{
    m4 WorldTransform;
    c4 Color;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;    
};

struct push_command_draw_textured_mesh : public push_command
{
    m4 WorldTransform;
    i64 TextureID;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;    
};

struct push_command_draw_colored_skinning_mesh : public push_command
{
    m4 WorldTransform;
    c4 Color;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_textured_skinning_mesh : public push_command
{
    m4 WorldTransform;
    i64 TextureID;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_lambertian_colored_mesh : public push_command
{
    m4 WorldTransform;
    c4 DiffuseColor;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
};

struct push_command_draw_lambertian_textured_mesh : public push_command
{
    m4 WorldTransform;
    i64 DiffuseID;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;    
};

struct push_command_draw_lambertian_colored_skinning_mesh : public push_command
{
    m4 WorldTransform;
    c4 DiffuseColor;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_lambertian_textured_skinning_mesh : public push_command
{
    m4 WorldTransform;
    i64 DiffuseID;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_phong_colored_mesh : public push_command
{    
    m4 WorldTransform;
    c4 DiffuseColor;
    c4 SpecularColor;
    i32 Shininess;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
};

struct push_command_draw_phong_textured_mesh : public push_command
{
    m4 WorldTransform;
    i64 MeshID;
    i64 DiffuseID;
    i64 SpecularID;
    i32 Shininess;
    
    graphics_draw_info DrawInfo;
};

struct push_command_draw_phong_colored_skinning_mesh : public push_command
{
    m4 WorldTransform;    
    c4 DiffuseColor;
    c4 SpecularColor;    
    i32 Shininess;
    i64 MeshID;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_phong_textured_skinning_mesh : public push_command
{
    m4 WorldTransform;
    i64 MeshID;
    i64 DiffuseID;
    i64 SpecularID;
    i32 Shininess;
    
    graphics_draw_info DrawInfo;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_imgui_ui : public push_command
{
    i64 MeshID;
    i64 TextureID;
    
    graphics_draw_info DrawInfo;
};

struct push_command_light_for_shadow_map : public push_command
{    
    m4  LightView;
    m4  LightProjection;
    shadow_map* ShadowMap;
};

struct push_command_draw_shadowed_mesh : public push_command
{
    i64 MeshID;
    m4 WorldTransform;
    
    graphics_draw_info DrawInfo;    
};

//CONFIRM(JJ): Is this alright to be fixed sized?
#define MAX_COMMAND_COUNT 1024
struct push_command_list
{
    push_command* Ptr[MAX_COMMAND_COUNT];
    u32 Count;
};

struct graphics;

#define ALLOCATE_SHADOW_MAP(name) shadow_map* name(graphics* Graphics, v2i Dimensions)
typedef ALLOCATE_SHADOW_MAP(allocate_shadow_map);

#define ALLOCATE_TEXTURE(name) i64 name(graphics* Graphics, void* Data, v2i Dimensions, b32 sRGB, graphics_sampler_info* SamplerInfo)
typedef ALLOCATE_TEXTURE(allocate_texture);

#define ALLOCATE_MESH(name) i64 name(graphics* Graphics, void* VertexData, ptr VertexDataSize, graphics_vertex_format VertexFormat, void* IndexData, ptr IndexDataSize, graphics_index_format IndexFormat)
typedef ALLOCATE_MESH(allocate_mesh);

#define ALLOCATE_DYNAMIC_MESH(name) i64 name(graphics* Graphics, graphics_vertex_format VertexFormat, graphics_index_format IndexFormat)
typedef ALLOCATE_DYNAMIC_MESH(allocate_dynamic_mesh);

#define STREAM_MESH_DATA(name) void name(graphics* Graphics, i64 MeshID, void* VertexData, ptr VertexSize, void* IndexData, ptr IndexSize)
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
    
    shadow_map* ShadowMaps[MAX_DIRECTIONAL_LIGHT_COUNT];
    
    allocate_shadow_map* AllocateShadowMap;
    allocate_texture* AllocateTexture;
    allocate_mesh* AllocateMesh;
    allocate_dynamic_mesh* AllocateDynamicMesh;
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
CreateDirectionalLight(v3f Position, c3 Color, f32 Intensity, v3f Direction)
{
    graphics_directional_light Result;
    Result.Position = Position;
    Result.Color = Color;
    Result.Intensity = Intensity;
    Result.Direction = Direction;
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

inline
m4 GetLightViewMatrix(v3f LightPosition, v3f Direction)
{    
    v3f Z = -Direction;
    v3f X;
    v3f Y;
    
    CreateBasis(Z, &X, &Y);    
    return InverseTransformM4(LightPosition, X, Y, Z);
}

inline m4 
GetLightProjectionMatrix()
{
    return OrthographicM4(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 8.0f);
}

#endif