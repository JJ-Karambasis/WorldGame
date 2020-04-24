#include "opengl.h"
#include "opengl_shaders.cpp"

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

LOAD_PROC(Win32_LoadProc)
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

load_proc* Win32_InitOpenGL(void* PlatformData)
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
        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
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
    
    return Win32_LoadProc;
    
    handle_error:
    return NULL;
}
#endif

extern "C"
EXPORT EXECUTE_RENDER_COMMANDS(ExecuteRenderCommands)
{
    Global_Platform = Platform;
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);
    
    if(!Graphics->Initialized)
    {
        load_proc* LoadProc = NULL;
        
#ifdef OS_WINDOWS        
        LoadProc = Win32_InitOpenGL(Graphics->PlatformData);
        BOOL_CHECK_AND_HANDLE(LoadProc, "Failed to initialize the win32 version of opengl.");        
#endif
        
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
        
        Graphics->Initialized = true;
    } 
    
    command_list* CommandList = &Graphics->CommandList;
    
    m4 Projection = IdentityM4();
    m4 CameraView = IdentityM4();
    
    for(u32 CommandIndex = 0; CommandIndex < CommandList->Count; CommandIndex++)
    {
        command* Command = CommandList->Ptr[CommandIndex];
        switch(Command->Type)
        {
            case COMMAND_SUBMIT_PROJECTION:
            {
                Projection = ((command_4x4_matrix*)Command)->Matrix;
            } break;
            
            case COMMAND_SUBMIT_CAMERA_VIEW:
            {
                CameraView = ((command_4x4_matrix*)Command)->Matrix;
            } break;
            
            case COMMAND_DRAW_SHADED_COLOR_ENTITY:
            {
            } break;
        }
    }
    
    CommandList->Count = 0;
    
    return;
    
    handle_error:
    //TODO(JJ): Probably a fatal error. Should output an error message and exit gracefully
    INVALID_CODE;    
}