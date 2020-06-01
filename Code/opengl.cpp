#include "opengl.h"
#include "opengl_shaders.cpp"

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

inline GLenum
GetIndexType(graphics_index_format IndexFormat)
{    
    ASSERT(IndexFormat != GRAPHICS_INDEX_FORMAT_UNKNOWN);
    GLenum IndexType = (IndexFormat == GRAPHICS_INDEX_FORMAT_32_BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    return IndexType;
}

inline ptr
GetIndexTypeSize(GLenum IndexType)
{        
    ASSERT((IndexType == GL_UNSIGNED_INT) || (IndexType == GL_UNSIGNED_SHORT));
    ptr Result = (IndexType == GL_UNSIGNED_INT) ? sizeof(u32) : sizeof(u16);
    return Result;
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
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Dimensions.width, Dimensions.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
    
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
    
    switch(VertexFormat)
    {
        case GRAPHICS_VERTEX_FORMAT_P3:
        {
            GLuint PAttribute = 0;
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);
            glVertexAttribPointer(PAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3, P));
            
            glEnableVertexAttribArray(PAttribute);
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3:
        {
            GLuint PAttribute = 0;
            GLuint NAttribute = 1;
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);
            glVertexAttribPointer(PAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3, P));
            glVertexAttribPointer(NAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3, N));            
            
            glEnableVertexAttribArray(PAttribute);
            glEnableVertexAttribArray(NAttribute);            
            
        } break;
        
        case GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS:
        {
            GLuint PAttribute = 0;
            GLuint NAttribute = 1;
            GLuint JointIAttribute = 2;
            GLuint JointWAttribute = 3;
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);
            glVertexAttribPointer(PAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, P));
            glVertexAttribPointer(NAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, N));            
            glVertexAttribIPointer(JointIAttribute, 1, GL_UNSIGNED_INT, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, JointI));                        
            glVertexAttribPointer(JointWAttribute, 4, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p3_n3_weights, JointW));
            
            glEnableVertexAttribArray(PAttribute);
            glEnableVertexAttribArray(NAttribute);
            glEnableVertexAttribArray(JointIAttribute);
            glEnableVertexAttribArray(JointWAttribute);
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
            GLuint PAttribute = 0;
            GLuint UVAttribute = 1;
            GLuint CAttribute = 2;
            
            GLsizei Stride = (GLsizei)GetVertexStride(VertexFormat);
            
            glVertexAttribPointer(PAttribute,  2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, P));
            glVertexAttribPointer(UVAttribute, 2, GL_FLOAT, GL_FALSE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, UV));
            glVertexAttribPointer(CAttribute,  4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (void*)OFFSET_OF(vertex_p2_uv_c, C));
            
            glEnableVertexAttribArray(PAttribute);
            glEnableVertexAttribArray(UVAttribute);
            glEnableVertexAttribArray(CAttribute);
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

GLuint CreateShaderProgram(const GLchar** VSCode, u32 VSCodeCount, const GLchar** FSCode, u32 FSCodeCount)
{    
    GLuint VertexShader   = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(VertexShader,   VSCodeCount, VSCode, 0);
    glShaderSource(FragmentShader, FSCodeCount, FSCode, 0);
    
    glCompileShader(VertexShader);
    glCompileShader(FragmentShader);
    
    GLint VSCompiled, FSCompiled;    
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &VSCompiled);
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &FSCompiled);
    
    if(!VSCompiled || !FSCompiled)
    {
        char VSError[4096];
        char FSError[4096];
        
        glGetShaderInfoLog(VertexShader,   sizeof(VSError), NULL, VSError);
        glGetShaderInfoLog(FragmentShader, sizeof(FSError), NULL, FSError);
        
        CONSOLE_LOG("%s\n", VSError);
        CONSOLE_LOG("%s\n", FSError);
        
        INVALID_CODE;
    }
    
    GLuint Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    glValidateProgram(Program);
    
    GLint Linked;
    glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
    
    if(!Linked)
    {
        char ProgramError[4096];
        glGetProgramInfoLog(Program, sizeof(ProgramError), NULL, ProgramError);
        
        CONSOLE_LOG("%s\n", ProgramError);
        
        INVALID_CODE;
    }
    
    glDetachShader(Program, VertexShader);
    glDetachShader(Program, FragmentShader);
    
    return Program;
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

b32 Platform_InitOpenGL(void* PlatformData)
{
    HWND Window = (HWND)PlatformData;
    HDC DeviceContext = GetDC(Window);
    
    PIXELFORMATDESCRIPTOR PixelFormat = {};
    PixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    PixelFormat.nVersion = 1;
    PixelFormat.dwFlags = PFD_DOUBLEBUFFER|PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW;
    PixelFormat.iPixelType = PFD_TYPE_RGBA;
    PixelFormat.cColorBits = 32;
    PixelFormat.cDepthBits = 32;
    PixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    int PixelFormatIndex = ChoosePixelFormat(DeviceContext, &PixelFormat);
    BOOL_CHECK_AND_HANDLE(PixelFormatIndex, "Failed to find a valid win32 pixel format.");
    
    BOOL_CHECK_AND_HANDLE(SetPixelFormat(DeviceContext, PixelFormatIndex, &PixelFormat),
                          "Failed to set the win32 pixel format.");
    
    HGLRC TempContext = wglCreateContext(DeviceContext);
    wglMakeCurrent(DeviceContext, TempContext);
    
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
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(TempContext);
    wglMakeCurrent(DeviceContext, RenderingContext);
    
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    BOOL_CHECK_AND_HANDLE(wglSwapIntervalEXT, "Failed to load the wglSwapIntervalEXT function.");
    
    wglSwapIntervalEXT(1);
    
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
    
    BOOL_CHECK_AND_HANDLE(Platform_InitOpenGL(Graphics->PlatformData), "Failed to initialize opengl.");
    
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
    
#if DEVELOPER_BUILD
    
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)Platform_LoadProc("glDebugMessageCallback");
    PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)Platform_LoadProc("glDebugMessageControl");
    
    ASSERT(glDebugMessageControl && glDebugMessageCallback);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageCallback((GLDEBUGPROC)glDebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    
#endif    
    
    Graphics->AllocateTexture = AllocateTexture;
    Graphics->AllocateMesh = AllocateMesh;
    Graphics->AllocateDynamicMesh = AllocateDynamicMesh;
    Graphics->StreamMeshData = StreamMeshData;
    
    return Graphics;
    
    handle_error:
    return NULL;
}

extern "C"
EXPORT EXECUTE_RENDER_COMMANDS(ExecuteRenderCommands)
{
    opengl_context* OpenGL = (opengl_context*)Graphics;
    
    if(!OpenGL->StandardPhongShader.Program)
    {
        const GLchar* VertexShader[] = { Shader_Header, FormatString(VertexShader_StandardWorldSpaceToClipSpace, 1, 0, 0) };
        const GLchar* FragmentShader[] = { Shader_Header, FragmentShader_StandardPhongShading };
        
        OpenGL->StandardPhongShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        OpenGL->StandardPhongShader.ProjectionLocation = glGetUniformLocation(OpenGL->StandardPhongShader.Program, "Projection");
        OpenGL->StandardPhongShader.ViewLocation = glGetUniformLocation(OpenGL->StandardPhongShader.Program, "View");
        OpenGL->StandardPhongShader.ModelLocation = glGetUniformLocation(OpenGL->StandardPhongShader.Program, "Model");
        OpenGL->StandardPhongShader.ColorLocation = glGetUniformLocation(OpenGL->StandardPhongShader.Program, "Color");        
    }
    
    if(!OpenGL->StandardColorShader.Program)
    {
        const GLchar* VertexShader[] = { Shader_Header, FormatString(VertexShader_StandardWorldSpaceToClipSpace, 0, 1, 0) };
        const GLchar* FragmentShader[] = { Shader_Header, FragmentShader_SimpleColor };
        
        OpenGL->StandardColorShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        OpenGL->StandardColorShader.ProjectionLocation = glGetUniformLocation(OpenGL->StandardColorShader.Program, "Projection");
        OpenGL->StandardColorShader.ViewLocation = glGetUniformLocation(OpenGL->StandardColorShader.Program, "View");
        OpenGL->StandardColorShader.ModelLocation = glGetUniformLocation(OpenGL->StandardColorShader.Program, "Model");
        OpenGL->StandardColorShader.ColorLocation = glGetUniformLocation(OpenGL->StandardColorShader.Program, "Color");        
    }
    
    if(!OpenGL->ImGuiShader.Program)
    {
        const GLchar* VertexShader[] = {Shader_Header, VertexShader_ImGui};
        const GLchar* FragmentShader[] = {Shader_Header, FragmentShader_ImGui};
        
        OpenGL->ImGuiShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        OpenGL->ImGuiShader.ProjectionLocation = glGetUniformLocation(OpenGL->ImGuiShader.Program, "Projection");
    }
    
    if(!OpenGL->QuadShader.Program)
    {
        const GLchar* VertexShader[] = {Shader_Header, VertexShader_Quad};
        const GLchar* FragmentShader[] = {Shader_Header, FragmentShader_SimpleColor};
        
        OpenGL->QuadShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        OpenGL->QuadShader.ProjectionLocation = glGetUniformLocation(OpenGL->QuadShader.Program, "Projection");
        OpenGL->QuadShader.ViewLocation = glGetUniformLocation(OpenGL->QuadShader.Program, "View");
        OpenGL->QuadShader.ColorLocation = glGetUniformLocation(OpenGL->QuadShader.Program, "Color");
        OpenGL->QuadShader.PositionLocation = glGetUniformLocation(OpenGL->QuadShader.Program, "Positions");
    }
    
    if(!OpenGL->StandardSkinningPhongShader.Program)
    {
        const GLchar* VertexShader[] = {Shader_Header, FormatString(VertexShader_StandardWorldSpaceToClipSpace, 0, 0, 1, MAX_JOINT_COUNT)};
        const GLchar* FragmentShader[] = {Shader_Header, FragmentShader_StandardPhongShading};
        
        OpenGL->StandardSkinningPhongShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        OpenGL->StandardSkinningPhongShader.ProjectionLocation = glGetUniformLocation(OpenGL->StandardSkinningPhongShader.Program, "Projection");
        OpenGL->StandardSkinningPhongShader.ViewLocation = glGetUniformLocation(OpenGL->StandardSkinningPhongShader.Program, "View");
        OpenGL->StandardSkinningPhongShader.ModelLocation = glGetUniformLocation(OpenGL->StandardSkinningPhongShader.Program, "Model");
        OpenGL->StandardSkinningPhongShader.ColorLocation = glGetUniformLocation(OpenGL->StandardSkinningPhongShader.Program, "Color");
        
        OpenGL->StandardSkinningPhongShader.SkinningIndex = glGetUniformBlockIndex(OpenGL->StandardSkinningPhongShader.Program, "SkinningBuffer");
        glUniformBlockBinding(OpenGL->StandardSkinningPhongShader.Program, OpenGL->StandardSkinningPhongShader.SkinningIndex, 0);
    }
    
    if(!OpenGL->SkinningBuffers.Ptr)
    {
        OpenGL->SkinningBuffers.Capacity = 32;
        OpenGL->SkinningBuffers.Ptr = PushArray(&OpenGL->Storage, OpenGL->SkinningBuffers.Capacity, GLuint, Clear, 0);
    }
    
    m4 Projection = IdentityM4();
    m4 CameraView = IdentityM4();
    
    glEnable(GL_SCISSOR_TEST);
    
    GLuint BoundProgram = (GLuint)-1;
    GLuint BoundVAO = (GLuint)-1;
    
    u32 SkinningIndex = 0;
    
    push_command_list* CommandList = &Graphics->CommandList;        
    for(u32 CommandIndex = 0; CommandIndex < CommandList->Count; CommandIndex++)
    {
        push_command* Command = CommandList->Ptr[CommandIndex];
        switch(Command->Type)
        {   
            case PUSH_COMMAND_CLEAR_COLOR:
            {
                push_command_clear_color* ClearColorCommand = (push_command_clear_color*)Command;
                
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                glClearColor(ClearColorCommand->R, ClearColorCommand->G, ClearColorCommand->B, ClearColorCommand->A);                                
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
                if(CommandCull->Enable) 
                    glEnable(GL_CULL_FACE);                
                else
                    glDisable(GL_CULL_FACE);
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
            
            case PUSH_COMMAND_CAMERA_VIEW:
            {
                CameraView = ((push_command_4x4_matrix*)Command)->Matrix;
            } break;
            
            case PUSH_COMMAND_DRAW_SHADED_COLORED_MESH:
            {
                push_command_draw_shaded_colored_mesh* DrawShadedColoredMesh = (push_command_draw_shaded_colored_mesh*)Command;                                                
                
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawShadedColoredMesh->MeshID);
                
                if(BindProgram(&BoundProgram, OpenGL->StandardPhongShader.Program))
                {                                        
                    glUniformMatrix4fv(OpenGL->StandardPhongShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(OpenGL->StandardPhongShader.ViewLocation, 1, GL_FALSE, CameraView.M);                    
                }
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniformMatrix4fv(OpenGL->StandardPhongShader.ModelLocation, 1, GL_FALSE, DrawShadedColoredMesh->WorldTransform.M);
                glUniform4f(OpenGL->StandardPhongShader.ColorLocation, DrawShadedColoredMesh->R, DrawShadedColoredMesh->G, DrawShadedColoredMesh->B, DrawShadedColoredMesh->A);
                
                glDrawElementsBaseVertex(GL_TRIANGLES, DrawShadedColoredMesh->IndexCount, Mesh->IndexType, 
                                         (void*)(DrawShadedColoredMesh->IndexOffset*GetIndexTypeSize(Mesh->IndexType)), 
                                         DrawShadedColoredMesh->VertexOffset);
                
            } break;            
            
            case PUSH_COMMAND_DRAW_SHADED_COLORED_SKINNING_MESH:
            {
                push_command_draw_shaded_colored_skinning_mesh* DrawShadedColoredSkinningMesh = (push_command_draw_shaded_colored_skinning_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawShadedColoredSkinningMesh->MeshID);                
                
                opengl_buffer_list* BufferList = &OpenGL->SkinningBuffers;      
                ASSERT(SkinningIndex <= BufferList->Count);
                
                if(SkinningIndex == BufferList->Count)
                {                    
                    ASSERT(BufferList->Count < BufferList->Capacity);
                    GLuint SkinningUBO; 
                    
                    glGenBuffers(1, &SkinningUBO);
                    glBindBuffer(GL_UNIFORM_BUFFER, SkinningUBO);
                    glBufferData(GL_UNIFORM_BUFFER, sizeof(m4)*MAX_JOINT_COUNT, NULL, GL_STATIC_DRAW);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);
                    
                    glBindBufferBase(GL_UNIFORM_BUFFER, 0, SkinningUBO);                    
                    
                    BufferList->Ptr[BufferList->Count++] = SkinningUBO;                                                                                                    
                }
                
                GLuint SkinningUBO = BufferList->Ptr[SkinningIndex];
                glBindBuffer(GL_UNIFORM_BUFFER, SkinningUBO);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m4)*DrawShadedColoredSkinningMesh->JointCount, DrawShadedColoredSkinningMesh->Joints);                                
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
                
                if(BindProgram(&BoundProgram, OpenGL->StandardSkinningPhongShader.Program))
                {
                    glUniformMatrix4fv(OpenGL->StandardSkinningPhongShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(OpenGL->StandardSkinningPhongShader.ViewLocation, 1, GL_FALSE, CameraView.M);
                }
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniformMatrix4fv(OpenGL->StandardSkinningPhongShader.ModelLocation, 1, GL_FALSE, DrawShadedColoredSkinningMesh->WorldTransform.M);
                glUniform4f(OpenGL->StandardSkinningPhongShader.ColorLocation, DrawShadedColoredSkinningMesh->R, DrawShadedColoredSkinningMesh->G, DrawShadedColoredSkinningMesh->G, DrawShadedColoredSkinningMesh->A);
                
                glDrawElementsBaseVertex(GL_TRIANGLES, DrawShadedColoredSkinningMesh->IndexCount, Mesh->IndexType, 
                                         (void*)(DrawShadedColoredSkinningMesh->IndexOffset*GetIndexTypeSize(Mesh->IndexType)),
                                         DrawShadedColoredSkinningMesh->VertexOffset);
                
                SkinningIndex++;
            } break;
            
            case PUSH_COMMAND_DRAW_IMGUI_UI:
            {
                push_command_draw_imgui_ui* DrawImGuiUI = (push_command_draw_imgui_ui*)Command;                                
                opengl_texture* Texture = GetByID(&OpenGL->TexturePool, DrawImGuiUI->TextureID);
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawImGuiUI->MeshID);
                ASSERT(Mesh->IsDynamic);
                
                if(BindProgram(&BoundProgram, OpenGL->ImGuiShader.Program))                    
                    glUniformMatrix4fv(OpenGL->ImGuiShader.ProjectionLocation, 1, GL_FALSE, Projection.M);                                                                    
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                
                glDrawElementsBaseVertex(GL_TRIANGLES, DrawImGuiUI->IndexCount, Mesh->IndexType, 
                                         (void*)(DrawImGuiUI->IndexOffset*GetIndexTypeSize(Mesh->IndexType)), 
                                         DrawImGuiUI->VertexOffset);
                
            } break;
            
            case PUSH_COMMAND_DRAW_QUAD:
            {
                push_command_draw_quad* DrawQuad = (push_command_draw_quad*)Command;
                
                if(BindProgram(&BoundProgram, OpenGL->QuadShader.Program))
                {
                    glUniformMatrix4fv(OpenGL->QuadShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(OpenGL->QuadShader.ViewLocation, 1, GL_FALSE, CameraView.M);
                }
                
                glUniform3fv(OpenGL->QuadShader.PositionLocation, 4, (f32*)DrawQuad->P);
                glUniform4f(OpenGL->QuadShader.ColorLocation, DrawQuad->R, DrawQuad->G, DrawQuad->B, DrawQuad->A);
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
            } break;
            
            case PUSH_COMMAND_DRAW_LINE_MESH:
            {
                push_command_draw_line_mesh* DrawLineMesh = (push_command_draw_line_mesh*)Command;                
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawLineMesh->MeshID);                
                
                if(BindProgram(&BoundProgram, OpenGL->StandardColorShader.Program))
                {                    
                    glUniformMatrix4fv(OpenGL->StandardColorShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(OpenGL->StandardColorShader.ViewLocation, 1, GL_FALSE, CameraView.M);
                }
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniformMatrix4fv(OpenGL->StandardColorShader.ModelLocation, 1, GL_FALSE, DrawLineMesh->WorldTransform.M);
                glUniform4f(OpenGL->StandardColorShader.ColorLocation, DrawLineMesh->R, DrawLineMesh->G, DrawLineMesh->B, DrawLineMesh->A);
                
                glDrawElementsBaseVertex(GL_LINES, DrawLineMesh->IndexCount, Mesh->IndexType, 
                                         (void*)(DrawLineMesh->IndexOffset*GetIndexTypeSize(Mesh->IndexType)),
                                         DrawLineMesh->VertexOffset);
                
            } break;
            
            case PUSH_COMMAND_DRAW_FILLED_MESH:
            {
                push_command_draw_filled_mesh* DrawFilledMesh = (push_command_draw_filled_mesh*)Command;
                opengl_mesh* Mesh = GetByID(&OpenGL->MeshPool, DrawFilledMesh->MeshID);
                
                if(BindProgram(&BoundProgram, OpenGL->StandardColorShader.Program))
                {                    
                    glUniformMatrix4fv(OpenGL->StandardColorShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(OpenGL->StandardColorShader.ViewLocation, 1, GL_FALSE, CameraView.M);
                }
                
                BindVAO(&BoundVAO, Mesh->VAO);
                
                glUniformMatrix4fv(OpenGL->StandardColorShader.ModelLocation, 1, GL_FALSE, DrawFilledMesh->WorldTransform.M);
                glUniform4f(OpenGL->StandardColorShader.ColorLocation, DrawFilledMesh->R, DrawFilledMesh->G, DrawFilledMesh->B, DrawFilledMesh->A);
                
                glDrawElementsBaseVertex(GL_TRIANGLES, DrawFilledMesh->IndexCount, Mesh->IndexType, 
                                         (void*)(DrawFilledMesh->IndexOffset*GetIndexTypeSize(Mesh->IndexType)),
                                         DrawFilledMesh->VertexOffset);                
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }    
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    CommandList->Count = 0;        
    
    Platform_SwapBuffers(Graphics->PlatformData);    
}