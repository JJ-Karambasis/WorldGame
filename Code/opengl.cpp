#include "opengl.h"
#include "opengl_shaders.cpp"

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

inline b32
SetProgramAndMVPUniforms(GLuint* BoundProgram, GLuint Program, mvp_uniforms* Uniforms, 
                         m4* Model, m4* ViewProjection)
{   
    b32 Result = BindProgram(BoundProgram, Program);
    if(Result)    
        glUniformMatrix4fv(Uniforms->ViewProjection, 1, GL_FALSE, ViewProjection->M);            
    
    glUniformMatrix4fv(Uniforms->Model, 1, GL_FALSE, Model->M);    
    return Result;
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
    
    i64 ResultID = AllocateFromPool(&OpenGL->TexturePool);
    opengl_texture* Texture = GetByID(&OpenGL->TexturePool, ResultID);
    
    GLenum MinFilter = GetFilterType(SamplerInfo->MinFilter);
    GLenum MagFilter = GetFilterType(SamplerInfo->MagFilter);
    
    glGenTextures(1, &Texture->Handle);
    glBindTexture(GL_TEXTURE_2D, Texture->Handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
    
    GLint InternalFormat = sRGB ? GL_SRGB_ALPHA : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Dimensions.width, Dimensions.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
    
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
        char* End = Iter;
        while(!IsWhitespace(*End)) End++;
        
        if(StringEquals("WGL_ARB_pixel_format", Iter, End-Iter)) wglPixelFormatExtensionFound = true;        
        if(StringEquals("WGL_EXT_swap_control", Iter, End-Iter)) wglSwapControlExtensionFound = true;
        if(StringEquals("WGL_ARB_framebuffer_sRGB", Iter, End-Iter)) wglSRBExtensionFound = true;
        if(StringEquals("WGL_EXT_framebuffer_sRGB", Iter, End-Iter)) wglSRBExtensionFound = true;        
        
        while(IsWhitespace(*End)) End++;
        Iter = End;
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
    
    BindGraphicsFunctions(Graphics);
    
    glGenTextures(1, &OpenGL->ShadowMapTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->ShadowMapTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, MAX_DIRECTIONAL_LIGHT_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    c4 BorderColor = White4();
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (f32*)&BorderColor);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    glGenFramebuffers(1, &OpenGL->ShadowMapFBO);    
    
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
    CHECK_SHADER(ColorSkinningShader);
    CHECK_SHADER(TextureSkinningShader);
    CHECK_SHADER(LambertianColorShader);
    CHECK_SHADER(LambertianTextureShader);
    CHECK_SHADER(LambertianColorSkinningShader);
    CHECK_SHADER(LambertianTextureSkinningShader);
    CHECK_SHADER(PhongColorShader);
    CHECK_SHADER(PhongTextureShader);
    CHECK_SHADER(PhongColorSkinningShader);
    CHECK_SHADER(PhongTextureSkinningShader);
    CHECK_SHADER(ShadowMapShader);
    
    m4 Projection = IdentityM4();
    m4 View = IdentityM4();
    m4 ViewProjection = IdentityM4();
    v3f ViewPosition = V3();    
    
    if(!OpenGL->LightUBO)
        OpenGL->LightUBO = AllocateUBO(sizeof(opengl_light_buffer), LIGHT_BUFFER_INDEX);
    
    if(!OpenGL->LightViewProjectionUBO)
        OpenGL->LightViewProjectionUBO = AllocateUBO(sizeof(m4)*MAX_DIRECTIONAL_LIGHT_COUNT, LIGHT_VIEW_PROJECTION_INDEX);
    
    glEnable(GL_SCISSOR_TEST);    
    glEnable(GL_FRAMEBUFFER_SRGB);                    
    
    GLuint BoundProgram = (GLuint)-1;
    GLuint BoundVAO = (GLuint)-1;
    
    u32 SkinningIndex = 0;        
    u32 DirectionalLightCounter = 0;
    
    push_command_list* CommandList = &Graphics->CommandList;        
    for(u32 CommandIndex = 0; CommandIndex < CommandList->Count; CommandIndex++)
    {
        push_command* Command = CommandList->Ptr[CommandIndex];
        switch(Command->Type)
        {   
            case PUSH_COMMAND_CLEAR_COLOR:
            {
                push_command_clear_color* ClearColorCommand = (push_command_clear_color*)Command;
                
                glClear(GL_COLOR_BUFFER_BIT);
                glClearColor(ClearColorCommand->R, ClearColorCommand->G, ClearColorCommand->B, ClearColorCommand->A);                                
            } break;
            
            case PUSH_COMMAND_CLEAR_DEPTH:
            {
                push_command_clear_depth* ClearDepthCommand = (push_command_clear_depth*)Command;
                
                glClear(GL_DEPTH_BUFFER_BIT);
                glClearDepth(ClearDepthCommand->Depth);
            } break;
            
            case PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH:
            {
                push_command_clear_color_and_depth* ClearColorAndDepthCommand = (push_command_clear_color_and_depth*)Command;                
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glClearColor(ClearColorAndDepthCommand->R, ClearColorAndDepthCommand->G, ClearColorAndDepthCommand->B, ClearColorAndDepthCommand->A);
                glClearDepth(ClearColorAndDepthCommand->Depth);                
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
                ViewProjection = View*Projection;
            } break;
            
            case PUSH_COMMAND_VIEW_TRANSFORM:
            {
                push_command_view_transform* CommandViewTransform = (push_command_view_transform*)Command;
                
                View = InverseTransformM4(CommandViewTransform->Position, CommandViewTransform->Orientation);
                ViewPosition = CommandViewTransform->Position;                                                
                ViewProjection = View*Projection;
            } break;
            
            case PUSH_COMMAND_SUBMIT_LIGHT_BUFFER:
            {
                graphics_light_buffer SrcLightBuffer = ((push_command_submit_light_buffer*)Command)->LightBuffer;
                
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                
                opengl_light_buffer DstLightBuffer = {};
                DstLightBuffer.DirectionalLightCount = SrcLightBuffer.DirectionalLightCount;
                DstLightBuffer.PointLightCount = SrcLightBuffer.PointLightCount;
                
                m4 LightViewProjectionBuffers[MAX_DIRECTIONAL_LIGHT_COUNT] = {};
                
                m4 LightProjection = GetLightProjectionMatrix();
                for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DstLightBuffer.DirectionalLightCount; DirectionalLightIndex++)
                {
                    graphics_directional_light* SrcLight = SrcLightBuffer.DirectionalLights + DirectionalLightIndex;
                    opengl_directional_light* DstLight = DstLightBuffer.DirectionalLights + DirectionalLightIndex;
                    
                    m4 LightView = GetLightViewMatrix(SrcLight->Position, SrcLight->Direction);                                                            
                    LightViewProjectionBuffers[DirectionalLightIndex] = LightView*LightProjection;
                    
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
                
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->ShadowMapTextureArray);
            } break;
            
            case PUSH_COMMAND_DRAW_COLORED_LINE_MESH:
            case PUSH_COMMAND_DRAW_COLORED_MESH:
            {
                push_command_draw_colored_mesh* DrawColoredMesh = (push_command_draw_colored_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawColoredMesh->MeshID);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->ColorShader.Program, &OpenGL->ColorShader.MVPUniforms,
                                         &DrawColoredMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                SetUniform4f(OpenGL->ColorShader.ColorUniform, DrawColoredMesh->Color);                
                
                GLenum PrimitiveType = (Command->Type == PUSH_COMMAND_DRAW_COLORED_LINE_MESH) ? GL_LINES : GL_TRIANGLES;
                
                if(Command->Type == PUSH_COMMAND_DRAW_COLORED_LINE_MESH)
                    DrawLines(Mesh, &DrawColoredMesh->DrawInfo);
                else
                    DrawTriangles(Mesh, &DrawColoredMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_TEXTURED_MESH:
            {
                push_command_draw_textured_mesh* DrawTexturedMesh = (push_command_draw_textured_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawTexturedMesh->MeshID);
                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawTexturedMesh->TextureID);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->TextureShader.Program, &OpenGL->TextureShader.MVPUniforms,
                                         &DrawTexturedMesh->WorldTransform, &ViewProjection);                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                
                DrawTriangles(Mesh, &DrawTexturedMesh->DrawInfo);
            } break;
            
            case PUSH_COMMAND_DRAW_COLORED_SKINNING_MESH:
            {
                push_command_draw_colored_skinning_mesh* DrawColoredSkinningMesh = (push_command_draw_colored_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawColoredSkinningMesh->MeshID);
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawColoredSkinningMesh->JointCount, DrawColoredSkinningMesh->Joints);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->ColorSkinningShader.Program, &OpenGL->ColorSkinningShader.MVPUniforms,
                                         &DrawColoredSkinningMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                SetUniform4f(OpenGL->ColorShader.ColorUniform, DrawColoredSkinningMesh->Color);
                
                DrawTriangles(Mesh, &DrawColoredSkinningMesh->DrawInfo);               
            } break;
            
            case PUSH_COMMAND_DRAW_TEXTURED_SKINNING_MESH:
            {
                push_command_draw_textured_skinning_mesh* DrawTexturedSkinningMesh = (push_command_draw_textured_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawTexturedSkinningMesh->MeshID);
                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawTexturedSkinningMesh->TextureID);
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawTexturedSkinningMesh->JointCount, DrawTexturedSkinningMesh->Joints);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->TextureSkinningShader.Program, &OpenGL->TextureSkinningShader.MVPUniforms,
                                         &DrawTexturedSkinningMesh->WorldTransform, &ViewProjection);
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                
                DrawTriangles(Mesh, &DrawTexturedSkinningMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_MESH:
            {
                push_command_draw_lambertian_colored_mesh* DrawLambertianColoredMesh = (push_command_draw_lambertian_colored_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLambertianColoredMesh->MeshID);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->LambertianColorShader.Program, &OpenGL->LambertianColorShader.MVPUniforms,
                                         &DrawLambertianColoredMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                SetUniform4f(OpenGL->LambertianColorShader.DiffuseColorUniform, DrawLambertianColoredMesh->DiffuseColor);
                
                DrawTriangles(Mesh, &DrawLambertianColoredMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_MESH:
            {
                push_command_draw_lambertian_textured_mesh* DrawLambertianTexturedMesh = (push_command_draw_lambertian_textured_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLambertianTexturedMesh->MeshID);
                opengl_texture* Diffuse = GetByID(&OpenGL->TexturePool, DrawLambertianTexturedMesh->DiffuseID);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->LambertianTextureShader.Program, &OpenGL->LambertianTextureShader.MVPUniforms,
                                         &DrawLambertianTexturedMesh->WorldTransform, &ViewProjection);                
                BindVAO(&BoundVAO, Mesh->VAO);                                                                
                
                glBindTexture(GL_TEXTURE_2D, Diffuse->Handle);
                
                DrawTriangles(Mesh, &DrawLambertianTexturedMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_SKINNING_MESH:
            {
                push_command_draw_lambertian_colored_skinning_mesh* DrawLambertianColoredSkinningMesh = (push_command_draw_lambertian_colored_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLambertianColoredSkinningMesh->MeshID);
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawLambertianColoredSkinningMesh->JointCount, DrawLambertianColoredSkinningMesh->Joints);
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->LambertianColorSkinningShader.Program, &OpenGL->LambertianColorShader.MVPUniforms,
                                         &DrawLambertianColoredSkinningMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                SetUniform4f(OpenGL->LambertianColorSkinningShader.DiffuseColorUniform, DrawLambertianColoredSkinningMesh->DiffuseColor);
                
                DrawTriangles(Mesh, &DrawLambertianColoredSkinningMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_SKINNING_MESH:
            {
                push_command_draw_lambertian_textured_skinning_mesh* DrawLambertianTexturedSkinningMesh = (push_command_draw_lambertian_textured_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLambertianTexturedSkinningMesh->MeshID);
                opengl_texture* Diffuse = GetByID(&OpenGL->TexturePool, DrawLambertianTexturedSkinningMesh->DiffuseID);
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawLambertianTexturedSkinningMesh->JointCount, DrawLambertianTexturedSkinningMesh->Joints);
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->LambertianTextureSkinningShader.Program, &OpenGL->LambertianTextureShader.MVPUniforms,
                                         &DrawLambertianTexturedSkinningMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glBindTexture(GL_TEXTURE_2D, Diffuse->Handle);
                
                DrawTriangles(Mesh, &DrawLambertianTexturedSkinningMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_PHONG_COLORED_MESH:
            {
                push_command_draw_phong_colored_mesh* DrawPhongColoredMesh = (push_command_draw_phong_colored_mesh*)Command;                                                
                
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawPhongColoredMesh->MeshID);
                
                if(SetProgramAndMVPUniforms(&BoundProgram, OpenGL->PhongColorShader.Program, &OpenGL->PhongColorShader.MVPUniforms,
                                            &DrawPhongColoredMesh->WorldTransform, &ViewProjection))                
                    SetUniform3f(OpenGL->PhongColorShader.ViewPositionUniform, ViewPosition);                
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                c4 DiffuseColor = DrawPhongColoredMesh->DiffuseColor;
                c4 SpecularColor = DrawPhongColoredMesh->SpecularColor;
                SetUniform4f(OpenGL->PhongColorShader.DiffuseColorUniform,  DiffuseColor);
                SetUniform4f(OpenGL->PhongColorShader.SpecularColorUniform, SpecularColor);                
                glUniform1i(OpenGL->PhongColorShader.ShininessUniform, DrawPhongColoredMesh->Shininess);
                
                DrawTriangles(Mesh, &DrawPhongColoredMesh->DrawInfo);                
                
            } break;            
            
            case PUSH_COMMAND_DRAW_PHONG_TEXTURED_MESH:
            {
                push_command_draw_phong_textured_mesh* DrawPhongTexturedMesh = (push_command_draw_phong_textured_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawPhongTexturedMesh->MeshID);
                opengl_texture* Diffuse = GetByID(&OpenGL->TexturePool, DrawPhongTexturedMesh->DiffuseID);
                opengl_texture* Specular = GetByID(&OpenGL->TexturePool, DrawPhongTexturedMesh->SpecularID);
                
                if(SetProgramAndMVPUniforms(&BoundProgram, OpenGL->PhongTextureShader.Program, &OpenGL->PhongTextureShader.MVPUniforms,
                                            &DrawPhongTexturedMesh->WorldTransform, &ViewProjection))
                {
                    SetUniform3f(OpenGL->PhongTextureShader.ViewPositionUniform, ViewPosition);                
                    glUniform1i(OpenGL->PhongTextureShader.ShadowMapUniform, 2);                                      
                }
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniform1i(OpenGL->PhongTextureShader.ShininessUniform, DrawPhongTexturedMesh->Shininess);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Diffuse->Handle);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Specular->Handle);
                
                glUniform1i(OpenGL->PhongTextureShader.DiffuseUniform, 0);
                glUniform1i(OpenGL->PhongTextureShader.SpecularUniform, 1);
                
                DrawTriangles(Mesh, &DrawPhongTexturedMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_PHONG_COLORED_SKINNING_MESH:
            {
                push_command_draw_phong_colored_skinning_mesh* DrawPhongColoredSkinningMesh = (push_command_draw_phong_colored_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawPhongColoredSkinningMesh->MeshID);                
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawPhongColoredSkinningMesh->JointCount, DrawPhongColoredSkinningMesh->Joints);
                if(SetProgramAndMVPUniforms(&BoundProgram, OpenGL->PhongColorSkinningShader.Program, &OpenGL->PhongColorSkinningShader.MVPUniforms,
                                            &DrawPhongColoredSkinningMesh->WorldTransform, &ViewProjection))
                    SetUniform3f(OpenGL->PhongColorSkinningShader.ViewPositionUniform, ViewPosition);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                c4 DiffuseColor = DrawPhongColoredSkinningMesh->DiffuseColor;
                c4 SpecularColor = DrawPhongColoredSkinningMesh->SpecularColor;
                SetUniform4f(OpenGL->PhongColorShader.DiffuseColorUniform,  DiffuseColor);
                SetUniform4f(OpenGL->PhongColorShader.SpecularColorUniform, SpecularColor);                
                glUniform1i(OpenGL->PhongColorShader.ShininessUniform, DrawPhongColoredSkinningMesh->Shininess);
                
                DrawTriangles(Mesh, &DrawPhongColoredSkinningMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_PHONG_TEXTURED_SKINNING_MESH:
            {
                push_command_draw_phong_textured_skinning_mesh* DrawPhongTexturedSkinningMesh = (push_command_draw_phong_textured_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawPhongTexturedSkinningMesh->MeshID);
                opengl_texture* Diffuse = GetByID(&OpenGL->TexturePool, DrawPhongTexturedSkinningMesh->DiffuseID);
                opengl_texture* Specular = GetByID(&OpenGL->TexturePool, DrawPhongTexturedSkinningMesh->SpecularID);
                
                UploadSkinningMatrices(&OpenGL->SkinningBuffers, SkinningIndex++, DrawPhongTexturedSkinningMesh->JointCount, DrawPhongTexturedSkinningMesh->Joints);
                if(SetProgramAndMVPUniforms(&BoundProgram, OpenGL->PhongTextureSkinningShader.Program, &OpenGL->PhongTextureSkinningShader.MVPUniforms,
                                            &DrawPhongTexturedSkinningMesh->WorldTransform, &ViewProjection))
                    SetUniform3f(OpenGL->PhongTextureSkinningShader.ViewPositionUniform, ViewPosition);
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniform1i(OpenGL->PhongTextureSkinningShader.ShininessUniform, DrawPhongTexturedSkinningMesh->Shininess);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Diffuse->Handle);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Specular->Handle);
                
                glUniform1i(OpenGL->PhongTextureSkinningShader.DiffuseUniform, 0);
                glUniform1i(OpenGL->PhongTextureSkinningShader.SpecularUniform, 1);
                
                DrawTriangles(Mesh, &DrawPhongTexturedSkinningMesh->DrawInfo);                
            } break;
            
            case PUSH_COMMAND_DRAW_IMGUI_UI:
            {
                push_command_draw_imgui_ui* DrawImGuiUI = (push_command_draw_imgui_ui*)Command;                                
                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawImGuiUI->TextureID);
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawImGuiUI->MeshID);
                ASSERT(Mesh->IsDynamic);
                
                if(BindProgram(&BoundProgram, OpenGL->ImGuiShader.Program))                    
                    glUniformMatrix4fv(OpenGL->ImGuiShader.ProjectionUniform, 1, GL_FALSE, Projection.M);                                                                    
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                
                DrawTriangles(Mesh, &DrawImGuiUI->DrawInfo);                
                
            } break;
            
            case PUSH_COMMAND_LIGHT_FOR_SHADOW_MAP:
            {
                push_command_light_for_shadow_map* LightForShadowMap = (push_command_light_for_shadow_map*)Command;                
                
                glBindFramebuffer(GL_FRAMEBUFFER, OpenGL->ShadowMapFBO);
                glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->ShadowMapTextureArray);                
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, OpenGL->ShadowMapTextureArray, 0, DirectionalLightCounter++);                
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);                
                
                View = LightForShadowMap->LightView;
                Projection = LightForShadowMap->LightProjection;                
                ViewProjection = View*Projection;
            } break;
            
            case PUSH_COMMAND_DRAW_SHADOWED_MESH:
            {
                push_command_draw_shadowed_mesh* DrawShadowedMesh = (push_command_draw_shadowed_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawShadowedMesh->MeshID);
                
                SetProgramAndMVPUniforms(&BoundProgram, OpenGL->ShadowMapShader.Program, &OpenGL->ShadowMapShader.MVPUniforms,
                                         &DrawShadowedMesh->WorldTransform, &ViewProjection);
                
                BindVAO(&BoundVAO, Mesh->VAO);                
                DrawTriangles(Mesh, &DrawShadowedMesh->DrawInfo);
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
    OpenGL->ImGuiShader.Valid = false;
    OpenGL->ColorShader.Valid = false;
    OpenGL->ColorSkinningShader.Valid = false;
    OpenGL->PhongColorShader.Valid = false;
    OpenGL->PhongColorSkinningShader.Valid = false;    
}