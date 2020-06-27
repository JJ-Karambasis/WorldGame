#ifndef OPENGL_H
#define OPENGL_H

#include "graphics.h"
#include "dev_world_game.h"

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
global PFNGLDELETESHADERPROC glDeleteShader;
global PFNGLDELETEPROGRAMPROC glDeleteProgram;
global PFNGLUNIFORM1IPROC glUniform1i;
global PFNGLACTIVETEXTUREPROC glActiveTexture;
global PFNGLUNIFORM3FPROC glUniform3f;
global PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
global PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
global PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
global PFNGLTEXIMAGE3DPROC glTexImage3D;
global PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
global PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
global PFNGLUNIFORM1FPROC glUniform1f;
global PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
global PFNGLDRAWBUFFERSPROC glDrawBuffers;

#include "opengl_shaders.h"

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

struct opengl_directional_light
{
    v4f Direction;
    c4 Color;
};

struct opengl_point_light
{
    v4f Position;
    c4 Color;
};

struct opengl_light_buffer
{
    opengl_directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    opengl_point_light PointLights[MAX_POINT_LIGHT_COUNT];
    i32 DirectionalLightCount;
    i32 PointLightCount;
};

struct shadow_map
{    
    GLuint FBO;
    GLuint DepthMap;
};

struct opengl_context
{
    graphics Graphics;
    
    arena Storage;    
    opengl_mesh_pool MeshPool;
    opengl_texture_pool TexturePool;        
    
    imgui_shader ImGuiShader;    
    color_shader ColorShader;
    texture_shader TextureShader;
    color_skinning_shader ColorSkinningShader;        
    texture_skinning_shader TextureSkinningShader;
    lambertian_color_shader LambertianColorShader;
    lambertian_texture_shader LambertianTextureShader;
    lambertian_color_skinning_shader LambertianColorSkinningShader;
    lambertian_texture_skinning_shader LambertianTextureSkinningShader;
    phong_color_shader PhongColorShader;
    phong_texture_shader PhongTextureShader;
    phong_color_skinning_shader PhongColorSkinningShader;
    phong_texture_skinning_shader PhongTextureSkinningShader;
    shadow_map_shader ShadowMapShader;
    omni_shadow_map_shader OmniShadowMapShader;
    
    GLuint ShadowMapFBO;
    GLuint ShadowMapTextureArray;
    
    GLuint OmniShadowMapFBO;
    GLuint OmniShadowMapTextureArray;
    
    GLuint LightUBO;    
    GLuint LightViewProjectionUBO;
    
    opengl_buffer_list SkinningBuffers;    
};

#define LOAD_FUNCTION(type, function) \
do \
{ \
function = (type)Platform_LoadProc(#function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the %.*s function", LiteralStringLength(#function), #function); \
} while(0)

#endif
