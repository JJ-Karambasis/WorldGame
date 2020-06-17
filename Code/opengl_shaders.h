#ifndef OPENGL_SHADERS_H
#define OPENGL_SHADERS_H

struct imgui_shader
{
    GLuint Program;
    
    GLint ProjectionUniform;    
};

struct color_shader
{
    GLuint Program;
    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;       
};

struct color_skinning_shader
{
    GLuint Program;
        
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;
        
    GLint SkinningIndex;    
};

struct phong_color_shader
{
    GLuint Program;
    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;
    
    GLint LightIndex;    
};

struct phong_color_skinning_shader
{
    GLuint Program;
    
    GLint ProjectionUniform;
    GLint ViewUniform;
    GLint ModelUniform;
    GLint ColorUniform;        
    
    GLint SkinningIndex;
    GLint LightIndex;    
};

#endif