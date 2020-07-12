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
layout (location = 3) in v4f Tangent;
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
#define b32 i32
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

inline char* ShaderDefinesOmniShadowMap()
{
    char* Result = "#define OMNI_SHADOW_MAP\n";
    return Result;
}

inline char* ShaderDefinesNormalMapping()
{
    char* Result = "#define HAS_NORMAL_MAPPING\n";
    return Result;
}

#define GET_UNIFORM_LOCATION(name) glGetUniformLocation(Program, name)
#define SET_UNIFORM_BLOCK(index, name) \
do \
{ \
    GLint Index = glGetUniformBlockIndex(Program, name); \
    glUniformBlockBinding(Program, Index, index); \
} while(0)

#define MVP_UNIFORMS() \
Result.ModelUniform = GET_UNIFORM_LOCATION("Model"); \
Result.ViewProjectionUniform = GET_UNIFORM_LOCATION("ViewProjection")

#define SHADOW_MAP_UNIFORMS() \
Result.ShadowMapUniform = GET_UNIFORM_LOCATION("ShadowMap"); \
Result.OmniShadowMapUniform = GET_UNIFORM_LOCATION("OmniShadowMap")

#define SET_LIGHT_UNIFORM_BLOCKS() \
SET_UNIFORM_BLOCK(LIGHT_BUFFER_INDEX, "LightBuffer"); \
SET_UNIFORM_BLOCK(LIGHT_VIEW_PROJECTION_INDEX, "LightViewProjectionBuffer")

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
        Result.ProjectionUniform  = GET_UNIFORM_LOCATION("Projection");
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
        
        MVP_UNIFORMS();        
        Result.ColorUniform = GET_UNIFORM_LOCATION("Color");
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
        MVP_UNIFORMS();        
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
        
        MVP_UNIFORMS();        
        SHADOW_MAP_UNIFORMS();
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");                
        
        SET_LIGHT_UNIFORM_BLOCKS();        
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
        
        MVP_UNIFORMS();        
        SHADOW_MAP_UNIFORMS();
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");                
        
        SET_LIGHT_UNIFORM_BLOCKS();        
    }
    
    return Result;
}

lambertian_color_normal_map_shader CreateLambertianColorNormalMapShader()
{
    lambertian_color_normal_map_shader Result = {};    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), MaxDirectionalLightString, ShaderDefinesLighting(), ShaderDefinesNormalMapping(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseColor(), ShaderDefinesNormalMapping(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();
    }
    
    return Result;
}

lambertian_texture_normal_map_shader CreateLambertianTextureNormalMapShader()
{
    lambertian_texture_normal_map_shader Result = {};    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);    
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), MaxDirectionalLightString, ShaderDefinesLighting(), ShaderDefinesNormalMapping(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesNormalMapping(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();
    }
    
    return Result;
}

phong_dcon_scon_shader CreatePhongDConSConShader()
{
    phong_dcon_scon_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesDiffuseColor(), ShaderDefinesSpecularColor(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();    
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");
        Result.SpecularColorUniform = GET_UNIFORM_LOCATION("SpecularColor");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");        
        
        SET_LIGHT_UNIFORM_BLOCKS();        
    }    
    
    return Result;
}

phong_dcon_scon_normal_map_shader CreatePhongDConSConNormalMapShader()
{
    phong_dcon_scon_normal_map_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesTextures(), MaxDirectionalLightString, ShaderDefinesLighting(), ShaderDefinesNormalMapping(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseColor(), ShaderDefinesSpecularColor(), ShaderDefinesNormalMapping(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();    
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");        
        Result.SpecularColorUniform = GET_UNIFORM_LOCATION("SpecularColor");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");      
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();        
    }    
    
    return Result;
}

phong_dcon_stex_shader CreatePhongDConSTexShader()
{
    phong_dcon_stex_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseColor(), ShaderDefinesSpecularTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");
        Result.SpecularTextureUniform = GET_UNIFORM_LOCATION("SpecularTexture");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        
        SET_LIGHT_UNIFORM_BLOCKS();        
    }
    
    return Result;
}

phong_dcon_stex_normal_map_shader CreatePhongDConSTexNormalMapShader()
{
    phong_dcon_stex_normal_map_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), ShaderDefinesNormalMapping(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesNormalMapping(), ShaderDefinesDiffuseColor(), ShaderDefinesSpecularTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseColorUniform = GET_UNIFORM_LOCATION("DiffuseColor");
        Result.SpecularTextureUniform = GET_UNIFORM_LOCATION("SpecularTexture");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();        
    }
    
    return Result;
}

phong_dtex_scon_shader CreatePhongDTexSConShader()
{
    phong_dtex_scon_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularColor(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");
        Result.SpecularColorUniform = GET_UNIFORM_LOCATION("SpecularColor");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        
        SET_LIGHT_UNIFORM_BLOCKS();
    }
    
    return Result;
}

phong_dtex_scon_normal_map_shader CreatePhongDTexSConNormalMapShader()
{
    phong_dtex_scon_normal_map_shader Result = {};
    
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesNormalMapping(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesNormalMapping(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularColor(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");
        Result.SpecularColorUniform = GET_UNIFORM_LOCATION("SpecularColor");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();
    }
    
    return Result;
}

phong_dtex_stex_shader CreatePhongDTexSTexShader()
{
    phong_dtex_stex_shader Result = {};
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");
        Result.SpecularTextureUniform = GET_UNIFORM_LOCATION("SpecularTexture");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        
        SET_LIGHT_UNIFORM_BLOCKS();
    }
    
    return Result;
}

phong_dtex_stex_normal_map_shader CreatePhongDTexSTexNormalMapShader()
{
    phong_dtex_stex_normal_map_shader Result = {};
    char* MaxDirectionalLightString = ShaderDefinesMaxDirectionalLightCount(MAX_DIRECTIONAL_LIGHT_COUNT);
    char* MaxPointLightString = ShaderDefinesMaxPointLightCount(MAX_POINT_LIGHT_COUNT);
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, MaxDirectionalLightString, ShaderDefinesTextures(), ShaderDefinesNormalMapping(), ShaderDefinesLighting(), VertexShader_LocalToClip};
    const GLchar* FragmentShaders[] = {Shader_Header, ShaderDefinesNormalMapping(), ShaderDefinesTextures(), ShaderDefinesDiffuseTexture(), ShaderDefinesSpecularTexture(), MaxDirectionalLightString, MaxPointLightString, FragmentShader_Lighting};
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));
    
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();
        SHADOW_MAP_UNIFORMS();
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("ViewPosition");
        Result.DiffuseTextureUniform = GET_UNIFORM_LOCATION("DiffuseTexture");
        Result.SpecularTextureUniform = GET_UNIFORM_LOCATION("SpecularTexture");
        Result.ShininessUniform = GET_UNIFORM_LOCATION("Shininess");
        Result.NormalMapUniform = GET_UNIFORM_LOCATION("NormalMap");
        
        SET_LIGHT_UNIFORM_BLOCKS();
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
        MVP_UNIFORMS();        
    }
    
    return Result;
}

omni_shadow_map_shader CreateOmniShadowMapShader()
{
    omni_shadow_map_shader Result = {};
    
    const GLchar* VertexShaders[] = {Shader_Header, Vertex_Attributes, ShaderDefinesOmniShadowMap(), VertexShader_LocalToClip};    
    const GLchar* FragmentShaders[] = {Shader_Header, FragmentShader_Shadow};    
    
    GLuint Program = CreateShaderProgram(VertexShaders, ARRAYCOUNT(VertexShaders), FragmentShaders, ARRAYCOUNT(FragmentShaders));                                         
    if(Program > 0)
    {
        Result.Valid = true;
        Result.Program = Program;
        
        MVP_UNIFORMS();        
        Result.ViewPositionUniform = GET_UNIFORM_LOCATION("LightPosition");
        Result.FarPlaneDistanceUniform = GET_UNIFORM_LOCATION("FarPlaneDistance");        
    }
    
    return Result;
}