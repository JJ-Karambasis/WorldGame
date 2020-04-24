#ifndef OPENGL_H
#define OPENGL_H

#include "graphics_2.h"

#include "opengl/gl.h"
#include "opengl/glext.h"
#include "opengl/glcorearb.h"

#ifdef OS_WINDOWS
#include "opengl/wglext.h"
#endif

#define LOAD_PROC(name) void* name(char* FunctionName)
typedef LOAD_PROC(load_proc);

global PFNGLCREATESHADERPROC glCreateShader;
global PFNGLSHADERSOURCEPROC glShaderSource;
global PFNGLCOMPILESHADERPROC glCompileShader;
global PFNGLGETSHADERIVPROC glGetShaderiv;
global PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
global PFNGLCREATEPROGRAMPROC glCreateProgram;
global PFNGLATTACHSHADERPROC glAttachShader;
global PFNGLLINKPROGRAMPROC glLinkProgram;
global PFNGLVALIDATEPROGRAMPROC glValidateProgram;
global PFNGLGETPROGRAMIVPROC glGetProgramiv;
global PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
global PFNGLDETACHSHADERPROC glDetachShader;

#define LOAD_FUNCTION(type, function) \
do \
{ \
function = (type)LoadProc(#function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the %.*s function", LiteralStringLength(#function), #function); \
} while(0)

#endif
