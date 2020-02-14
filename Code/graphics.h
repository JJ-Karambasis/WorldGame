#ifndef GRAPHICS_H
#define GRAPHICS_H

struct graphics
{
    arena Storage;    
};

#ifdef OS_WINDOWS
#define WIN32_GRAPHICS_INIT(name) graphics* name(HWND Window, platform* Platform)
typedef WIN32_GRAPHICS_INIT(win32_graphics_init);
#endif

#endif