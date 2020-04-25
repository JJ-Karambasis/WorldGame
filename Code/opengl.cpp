#include "opengl.h"
#include "opengl_shaders.cpp"

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

inline GLenum
GetIndexType(graphics_mesh* Mesh)
{    
    ASSERT(Mesh->IndexBuffer.Format != GRAPHICS_INDEX_FORMAT_UNKNOWN);
    GLenum IndexType = (Mesh->IndexBuffer.Format == GRAPHICS_INDEX_FORMAT_32_BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    return IndexType;
}

extern "C"
EXPORT ALLOCATE_MESH(AllocateMesh)
{   
    //TODO(JJ): We should allocate this data structure from a pool of opengl graphics meshes later
    opengl_graphics_mesh* Result = PushStruct(&Graphics->GraphicsStorage, opengl_graphics_mesh, Clear, 0);
    
    Result->VertexBuffer = VertexBuffer;
    Result->IndexBuffer = IndexBuffer;
    
    glGenVertexArrays(1, &Result->VAO);
    glGenBuffers(1, &Result->VBO);
    glGenBuffers(1, &Result->EBO);
    
    glBindVertexArray(Result->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Result->VBO);
    glBufferData(GL_ARRAY_BUFFER, GetVertexBufferSize(&Result->VertexBuffer), Result->VertexBuffer.Data, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Result->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GetIndexBufferSize(&Result->IndexBuffer), Result->IndexBuffer.Data, GL_STATIC_DRAW);
    
    switch(VertexBuffer.Format)
    {
        case GRAPHICS_VERTEX_FORMAT_P3_N3:
        {
            GLuint PAttribute = 0;
            GLuint NAttribute = 1;
            
            GLsizei Stride = (GLsizei)GetVertexSize(VertexBuffer.Format);
            glVertexAttribPointer(PAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)0);
            glVertexAttribPointer(NAttribute, 3, GL_FLOAT, GL_FALSE, Stride, (void*)(sizeof(v3f)));
            
            
            glEnableVertexAttribArray(PAttribute);
            glEnableVertexAttribArray(NAttribute);            
            
        } break;
    }
    
    glBindVertexArray(0);    
    
    return Result;
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
#if DEBUG_BUILD
    AttribFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
    
    int ProfileMask = WGL_CONTEXT_PROFILE_MASK_ARB;
    
    int Attribs[] = 
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, AttribFlags,                
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

extern "C"
EXPORT EXECUTE_RENDER_COMMANDS(ExecuteRenderCommands)
{
    Global_Platform = Platform;
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);
    
    if(!Graphics->Initialized)
    {        
        BOOL_CHECK_AND_HANDLE(Platform_InitOpenGL(Graphics->PlatformData), "Failed to initialize the win32 version of opengl.");                
        
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
        
        const GLchar* VertexShader[] = { Shader_Header, VertexShader_StandardWorldSpaceToClipSpace };
        const GLchar* FragmentShader[] = { Shader_Header, FragmentShader_StandardPhongShading };
        
        Global_StandardPhongShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        Global_StandardPhongShader.ProjectionLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Projection");
        Global_StandardPhongShader.ViewLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "View");
        Global_StandardPhongShader.ModelLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Model");
        Global_StandardPhongShader.ColorLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Color");
        
        Graphics->Initialized = true;
    } 
    
    m4 Projection = IdentityM4();
    m4 CameraView = IdentityM4();
    
    GLuint BoundProgram = (GLuint)-1;
    
    glViewport(0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
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
                                
                opengl_graphics_mesh* Mesh = (opengl_graphics_mesh*)DrawShadedColoredMesh->Mesh;
                if(BoundProgram != Global_StandardPhongShader.Program)
                {
                    BoundProgram = Global_StandardPhongShader.Program;
                    glUseProgram(BoundProgram);
                    
                    glUniformMatrix4fv(Global_StandardPhongShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(Global_StandardPhongShader.ViewLocation, 1, GL_FALSE, CameraView.M);                    
                }
                
                glUniformMatrix4fv(Global_StandardPhongShader.ModelLocation, 1, GL_FALSE, DrawShadedColoredMesh->WorldTransform.M);
                glUniform4f(Global_StandardPhongShader.ColorLocation, DrawShadedColoredMesh->R, DrawShadedColoredMesh->G, DrawShadedColoredMesh->B, DrawShadedColoredMesh->A);
                                
                glBindVertexArray(Mesh->VAO);
                
                GLenum IndexType = GetIndexType(Mesh);                
                glDrawElementsBaseVertex(GL_TRIANGLES, Mesh->IndexBuffer.IndexCount, IndexType, 0, 0);
                
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    Platform_SwapBuffers(Graphics->PlatformData);
    
    CommandList->Count = 0;
    
    return;
    
    handle_error:
    //TODO(JJ): Probably a fatal error. Should output an error message and exit gracefully
    INVALID_CODE;    
}