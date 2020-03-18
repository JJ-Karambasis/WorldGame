#version 450 
#include "../glsl_common.h"

#ifdef VERTEX

layout(location = 0) in v2f Position;
layout(location = 1) in v2f UV;
layout(location = 2) in c4 Color;

layout(push_constant) uniform ScaleTranslateConstant 
{
    v2f Scale;
    v2f Translate;
};

layout(location = 0) out struct 
{
    c4 Color;
    v2f UV;
} Out;


void main()
{
    Out.Color = Color;
    Out.UV = UV;
    gl_Position = v4f((Position*Scale)+Translate, 0.0f, 1.0f);    
}

#endif

#ifdef FRAGMENT

layout(set=0, binding=0) uniform sampler2D Texture;
layout(location = 0) in struct
{
    c4 Color;
    v2f UV;
} In;

layout(location = 0) out c4 FragColor;

void main()
{
    FragColor = In.Color * texture(Texture, In.UV);
}

#endif