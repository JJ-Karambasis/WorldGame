#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "AKCommon/common.h"
#include "platform.h"

#define MAX_JOINT_COUNT 256

enum graphics_vertex_format
{
    GRAPHICS_VERTEX_FORMAT_UNKNOWN,
    GRAPHICS_VERTEX_FORMAT_P2_UV_C,
    GRAPHICS_VERTEX_FORMAT_P3,
    GRAPHICS_VERTEX_FORMAT_P3_N3,    
    GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS
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

struct graphics_sampler_info
{
    graphics_filter MinFilter;
    graphics_filter MagFilter;
};

enum push_command_type
{
    PUSH_COMMAND_UNKNOWN,
    PUSH_COMMAND_CLEAR_COLOR,
    PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH,
    PUSH_COMMAND_DEPTH,
    PUSH_COMMAND_CULL,
    PUSH_COMMAND_WIREFRAME,
    PUSH_COMMAND_BLEND,
    PUSH_COMMAND_SCISSOR,    
    PUSH_COMMAND_VIEWPORT,
    PUSH_COMMAND_PROJECTION,
    PUSH_COMMAND_CAMERA_VIEW,    
    PUSH_COMMAND_DRAW_SHADED_COLORED_MESH,
    PUSH_COMMAND_DRAW_SHADED_COLORED_SKINNING_MESH,
    PUSH_COMMAND_DRAW_LINE_MESH,
    PUSH_COMMAND_DRAW_FILLED_MESH, 
    PUSH_COMMAND_DRAW_IMGUI_UI,
    PUSH_COMMAND_DRAW_QUAD,    
};

struct push_command
{
    push_command_type Type;
};

struct push_command_clear_color : public push_command
{
    f32 R, G, B, A;
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
    b32 Enable;
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

struct push_command_draw_shaded_colored_mesh : public push_command
{    
    m4 WorldTransform;    
    f32 R, G, B, A;
    i64 MeshID;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct push_command_draw_shaded_colored_skinning_mesh : public push_command
{
    m4 WorldTransform;    
    f32 R, G, B, A;
    i64 MeshID;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
    
    m4* Joints;
    u32 JointCount;
};

struct push_command_draw_line_mesh : public push_command
{    
    m4 WorldTransform;
    f32 R, G, B, A;
    i64 MeshID;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct push_command_draw_filled_mesh : public push_command
{
    m4 WorldTransform;
    f32 R, G, B, A;
    i64 MeshID;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct push_command_draw_imgui_ui : public push_command
{
    i64 MeshID;
    i64 TextureID;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct push_command_draw_quad : public push_command
{
    union
    {
        v3f P[4];
        struct { v3f P0, P1, P2, P3; };
    };
    
    f32 R, G, B, A;
};

//CONFIRM(JJ): Is this alright to be fixed sized?
#define MAX_COMMAND_COUNT 1024
struct push_command_list
{
    push_command* Ptr[MAX_COMMAND_COUNT];
    u32 Count;
};

struct graphics;

#define ALLOCATE_TEXTURE(name) i64 name(graphics* Graphics, void* Data, v2i Dimensions, graphics_sampler_info* SamplerInfo)
typedef ALLOCATE_TEXTURE(allocate_texture);

#define ALLOCATE_MESH(name) i64 name(graphics* Graphics, void* VertexData, ptr VertexDataSize, graphics_vertex_format VertexFormat, void* IndexData, ptr IndexDataSize, graphics_index_format IndexFormat)
typedef ALLOCATE_MESH(allocate_mesh);

#define ALLOCATE_DYNAMIC_MESH(name) i64 name(graphics* Graphics, graphics_vertex_format VertexFormat, graphics_index_format IndexFormat)
typedef ALLOCATE_DYNAMIC_MESH(allocate_dynamic_mesh);

#define STREAM_MESH_DATA(name) void name(graphics* Graphics, i64 MeshID, void* VertexData, ptr VertexSize, void* IndexData, ptr IndexSize)
typedef STREAM_MESH_DATA(stream_mesh_data);

#define INIT_GRAPHICS(name) graphics* name(platform* Platform, void* PlatformData)
typedef INIT_GRAPHICS(init_graphics);

#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics)
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

struct graphics
{       
    v2i RenderDim;
    push_command_list CommandList;    
    void* PlatformData;                        
    
    allocate_texture* AllocateTexture;
    allocate_mesh* AllocateMesh;
    allocate_dynamic_mesh* AllocateDynamicMesh;
    stream_mesh_data* StreamMeshData;
};

INIT_GRAPHICS(Graphics_InitGraphicsStub)
{
    return NULL;
}

EXECUTE_RENDER_COMMANDS(Graphics_ExecuteRenderCommandsStub)
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
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS: { Result = sizeof(vertex_p3_n3_weights); } break;
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

#endif