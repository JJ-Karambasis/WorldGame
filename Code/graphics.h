#ifndef GRAPHICS_H
#define GRAPHICS_H

#define RENDER_GAME(name) b32 name(struct game* Game, v2i WindowDim)
typedef RENDER_GAME(render_game);

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