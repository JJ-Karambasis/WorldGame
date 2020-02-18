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

layout(push_constant) uniform line_buffer
{
    v4f Point0; //NOTE(EVERYONE): w is line width
    v4f Point1; //NOTE(EVERYONE): w is line height
};

layout (points) in;
layout(triangle_strip, max_vertices = 24) out;

void main()
{
    v3f X, Y;
    
    CreateBasis(Point1.xyz-Point0.xyz, X, Y);
    
    f32 LineHalfWidth  = Point0.w*0.5f;
    f32 LineHalfHeight = Point1.w*0.5f;
    
    //TODO(JJ): This is probably buggy, the lines look weird. Wireframe render to debug,
    //but I think its because we aren't consistently following the vertices order that
    //we usually have for boxes (front vertices first and then back vertices). Since we
    //draw lines by points we are forming vertices around those points instead of by the 
    //proper faces
    v4f V[8] = 
    {
        v4f(Point0.xyz - X*LineHalfWidth - Y*LineHalfHeight, 1.0f),
        v4f(Point0.xyz + X*LineHalfWidth - Y*LineHalfHeight, 1.0f),
        v4f(Point0.xyz + X*LineHalfWidth + Y*LineHalfHeight, 1.0f),
        v4f(Point0.xyz - X*LineHalfWidth + Y*LineHalfHeight, 1.0f),
        
        v4f(Point1.xyz - X*LineHalfWidth - Y*LineHalfHeight, 1.0f), 
        v4f(Point1.xyz + X*LineHalfWidth - Y*LineHalfHeight, 1.0f),
        v4f(Point1.xyz + X*LineHalfWidth + Y*LineHalfHeight, 1.0f),
        v4f(Point1.xyz - X*LineHalfWidth + Y*LineHalfHeight, 1.0f)
    };
    
    OutputBoxResults(Proj, View, V);
}

#endif

#ifdef FRAGMENT
layout(push_constant) uniform color_constant
{
    layout(offset = 32) c4 Color;
};

layout(location = 0) out c4 FragColor;

void main()
{
    FragColor = Color;
}
#endif