#version 450
#include "../glsl_common.h"

#ifdef VERTEX
void main()
{
    gl_Position = v4f(0.0f, 0.0f, 0.0f, 1.0f);
}
#endif

#ifdef GEOMETRY
layout(set = 0, binding = 0) uniform camera_buffer
{
    m4 Proj;
    m4 View;
};

layout(push_constant) uniform point_buffer
{
    v4f Transform; //NOTE(EVERYONE): w is point size
};

layout (points) in;
layout(triangle_strip, max_vertices = 24) out;

void main()
{
    v3f BoxCenter = Transform.xyz;
    v3f BoxHalfDim = v3f(Transform.w, Transform.w, Transform.w)*0.5f;
    
    v4f V[8] =  
    {
        v4f(BoxCenter.x - BoxHalfDim.x, BoxCenter.y - BoxHalfDim.y, BoxCenter.z + BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x + BoxHalfDim.x, BoxCenter.y - BoxHalfDim.y, BoxCenter.z + BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x + BoxHalfDim.x, BoxCenter.y + BoxHalfDim.y, BoxCenter.z + BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x - BoxHalfDim.x, BoxCenter.y + BoxHalfDim.y, BoxCenter.z + BoxHalfDim.z, 1.0f),
        
        v4f(BoxCenter.x + BoxHalfDim.x, BoxCenter.y - BoxHalfDim.y, BoxCenter.z - BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x - BoxHalfDim.x, BoxCenter.y - BoxHalfDim.y, BoxCenter.z - BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x - BoxHalfDim.x, BoxCenter.y + BoxHalfDim.y, BoxCenter.z - BoxHalfDim.z, 1.0f),
        v4f(BoxCenter.x + BoxHalfDim.x, BoxCenter.y + BoxHalfDim.y, BoxCenter.z - BoxHalfDim.z, 1.0f)
    };
    
    OutputBoxResults(Proj, View, V);
}
#endif

#ifdef FRAGMENT
layout(push_constant) uniform color_constant
{
    layout(offset = 16) c4 Color;
};

layout(location = 0) out c4 FragColor;

void main()
{
    FragColor = Color;
}
#endif