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
    GLint Projection;
    GLint View;
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
    
    GLint LightIndex;    
};

struct phong_texture_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint ShininessUniform;
    GLint DiffuseUniform;
    GLint SpecularUniform;
    
    GLint LightIndex;
};

struct phong_color_skinning_shader : public base_shader
{        
    mvp_uniforms MVPUniforms;
    GLint DiffuseColorUniform;        
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;    
};

struct phong_texture_skinning_shader : public base_shader
{
    mvp_uniforms MVPUniforms;
    GLint ShininessUniform;
    GLint DiffuseUniform;
    GLint SpecularUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;
};

#endif