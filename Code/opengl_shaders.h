#ifndef OPENGL_SHADERS_H
#define OPENGL_SHADERS_H

struct base_shader
{
    b32 Valid;
    GLuint Program;
};

struct imgui_shader : public base_shader
{    
    GLint ProjectionUniform;    
};

struct mvp_uniforms
{
    GLint ViewProjection;    
    GLint Model;
};

struct color_shader : public base_shader
{    
    mvp_uniforms MVPUniforms;    
    GLint ColorUniform;       
};

struct texture_shader : public base_shader
{
    mvp_uniforms MVPUniforms;    
};

struct color_skinning_shader : public base_shader
{   
    mvp_uniforms MVPUniforms;    
    GLint ColorUniform;
    
    GLint SkinningIndex;    
};

struct texture_skinning_shader : public base_shader
{
    mvp_uniforms MVPUniforms;    
    GLint SkinningIndex;
};

struct lambertian_color_shader : public base_shader
{    
    mvp_uniforms MVPUniforms;    
    GLint DiffuseColorUniform;    
    
    GLint LightIndex;
};

struct lambertian_texture_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint LightIndex;
};

struct lambertian_color_skinning_shader : public base_shader
{    
    mvp_uniforms MVPUniforms;
    GLint DiffuseColorUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;    
};

struct lambertian_texture_skinning_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint SkinningIndex;
    GLint LightIndex;
};

struct phong_color_shader : public base_shader
{   
    mvp_uniforms MVPUniforms;    
    GLint DiffuseColorUniform;
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    GLint ViewPositionUniform;
    
    GLint LightIndex;    
};

struct phong_texture_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint ShininessUniform;
    GLint DiffuseUniform;
    GLint SpecularUniform;
    GLint ViewPositionUniform;    
    GLint ShadowMapUniform;
    
    GLint LightIndex;
    GLint LightViewProjectionIndex;
};

struct phong_color_skinning_shader : public base_shader
{        
    mvp_uniforms MVPUniforms;
    GLint DiffuseColorUniform;        
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    GLint ViewPositionUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;    
};

struct phong_texture_skinning_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint ShininessUniform;
    GLint DiffuseUniform;
    GLint SpecularUniform;
    GLint ViewPositionUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;    
};


//TODO(JJ): Will probably need skinning versions of these shaders
struct shadow_map_shader : public base_shader
{
    mvp_uniforms MVPUniforms;        
};

struct onmi_shader_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint LightViewProjectionUniforms[6];
    GLint LightPositionUniform;
    GLint FarPlaneDistanceUniform;
};

#endif