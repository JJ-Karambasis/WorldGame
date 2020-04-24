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

enum command_type
{
    COMMAND_SUBMIT_PROJECTION,
    COMMAND_SUBMIT_CAMERA_VIEW,
    COMMAND_DRAW_SHADED_COLOR_ENTITY
};

struct command
{
    command_type Type;
};

struct command_4x4_matrix : public command
{
    m4 Matrix;
};

struct command_draw_shaded_entity : public command
{
    
};

//CONFIRM(JJ): Is this alright to be fixed sized?
#define MAX_COMMAND_COUNT 1024
struct command_list
{
    command* Ptr[MAX_COMMAND_COUNT];
    u32 Count;
};

struct graphics
{    
    command_list CommandList;
    
    void* PlatformData;
    
    b32 Initialized;
};

#define GRAPHICS_GET_RENDER_COMMANDS(name) void name(graphics* Graphics)
#define EXECUTE_RENDER_COMMANDS(name) void name(graphics* Graphics, platform* Platform)

typedef GRAPHICS_GET_RENDER_COMMANDS(graphics_get_render_commands);
typedef EXECUTE_RENDER_COMMANDS(execute_render_commands);

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