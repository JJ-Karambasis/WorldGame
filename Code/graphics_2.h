#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include "platform.h"

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
    graphics_vertex_buffer VertexBuffer;
    graphics_index_buffer IndexBuffer;
};

enum push_command_type
{
    PUSH_COMMAND_UNKNOWN,
    PUSH_COMMAND_CLEAR,
    PUSH_COMMAND_PROJECTION,
    PUSH_COMMAND_CAMERA_VIEW,
    PUSH_COMMAND_DRAW_SHADED_COLORED_MESH
};

struct push_command
{
    push_command_type Type;
};

struct push_command_clear : public push_command
{
    f32 R, G, B, A;
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

//CONFIRM(JJ): Is this alright to be fixed sized?
#define MAX_COMMAND_COUNT 1024
struct push_command_list
{
    push_command* Ptr[MAX_COMMAND_COUNT];
    u32 Count;
};

#define ALLOCATE_MESH(name) graphics_mesh* name(struct graphics* Graphics, graphics_vertex_buffer VertexBuffer, graphics_index_buffer IndexBuffer)
typedef ALLOCATE_MESH(allocate_mesh);

#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics, platform* Platform)
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

struct graphics
{   
    arena GraphicsStorage;
    
    v2i RenderDim;
    push_command_list CommandList;    
    void* PlatformData;    
    b32 Initialized;        
    
    allocate_mesh* AllocateMesh;
};

ALLOCATE_MESH(Graphics_AllocateMeshStub)
{
    return NULL;
}

EXECUTE_RENDER_COMMANDS(Graphics_ExecuteRenderCommandsStub)
{
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
GetIndexBufferSize(graphics_index_buffer* IndexBuffer)
{    
    ptr Result = GetIndexSize(IndexBuffer->Format) * IndexBuffer->IndexCount;
    return Result;
}

#endif