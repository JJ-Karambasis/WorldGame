#include "opengl.h"
#include "opengl_shaders.cpp"

#define SHADOW_MAP_TEXTURE_UNIT 2
#define OMNI_SHADOW_MAP_TEXTURE_UNIT 3

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
        
        INVALID_DEFAULT_CASE;
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
        
        INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline ptr
GetIndexTypeSize(GLenum IndexType)
{        
    ASSERT((IndexType == GL_UNSIGNED_INT) || (IndexType == GL_UNSIGNED_SHORT));
    ptr Result = (IndexType == GL_UNSIGNED_INT) ? sizeof(u32) : sizeof(u16);
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
        
        INVALID_DEFAULT_CASE;
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
        
        INVALID_DEFAULT_CASE;
    }
    
    return (GLenum)-1;
}

inline void
SetUniformM4(GLint Uniform, m4 Value)
{
    glUniformMatrix4fv(Uniform, 1, GL_FALSE, Value.M);
}

inline void 
SetUniform4f(GLint Uniform, v4f Value)
{
    glUniform4f(Uniform, Value.x, Value.y, Value.z, Value.w);
}

inline void 
SetUniform3f(GLint Uniform, v3f Value)
{
    glUniform3f(Uniform, Value.x, Value.y, Value.z);
}

b32 BindProgram(GLuint* BoundProgram, GLuint NewProgram)
{
    if(*BoundProgram != NewProgram)
    {
        *BoundProgram = NewProgram;
        glUseProgram(*BoundProgram);
        return true;
    }
    
    return false;
}

b32 BindVAO(GLuint* BoundVAO, GLuint NewVAO)
{    
    if(*BoundVAO != NewVAO)
    {
        *BoundVAO = NewVAO;
        glBindVertexArray(*BoundVAO);
        return true;
    }
    
    return false;
}

GLuint AllocateShadowMapArray(u32 ArrayCount)
{
    GLuint Result;
    glGenTextures(1, &Result);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Result);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, ArrayCount, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    c4 BorderColor = White4();
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (f32*)&BorderColor);            
    
    return Result;
}

GLuint AllocateUBO(ptr UBOSize, u32 UBOIndex)
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
UploadUBO(GLuint UBO, ptr UploadSize, void* Data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, UploadSize, Data);                                
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UploadSkinningMatrices(opengl_buffer_list* SkinningBuffers, u32 SkinningIndex, u32 JointCount, m4* Joints)
{    
    ASSERT(SkinningIndex <= SkinningBuffers->Count);    
    
    if(SkinningIndex == SkinningBuffers->Count)
    {                    
        ASSERT(SkinningBuffers->Count < SkinningBuffers->Capacity);
        GLuint SkinningUBO = AllocateUBO(sizeof(m4)*MAX_JOINT_COUNT, SKINNING_BUFFER_INDEX);         
        SkinningBuffers->Ptr[SkinningBuffers->Count++] = SkinningUBO;                                                                                                    
    }
    
    UploadUBO(SkinningBuffers->Ptr[SkinningIndex], sizeof(m4)*JointCount, Joints);    
}

inline GLenum
GetIndexType(graphics_index_format IndexFormat)
{    
    ASSERT(IndexFormat != GRAPHICS_INDEX_FORMAT_UNKNOWN);
    GLenum IndexType = (IndexFormat == GRAPHICS_INDEX_FORMAT_32_BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    return IndexType;
}

ALLOCATE_TEXTURE(AllocateTexture)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    graphics_texture_id ResultID = AllocateFromPool(&OpenGL->TexturePool);
    opengl_texture* Texture = GetByID(&OpenGL->TexturePool, ResultID);
    
    GLenum MinFilter = GetFilterType(SamplerInfo->MinFilter);
    GLenum MagFilter = GetFilterType(SamplerInfo->MagFilter);
    
    glGenTextures(1, &Texture->Handle);
    glBindTexture(GL_TEXTURE_2D, Texture->Handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLint InternalFormat = GetInternalFormat(TextureFormat);    
    GLint Format = GetFormat(TextureFormat);
    
    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Dimensions.width, Dimensions.height, 0, Format, GL_UNSIGNED_BYTE, Data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return ResultID;
}

ALLOCATE_MESH(AllocateMesh)
{   
    //TODO(JJ): We should allocate this data structure from a pool of opengl graphics meshes later
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    i64 ResultID = AllocateFromPool(&OpenGL->MeshPool);
    opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, ResultID);
    
    Mesh->IsDynamic = false;
    Mesh->IndexType = GetIndexType(IndexFormat);
    
    glGenVertexArrays(1, &Mesh->VAO);
    glGenBuffers(ARRAYCOUNT(Mesh->Buffers), Mesh->Buffers);    
    
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
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3, P));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3:
        {                        
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3, N));            
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);            
            
        } break; 
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV:
        {            
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv, N));
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv, UV)); 
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS:
        {                        
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, N));            
            glVertexAttribIPointer(JOINT_INDEX_ATTRIBUTE_INDEX, 1, GL_UNSIGNED_INT, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, JointI));                        
            glVertexAttribPointer(JOINT_WEIGHT_ATTRIBUTE_INDEX, 4, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, JointW));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_INDEX_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_WEIGHT_ATTRIBUTE_INDEX);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS:
        {
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv_weights, P));
            glVertexAttribPointer(NORMAL_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv_weights, N));
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv_weights, UV)); 
            glVertexAttribIPointer(JOINT_INDEX_ATTRIBUTE_INDEX, 1, GL_UNSIGNED_INT, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv_weights, JointI));                        
            glVertexAttribPointer(JOINT_WEIGHT_ATTRIBUTE_INDEX, 4, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_uv_weights, JointW));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_INDEX_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(JOINT_WEIGHT_ATTRIBUTE_INDEX);
        } break;        
        
        INVALID_DEFAULT_CASE;
    }
    
    glBindVertexArray(0);    
    
    return ResultID;
}

ALLOCATE_DYNAMIC_MESH(AllocateDynamicMesh)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    i64 ResultID = AllocateFromPool(&OpenGL->MeshPool);
    opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, ResultID);
    
    Mesh->IsDynamic = true;
    Mesh->IndexType = GetIndexType(IndexFormat);
    
    glGenVertexArrays(1, &Mesh->VAO);
    glGenBuffers(ARRAYCOUNT(Mesh->Buffers), Mesh->Buffers);    
    
    glBindVertexArray(Mesh->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
    
    switch(VertexFormat)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C:
        {
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);            
            glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX,  2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, P));
            glVertexAttribPointer(UV_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, UV));
            glVertexAttribPointer(COLOR_ATTRIBUTE_INDEX,  4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, C));
            
            glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(UV_ATTRIBUTE_INDEX);
            glEnableVertexAttribArray(COLOR_ATTRIBUTE_INDEX);
        } break;
    }
    
    glBindVertexArray(0);
    
    return ResultID;
}

STREAM_MESH_DATA(StreamMeshData)
{
    ASSERT(IsAllocatedID(MeshID));
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, MeshID);
    
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

b32 Platform_InitOpenGL(void** PlatformData)
{
    HWND Window = (HWND)PlatformData[0];
    HINSTANCE Instance = (HINSTANCE)PlatformData[1];
    char ClassName[256];
    GetClassName(Window, ClassName, sizeof(ClassName));
    
    HWND TempWindow = CreateWindow(ClassName, "Temp", WS_SYSMENU, 0, 0, 1, 1, NULL, NULL, Instance, NULL);
    BOOL_CHECK_AND_HANDLE(TempWindow, "Failed to create the temporary window.");
    
    PIXELFORMATDESCRIPTOR TempPixelFormatDescriptor = 
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1, 
        PFD_SUPPORT_OPENGL
    };
    
    HDC TempDeviceContext = GetDC(TempWindow);
    
    i32 TempPixelFormatIndex = ChoosePixelFormat(TempDeviceContext, &TempPixelFormatDescriptor);
    BOOL_CHECK_AND_HANDLE(TempPixelFormatIndex, "Failed to get the temporary pixel format index.");
    BOOL_CHECK_AND_HANDLE(SetPixelFormat(TempDeviceContext, TempPixelFormatIndex, &TempPixelFormatDescriptor),
                          "Failed to set the temporary pixel format.");
    
    HGLRC TempRenderingContext = wglCreateContext(TempDeviceContext);
    BOOL_CHECK_AND_HANDLE(TempRenderingContext, "Failed to create the temporary rendering context.");    
    BOOL_CHECK_AND_HANDLE(wglMakeCurrent(TempDeviceContext, TempRenderingContext), "Failed to set the temporary context's as the current context.");
    
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    BOOL_CHECK_AND_HANDLE(wglGetExtensionsStringARB, "wglGetExtensionsStringARB function not found");
    
    char* Extensions = (char*)wglGetExtensionsStringARB(TempDeviceContext);
    char* Iter = Extensions;
    
    b32 wglPixelFormatExtensionFound = false;
    b32 wglSRBExtensionFound = false;
    b32 wglSwapControlExtensionFound = false;
    
    while(*Iter)
    {
        char* End = ProcessToken(Iter);
        
        if(StringEquals("WGL_ARB_pixel_format", Iter, End-Iter)) wglPixelFormatExtensionFound = true;        
        if(StringEquals("WGL_EXT_swap_control", Iter, End-Iter)) wglSwapControlExtensionFound = true;
        if(StringEquals("WGL_ARB_framebuffer_sRGB", Iter, End-Iter)) wglSRBExtensionFound = true;
        if(StringEquals("WGL_EXT_framebuffer_sRGB", Iter, End-Iter)) wglSRBExtensionFound = true;        
        
        Iter = EatWhitespace(End);        
    }
    
    BOOL_CHECK_AND_HANDLE(wglPixelFormatExtensionFound, "WGL_ARB_pixel_format extension not found");
    BOOL_CHECK_AND_HANDLE(wglSwapControlExtensionFound, "WGL_EXT_swap_control extension not found");
    BOOL_CHECK_AND_HANDLE(wglSRBExtensionFound, "WGL_ARB_framebuffer_sRGB or WGL_EXT_framebuffer_sRGB extension not found");
    
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    BOOL_CHECK_AND_HANDLE(wglChoosePixelFormatARB, "wglChoosePixelFormatARB function not found");
    
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
    i32 PixelFormatIndex;
    BOOL_CHECK_AND_HANDLE(wglChoosePixelFormatARB(DeviceContext, PixelFormatAttributes, NULL, 1, &PixelFormatIndex, &NumFormat) || !NumFormat,
                          "Failed to choose an actual pixel format and retrieve its index.");
    
    PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {};
    BOOL_CHECK_AND_HANDLE(DescribePixelFormat(DeviceContext, PixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &PixelFormatDescriptor),
                          "Failed to get a description of the actual pixel format descriptor.");
    BOOL_CHECK_AND_HANDLE(SetPixelFormat(DeviceContext, PixelFormatIndex, &PixelFormatDescriptor), 
                          "Failed to set the actual pixel format descriptor.");
    
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
    BOOL_CHECK_AND_HANDLE(wglCreateContextAttribsARB, "Failed to load the wglCreateContextAttribsARB function");
    
    HGLRC RenderingContext = wglCreateContextAttribsARB(DeviceContext, 0, Attribs);
    BOOL_CHECK_AND_HANDLE(RenderingContext, "Failed to create the opengl rendering context.");
    
    BOOL_CHECK_AND_HANDLE(wglMakeCurrent(NULL, NULL), "Failed to unset the current contexts.");
    BOOL_CHECK_AND_HANDLE(wglDeleteContext(TempRenderingContext), "Failed to delete the rendering temporary context.");
    BOOL_CHECK_AND_HANDLE(wglMakeCurrent(DeviceContext, RenderingContext), "Failed to set the actual context as the current context.");
    
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    BOOL_CHECK_AND_HANDLE(wglSwapIntervalEXT, "Failed to load the wglSwapIntervalEXT function.");
    
    wglSwapIntervalEXT(1);
    
    DestroyWindow(TempWindow);
    
    return true;
    
    handle_error:
    
    return false;
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
    if((ID == 131185) || (ID == 131204) || (ID == 131218) || (ID == 131139))        
        return;
    
    CONSOLE_LOG("GL Debug Message: %s\n", Message);    
    ASSERT(false);
}

#endif

extern "C"
EXPORT BIND_GRAPHICS_FUNCTIONS(BindGraphicsFunctions)
{       
    Graphics->AllocateTexture = AllocateTexture;
    Graphics->AllocateMesh = AllocateMesh;
    Graphics->AllocateDynamicMesh = AllocateDynamicMesh;
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
    
#if DEVELOPER_BUILD
    
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)Platform_LoadProc("glDebugMessageCallback");
    PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)Platform_LoadProc("glDebugMessageControl");
    
    ASSERT(glDebugMessageControl && glDebugMessageCallback);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageCallback((GLDEBUGPROC)glDebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    
#endif        
    
    return true;
    
    handle_error:
    return false;
}

extern "C"
EXPORT INIT_GRAPHICS(InitGraphics)
{
    Global_Platform = Platform;
    InitMemory(Platform->TempArena, Platform->AllocateMemory, Platform->FreeMemory);
    
    arena GraphicsStorage = CreateArena(KILOBYTE(128));    
    opengl_context* OpenGL = PushStruct(&GraphicsStorage, opengl_context, Clear, 0);
    
    OpenGL->Storage = GraphicsStorage;
    OpenGL->MeshPool = CreatePool<opengl_mesh>(&OpenGL->Storage, 128);
    OpenGL->TexturePool = CreatePool<opengl_texture>(&OpenGL->Storage, 128);
    
    graphics* Graphics = &OpenGL->Graphics;
    Graphics->PlatformData = PlatformData;
    
    BOOL_CHECK_AND_HANDLE(Platform_InitOpenGL(PlatformData), "Failed to initialize opengl.");
    
    PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)Platform_LoadProc("glGetStringi");
    BOOL_CHECK_AND_HANDLE(glGetStringi, "Failed to load the glGetStringi function.");
    
    b32 GeometryShaderExtensionFound = false;
    for(u32 ExtensionIndex = 0; ; ExtensionIndex++)
    {
        const char* Extension = (const char*)glGetStringi(GL_EXTENSIONS, ExtensionIndex);
        if(!Extension)
            break;
        
        if(StringEquals("GL_ARB_geometry_shader4", Extension)) GeometryShaderExtensionFound = true;        
    }
    
    //CONFIRM(JJ): Do we need a geometry shader?
    //BOOL_CHECK_AND_HANDLE(GeometryShaderExtensionFound, "Failed GL_ARB_geometry_shader4 extension not found.");
    
    BindGraphicsFunctions(Graphics);
    
    glGenFramebuffers(1, &OpenGL->ShadowMapFBO);
    glGenFramebuffers(1, &OpenGL->OmniShadowMapFBO);
    OpenGL->ShadowMapTextureArray     = AllocateShadowMapArray(MAX_DIRECTIONAL_LIGHT_COUNT);
    OpenGL->OmniShadowMapTextureArray = AllocateShadowMapArray(MAX_POINT_LIGHT_COUNT*6);
    
    return Graphics;
    
    handle_error:
    return NULL;
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
               ASSERT(OpenGL->##shader.Program > 0); \
                      OpenGL->##shader.Valid = true; \
           } \
       } \
} while(0)

b32 BindShadowMapFBO(GLuint FBO, GLuint TextureArray, u32* Counter)
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindTexture(GL_TEXTURE_2D_ARRAY, TextureArray);                
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TextureArray, 0, *Counter);                
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);                
    
    b32 Result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    
    (*Counter)++;
    return Result;
}

extern "C"
EXPORT EXECUTE_RENDER_COMMANDS(ExecuteRenderCommands)
{
    SET_DEVELOPER_CONTEXT(DevContext);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    if(!OpenGL->SkinningBuffers.Ptr)
    {
        OpenGL->SkinningBuffers.Capacity = 32;
        OpenGL->SkinningBuffers.Ptr = PushArray(&OpenGL->Storage, OpenGL->SkinningBuffers.Capacity, GLuint, Clear, 0);
    }    
    
    CHECK_SHADER(ImGuiShader);
    CHECK_SHADER(ColorShader);
    CHECK_SHADER(TextureShader);        
    CHECK_SHADER(LambertianColorShader);
    CHECK_SHADER(LambertianTextureShader);
    CHECK_SHADER(PhongDConSConShader);
    CHECK_SHADER(PhongDConSTexShader);
    CHECK_SHADER(PhongDTexSConShader);
    CHECK_SHADER(PhongDTexSTexShader);
    CHECK_SHADER(ShadowMapShader);
    CHECK_SHADER(OmniShadowMapShader);
    
    m4 Projection = IdentityM4();    
    m4 ViewProjection = IdentityM4();
    v3f ViewPosition = V3();    
    
    if(!OpenGL->LightUBO)
        OpenGL->LightUBO = AllocateUBO(sizeof(opengl_light_buffer), LIGHT_BUFFER_INDEX);
    
    if(!OpenGL->LightViewProjectionUBO)
        OpenGL->LightViewProjectionUBO = AllocateUBO(sizeof(m4)*MAX_DIRECTIONAL_LIGHT_COUNT, LIGHT_VIEW_PROJECTION_INDEX);
    
    glEnable(GL_SCISSOR_TEST);        
    
    GLuint BoundProgram = (GLuint)-1;
    GLuint BoundVAO = (GLuint)-1;
    GLuint WorldTransformUniform = (GLuint)-1;
    
    u32 SkinningIndex = 0;        
    u32 DirectionalLightCounter = 0;
    
    shadow_pass ShadowPass = {};
    opengl_forward_pass ForwardPass = {};
    
    GLint ModelUniform = -1;
    
    push_command_list* CommandList = &Graphics->CommandList;            
    for(u32 CommandIndex = 0; CommandIndex < CommandList->Count; CommandIndex++)
    {
        push_command* Command = CommandList->Ptr[CommandIndex];
        switch(Command->Type)
        {   
            case PUSH_COMMAND_SHADOW_MAP:
            {                
                ForwardPass = {};                
                ShadowPass.Current = true;
                
                if(!BindShadowMapFBO(OpenGL->ShadowMapFBO, OpenGL->ShadowMapTextureArray, &ShadowPass.ShadowMapCounter))
                    INVALID_CODE;
                
                ShadowPass.State = SHADOW_PASS_STATE_DIRECTIONAL;                
            } break;
            
            case PUSH_COMMAND_OMNI_SHADOW_MAP:
            {
                glDisable(GL_FRAMEBUFFER_SRGB);
                
                ForwardPass = {};                
                ShadowPass.Current = true;
                
                push_command_omni_shadow_map* OmniShadowMap = (push_command_omni_shadow_map*)Command;                
                if(!BindShadowMapFBO(OpenGL->OmniShadowMapFBO, OpenGL->OmniShadowMapTextureArray, &ShadowPass.OmniShadowMapCounter))
                    INVALID_CODE;
                
                ShadowPass.State = SHADOW_PASS_STATE_OMNI_DIRECTIONAL;
                ShadowPass.FarPlaneDistance = OmniShadowMap->FarPlaneDistance;                
            } break;
            
            case PUSH_COMMAND_LIGHT_BUFFER:
            {        
                glEnable(GL_FRAMEBUFFER_SRGB);                    
                ShadowPass = {};
                ForwardPass.Current = true;
                
                graphics_light_buffer SrcLightBuffer = ((push_command_light_buffer*)Command)->LightBuffer;
                
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                
                opengl_light_buffer DstLightBuffer = {};
                DstLightBuffer.DirectionalLightCount = SrcLightBuffer.DirectionalLightCount;
                DstLightBuffer.PointLightCount = SrcLightBuffer.PointLightCount;
                
                m4 LightViewProjectionBuffers[MAX_DIRECTIONAL_LIGHT_COUNT] = {};
                
                for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DstLightBuffer.DirectionalLightCount; DirectionalLightIndex++)
                {
                    graphics_directional_light* SrcLight = SrcLightBuffer.DirectionalLights + DirectionalLightIndex;
                    opengl_directional_light* DstLight = DstLightBuffer.DirectionalLights + DirectionalLightIndex;
                    
                    LightViewProjectionBuffers[DirectionalLightIndex] = SrcLight->ViewProjection;
                    
                    DstLight->Direction =  V4(SrcLight->Direction, 0.0f);
                    DstLight->Color = V4(SrcLight->Color*SrcLight->Intensity, 0.0f);                    
                }
                
                for(i32 PointLightIndex = 0; PointLightIndex < DstLightBuffer.PointLightCount; PointLightIndex++)
                {
                    graphics_point_light* SrcLight = SrcLightBuffer.PointLights + PointLightIndex;
                    opengl_point_light* DstLight = DstLightBuffer.PointLights + PointLightIndex;
                    
                    DstLight->Color = V4(SrcLight->Color*SrcLight->Intensity, 0.0f);
                    DstLight->Position = V4(SrcLight->Position, SrcLight->Radius);                                                           
                }
                
                UploadUBO(OpenGL->LightViewProjectionUBO, sizeof(m4)*MAX_DIRECTIONAL_LIGHT_COUNT, LightViewProjectionBuffers);
                UploadUBO(OpenGL->LightUBO, sizeof(opengl_light_buffer), &DstLightBuffer);                
                
                glActiveTexture(GL_TEXTURE0+SHADOW_MAP_TEXTURE_UNIT);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->ShadowMapTextureArray);
                
                glActiveTexture(GL_TEXTURE0+OMNI_SHADOW_MAP_TEXTURE_UNIT);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->OmniShadowMapTextureArray);
            } break;
            
            case PUSH_COMMAND_MATERIAL:
            {
                ASSERT(ForwardPass.Current);
                push_command_material* PushCommandMaterial = (push_command_material*)Command;
                ForwardPass.BoundMaterial = PushCommandMaterial->Material;                
            } break;
            
            case PUSH_COMMAND_DRAW_MESH:
            {    
                ASSERT(ForwardPass.Current != ShadowPass.Current);
                if(ForwardPass.Current)
                {                    
                    if(ForwardPass.BoundMaterial && (ForwardPass.BoundMaterial != ForwardPass.PrevBoundMaterial))
                    {
                        ForwardPass.PrevBoundMaterial = ForwardPass.BoundMaterial;
                        
                        graphics_material* BoundMaterial = ForwardPass.BoundMaterial;
                        if(!BoundMaterial->Specular.InUse && !BoundMaterial->Normal.InUse)
                        {
                            if(BoundMaterial->Diffuse.IsTexture)
                            {   
                                lambertian_texture_shader* Shader = &OpenGL->LambertianTextureShader;
                                SET_ILLUMINATION_PROGRAM();
                                
                                SetUniformM4(Shader->ViewProjectionUniform, ViewProjection);
                                
                                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, BoundMaterial->Diffuse.DiffuseID);
                                BindTextureToUnit(Texture->Handle, Shader->DiffuseTextureUniform, 0);                                
                            }
                            else
                            {
                                lambertian_color_shader* Shader = &OpenGL->LambertianColorShader;
                                SET_ILLUMINATION_PROGRAM();
                                
                                SetUniformM4(Shader->ViewProjectionUniform, ViewProjection);       
                                SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial->Diffuse.Diffuse);                                
                            }
                        }
                        else if(BoundMaterial->Specular.InUse && !BoundMaterial->Normal.InUse)
                        {                       
                            if(BoundMaterial->Diffuse.IsTexture)
                            {   
                                if(BoundMaterial->Specular.IsTexture)
                                {
                                    phong_dtex_stex_shader* Shader = &OpenGL->PhongDTexSTexShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial->Specular.Shininess);
                                    
                                    opengl_texture* DiffuseTexture = GetByID(&OpenGL->TexturePool, BoundMaterial->Diffuse.DiffuseID);
                                    opengl_texture* SpecularTexture = GetByID(&OpenGL->TexturePool, BoundMaterial->Specular.SpecularID);
                                    
                                    BindTextureToUnit(DiffuseTexture->Handle, Shader->DiffuseTextureUniform, 0);
                                    BindTextureToUnit(SpecularTexture->Handle, Shader->SpecularTextureUniform, 1);
                                }
                                else
                                {
                                    phong_dtex_scon_shader* Shader = &OpenGL->PhongDTexSConShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial->Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial->Specular.Shininess);
                                    
                                    opengl_texture* Texture = GetByID(&OpenGL->TexturePool, BoundMaterial->Diffuse.DiffuseID);
                                    BindTextureToUnit(Texture->Handle, Shader->DiffuseTextureUniform, 0);
                                }
                            }
                            else
                            {
                                if(BoundMaterial->Specular.IsTexture)
                                {
                                    phong_dcon_stex_shader* Shader = &OpenGL->PhongDConSTexShader;
                                    SET_ILLUMINATION_PROGRAM();
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial->Diffuse.Diffuse);
                                    
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial->Specular.Shininess);                                                                                                
                                    
                                    opengl_texture* Texture = GetByID(&OpenGL->TexturePool, BoundMaterial->Specular.SpecularID);                                    
                                    BindTextureToUnit(Texture->Handle, Shader->SpecularTextureUniform, 0);                                    
                                }
                                else
                                {
                                    phong_dcon_scon_shader* Shader = &OpenGL->PhongDConSConShader;
                                    SET_ILLUMINATION_PROGRAM();                                    
                                    SET_VIEW_UNIFORMS();
                                    
                                    SetUniform3f(Shader->DiffuseColorUniform, BoundMaterial->Diffuse.Diffuse);
                                    glUniform1f(Shader->SpecularColorUniform, BoundMaterial->Specular.Specular);
                                    glUniform1i(Shader->ShininessUniform, BoundMaterial->Specular.Shininess);
                                }
                            }
                        }
                        else if(!BoundMaterial->Specular.InUse && BoundMaterial->Normal.InUse)
                        {                            
                            NOT_IMPLEMENTED;
                        }
                        else
                        {                        
                            NOT_IMPLEMENTED;
                        }
                    }
                }
                else if(ShadowPass.Current)
                {               
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
                    INVALID_ELSE;
                }
                
                push_command_draw_mesh* DrawMesh = (push_command_draw_mesh*)Command;                
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawMesh->MeshID);
                
                SetUniformM4(ModelUniform, DrawMesh->WorldTransform);
                BindVAO(&BoundVAO, Mesh->VAO);
                DrawTriangles(Mesh, &DrawMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_SKELETON_MESH:
            {
                NOT_IMPLEMENTED;
                ASSERT(ForwardPass.Current != ShadowPass.Current);
                if(ForwardPass.Current)
                {                    
                    if(ForwardPass.BoundMaterial && (ForwardPass.BoundMaterial != ForwardPass.PrevBoundMaterial))
                    {
                        ForwardPass.PrevBoundMaterial = ForwardPass.BoundMaterial;
                        
                        graphics_material* BoundMaterial = ForwardPass.BoundMaterial;
                        if(!BoundMaterial->Specular.InUse && BoundMaterial->Normal.InUse)
                        {
                            NOT_IMPLEMENTED;
                        }
                        else if(BoundMaterial->Specular.InUse && !BoundMaterial->Normal.InUse)
                        {                        
                        }
                        else if(!BoundMaterial->Specular.InUse && BoundMaterial->Normal.InUse)
                        {
                            NOT_IMPLEMENTED;
                        }
                        else
                        {                        
                            NOT_IMPLEMENTED;
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
                    INVALID_ELSE;
                }
                
            } break;
            
            case PUSH_COMMAND_DRAW_UNLIT_MESH:
            {
                ForwardPass = {};
                ShadowPass = {};
                
                push_command_draw_unlit_mesh* DrawUnlitMesh = (push_command_draw_unlit_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawUnlitMesh->MeshID);
                if(DrawUnlitMesh->DiffuseSlot.IsTexture)
                {
                    if(BindProgram(&BoundProgram, OpenGL->TextureShader.Program))
                    {
                        ModelUniform = OpenGL->TextureShader.ModelUniform;
                        SetUniformM4(OpenGL->TextureShader.ViewProjectionUniform, ViewProjection);
                    }
                    
                    opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawUnlitMesh->DiffuseSlot.DiffuseID);
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
                NOT_IMPLEMENTED;
            } break;
            
            case PUSH_COMMAND_DRAW_LINE_MESH:            
            {
                ForwardPass = {};
                ShadowPass = {};
                
                push_command_draw_line_mesh* DrawLineMesh = (push_command_draw_line_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLineMesh->MeshID);
                
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
                ForwardPass = {};     
                ShadowPass = {};
                
                push_command_draw_imgui_ui* DrawImGuiUI = (push_command_draw_imgui_ui*)Command;                                
                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawImGuiUI->TextureID);
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawImGuiUI->MeshID);
                ASSERT(Mesh->IsDynamic);
                
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
            } break;
            
            case PUSH_COMMAND_VIEW_PROJECTION:
            {
                ViewProjection = ((push_command_4x4_matrix*)Command)->Matrix;
            } break;
            
            case PUSH_COMMAND_VIEW_POSITION:
            {
                ViewPosition = ((push_command_view_position*)Command)->Position;
            } break;
            
            INVALID_DEFAULT_CASE;                        
        }
    }    
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    CommandList->Count = 0;        
    
    Platform_SwapBuffers(Graphics->PlatformData[0]);    
}

extern "C"
EXPORT INVALIDATE_SHADERS(InvalidateShaders)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
#define INVALIDATE_SHADER(shader) OpenGL->shader.Valid = false
    
    INVALIDATE_SHADER(ImGuiShader);
    INVALIDATE_SHADER(ColorShader);        
    INVALIDATE_SHADER(LambertianColorShader);
    INVALIDATE_SHADER(LambertianTextureShader);                    
    INVALIDATE_SHADER(PhongDConSConShader);
    INVALIDATE_SHADER(PhongDConSTexShader);
    INVALIDATE_SHADER(PhongDTexSConShader);
    INVALIDATE_SHADER(PhongDTexSTexShader);
    INVALIDATE_SHADER(ShadowMapShader);
    INVALIDATE_SHADER(OmniShadowMapShader);    
}