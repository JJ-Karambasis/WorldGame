#version 450
#include "../glsl_common.h"

#ifdef VERTEX

layout(set = 0, binding = 0) uniform camera_buffer
{
    m4 Proj;
    m4 View;
};

layout(push_constant) uniform quad_buffer
{
    v4f Vertices[4];
};

int Indices[6] = 
{
    0, 1, 2,
    0, 2, 3
};

void main()
{
    gl_Position = VulkanTransform(Proj*View*Vertices[Indices[VertexIndex]]);
}

#endif

#ifdef FRAGMENT

layout(push_constant) uniform color_constant
{
    layout(offset = 64) c4 Color;
};

layout(location = 0) out c4 FragColor;
void main()
{
    FragColor = Color;
}

#endif