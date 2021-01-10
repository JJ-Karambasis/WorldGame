#ifndef OPENGL_SHADERS_H
#define OPENGL_SHADERS_H

struct base_shader
{
    ak_bool Valid;
    GLuint Program;
};

struct imgui_shader : public base_shader
{    
    GLint ProjectionUniform;    
};

struct color_shader : public base_shader
{    
    GLint ModelUniform;
    GLint ViewProjectionUniform;     
    GLint ColorUniform;       
    GLint AlphaUniform;
};

struct texture_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;     
    GLint AlphaUniform;
};

struct lambertian_color_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;     
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;    
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;            
    GLint AlphaUniform;
};

struct lambertian_texture_shader : public base_shader
{ 
    GLint ModelUniform;
    GLint ViewProjectionUniform;               
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;            
    GLint AlphaUniform;
};

struct lambertian_color_normal_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct lambertian_texture_normal_map_shader  : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dcon_scon_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;        
    GLint AlphaUniform;
};

struct phong_dcon_scon_normal_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;
    GLint SpecularColorUniform;    
    GLint ShininessUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;        
    GLint AlphaUniform;
};

struct phong_dcon_stex_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;
    GLint SpecularTextureUniform;
    GLint ShininessUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dcon_stex_normal_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseColorUniform;
    GLint SpecularTextureUniform;    
    GLint ShininessUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dtex_scon_shader : public base_shader
{    
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dtex_scon_normal_map_shader : public base_shader
{    
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint SpecularColorUniform;    
    GLint ShininessUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dtex_stex_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint SpecularTextureUniform;    
    GLint ShininessUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

struct phong_dtex_stex_normal_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint DiffuseTextureUniform;
    GLint SpecularTextureUniform;        
    GLint ShininessUniform;
    GLint NormalMapUniform;
    GLint ShadowMapUniform;
    GLint OmniShadowMapUniform;
    GLint AlphaUniform;
};

//TODO(JJ): Will probably need skinning versions of these shaders
struct shadow_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
};

struct omni_shadow_map_shader : public base_shader
{
    GLint ModelUniform;
    GLint ViewProjectionUniform;
    GLint ViewPositionUniform;
    GLint FarPlaneDistanceUniform;
};

#endif