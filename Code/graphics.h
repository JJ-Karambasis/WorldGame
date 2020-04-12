#ifndef GRAPHICS_H
#define GRAPHICS_H

#define RENDER_GAME(name) b32 name(struct game* Game, v2i WindowDim)
typedef RENDER_GAME(render_game);

#define DEBUG_DRAW_POINT(name) void name(v3f Position, f32 Size, c4 Color)
typedef DEBUG_DRAW_POINT(debug_draw_point);

#define DEBUG_DRAW_LINE(name) void name(v3f Position0, v3f Position1, f32 Width, f32 Height, c4 Color)
typedef DEBUG_DRAW_LINE(debug_draw_line);

struct debug_graphics_vertex_array
{
    u32 Count;
    v3f* Ptr;
};

struct debug_graphics_mesh
{
    debug_graphics_vertex_array Vertices;
    graphics_index_array Indices;
};

struct debug_capsule_mesh
{    
    debug_graphics_mesh Cap;
    debug_graphics_mesh Body;
};

struct camera
{
    v3f Velocity;
    v3f Position;    
    v3f FocalPoint;
    m3 Orientation;
    v3f AngularVelocity;
    f32 Distance;
};

struct graphics
{
    arena Storage;        
    render_game* RenderGame;
    
    debug_draw_point* DEBUGDrawPoint;
    debug_draw_line*  DEBUGDrawLine;
};

#ifdef OS_WINDOWS
//TODO(JJ): Remove this
#include "imgui/imgui.h"
#define WIN32_GRAPHICS_INIT(name) graphics* name(HWND Window, platform* Platform, assets* Assets, ImGuiContext* Context)
typedef WIN32_GRAPHICS_INIT(win32_graphics_init);
#endif

#endif