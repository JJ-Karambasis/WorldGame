#version 450
#include "../glsl_common.h"

#ifdef VERTEX

layout(set = 0, binding = 0) uniform camera_buffer
{
    m4 Proj;
    m4 View;
};

layout(push_constant) uniform model_constant
{
    m4 Model;
};

v4f Vertices[3] = v4f[3]
(
 v4f(-0.5f, -0.5f,  0.0f, 1.0f),    
 v4f( 0.5f, -0.5f,  0.0f, 1.0f),
 v4f( 0.0f,  0.5f,  0.0f, 1.0f)
);

void main()
{        
    gl_Position = Proj*View*Model*Vertices[VertexIndex];        
    gl_Position.y = -gl_Position.y;   
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5f;
}
#endif

#ifdef FRAGMENT

layout(push_constant) uniform color_constant
{
    layout(offset = 64) c4 Color;
};

layout (location = 0) out c4 FragColor;
void main()
{
    FragColor = Color;
}
#endif