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

struct color_shader : public base_shader
{    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;       
};

struct color_skinning_shader : public base_shader
{    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;
    
    GLint SkinningIndex;    
};

struct phong_color_shader : public base_shader
{    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint SurfaceColorUniform;
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    
    GLint LightIndex;    
};

struct phong_color_skinning_shader : public base_shader
{        
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint SurfaceColorUniform;        
    GLint SpecularColorUniform;
    GLint ShininessUniform;
    
    GLint SkinningIndex;
    GLint LightIndex;    
};

#endif