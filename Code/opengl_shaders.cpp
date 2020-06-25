#define POSITION_ATTRIBUTE_INDEX 0
#define NORMAL_ATTRIBUTE_INDEX 1
#define UV_ATTRIBUTE_INDEX 2
#define TANGENT_ATTRIBUTE_INDEX 3
#define COLOR_ATTRIBUTE_INDEX 4
#define JOINT_INDEX_ATTRIBUTE_INDEX 5
#define JOINT_WEIGHT_ATTRIBUTE_INDEX 6

#define SKINNING_BUFFER_INDEX 0
#define LIGHT_BUFFER_INDEX 1
#define LIGHT_VIEW_PROJECTION_INDEX 2

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
#include "opengl_geometry_shaders.cpp"

GLuint CompileShader(const GLchar** Code, u32 CodeCount, GLenum Type)
{
    GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, CodeCount, Code, 0);
    glCompileShader(Shader);
    
    GLint Compiled;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Compiled);
    if(!Compiled)
    {
        char Error[4096];
        glGetShaderInfoLog(Shader, sizeof(Error), NULL, Error);
        
        if(!Compiled)
        {
            switch(Type)
            {
                case GL_VERTEX_SHADER:
                {
                    DEBUG_LOG("Vertex Shader Error: %s", Error);
                } break;
                
                case GL_FRAGMENT_SHADER:
                {
                    DEBUG_LOG("Fragment Shader Error: %s", Error);
                } break;
                
                case GL_GEOMETRY_SHADER:
                {
                    DEBUG_LOG("Geometry Shader Error: %s", Error);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }
        
        glDeleteShader(Shader);
        return 0;
    }
    
    return Shader;
}

GLuint CreateShaderProgram(const GLchar** VSCode, u32 VSCodeCount, const GLchar** FSCode, u32 FSCodeCount, 
                           const GLchar** GSCode=NULL, u32 GSCodeCount=0)
{   
    GLuint VertexShader = CompileShader(VSCode, VSCodeCount, GL_VERTEX_SHADER);
    GLuint FragmentShader = CompileShader(FSCode, FSCodeCount, GL_FRAGMENT_SHADER);
    
    GLuint GeometryShader = 0;    
    if(GSCodeCount)    
        GeometryShader = CompileShader(GSCode, GSCodeCount, GL_GEOMETRY_SHADER);    
    
    if(!VertexShader || !FragmentShader)
        return 0;
    
    if(GSCodeCount && !GeometryShader)
        return 0;
    
    GLuint Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    
    if(GeometryShader)
        glAttachShader(Program, GeometryShader);
    
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
    
    if(GeometryShader)
        glDetachShader(Program, GeometryShader);
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    
    if(GeometryShader)
        glDeleteShader(GeometryShader);
    
    return Program;
}

inline char* ShaderDefinesLighting()
{
    char* Result = "#define HAS_LIGHTING\n";
    return Result;
}

inline char* ShaderDefinesSkinning(u32 MaxJointCount)
{
    char* Result = FormatString("#define HAS_SKINNING\n#define MAX_JOINT_COUNT %d\n", MaxJointCount).Data;
    return Result;
}

inline char* ShaderDefinesTextures()
{
    char* Result = "#define HAS_TEXTURES\n";
    return Result;
}

inline char* ShaderDefinesDiffuseColor()
{
    char* Result = "#define DIFFUSE_COLOR\n";
    return Result;
}

inline char* ShaderDefinesDiffuseTexture()
{
    char* Result = "#define DIFFUSE_TEXTURE\n";
    return Result;
}

inline char* ShaderDefinesSpecularColor()
{
    char* Result = "#define SPECULAR_COLOR\n#define SPECULAR_SHININESS\n";
    return Result;
}

inline char* ShaderDefinesSpecularTexture()
{
    char* Result = "#define SPECULAR_TEXTURE\n#define SPECULAR_SHININESS\n";
    return Result;
}

inline char* ShaderDefinesShadowOutput()
{
    char* Result = "#define SHADOW_OUTPUT\n";
    return Result;
}

inline char* ShaderDefinesMaxDirectionalLightCount(u32 DirectionalLightCount)
{
    char* Result = FormatString("#define MAX_DIRECTIONAL_LIGHT_COUNT %d\n", DirectionalLightCount).Data;
    return Result;
}

inline char* ShaderDefinesMaxPointLightCount(u32 PointLightCount)
{
    char* Result = FormatString("#define MAX_POINT_LIGHT_COUNT %d\n", PointLightCount).Data;
    return Result;
}

mvp_uniforms GetMVPUniforms(GLint Program)
{
    mvp_uniforms Result;
    Result.ViewProjection = glGetUniformLocation(Program, "ViewProjection");    
    Result.Model          = glGetUniformLocation(Program, "Model");
    return Result;
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
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_Simple};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.ColorUniform = glGetUniformLocation(Program, "Color");
    }
    
    return Result;
}

texture_shader CreateTextureShader()
{
    texture_shader Result = {};
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), FragmentShader_Simple};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        Result.MVPUniforms = GetMVPUniforms(Program);
    }
    return Result;
}

color_skinning_shader CreateColorSkinningShader()
{
    color_skinning_shader Result = {};
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_Simple};    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.ColorUniform = glGetUniformLocation(Program, "Color");
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
    }
    
    return Result;    
}

texture_skinning_shader CreateTextureSkinningShader()
{
    texture_skinning_shader Result = {};
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), FragmentShader_Simple};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
    }
    
    return Result;
}

lambertian_color_shader CreateLambertianColorShader()
{
    lambertian_color_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesDiffuseColor(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.DiffuseColorUniform = glGetUniformLocation(Program, "DiffuseColor");
        
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

lambertian_texture_shader CreateLambertianTextureShader()
{
    lambertian_texture_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), MaxDirectionalLightString, ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

lambertian_color_skinning_shader CreateLambertianColorSkinningShader()
{
    lambertian_color_skinning_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesLighting(), ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesDiffuseColor(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.DiffuseColorUniform = glGetUniformLocation(Program, "DiffuseColor");
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        Result.LightIndex    = glGetUniformBlockIndex(Program, "LightBuffer");
        
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);    
    }
    
    return Result;
}

lambertian_texture_skinning_shader CreateLambertianTextureSkinningShader()
{
    lambertian_texture_skinning_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

phong_color_shader CreatePhongColorShader()
{
    phong_color_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, MaxDirectionalLightString, MaxPointLightString, ShaderDefinesDiffuseColor(), ShaderDefinesSpecularColor(), FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);        
        Result.DiffuseColorUniform  = glGetUniformLocation(Program, "DiffuseColor");
        Result.SpecularColorUniform = glGetUniformLocation(Program, "SpecularColor");
        Result.ShininessUniform     = glGetUniformLocation(Program, "Shininess");
        Result.ViewPositionUniform  = glGetUniformLocation(Program, "ViewPosition");
        
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

phong_texture_shader CreatePhongTextureShader()
{
    phong_texture_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, MaxDirectionalLightString, MaxPointLightString, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularTexture(), FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.ShininessUniform    = glGetUniformLocation(Program, "Shininess");
        Result.DiffuseUniform      = glGetUniformLocation(Program, "DiffuseTexture");
        Result.SpecularUniform     = glGetUniformLocation(Program, "SpecularTexture");
        Result.ViewPositionUniform = glGetUniformLocation(Program, "ViewPosition");
        Result.ShadowMapUniform    = glGetUniformLocation(Program, "ShadowMap");
        
        Result.LightViewProjectionIndex = glGetUniformBlockIndex(Program, "LightViewProjectionBuffer");
        Result.LightIndex               = glGetUniformBlockIndex(Program, "LightBuffer");
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightViewProjectionIndex, LIGHT_VIEW_PROJECTION_INDEX); 
    }
    
    return Result;
}

phong_color_skinning_shader CreatePhongColorSkinningShader()
{
    phong_color_skinning_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[]   = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesLighting(), ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, MaxDirectionalLightString, MaxPointLightString, ShaderDefinesDiffuseColor(), ShaderDefinesSpecularColor(), FragmentShader_Lighting};        
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.DiffuseColorUniform  = glGetUniformLocation(Program, "DiffuseColor");
        Result.SpecularColorUniform = glGetUniformLocation(Program, "SpecularColor");
        Result.ShininessUniform     = glGetUniformLocation(Program, "Shininess");
        Result.ViewPositionUniform  = glGetUniformLocation(Program, "ViewPosition");
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        Result.LightIndex    = glGetUniformBlockIndex(Program, "LightBuffer");
        
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);                
    }
    return Result;
}

phong_texture_skinning_shader CreatePhongTextureSkinningShader()
{
    phong_texture_skinning_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), ShaderDefinesSkinning(MAX_JOINT_COUNT), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, MaxDirectionalLightString, MaxPointLightString, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularTexture(), FragmentShader_Lighting};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        Result.MVPUniforms = GetMVPUniforms(Program);
        Result.DiffuseUniform      = glGetUniformLocation(Program, "DiffuseTexture");
        Result.SpecularUniform     = glGetUniformLocation(Program, "SpecularTexture");
        Result.ShininessUniform    = glGetUniformLocation(Program, "Shininess");
        Result.ViewPositionUniform = glGetUniformLocation(Program, "ViewPosition");        
        
        Result.SkinningIndex = glGetUniformBlockIndex(Program, "SkinningBuffer");
        Result.LightIndex = glGetUniformBlockIndex(Program, "LightBuffer");
        
        glUniformBlockBinding(Program, Result.SkinningIndex, SKINNING_BUFFER_INDEX);
        glUniformBlockBinding(Program, Result.LightIndex, LIGHT_BUFFER_INDEX);
    }
    
    return Result;
}

shadow_map_shader CreateShadowMapShader()
{
    shadow_map_shader Result = {};
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_None};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;        
        Result.MVPUniforms = GetMVPUniforms(Program);               
    }
    
    return Result;
}