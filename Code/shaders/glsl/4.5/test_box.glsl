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

v4f Vertices[8] = v4f[8]
(
 v4f(-0.5f, -0.5f, 1.0f, 1.0f),    
 v4f( 0.5f, -0.5f, 1.0f, 1.0f),
 v4f( 0.5f,  0.5f, 1.0f, 1.0f),
 v4f(-0.5f,  0.5f, 1.0f, 1.0f),
 
 v4f( 0.5f, -0.5f, 0.0f, 1.0f),
 v4f(-0.5f, -0.5f, 0.0f, 1.0f),
 v4f(-0.5f,  0.5f, 0.0f, 1.0f),
 v4f( 0.5f,  0.5f, 0.0f, 1.0f)
);

int Indices[36] = int[36]
(
 0, 1, 2, 
 0, 2, 3,
 
 1, 4, 7,
 1, 7, 2,
 
 4, 5, 6,
 4, 6, 7,
 
 5, 0, 3, 
 5, 3, 6,
 
 3, 2, 7,
 3, 7, 6, 
 
 4, 5, 0, 
 4, 0, 1  
);

void main()
{        
    gl_Position = Proj*View*Model*Vertices[Indices[VertexIndex]];        
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