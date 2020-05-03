#ifndef OPENGL_H
#define OPENGL_H

#include "graphics.h"

#include "opengl/gl.h"
#include "opengl/glext.h"
#include "opengl/glcorearb.h"

#ifdef OS_WINDOWS
#include "opengl/wglext.h"
#endif

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
global PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
global PFNGLGENBUFFERSPROC glGenBuffers;
global PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
global PFNGLBINDBUFFERPROC glBindBuffer;
global PFNGLBUFFERDATAPROC glBufferData;
global PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
global PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
global PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
global PFNGLUSEPROGRAMPROC glUseProgram;
global PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
global PFNGLUNIFORM4FPROC glUniform4f;
global PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;
global PFNGLUNIFORM3FVPROC glUniform3fv;
global PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
global PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
global PFNGLBINDBUFFERBASEPROC glBindBufferBase;
global PFNGLBUFFERSUBDATAPROC glBufferSubData;
global PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;

struct standard_color_shader
{
    GLuint Program;
    GLint ProjectionLocation;
    GLint ViewLocation;
    GLint ModelLocation;
    GLint ColorLocation;
};

struct standard_color_skinning_shader
{
    GLuint Program;
    GLint ProjectionLocation;
    GLint ViewLocation;
    GLint ModelLocation;
    GLint ColorLocation;    
    GLint SkinningIndex;
};

struct imgui_shader
{
    GLuint Program;
    GLint ProjectionLocation;
};

struct quad_shader
{
    GLuint Program;
    GLint ProjectionLocation;
    GLint ViewLocation;
    GLint ColorLocation;
    GLint PositionLocation;
};

//TODO(JJ): When we are formalizing the asset/resource loading process, it would probably
//be best to manage the memory of meshes in one VBO|EBO|VAO per vertex format and then use a draw call 
//that can specify a vertex and index offset into the buffer. For now we will just have each mesh
//hold one of each
struct opengl_mesh
{
    b32 IsDynamic;
    
    GLenum IndexType;
    union
    {
        GLuint Buffers[2];
        struct { GLuint VBO, EBO; };
    };
    GLuint VAO;
};

struct opengl_texture
{
    GLuint Handle;
};

typedef pool<opengl_mesh> opengl_mesh_pool; 
typedef pool<opengl_texture> opengl_texture_pool;

struct opengl_buffer_list
{    
    u32 Capacity;
    u32 Count;
    GLuint* Ptr;
};

struct opengl_context
{
    graphics Graphics;
    
    arena Storage;    
    opengl_mesh_pool MeshPool;
    opengl_texture_pool TexturePool;        
    
    standard_color_shader StandardPhongShader;        
    standard_color_shader StandardLineShader;
    imgui_shader ImGuiShader;
    quad_shader QuadShader;        
    standard_color_skinning_shader StandardSkinningPhongShader;
    
    opengl_buffer_list SkinningBuffers;    
};

#define LOAD_FUNCTION(type, function) \
do \
{ \
function = (type)Platform_LoadProc(#function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the %.*s function", LiteralStringLength(#function), #function); \
} while(0)

#endif
