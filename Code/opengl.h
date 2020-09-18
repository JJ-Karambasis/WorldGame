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
global PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
global PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
global PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
global PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
global PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
global PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
global PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;

#include "opengl_shaders.h"

//TODO(JJ): When we are formalizing the asset/resource loading process, it would probably
//be best to manage the memory of meshes in one VBO|EBO|VAO per vertex format and then use a draw call 
//that can specify a vertex and index offset into the buffer. For now we will just have each mesh
//hold one of each
struct opengl_mesh
{
    ak_bool IsDynamic;
    
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

typedef ak_pool<opengl_mesh> opengl_mesh_pool; 
typedef ak_pool<opengl_texture> opengl_texture_pool;

struct opengl_buffer_list
{    
    ak_u32 Capacity;
    ak_u32 Count;
    GLuint* Ptr;
};

struct opengl_directional_light
{
    ak_v4f Direction;
    ak_color4f Color;
};

struct opengl_point_light
{
    ak_v4f Position;
    ak_color4f Color;
};

struct opengl_light_buffer
{
    opengl_directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    opengl_point_light PointLights[MAX_POINT_LIGHT_COUNT];
    ak_i32 DirectionalLightCount;
    ak_i32 PointLightCount;
};

enum shadow_pass_state
{
    SHADOW_PASS_STATE_NONE,
    SHADOW_PASS_STATE_DIRECTIONAL,
    SHADOW_PASS_STATE_OMNI_DIRECTIONAL
};

struct shadow_pass
{
    ak_bool Current;    
    ak_u32 ShadowMapCounter;
    ak_u32 OmniShadowMapCounter;
    ak_f32 FarPlaneDistance;    
    shadow_pass_state LastState;
    shadow_pass_state State;
};

struct opengl_forward_pass
{
    ak_bool Current;
    graphics_material PrevBoundMaterial;
    graphics_material BoundMaterial;        
};

struct opengl_render_buffer : public graphics_render_buffer
{
    GLuint Framebuffer;
    GLuint ColorAttachment;
    GLuint DepthStencilAttachment;
};

struct opengl_context
{
    graphics Graphics;
    
    ak_arena* TempStorage;    
    ak_arena* Storage;        
    opengl_mesh_pool MeshPool;
    opengl_texture_pool TexturePool;        
    
    imgui_shader                         ImGuiShader;
    color_shader                         ColorShader;
    texture_shader                       TextureShader;
    lambertian_color_shader              LambertianColorShader;
    lambertian_texture_shader            LambertianTextureShader;    
    lambertian_color_normal_map_shader   LambertianColorNormalMapShader;
    lambertian_texture_normal_map_shader LambertianTextureNormalMapShader;
    phong_dcon_scon_shader               PhongDConSConShader;
    phong_dcon_scon_normal_map_shader    PhongDConSConNormalMapShader;
    phong_dcon_stex_shader               PhongDConSTexShader;
    phong_dcon_stex_normal_map_shader    PhongDConSTexNormalMapShader;
    phong_dtex_scon_shader               PhongDTexSConShader;
    phong_dtex_scon_normal_map_shader    PhongDTexSConNormalMapShader;
    phong_dtex_stex_shader               PhongDTexSTexShader;
    phong_dtex_stex_normal_map_shader    PhongDTexSTexNormalMapShader;
    shadow_map_shader                    ShadowMapShader;
    omni_shadow_map_shader               OmniShadowMapShader;
    
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
if(!function) \
{ \
AK_Assert(false, "Failed to load the %.*s function", AK_StringLength(#function), #function); \
return false; \
} \
} while(0)
                                                                                                         
#endif
