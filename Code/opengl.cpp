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

inline GLenum
GetIndexType(graphics_mesh* Mesh)
{    
    ASSERT(Mesh->IndexBuffer.Format != GRAPHICS_INDEX_FORMAT_UNKNOWN);
    GLenum IndexType = (Mesh->IndexBuffer.Format == GRAPHICS_INDEX_FORMAT_32_BIT) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    return IndexType;
}

ALLOCATE_TEXTURE(AllocateTexture)
{
    opengl_graphics_texture* Result = PushStruct(&Graphics->GraphicsStorage, opengl_graphics_texture, Clear, 0);
    Result->Data = Data;
    Result->Dimensions = Dimensions;
    
    GLenum MinFilter = GetFilterType(SamplerInfo->MinFilter);
    GLenum MagFilter = GetFilterType(SamplerInfo->MagFilter);
    
    glGenTextures(1, &Result->Handle);
    glBindTexture(GL_TEXTURE_2D, Result->Handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Dimensions.width, Dimensions.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return Result;
}

ALLOCATE_MESH(AllocateMesh)
{   
    //TODO(JJ): We should allocate this data structure from a pool of opengl graphics meshes later
    opengl_graphics_mesh* Result = PushStruct(&Graphics->GraphicsStorage, opengl_graphics_mesh, Clear, 0);
    
    Result->IsDynamic = false;
    
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

ALLOCATE_DYNAMIC_MESH(AllocateDynamicMesh)
{
    opengl_graphics_mesh* Result = PushStruct(&Graphics->GraphicsStorage, opengl_graphics_mesh, Clear, 0);
    Result->IsDynamic = true;
    
    Result->VertexBuffer.Format = VertexFormat;
    Result->IndexBuffer.Format = IndexFormat;
    
    glGenVertexArrays(1, &Result->VAO);
    glGenBuffers(1, &Result->VBO);
    glGenBuffers(1, &Result->EBO);
    
    glBindVertexArray(Result->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, Result->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Result->EBO);
    
    switch(VertexFormat)
    {
        case GRAPHICS_VERTEX_FORMAT_P2_UV_C:
        {
            GLuint PAttribute = 0;
            GLuint UVAttribute = 1;
            GLuint CAttribute = 2;
            
            GLsizei Stride = (GLsizei)GetVertexSize(VertexFormat);
            
            glVertexAttribPointer(PAttribute,  2, GL_FLOAT, GL_FALSE, Stride, (void*)0);
            glVertexAttribPointer(UVAttribute, 2, GL_FLOAT, GL_FALSE, Stride, (void*)(sizeof(v2f)));
            glVertexAttribPointer(CAttribute,  4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (void*)(sizeof(v2f)*2));
            
            glEnableVertexAttribArray(PAttribute);
            glEnableVertexAttribArray(UVAttribute);
            glEnableVertexAttribArray(CAttribute);
        } break;
    }
    
    glBindVertexArray(0);
    
    return Result;
}

STREAM_MESH_DATA(StreamMeshData)
{
    opengl_graphics_mesh* OpenGLMesh = (opengl_graphics_mesh*)Mesh;
    
    glBindVertexArray(OpenGLMesh->VAO);
    
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
EXPORT INIT_GRAPHICS(InitGraphics)
{
    Global_Platform = Platform;
    InitMemory(Platform->TempArena, Platform->AllocateMemory, Platform->FreeMemory);
    
    arena GraphicsStorage = CreateArena(KILOBYTE(32));    
    graphics* Graphics = PushStruct(&GraphicsStorage, graphics, Clear, 0);
    
    Graphics->GraphicsStorage = GraphicsStorage;
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
    if(!Global_StandardPhongShader.Program)
    {
        const GLchar* VertexShader[] = { Shader_Header, VertexShader_StandardWorldSpaceToClipSpace };
        const GLchar* FragmentShader[] = { Shader_Header, FragmentShader_StandardPhongShading };
        
        Global_StandardPhongShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        Global_StandardPhongShader.ProjectionLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Projection");
        Global_StandardPhongShader.ViewLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "View");
        Global_StandardPhongShader.ModelLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Model");
        Global_StandardPhongShader.ColorLocation = glGetUniformLocation(Global_StandardPhongShader.Program, "Color");        
    }
    
    if(!Global_ImGuiShader.Program)
    {
        const GLchar* VertexShader[] = {Shader_Header, VertexShader_ImGui};
        const GLchar* FragmentShader[] = {Shader_Header, FragmentShader_ImGui};
        
        Global_ImGuiShader.Program = CreateShaderProgram(VertexShader, ARRAYCOUNT(VertexShader), FragmentShader, ARRAYCOUNT(FragmentShader));
        Global_ImGuiShader.ProjectionLocation = glGetUniformLocation(Global_ImGuiShader.Program, "Projection");
    }
    
    m4 Projection = IdentityM4();
    m4 CameraView = IdentityM4();
        
    glViewport(0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    GLuint BoundProgram = (GLuint)-1;
    
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
                                                
                if(BoundProgram != Global_StandardPhongShader.Program)
                {
                    BoundProgram = Global_StandardPhongShader.Program;
                    glUseProgram(BoundProgram);
                    
                    glUniformMatrix4fv(Global_StandardPhongShader.ProjectionLocation, 1, GL_FALSE, Projection.M);
                    glUniformMatrix4fv(Global_StandardPhongShader.ViewLocation, 1, GL_FALSE, CameraView.M);                    
                }
                
                opengl_graphics_mesh* Mesh = (opengl_graphics_mesh*)DrawShadedColoredMesh->Mesh;
                
                glUniformMatrix4fv(Global_StandardPhongShader.ModelLocation, 1, GL_FALSE, DrawShadedColoredMesh->WorldTransform.M);
                glUniform4f(Global_StandardPhongShader.ColorLocation, DrawShadedColoredMesh->R, DrawShadedColoredMesh->G, DrawShadedColoredMesh->B, DrawShadedColoredMesh->A);
                
                glBindVertexArray(Mesh->VAO);              
                
                GLenum IndexType = GetIndexType(Mesh);                
                glDrawElementsBaseVertex(GL_TRIANGLES, Mesh->IndexBuffer.IndexCount, IndexType, 0, 0);
                
            } break;
            
            case PUSH_COMMAND_DRAW_IMGUI_UI:
            {
                push_command_draw_imgui_ui* DrawImGuiUI = (push_command_draw_imgui_ui*)Command;
                if(BoundProgram != Global_ImGuiShader.Program)
                {
                    BoundProgram = Global_ImGuiShader.Program;
                    glUseProgram(BoundProgram);
                    
                    glUniformMatrix4fv(Global_ImGuiShader.ProjectionLocation, 1, GL_FALSE, Projection.M);                    
                }                                
                
                opengl_graphics_mesh* Mesh = (opengl_graphics_mesh*)DrawImGuiUI->Mesh;
                ASSERT(Mesh->IsDynamic);
                
                opengl_graphics_texture* Texture = (opengl_graphics_texture*)DrawImGuiUI->Texture;
                
                glBindVertexArray(Mesh->VAO);              
                
                glBindTexture(GL_TEXTURE_2D, Texture->Handle);
                
                GLenum IndexType = GetIndexType(Mesh);
                glDrawElementsBaseVertex(GL_TRIANGLES, DrawImGuiUI->IndexCount, IndexType, 
                                         (void*)(DrawImGuiUI->IndexOffset*GetIndexSize(Mesh)), 
                                         DrawImGuiUI->VertexOffset);
                
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }    
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    CommandList->Count = 0;        
    
    Platform_SwapBuffers(Graphics->PlatformData);    
}