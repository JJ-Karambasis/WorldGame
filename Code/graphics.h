#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include "platform.h"

struct graphics_rect
{
    v2i Min;
    v2i Max;    
};

enum graphics_vertex_format
{
    GRAPHICS_VERTEX_FORMAT_UNKNOWN,
    GRAPHICS_VERTEX_FORMAT_P2_UV_C,
    GRAPHICS_VERTEX_FORMAT_P3,
    GRAPHICS_VERTEX_FORMAT_P3_N3,    
};

struct graphics_vertex_p2_uv_c
{
    v2f P;
    v2f UV;
    u32 C;
};

struct graphics_vertex_p3
{
    v3f P;
};

struct graphics_vertex_p3_n3
{
    v3f P;
    v3f N;
};

struct graphics_vertex_buffer
{
    graphics_vertex_format Format;    
    u32 VertexCount;
    void* Data;
};

enum graphics_index_format
{
    GRAPHICS_INDEX_FORMAT_UNKNOWN,
    GRAPHICS_INDEX_FORMAT_16_BIT,
    GRAPHICS_INDEX_FORMAT_32_BIT
};

struct graphics_index_buffer
{
    graphics_index_format Format;
    u32 IndexCount;
    void* Data;
};

struct graphics_mesh
{
    b32 IsDynamic;
    graphics_vertex_buffer VertexBuffer;
    graphics_index_buffer IndexBuffer;
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

struct graphics_texture
{
    void* Data;
    v2i Dimensions;
};

enum push_command_type
{
    PUSH_COMMAND_UNKNOWN,
    PUSH_COMMAND_CLEAR_COLOR,
    PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH,
    PUSH_COMMAND_DEPTH,
    PUSH_COMMAND_CULL,
    PUSH_COMMAND_BLEND,
    PUSH_COMMAND_SCISSOR,
    PUSH_COMMAND_PROJECTION,
    PUSH_COMMAND_CAMERA_VIEW,    
    PUSH_COMMAND_DRAW_SHADED_COLORED_MESH,
    PUSH_COMMAND_DRAW_LINE_MESH,
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

struct push_command_scissor : public push_command
{
    b32 Enable;
    i32 X, Y;
    i32 Width, Height;
};

struct push_command_4x4_matrix : public push_command
{
    m4 Matrix;
};

struct push_command_draw_shaded_colored_mesh : public push_command
{
    graphics_mesh* Mesh;
    m4 WorldTransform;
    f32 R, G, B, A;
};

struct push_command_draw_line_mesh : public push_command
{
    graphics_mesh* Mesh;
    m4 WorldTransform;
    f32 R, G, B, A;
    
    u32 IndexCount;
    u32 IndexOffset;
    u32 VertexOffset;
};

struct push_command_draw_imgui_ui : public push_command
{
    graphics_mesh* Mesh;
    graphics_texture* Texture;
    
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

#define ALLOCATE_TEXTURE(name) graphics_texture* name(graphics* Graphics, void* Data, v2i Dimensions, graphics_sampler_info* SamplerInfo)
typedef ALLOCATE_TEXTURE(allocate_texture);

#define ALLOCATE_MESH(name) graphics_mesh* name(graphics* Graphics, graphics_vertex_buffer VertexBuffer, graphics_index_buffer IndexBuffer)
typedef ALLOCATE_MESH(allocate_mesh);

#define ALLOCATE_DYNAMIC_MESH(name) graphics_mesh* name(graphics* Graphics, graphics_vertex_format VertexFormat, graphics_index_format IndexFormat)
typedef ALLOCATE_DYNAMIC_MESH(allocate_dynamic_mesh);

#define STREAM_MESH_DATA(name) void name(graphics_mesh* Mesh, void* VertexData, ptr VertexSize, void* IndexData, ptr IndexSize)
typedef STREAM_MESH_DATA(stream_mesh_data);

#define INIT_GRAPHICS(name) graphics* name(platform* Platform, void* PlatformData)
typedef INIT_GRAPHICS(init_graphics);

#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics)
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

struct graphics
{   
    arena GraphicsStorage;
    
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

inline graphics_rect 
CreateGraphicsRect(i32 MinX, i32 MinY, i32 MaxX, i32 MaxY)
{
    graphics_rect Rect = {{MinX, MinY}, {MaxX, MaxY}};
    return Rect;
}

inline graphics_rect 
CreateGraphicsRect(f32 MinX, f32 MinY, f32 MaxX, f32 MaxY)
{
    graphics_rect Rect = CreateGraphicsRect((i32)MinX, (i32)MinY, (i32)MaxX, (i32)MaxY);
    return Rect;
}

inline ptr 
GetVertexSize(graphics_vertex_format Format)
{
    ptr Result = 0;
    
    switch(Format)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C: { Result = sizeof(graphics_vertex_p2_uv_c); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3: { Result = sizeof(graphics_vertex_p3); } break;        
        case GRAPHICS_VERTEX_FORMAT_P3_N3: { Result = sizeof(graphics_vertex_p3_n3); } break;        
        INVALID_DEFAULT_CASE;
    }
    
    return Result;
}

inline ptr 
GetVertexBufferSize(graphics_vertex_buffer* VertexBuffer)
{
    ptr Result = GetVertexSize(VertexBuffer->Format)*VertexBuffer->VertexCount;
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
GetIndexSize(graphics_mesh* Mesh)
{
    ptr Result = GetIndexSize(Mesh->IndexBuffer.Format);
    return Result;
}

inline ptr
GetIndexBufferSize(graphics_index_buffer* IndexBuffer)
{    
    ptr Result = GetIndexSize(IndexBuffer->Format) * IndexBuffer->IndexCount;
    return Result;
}

#endif