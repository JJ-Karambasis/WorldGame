#ifndef GRAPHICS_H
#define GRAPHICS_H

#define RENDER_GAME(name) b32 name(struct game* Game, v2i WindowDim)
typedef RENDER_GAME(render_game);

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
};

#ifdef OS_WINDOWS
#define WIN32_GRAPHICS_INIT(name) graphics* name(HWND Window, platform* Platform)
typedef WIN32_GRAPHICS_INIT(win32_graphics_init);
#endif

#endif