#define POSITION_ATTRIBUTE_INDEX 0
#define NORMAL_ATTRIBUTE_INDEX 1
#define UV_ATTRIBUTE_INDEX 2
#define TANGENT_ATTRIBUTE_INDEX 3
#define COLOR_ATTRIBUTE_INDEX 4
#define JOINT_INDEX_ATTRIBUTE_INDEX 5
#define JOINT_WEIGHT_ATTRIBUTE_INDEX 6

#define SKINNING_BUFFER_INDEX 0
#define LIGHT_BUFFER_INDEX 1

global const char* Vertex_Attributes = R"(
layout (location = 0) in v2f Position2D;
layout (location = 0) in v3f Position;
layout (location = 1) in v3f Normal;
layout (location = 2) in v2f UV;
layout (location = 3) in v3f Tangent;
layout (location = 4) in c4  Color;
layout (location = 5) in u32 JointI;
layout (location = 6) in v4f JointW;
)";

global const char* Shader_Header = R"(
#version 330 core
#define v4f vec4
#define v3f vec3
#define v2f vec2
#define m4 mat4
#define m3 mat3
#define c4 v4f
#define c3 v3f
#define f32 float
#define f64 double
#define u32 uint
#define i32 int
)";

#include "opengl_vertex_shaders.cpp"
#include "opengl_fragment_shaders.cpp"

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
        
        if(!VSCompiled)
            DEBUG_LOG("Vertex Shader Error: %s", VSError);
        
        if(!FSCompiled)
            DEBUG_LOG("Fragment Shader Error: %s", FSError);
        
        glDeleteShader(VertexShader);
        glDeleteShader(FragmentShader);
        
        return 0;
    }
    
    GLuint Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    glValidateProgram(Program);
    
    GLint Linked;
    glGetProgramiv(Program, GL_LINK_STATUS, &Linked);        
    
    GLuint Result = Program;
    if(!Linked)
    {
        Result = 0;
        char ProgramError[4096];
        glGetProgramInfoLog(Program, sizeof(ProgramError), NULL, ProgramError);        
        DEBUG_LOG("Program Link Error: %s", ProgramError);        
    }
    
    glDetachShader(Program, VertexShader);
    glDetachShader(Program, FragmentShader);
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    
    return Program;
}

imgui_shader CreateImGuiShader()
{
    imgui_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, VertexShader_ImGui};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_ImGui};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;        
        
        Result.ProjectionUniform  = glGetUniformLocation(Program, "Projection");        
    }
    
    return Result;    
}

color_shader CreateColorShader()
{
    color_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, FormatString(VertexShader_LocalToClip, 0, 0).Data};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_Color};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.ProjectionUniform = glGetUniformLocation(Program, "Projection");
        Result.ViewUniform       = glGetUniformLocation(Program, "View");
        Result.ModelUniform      = glGetUniformLocation(Program, "Model");    
        Result.ColorUniform      = glGetUniformLocation(Program, "Color");
    }
        
    return Result;
}

color_skinning_shader CreateColorSkinningShader()
{
    color_skinning_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, FormatString(VertexShader_LocalToClip, 1, 0, MAX_JOINT_COUNT).Data};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_Color};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.ProjectionUniform = glGetUniformLocation(Program, "Projection");
        Result.ViewUniform       = glGetUniformLocation(Program, "View");
        Result.ModelUniform      = glGetUniformLocation(Program, "Model");
        Result.ColorUniform      = glGetUniformLocation(Program, "Color");
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
    }
        
    return Result;
    
}

phong_color_shader CreatePhongColorShader()
{
    phong_color_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, FormatString(VertexShader_LocalToClip, 0, 1).Data};
    const GLchar* FragmentShaders[] = {Shader_Header, FormatString(FragmentShader_Phong, MAX_DIRECTIONAL_LIGHT_COUNT, MAX_POINT_LIGHT_COUNT).Data};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.ProjectionUniform    = glGetUniformLocation(Program, "Projection");
        Result.ViewUniform          = glGetUniformLocation(Program,  "View");
        Result.ModelUniform         = glGetUniformLocation(Program, "Model");
        Result.SurfaceColorUniform  = glGetUniformLocation(Program, "SurfaceColor");
        Result.SpecularColorUniform = glGetUniformLocation(Program, "SpecularColor");
        Result.ShininessUniform     = glGetUniformLocation(Program, "Shininess");
        
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

phong_color_skinning_shader CreatePhongColorSkinningShader()
{
    phong_color_skinning_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, FormatString(VertexShader_LocalToClip, 1, 1, MAX_JOINT_COUNT).Data};
    const GLchar* FragmentShaders[] = {Shader_Header, FormatString(FragmentShader_Phong, MAX_DIRECTIONAL_LIGHT_COUNT, MAX_POINT_LIGHT_COUNT).Data};        
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.ProjectionUniform    = glGetUniformLocation(Program, "Projection");
        Result.ViewUniform          = glGetUniformLocation(Program, "View");
        Result.ModelUniform         = glGetUniformLocation(Program, "Model");
        Result.SurfaceColorUniform  = glGetUniformLocation(Program, "SurfaceColor");
        Result.SpecularColorUniform = glGetUniformLocation(Program, "SpecularColor");
        Result.ShininessUniform     = glGetUniformLocation(Program, "Shininess");
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        Result.LightIndex    = glGetUniformBlockIndex(Program, "LightBuffer");
        
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);                
    }
    return Result;
}