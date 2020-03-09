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

layout(location = 0) in v3f Position;

void main()
{        
    gl_Position = Proj*View*Model*v4f(Position, 1.0f);
    gl_Position = VulkanTransform(gl_Position);    
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