#include "../include/opengl.h"
#include "opengl_shaders.cpp"

#define SHADOW_MAP_TEXTURE_UNIT 3
#define OMNI_SHADOW_MAP_TEXTURE_UNIT 4

#define SET_PROGRAM() \
ModelUniform = Shader->ModelUniform; \
BindProgram(&BoundProgram, Shader->Program)

#define SET_ILLUMINATION_PROGRAM() \
SET_PROGRAM(); \
glUniform1i(Shader->ShadowMapUniform, SHADOW_MAP_TEXTURE_UNIT); \
glUniform1i(Shader->OmniShadowMapUniform, OMNI_SHADOW_MAP_TEXTURE_UNIT)

#define SET_VIEW_UNIFORMS() \
SetUniformM4(Shader->ViewProjectionUniform, ViewProjection); \
SetUniform3f(Shader->ViewPositionUniform, ViewPosition)

inline opengl_forward_pass InitForwardPass()
{
    opengl_forward_pass ForwardPass = {};    
    ForwardPass.PrevBoundMaterial = InvalidGraphicsMaterial();
    ForwardPass.BoundMaterial = InvalidGraphicsMaterial();
    return ForwardPass;
}

inline void
BindTextureToUnit(GLuint Texture, GLint Uniform, GLuint Unit)
{
    glActiveTexture(GL_TEXTURE0+Unit);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glUniform1i(Uniform, Unit);
}

inline GLenum GetInternalFormat(graphics_texture_format Format)
{
    switch(Format)
    {
        case GRAPHICS_TEXTURE_FORMAT_R8:
        {
            return GL_R8;
        } break;
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8:
        {
            return GL_RGB8;
        } 
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB:
        {
            return GL_SRGB8;
        }
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8:
        {
            return GL_RGBA8;
        }
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8:
        {
            return GL_SRGB8_ALPHA8;
        }        
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline GLenum GetFormat(graphics_texture_format Format)
{
    switch(Format)
    {
        case GRAPHICS_TEXTURE_FORMAT_R8:
        {
            return GL_RED;
        } break;
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8:
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB:
        {
            return GL_RGB;
        } 
        
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8:
        case GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8:
        {
            return GL_RGBA;
        }
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline ak_uaddr
GetIndexTypeSize(GLenum IndexType)
{        
    AK_Assert((IndexType == GL_UNSIGNED_INT) || (IndexType == GL_UNSIGNED_SHORT), "Invalid index type");
    ak_uaddr Result = (IndexType == GL_UNSIGNED_INT) ? sizeof(ak_u32) : sizeof(ak_u16);
    return Result;
}

inline void 
DrawTriangles(opengl_mesh* Mesh, graphics_draw_info* DrawInfo)
{    
    glDrawElementsBaseVertex(GL_TRIANGLES, DrawInfo->IndexCount, Mesh->IndexType, 
                             (void*)(DrawInfo->IndexOffset*GetIndexTypeSize(Mesh->IndexType)), 
                             DrawInfo->VertexOffset);
}

inline void 
DrawLines(opengl_mesh* Mesh, graphics_draw_info* DrawInfo)
{    
    glDrawElementsBaseVertex(GL_LINES, DrawInfo->IndexCount, Mesh->IndexType, 
                             (void*)(DrawInfo->IndexOffset*GetIndexTypeSize(Mesh->IndexType)), 
                             DrawInfo->VertexOffset);
}

inline GLenum
GetFilterType(graphics_filter Filter)
{
    switch(Filter)
    {
        case GRAPHICS_FILTER_LINEAR:
        return GL_LINEAR;                
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline GLenum
GetBlendFactor(graphics_blend Blend)
{    
    switch(Blend)
    {
        case GRAPHICS_BLEND_SRC_ALPHA:
        return GL_SRC_ALPHA;
        
        case GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline void
SetUniformM4(GLint Uniform, ak_m4f Value)
{
    glUniformMatrix4fv(Uniform, 1, GL_FALSE, Value.Data);
}

inline void 
SetUniform4f(GLint Uniform, ak_v4f Value)
{
    glUniform4f(Uniform, Value.x, Value.y, Value.z, Value.w);
}

inline void 
SetUniform3f(GLint Uniform, ak_v3f Value)
{
    glUniform3f(Uniform, Value.x, Value.y, Value.z);
}

ak_bool BindProgram(GLuint* BoundProgram, GLuint NewProgram)
{
    if(*BoundProgram != NewProgram)
    {
        *BoundProgram = NewProgram;
        glUseProgram(*BoundProgram);
        return true;
    }
    
    return false;
}

ak_bool BindVAO(GLuint* BoundVAO, GLuint NewVAO)
{    
    if(*BoundVAO != NewVAO)
    {
        *BoundVAO = NewVAO;
        glBindVertexArray(*BoundVAO);
        return true;
    }
    
    return false;
}

GLuint AllocateShadowMapArray(ak_u32 ArrayCount)
{
    GLuint Result;
    glGenTextures(1, &Result);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Result);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, ArrayCount, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    ak_color4f BorderColor = AK_White4();
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (ak_f32*)&BorderColor);            
    
    return Result;
}

GLuint AllocateUBO(ak_uaddr UBOSize, ak_u32 UBOIndex)
{
    GLuint Result;
    glGenBuffers(1, &Result);
    glBindBuffer(GL_UNIFORM_BUFFER, Result);
    glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);    
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOIndex, Result); 
    return Result;
}

inline void 
UploadUBO(GLuint UBO, ak_uaddr UploadSize, void* Data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, UploadSize, Data);                                
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UploadSkinningMatrices(opengl_buffer_list* SkinningBuffers, ak_u32 SkinningIndex, ak_u32 JointCount, ak_m4f* Joints)
{    
    AK_Assert(SkinningIndex <= SkinningBuffers->Count, "Index out of bounds");    
    
    if(SkinningIndex == SkinningBuffers->Count)
    {                    
        AK_Assert(SkinningBuffers->Count < SkinningBuffers->Capacity, "Allocating to many skinning buffers");
        GLuint SkinningUBO = AllocateUBO(sizeof(ak_m4f)*MAX_JOINT_COUNT, SKINNING_BUFFER_INDEX);         
        SkinningBuffers->Ptr[SkinningBuffers->Count++] = SkinningUBO;                                                                                                    
    }
    
    UploadUBO(SkinningBuffers->Ptr[SkinningIndex], sizeof(ak_m4f)*JointCount, Joints);    
}

inline GLenum
GetIndexType(graphics_index_format IndexFormat)
{    
    AK_Assert(IndexFormat != GRAPHICS_INDEX_FORMAT_UNKNOWN, "Invalid index format");
    GLenum IndexType = (IndexFormat == GRAPHICS_INDEX_FORMAT_32_BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    return IndexType;
}

ALLOCATE_TEXTURE(AllocateTexture)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    graphics_texture_id ResultID = OpenGL->TexturePool.Allocate();
    opengl_texture* Texture = OpenGL->TexturePool.Get(ResultID);
    
    GLenum MinFilter = GetFilterType(SamplerInfo->MinFilter);
    GLenum MagFilter = GetFilterType(SamplerInfo->MagFilter);
    
    glGenTextures(1, &Texture->Handle);
    glBindTexture(GL_TEXTURE_2D, Texture->Handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLint InternalFormat = GetInternalFormat(TextureFormat);    
    GLint Format = GetFormat(TextureFormat);
    
    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return ResultID;
}

ALLOCATE_MESH(AllocateMesh)
{   
    //TODO(JJ): We should allocate this data structure from a pool of opengl graphics meshes later
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    ak_u64 ResultID = OpenGL->MeshPool.Allocate();
    opengl_mesh* Mesh = OpenGL->MeshPool.Get(ResultID);
    
    Mesh->IsDynamic = false;
    Mesh->IndexType = GetIndexType(IndexFormat);
    
    glGenVertexArrays(1, &Mesh->VAO);
    glGenBuffers(AK_Count(Mesh->Buffers), Mesh->Buffers);    
    
    glBindVertexArray(Mesh->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, VertexDataSize, VertexData, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexDataSize, IndexData, GL_STATIC_DRAW);
    
    GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);
    switch(VertexFormat)
    {
        case GRAPHICS_VERTEX_FORMAT_P3:
        {                                    
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3, P));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3:
        {                        
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3, N));            
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);            
            
        } break; 
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS:
        {                        
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_w, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_w, N));            
            glVertexAttribIPointer(JOINT_INDEX_ATTRIBUTE_INDEX, 1, GL_UNSIGNED_INT, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_w, JointI));                        
            glVertexAttribPointer(JOINT_WEIGHT_ATTRIBUTE_INDEX, 4, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_w, JointW));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_INDEX_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_WEIGHT_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV:
        {
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv, N));            
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv, UV));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);            
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS:
        {
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv_w, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv_w, N));            
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv_w, UV));
            glVertexAttribIPointer(JOINT_INDEX_ATTRIBUTE_INDEX, 1, GL_UNSIGNED_INT, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv_w, JointI));                        
            glVertexAttribPointer(JOINT_WEIGHT_ATTRIBUTE_INDEX, 4, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p3_n3_uv_w, JointW));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);            
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_INDEX_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_WEIGHT_ATTRIBUTE_INDEX);
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    glBindVertexArray(0);    
    
    return ResultID;
}

ALLOCATE_DYNAMIC_MESH(AllocateDynamicMesh)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    ak_u64 ResultID = OpenGL->MeshPool.Allocate();
    opengl_mesh* Mesh = OpenGL->MeshPool.Get(ResultID);
    
    Mesh->IsDynamic = true;
    Mesh->IndexType = GetIndexType(IndexFormat);
    
    glGenVertexArrays(1, &Mesh->VAO);
    glGenBuffers(AK_Count(Mesh->Buffers), Mesh->Buffers);    
    
    glBindVertexArray(Mesh->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
    
    switch(VertexFormat)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C:
        {
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);            
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX,  2, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p2_uv_c, P));
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)AK_FieldOffset(ak_vertex_p2_uv_c, UV));
            glVertexAttribPointer(COLOR_ATTRIBUTE_INDEX,  4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (void*)AK_FieldOffset(ak_vertex_p2_uv_c, C));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(COLOR_ATTRIBUTE_INDEX);
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    glBindVertexArray(0);
    
    return ResultID;
}

ALLOCATE_RENDER_BUFFER(AllocateRenderBuffer)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;    
    opengl_render_buffer* Result = OpenGL->Storage->Push<opengl_render_buffer>();
    
    glGenFramebuffers(1, &Result->Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, Result->Framebuffer);
    
    glGenTextures(1, &Result->ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, Result->ColorAttachment);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, Resolution.w, Resolution.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result->ColorAttachment, 0);
    
    glGenRenderbuffers(1, &Result->DepthStencilAttachment);
    glBindRenderbuffer(GL_RENDERBUFFER, Result->DepthStencilAttachment);    
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Resolution.w, Resolution.h);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Result->DepthStencilAttachment);    
    
    Result->Resolution = Resolution;
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)    
        Result = NULL;            
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return Result;
}

FREE_RENDER_BUFFER(FreeRenderBuffer)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    opengl_render_buffer* OpenGLRenderBuffer = (opengl_render_buffer*)RenderBuffer;
    
    glDeleteRenderbuffers(1, &OpenGLRenderBuffer->DepthStencilAttachment);
    glDeleteTextures(1, &OpenGLRenderBuffer->ColorAttachment);
    glDeleteFramebuffers(1, &OpenGLRenderBuffer->Framebuffer);
}

STREAM_MESH_DATA(StreamMeshData)
{    
    opengl_context* OpenGL = (opengl_context*)Graphics;        
    opengl_mesh* Mesh = OpenGL->MeshPool.Get(MeshID);
    
    glBindVertexArray(Mesh->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
    
    glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexSize, IndexData, GL_STREAM_DRAW);    
    
    glBindVertexArray(0);    
}

#ifdef OS_WINDOWS

void* Platform_LoadProc(char* FunctionName)
{
    void* Function = (void*)wglGetProcAddress(FunctionName);
    if(Function == 0 ||
       (Function == (void*)0x1) || (Function == (void*)0x2) || (Function == (void*)0x3) ||
       (Function == (void*)-1) )
    {
        HMODULE OpenGL = LoadLibraryA("opengl32.dll");
        Function = (void *)GetProcAddress(OpenGL, FunctionName);
    }
    
    return Function;
}

ak_bool Platform_InitOpenGL(void** PlatformData)
{
    HWND Window = (HWND)PlatformData[0];
    HINSTANCE Instance = (HINSTANCE)PlatformData[1];
    ak_char ClassName[256];
    GetClassName(Window, ClassName, sizeof(ClassName));
    
    HWND TempWindow = CreateWindow(ClassName, "Temp", WS_SYSMENU, 0, 0, 1, 1, NULL, NULL, Instance, NULL);
    if(!TempWindow)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }    
    
    PIXELFORMATDESCRIPTOR TempPixelFormatDescriptor = 
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1, 
        PFD_SUPPORT_OPENGL
    };
    
    HDC TempDeviceContext = GetDC(TempWindow);
    
    ak_i32 TempPixelFormatIndex = ChoosePixelFormat(TempDeviceContext, &TempPixelFormatDescriptor);
    if(!TempPixelFormatIndex)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!SetPixelFormat(TempDeviceContext, TempPixelFormatIndex, &TempPixelFormatDescriptor))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
        
    }
    
    HGLRC TempRenderingContext = wglCreateContext(TempDeviceContext);
    if(!TempRenderingContext)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;        
    }
    
    if(!wglMakeCurrent(TempDeviceContext, TempRenderingContext))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;        
    }
    
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if(!wglGetExtensionsStringARB)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "wglGetExtensionsStringARB function not found");
        return false;
    }    
    
    ak_char* Extensions = (ak_char*)wglGetExtensionsStringARB(TempDeviceContext);
    ak_char* Iter = Extensions;
    
    ak_bool wglPixelFormatExtensionFound = false;
    ak_bool wglSRBExtensionFound = false;
    ak_bool wglSwapControlExtensionFound = false;
    
    while(*Iter)
    {
        ak_string Token = AK_ReadToken(Iter);        
        
        if(AK_StringEquals("WGL_ARB_pixel_format", Token)) wglPixelFormatExtensionFound = true;        
        if(AK_StringEquals("WGL_EXT_swap_control", Token)) wglSwapControlExtensionFound = true;
        if(AK_StringEquals("WGL_ARB_framebuffer_sRGB", Token)) wglSRBExtensionFound = true;
        if(AK_StringEquals("WGL_EXT_framebuffer_sRGB", Token)) wglSRBExtensionFound = true;        
        
        Iter += Token.Length;        
    }
    
    if(!wglPixelFormatExtensionFound)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!wglSwapControlExtensionFound)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!wglSRBExtensionFound)
    {
        //TODO(JJ): Diagnostic and error loggin
        return false;
    }    
    
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    if(!wglChoosePixelFormatARB)
    {
        //TOOD(JJ): Diagnostic and error logging
        AK_Assert(false, "wglChoosePixelFormatARG function not found");
        return false;
    }    
    
    int PixelFormatAttributes[] = 
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, 
        WGL_COLOR_BITS_ARB, 32, 
        WGL_DEPTH_BITS_ARB, 24, 
        WGL_STENCIL_BITS_ARB, 8,
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
        0
    };
    
    HDC DeviceContext = GetDC(Window);
    
    UINT NumFormat;
    ak_i32 PixelFormatIndex;
    if(!wglChoosePixelFormatARB(DeviceContext, PixelFormatAttributes, NULL, 1, &PixelFormatIndex, &NumFormat) || !NumFormat)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {};
    if(!DescribePixelFormat(DeviceContext, PixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &PixelFormatDescriptor))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!SetPixelFormat(DeviceContext, PixelFormatIndex, &PixelFormatDescriptor))
    {
        //TODO(JJ): Diangostic and error logging
        return false;        
    }    
    
    int AttribFlags = 0;
#if DEVELOPER_BUILD
    AttribFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
    
    int Attribs[] = 
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, AttribFlags,       
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if(!wglCreateContextAttribsARB)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    HGLRC RenderingContext = wglCreateContextAttribsARB(DeviceContext, 0, Attribs);
    if(!RenderingContext)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }    
    
    if(!wglMakeCurrent(NULL, NULL))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!wglDeleteContext(TempRenderingContext))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!wglMakeCurrent(DeviceContext, RenderingContext))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(!wglSwapIntervalEXT)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_Assert(false, "Failed to load the wglSwapIntervalEXT function");
        return false;
    }    
    
    wglSwapIntervalEXT(1);    
    DestroyWindow(TempWindow);
    
    return true;    
}

void Platform_SwapBuffers(void* PlatformData)
{
    HWND Window = (HWND)PlatformData;
    HDC DeviceContext = GetDC(Window);
    SwapBuffers(DeviceContext);
}

#endif

#if DEVELOPER_BUILD

void glDebugCallback(GLenum Source, GLenum Type, GLuint ID, GLenum Severity, GLsizei Length, const GLchar* Message, void* UserData)
{           
    if((ID == 131185) || (ID == 131204) || (ID == 131218) || (ID == 131139) || (ID == 131169) || (ID == 8))        
        return;
    
    AK_Assert(false, "GL Debug Message(%u): %s\n", ID, Message);
}

#endif

extern "C"
AK_EXPORT BIND_GRAPHICS_FUNCTIONS(BindGraphicsFunctions)
{       
    Graphics->AllocateTexture = AllocateTexture;
    Graphics->AllocateMesh = AllocateMesh;
    Graphics->AllocateDynamicMesh = AllocateDynamicMesh;
    Graphics->AllocateRenderBuffer = AllocateRenderBuffer;
    Graphics->FreeRenderBuffer = FreeRenderBuffer;
    Graphics->StreamMeshData = StreamMeshData;        
    
    LOAD_FUNCTION(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_FUNCTION(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_FUNCTION(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_FUNCTION(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_FUNCTION(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_FUNCTION(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_FUNCTION(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_FUNCTION(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_FUNCTION(PFNGLVALIDATEPROGRAMPROC, glValidateProgram);
    LOAD_FUNCTION(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    LOAD_FUNCTION(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);        
    LOAD_FUNCTION(PFNGLDETACHSHADERPROC, glDetachShader);
    LOAD_FUNCTION(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    LOAD_FUNCTION(PFNGLGENBUFFERSPROC, glGenBuffers);
    LOAD_FUNCTION(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    LOAD_FUNCTION(PFNGLBINDBUFFERPROC, glBindBuffer);
    LOAD_FUNCTION(PFNGLBUFFERDATAPROC, glBufferData);
    LOAD_FUNCTION(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    LOAD_FUNCTION(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    LOAD_FUNCTION(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_FUNCTION(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_FUNCTION(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    LOAD_FUNCTION(PFNGLUNIFORM4FPROC, glUniform4f);
    LOAD_FUNCTION(PFNGLDRAWELEMENTSBASEVERTEXPROC, glDrawElementsBaseVertex);    
    LOAD_FUNCTION(PFNGLUNIFORM3FVPROC, glUniform3fv);    
    LOAD_FUNCTION(PFNGLGETUNIFORMBLOCKINDEXPROC, glGetUniformBlockIndex);
    LOAD_FUNCTION(PFNGLUNIFORMBLOCKBINDINGPROC, glUniformBlockBinding);
    LOAD_FUNCTION(PFNGLBINDBUFFERBASEPROC, glBindBufferBase);
    LOAD_FUNCTION(PFNGLBUFFERSUBDATAPROC, glBufferSubData);
    LOAD_FUNCTION(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer);
    LOAD_FUNCTION(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_FUNCTION(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_FUNCTION(PFNGLUNIFORM1IPROC, glUniform1i);
    LOAD_FUNCTION(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    LOAD_FUNCTION(PFNGLUNIFORM3FPROC, glUniform3f);
    LOAD_FUNCTION(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
    LOAD_FUNCTION(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
    LOAD_FUNCTION(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
    LOAD_FUNCTION(PFNGLTEXIMAGE3DPROC, glTexImage3D);
    LOAD_FUNCTION(PFNGLFRAMEBUFFERTEXTURE3DPROC, glFramebufferTexture3D);
    LOAD_FUNCTION(PFNGLFRAMEBUFFERTEXTURELAYERPROC, glFramebufferTextureLayer);
    LOAD_FUNCTION(PFNGLUNIFORM1FPROC, glUniform1f);
    LOAD_FUNCTION(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
    LOAD_FUNCTION(PFNGLDRAWBUFFERSPROC, glDrawBuffers);
    LOAD_FUNCTION(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
    LOAD_FUNCTION(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
    LOAD_FUNCTION(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
    LOAD_FUNCTION(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
    LOAD_FUNCTION(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer);
    LOAD_FUNCTION(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);        
    LOAD_FUNCTION(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers);
    
#if DEVELOPER_BUILD
    
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)Platform_LoadProc("glDebugMessageCallback");
    PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)Platform_LoadProc("glDebugMessageControl");
    
    AK_Assert(glDebugMessageControl && glDebugMessageCallback, "Failed to load glDebugMessageControl or glDebugMessageCallback");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageCallback((GLDEBUGPROC)glDebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    
#endif        
    
    return true;    
}

extern "C"
AK_EXPORT INIT_GRAPHICS(InitGraphics)
{       
    ak_arena* GraphicsStorage = AK_CreateArena(AK_Kilobyte(128));    
    opengl_context* OpenGL = GraphicsStorage->Push<opengl_context>();    
    
    OpenGL->Storage = GraphicsStorage;
    OpenGL->MeshPool = AK_CreatePool<opengl_mesh>();
    OpenGL->TexturePool = AK_CreatePool<opengl_texture>();
    
    graphics* Graphics = &OpenGL->Graphics;
    Graphics->PlatformData = PlatformData;
    
    if(!Platform_InitOpenGL(PlatformData))
    {
        //TODO(JJ): Diangostic and error logging
        return NULL;
    }    
    
    PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)Platform_LoadProc("glGetStringi");
    if(!glGetStringi)
    {
        //TODO(JJ: Diagnostic and error logging
        AK_Assert(false, "Failed to load the glGetStringi function");
        return false;
    }
    
    ak_bool GeometryShaderExtensionFound = false;
    for(ak_u32 ExtensionIndex = 0; ; ExtensionIndex++)
    {
        const char* Extension = (const ak_char*)glGetStringi(GL_EXTENSIONS, ExtensionIndex);
        if(!Extension)
            break;
        
        if(AK_StringEquals("GL_ARB_geometry_shader4", Extension)) GeometryShaderExtensionFound = true;        
    }
    
    //CONFIRM(JJ): Do we need a geometry shader?   
    
    if(!BindGraphicsFunctions(Graphics))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    glGenFramebuffers(1, &OpenGL->ShadowMapFBO);
    glGenFramebuffers(1, &OpenGL->OmniShadowMapFBO);
    OpenGL->ShadowMapTextureArray     = AllocateShadowMapArray(MAX_DIRECTIONAL_LIGHT_COUNT);
    OpenGL->OmniShadowMapTextureArray = AllocateShadowMapArray(MAX_POINT_LIGHT_COUNT*6);
    
    return Graphics;    
}

#define CHECK_SHADER(shader) \
do \
{ \
if(!OpenGL->##shader.Valid) \
{ \
auto Shader = Create##shader(); \
if(Shader.Valid) \
{ \
glDeleteProgram(OpenGL->##shader.Program); \
OpenGL->##shader = Shader; \
} \
else \
{ \
AK_Assert(OpenGL->##shader.Program > 0, "Failed to create the shader %s", #shader); \
OpenGL->##shader.Valid = true; \
} \
} \
} while(0)

ak_bool BindShadowMapFBO(GLuint FBO, GLuint TextureArray, ak_u32* Counter)
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindTexture(GL_TEXTURE_2D_ARRAY, TextureArray);                
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TextureArray, 0, *Counter);                
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);                
    
    ak_bool Result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    
    (*Counter)++;
    return Result;
}

extern "C"
AK_EXPORT EXECUTE_RENDER_COMMANDS(ExecuteRenderCommands)
{            
    opengl_context* OpenGL = (opengl_context*)Graphics;            
    AK_SetGlobalArena(OpenGL->TempStorage);
    
    if(!OpenGL->SkinningBuffers.Ptr)
    {
        OpenGL->SkinningBuffers.Capacity = 32;
        OpenGL->SkinningBuffers.Ptr = OpenGL->Storage->PushArray<GLuint>(OpenGL->SkinningBuffers.Capacity);
    }    
    
    CHECK_SHADER(ImGuiShader);
    CHECK_SHADER(ColorShader);
    CHECK_SHADER(TextureShader);        
    CHECK_SHADER(LambertianColorShader);
    CHECK_SHADER(LambertianTextureShader);
    CHECK_SHADER(LambertianColorNormalMapShader);
    CHECK_SHADER(LambertianTextureNormalMapShader);
    CHECK_SHADER(PhongDConSConShader);
    CHECK_SHADER(PhongDConSConNormalMapShader);
    CHECK_SHADER(PhongDConSTexShader);
    CHECK_SHADER(PhongDConSTexNormalMapShader);
    CHECK_SHADER(PhongDTexSConShader);
    CHECK_SHADER(PhongDTexSConNormalMapShader);
    CHECK_SHADER(PhongDTexSTexShader);
    CHECK_SHADER(PhongDTexSTexNormalMapShader);
    CHECK_SHADER(ShadowMapShader);
    CHECK_SHADER(OmniShadowMapShader);
    
    ak_m4f Projection = AK_IdentityM4<ak_f32>();    
    ak_m4f ViewProjection = AK_IdentityM4<ak_f32>();
    ak_v3f ViewPosition = AK_V3<ak_f32>();    
    
    if(!OpenGL->LightUBO)
        OpenGL->LightUBO = AllocateUBO(sizeof(opengl_light_buffer), LIGHT_BUFFER_INDEX);
    
    if(!OpenGL->LightViewProjectionUBO)
        OpenGL->LightViewProjectionUBO = AllocateUBO(sizeof(ak_m4f)*MAX_DIRECTIONAL_LIGHT_COUNT, LIGHT_VIEW_PROJECTION_INDEX);
    
    glEnable(GL_SCISSOR_TEST);        
    
    GLuint BoundProgram = (GLuint)-1;
    GLuint BoundVAO = (GLuint)-1;
    GLuint WorldTransformUniform = (GLuint)-1;
    
    opengl_texture_pool* TexturePool = &OpenGL->TexturePool;
    opengl_mesh_pool* MeshPool = &OpenGL->MeshPool;
    
    ak_u32 SkinningIndex = 0;            
    
    shadow_pass ShadowPass = {};
    opengl_forward_pass ForwardPass = InitForwardPass();        
    
    GLint ModelUniform = -1;
    
    AK_ForEach(CommandAt, &Graphics->CommandList)
    {        
        push_command* Command = *CommandAt;
        switch(Command->Type)
        {   
            case PUSH_COMMAND_SHADOW_MAP:
            {                
                ForwardPass = InitForwardPass();                                         
                ShadowPass.LastState = SHADOW_PASS_STATE_NONE;                
                ShadowPass.Current = true;                
                
                ak_bool Result = BindShadowMapFBO(OpenGL->ShadowMapFBO, OpenGL->ShadowMapTextureArray, &ShadowPass.ShadowMapCounter);
                AK_Assert(Result, "Shadow map failed to bind");
                
                ShadowPass.State = SHADOW_PASS_STATE_DIRECTIONAL;                
            } break;
            
            case PUSH_COMMAND_OMNI_SHADOW_MAP:
            {                
                ForwardPass = InitForwardPass();      
                ShadowPass.LastState = SHADOW_PASS_STATE_NONE;                
                ShadowPass.Current = true;                     
                
                push_command_omni_shadow_map* OmniShadowMap = (push_command_omni_shadow_map*)Command;                
                ak_bool Result = BindShadowMapFBO(OpenGL->OmniShadowMapFBO, OpenGL->OmniShadowMapTextureArray, &ShadowPass.OmniShadowMapCounter);
                AK_Assert(Result, "Shadow map failed to bind");
                
                ShadowPass.State = SHADOW_PASS_STATE_OMNI_DIRECTIONAL;
                ShadowPass.FarPlaneDistance = OmniShadowMap->FarPlaneDistance;                
            } break;
            
            case PUSH_COMMAND_SRGB_RENDER_BUFFER_WRITES:
            {
                (((push_command_srgb_render_buffer_writes*)Command)->Enable) ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
            } break;
            
            case PUSH_COMMAND_RENDER_BUFFER:
            {
                opengl_render_buffer* RenderBuffer = (opengl_render_buffer*)((push_command_render_buffer*)Command)->RenderBuffer;                                
                glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer->Framebuffer);                
            } break;
            
            case PUSH_COMMAND_LIGHT_BUFFER:
            {                                        
                ShadowPass = {};                
                ForwardPass.Current = true;
                
                push_command_light_buffer* CommandLightBuffer = (push_command_light_buffer*)Command;
                
                graphics_light_buffer SrcLightBuffer = CommandLightBuffer->LightBuffer;
                
                opengl_light_buffer DstLightBuffer = {};
                DstLightBuffer.DirectionalLightCount = SrcLightBuffer.DirectionalLightCount;
                DstLightBuffer.PointLightCount = SrcLightBuffer.PointLightCount;
                
                ak_m4f LightViewProjectionBuffers[MAX_DIRECTIONAL_LIGHT_COUNT] = {};
                
                for(ak_i32 DirectionalLightIndex = 0; DirectionalLightIndex < DstLightBuffer.DirectionalLightCount; DirectionalLightIndex++)
                {
                    graphics_directional_light* SrcLight = SrcLightBuffer.DirectionalLights + DirectionalLightIndex;
                    opengl_directional_light* DstLight = DstLightBuffer.DirectionalLights + DirectionalLightIndex;
                    
                    LightViewProjectionBuffers[DirectionalLightIndex] = SrcLight->ViewProjection;
                    
                    DstLight->Direction =  AK_V4(SrcLight->Direction, 0.0f);
                    DstLight->Color = AK_V4(SrcLight->Color*SrcLight->Intensity, 0.0f);                    
                }
                
                for(ak_i32 PointLightIndex = 0; PointLightIndex < DstLightBuffer.PointLightCount; PointLightIndex++)
                {
                    graphics_point_light* SrcLight = SrcLightBuffer.PointLights + PointLightIndex;
                    opengl_point_light* DstLight = DstLightBuffer.PointLights + PointLightIndex;
                    
                    DstLight->Color = AK_V4(SrcLight->Color*SrcLight->Intensity, 0.0f);
                    DstLight->Position = AK_V4(SrcLight->Position, SrcLight->Radius);                                                           
                }
                
                UploadUBO(OpenGL->LightViewProjectionUBO, sizeof(ak_m4f)*MAX_DIRECTIONAL_LIGHT_COUNT, LightViewProjectionBuffers);
                UploadUBO(OpenGL->LightUBO, sizeof(opengl_light_buffer), &DstLightBuffer);                
                
                glActiveTexture(GL_TEXTURE0+SHADOW_MAP_TEXTURE_UNIT);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->ShadowMapTextureArray);
                
                glActiveTexture(GL_TEXTURE0+OMNI_SHADOW_MAP_TEXTURE_UNIT);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->OmniShadowMapTextureArray);
            } break;
            
            case PUSH_COMMAND_MATERIAL:
            {
                AK_Assert(ForwardPass.Current, "Forward pass is not set. This is a programming error");
                push_command_material* PushCommandMaterial = (push_command_material*)Command;
                ForwardPass.BoundMaterial = PushCommandMaterial->Material;                
            } break;
            
            case PUSH_COMMAND_DRAW_MESH:
            {    
                AK_Assert(ForwardPass.Current != ShadowPass.Current, "Cannot set both shaodw pass and forward pass at the same time");
                if(ForwardPass.Current)
                {                       
                    if(ShouldUpdateMaterial(ForwardPass.BoundMaterial, ForwardPass.PrevBoundMaterial))
                    {
                        ForwardPass.PrevBoundMaterial = ForwardPass.BoundMaterial;
                        
                        graphics_material BoundMaterial = ForwardPass.BoundMaterial;
                        if(!BoundMaterial.Specular.InUse && !BoundMaterial.Normal.InUse)
                        {
                            if(BoundMaterial.Diffuse.IsTexture)
                            {   
                                lambertian_texture_shader* Shader = &OpenGL->LambertianTextureShader;
                                SET_ILLUMINATION_PROGRAM();
                                SET_VIEW_UNIFORMS();
                                
                                SetUniformM4(Shader->ViewProjectionUniform, ViewProjection);
                                
                                opengl_texture* Texture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                BindTextureToUnit(Texture->Handle, Shader->DiffuseTextureUniform, 0);                                
                            }
                            else
                            {
                                lambertian_color_shader* Shader = &OpenGL->LambertianColorShader;
                                SET_ILLUMINATION_PROGRAM();
                                SET_VIEW_UNIFORMS();
                                
                                SetUniformM4(Shader->ViewProjectionUniform, ViewProjection);       
                                SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);                                
                            }
                        }
                        else if(BoundMaterial.Specular.InUse && !BoundMaterial.Normal.InUse)
                        {                       
                            if(BoundMaterial.Diffuse.IsTexture)
                            {   
                                if(BoundMaterial.Specular.IsTexture)
                                {
                                    phong_dtex_stex_shader* Shader = &OpenGL->PhongDTexSTexShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* DiffuseTexture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                    opengl_texture* SpecularTexture = TexturePool->Get(BoundMaterial.Specular.SpecularID);
                                    
                                    BindTextureToUnit(DiffuseTexture->Handle, Shader->DiffuseTextureUniform, 0);
                                    BindTextureToUnit(SpecularTexture->Handle, Shader->SpecularTextureUniform, 1);
                                }
                                else
                                {
                                    phong_dtex_scon_shader* Shader = &OpenGL->PhongDTexSConShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial.Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* Texture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                    BindTextureToUnit(Texture->Handle, Shader->DiffuseTextureUniform, 0);
                                }
                            }
                            else
                            {
                                if(BoundMaterial.Specular.IsTexture)
                                {
                                    phong_dcon_stex_shader* Shader = &OpenGL->PhongDConSTexShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);
                                    
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);                                                                                                
                                    
                                    opengl_texture* Texture = TexturePool->Get(BoundMaterial.Specular.SpecularID);
                                    BindTextureToUnit(Texture->Handle, Shader->SpecularTextureUniform, 0);                                    
                                }
                                else
                                {
                                    phong_dcon_scon_shader* Shader = &OpenGL->PhongDConSConShader;
                                    SET_ILLUMINATION_PROGRAM();                                    
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial.Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                }
                            }
                        }
                        else if(!BoundMaterial.Specular.InUse && BoundMaterial.Normal.InUse)
                        {   
                            if(BoundMaterial.Diffuse.IsTexture)
                            {
                                lambertian_texture_normal_map_shader* Shader = &OpenGL->LambertianTextureNormalMapShader;
                                SET_ILLUMINATION_PROGRAM();
                                SET_VIEW_UNIFORMS();
                                
                                opengl_texture* DiffuseTexture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                
                                BindTextureToUnit(DiffuseTexture->Handle, Shader->DiffuseTextureUniform, 0);
                                BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 1);
                            }
                            else
                            {
                                lambertian_color_normal_map_shader* Shader = &OpenGL->LambertianColorNormalMapShader;
                                SET_ILLUMINATION_PROGRAM();
                                SET_VIEW_UNIFORMS();                                
                                SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);
                                
                                opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 0);                                
                            }                                                        
                        }
                        else
                        {   
                            if(BoundMaterial.Diffuse.IsTexture)
                            {
                                if(BoundMaterial.Specular.IsTexture)
                                {                   
                                    phong_dtex_stex_normal_map_shader* Shader = &OpenGL->PhongDTexSTexNormalMapShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* DiffuseTexture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                    opengl_texture* SpecularTexture = TexturePool->Get(BoundMaterial.Specular.SpecularID);
                                    opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                    
                                    BindTextureToUnit(DiffuseTexture->Handle, Shader->DiffuseTextureUniform, 0);
                                    BindTextureToUnit(SpecularTexture->Handle, Shader->SpecularTextureUniform, 1);
                                    BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 2);
                                }
                                else
                                {
                                    phong_dtex_scon_normal_map_shader* Shader = &OpenGL->PhongDTexSConNormalMapShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial.Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* DiffuseTexture = TexturePool->Get(BoundMaterial.Diffuse.DiffuseID);
                                    opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                    
                                    BindTextureToUnit(DiffuseTexture->Handle, Shader->DiffuseTextureUniform, 0);
                                    BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 1);                                    
                                }
                            }
                            else
                            {
                                if(BoundMaterial.Specular.IsTexture)
                                {
                                    phong_dcon_stex_normal_map_shader* Shader = &OpenGL->PhongDConSTexNormalMapShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* SpecularTexture = TexturePool->Get(BoundMaterial.Specular.SpecularID);
                                    opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                    
                                    BindTextureToUnit(SpecularTexture->Handle, Shader->SpecularTextureUniform, 0);
                                    BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 1);                                    
                                }
                                else
                                {
                                    phong_dcon_scon_normal_map_shader* Shader = &OpenGL->PhongDConSConNormalMapShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial.Diffuse.Diffuse);
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial.Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial.Specular.Shininess);
                                    
                                    opengl_texture* NormalMapTexture = TexturePool->Get(BoundMaterial.Normal.NormalID);
                                    BindTextureToUnit(NormalMapTexture->Handle, Shader->NormalMapUniform, 0);
                                }
                            }                                                        
                        }
                    }
                }
                else if(ShadowPass.Current)
                {                    
                    if(ShadowPass.LastState != ShadowPass.State)
                    {            
                        ShadowPass.LastState = ShadowPass.State;
                        if(ShadowPass.State == SHADOW_PASS_STATE_DIRECTIONAL)
                        {
                            shadow_map_shader* Shader = &OpenGL->ShadowMapShader;
                            SET_PROGRAM();                        
                            SetUniformM4(Shader->ViewProjectionUniform, ViewProjection);                                                
                        }
                        else if(ShadowPass.State == SHADOW_PASS_STATE_OMNI_DIRECTIONAL)
                        {
                            omni_shadow_map_shader* Shader = &OpenGL->OmniShadowMapShader;
                            SET_PROGRAM();
                            SET_VIEW_UNIFORMS();                        
                            glUniform1f(Shader->FarPlaneDistanceUniform, ShadowPass.FarPlaneDistance);
                        }
                        AK_INVALID_ELSE;
                    }
                }
                
                push_command_draw_mesh* DrawMesh = (push_command_draw_mesh*)Command;                
                opengl_mesh* Mesh = MeshPool->Get(DrawMesh->MeshID);
                
                SetUniformM4(ModelUniform, DrawMesh->WorldTransform);
                BindVAO(&BoundVAO, Mesh->VAO);
                DrawTriangles(Mesh, &DrawMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_SKELETON_MESH:
            {
                AK_NotImplemented();
                AK_Assert(ForwardPass.Current != ShadowPass.Current, "Cannot set both forward pass and shadow pass at the same time");
                if(ForwardPass.Current)
                {                    
                    if(ShouldUpdateMaterial(ForwardPass.BoundMaterial, ForwardPass.PrevBoundMaterial))
                    {
                        ForwardPass.PrevBoundMaterial = ForwardPass.BoundMaterial;
                        
                        graphics_material BoundMaterial = ForwardPass.BoundMaterial;
                        if(!BoundMaterial.Specular.InUse && BoundMaterial.Normal.InUse)
                        {
                            AK_NotImplemented();
                        }
                        else if(BoundMaterial.Specular.InUse && !BoundMaterial.Normal.InUse)
                        {                        
                        }
                        else if(!BoundMaterial.Specular.InUse && BoundMaterial.Normal.InUse)
                        {
                            AK_NotImplemented();
                        }
                        else
                        {                        
                            AK_NotImplemented();
                        }
                    }
                }
                else if(ShadowPass.Current)
                {               
                    if(ShadowPass.State == SHADOW_PASS_STATE_DIRECTIONAL)
                    {
                    }
                    else if(ShadowPass.State == SHADOW_PASS_STATE_OMNI_DIRECTIONAL)
                    {                        
                    }
                    AK_INVALID_ELSE;
                }
                
            } break;
            
            case PUSH_COMMAND_DRAW_UNLIT_MESH:
            {
                ForwardPass = InitForwardPass();
                ShadowPass = {};
                
                push_command_draw_unlit_mesh* DrawUnlitMesh = (push_command_draw_unlit_mesh*)Command;
                opengl_mesh* Mesh = MeshPool->Get(DrawUnlitMesh->MeshID);
                if(DrawUnlitMesh->DiffuseSlot.IsTexture)
                {
                    if(BindProgram(&BoundProgram, OpenGL->TextureShader.Program))
                    {
                        ModelUniform = OpenGL->TextureShader.ModelUniform;
                        SetUniformM4(OpenGL->TextureShader.ViewProjectionUniform, ViewProjection);
                    }
                    
                    opengl_texture* Texture = TexturePool->Get(DrawUnlitMesh->DiffuseSlot.DiffuseID);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                }
                else
                {
                    if(BindProgram(&BoundProgram, OpenGL->ColorShader.Program))
                    {
                        ModelUniform = OpenGL->ColorShader.ModelUniform;
                        SetUniformM4(OpenGL->ColorShader.ViewProjectionUniform, ViewProjection);
                    }
                    
                    SetUniform3f(OpenGL->ColorShader.ColorUniform, DrawUnlitMesh->DiffuseSlot.Diffuse);
                }
                
                SetUniformM4(ModelUniform, DrawUnlitMesh->WorldTransform);
                BindVAO(&BoundVAO, Mesh->VAO);                
                DrawTriangles(Mesh, &DrawUnlitMesh->DrawInfo);                                
            } break;
            
            case PUSH_COMMAND_DRAW_UNLIT_SKELETON_MESH:
            {
                AK_NotImplemented();
            } break;
            
            case PUSH_COMMAND_DRAW_LINE_MESH:            
            {
                ForwardPass = InitForwardPass();
                ShadowPass = {};
                
                push_command_draw_line_mesh* DrawLineMesh = (push_command_draw_line_mesh*)Command;
                opengl_mesh* Mesh = MeshPool->Get(DrawLineMesh->MeshID);
                
                if(BindProgram(&BoundProgram, OpenGL->ColorShader.Program))
                {
                    ModelUniform = OpenGL->ColorShader.ModelUniform;
                    SetUniformM4(OpenGL->ColorShader.ViewProjectionUniform, ViewProjection);                    
                }
                
                SetUniform3f(OpenGL->ColorShader.ColorUniform, DrawLineMesh->Color);
                SetUniformM4(ModelUniform, DrawLineMesh->WorldTransform);
                BindVAO(&BoundVAO, Mesh->VAO);
                DrawLines(Mesh, &DrawLineMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_IMGUI_UI:
            {                
                ForwardPass = InitForwardPass();     
                ShadowPass = {};
                
                push_command_draw_imgui_ui* DrawImGuiUI = (push_command_draw_imgui_ui*)Command;                                
                opengl_texture* Texture = TexturePool->Get(DrawImGuiUI->TextureID);
                opengl_mesh* Mesh = MeshPool->Get(DrawImGuiUI->MeshID);
                AK_Assert(Mesh->IsDynamic, "Mesh must be dynamic in order for it to be drawn in IMGUI");
                
                if(BindProgram(&BoundProgram, OpenGL->ImGuiShader.Program))                    
                    SetUniformM4(OpenGL->ImGuiShader.ProjectionUniform, Projection);                                                                    
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);                                
                
                DrawTriangles(Mesh, &DrawImGuiUI->DrawInfo);   
            } break;
            
            case PUSH_COMMAND_CLEAR_COLOR:
            {
                push_command_clear_color* ClearColorCommand = (push_command_clear_color*)Command;
                
                glClear(GL_COLOR_BUFFER_BIT);
                glClearColor(ClearColorCommand->R, ClearColorCommand->G, ClearColorCommand->B, ClearColorCommand->A);                                
            } break;
            
            case PUSH_COMMAND_CLEAR_DEPTH:
            {
                push_command_clear_depth* ClearDepthCommand = (push_command_clear_depth*)Command;
                
                glClearDepth(ClearDepthCommand->Depth);
                glClear(GL_DEPTH_BUFFER_BIT);                
            } break;
            
            case PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH:
            {
                push_command_clear_color_and_depth* ClearColorAndDepthCommand = (push_command_clear_color_and_depth*)Command;                
                glClearColor(ClearColorAndDepthCommand->R, ClearColorAndDepthCommand->G, ClearColorAndDepthCommand->B, ClearColorAndDepthCommand->A);
                glClearDepth(ClearColorAndDepthCommand->Depth);                
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);                
            } break;
            
            case PUSH_COMMAND_DEPTH:
            {
                push_command_depth* CommandDepth = (push_command_depth*)Command;
                if(CommandDepth->Enable)
                {                
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                }
                else
                    glDisable(GL_DEPTH_TEST);
            } break;
            
            case PUSH_COMMAND_CULL:
            {
                push_command_cull* CommandCull = (push_command_cull*)Command;
                if(CommandCull->CullMode == GRAPHICS_CULL_MODE_NONE)
                    glDisable(GL_CULL_FACE);
                else
                {
                    glEnable(GL_CULL_FACE);
                    if(CommandCull->CullMode == GRAPHICS_CULL_MODE_FRONT)                        
                        glCullFace(GL_FRONT);
                    else
                        glCullFace(GL_BACK);
                }                
            } break;
            
            case PUSH_COMMAND_WIREFRAME:
            {
                push_command_wireframe* CommandWireframe = (push_command_wireframe*)Command;
                if(CommandWireframe->Enable)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            } break;
            
            case PUSH_COMMAND_BLEND:
            {
                push_command_blend* CommandBlend = (push_command_blend*)Command;
                if(CommandBlend->Enable)
                {
                    glEnable(GL_BLEND);
                    
                    GLenum SrcFactor = GetBlendFactor(CommandBlend->SrcGraphicsBlend);
                    GLenum DstFactor = GetBlendFactor(CommandBlend->DstGraphicsBlend);
                    
                    glBlendFunc(SrcFactor, DstFactor);                                        
                }
                else
                    glDisable(GL_BLEND);
                
            } break;
            
            case PUSH_COMMAND_SCISSOR:
            {
                push_command_rect* CommandRect = (push_command_rect*)Command;
                glScissor(CommandRect->X, CommandRect->Y, CommandRect->Width, CommandRect->Height);                
            } break;
            
            case PUSH_COMMAND_VIEWPORT:
            {
                push_command_rect* CommandRect = (push_command_rect*)Command;
                glViewport(CommandRect->X, CommandRect->Y, CommandRect->Width, CommandRect->Height);
            } break;
            
            case PUSH_COMMAND_PROJECTION:
            {
                Projection = ((push_command_4x4_matrix*)Command)->Matrix;                                
                BoundProgram = (GLuint)-1;
            } break;
            
            case PUSH_COMMAND_VIEW_PROJECTION:
            {
                ViewProjection = ((push_command_4x4_matrix*)Command)->Matrix;
                BoundProgram = (GLuint)-1;
            } break;
            
            case PUSH_COMMAND_VIEW_POSITION:
            {
                ViewPosition = ((push_command_view_position*)Command)->Position;
                BoundProgram = (GLuint)-1;
            } break;
            
            case PUSH_COMMAND_COPY_TO_OUTPUT:
            {
                push_command_copy_to_output* CopyToOutput = (push_command_copy_to_output*)Command;                
                opengl_render_buffer* SrcBuffer = (opengl_render_buffer*)CopyToOutput->RenderBuffer;
                
                glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcBuffer->Framebuffer);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                
                glBlitFramebuffer(0, 0, SrcBuffer->Resolution.w, SrcBuffer->Resolution.h, 
                                  CopyToOutput->DstOffset.x, CopyToOutput->DstOffset.y, 
                                  CopyToOutput->DstOffset.x+CopyToOutput->DstResolution.w, CopyToOutput->DstOffset.y+CopyToOutput->DstResolution.h, 
                                  GL_COLOR_BUFFER_BIT, GL_LINEAR);
                
            } break;
            
            case PUSH_COMMAND_COPY_TO_RENDER_BUFFER:
            {
                push_command_copy_to_render_buffer* CopyToRenderBuffer = (push_command_copy_to_render_buffer*)Command;
                opengl_render_buffer* SrcBuffer = (opengl_render_buffer*)CopyToRenderBuffer->SrcRenderBuffer;                
                
                glBindFramebuffer(GL_READ_FRAMEBUFFER, SrcBuffer->Framebuffer);
                
                ak_v2i DstOffset = CopyToRenderBuffer->DstOffset;
                ak_v2i DstResolution = CopyToRenderBuffer->DstResolution;
                
                glBlitFramebuffer(0, 0, SrcBuffer->Resolution.w, SrcBuffer->Resolution.h, 
                                  DstOffset.x, DstOffset.y, DstOffset.x+DstResolution.w, DstOffset.y+DstResolution.h, 
                                  GL_COLOR_BUFFER_BIT, GL_LINEAR);                
            } break;
            
            AK_INVALID_DEFAULT_CASE;                        
        }
    }    
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    Graphics->CommandList.Clear();
    
    Platform_SwapBuffers(Graphics->PlatformData[0]);    
}

extern "C"
AK_EXPORT INVALIDATE_SHADERS(InvalidateShaders)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
#define INVALIDATE_SHADER(shader) OpenGL->shader.Valid = false
    
    INVALIDATE_SHADER(ImGuiShader);
    INVALIDATE_SHADER(ColorShader);        
    INVALIDATE_SHADER(LambertianColorShader);
    INVALIDATE_SHADER(LambertianTextureShader);                    
    INVALIDATE_SHADER(LambertianColorNormalMapShader);
    INVALIDATE_SHADER(LambertianTextureNormalMapShader);
    INVALIDATE_SHADER(PhongDConSConShader);
    INVALIDATE_SHADER(PhongDConSConNormalMapShader);
    INVALIDATE_SHADER(PhongDConSTexShader);
    INVALIDATE_SHADER(PhongDConSTexNormalMapShader);
    INVALIDATE_SHADER(PhongDTexSConShader);
    INVALIDATE_SHADER(PhongDTexSConNormalMapShader);
    INVALIDATE_SHADER(PhongDTexSTexShader);
    INVALIDATE_SHADER(PhongDTexSTexNormalMapShader);
    INVALIDATE_SHADER(ShadowMapShader);
    INVALIDATE_SHADER(OmniShadowMapShader);    
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>