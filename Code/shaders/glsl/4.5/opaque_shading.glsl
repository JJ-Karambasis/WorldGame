#version 450
#include "../glsl_common.h"

#ifdef VERTEX

layout(set = 0, binding=0) uniform camera_buffer
{
    m4 Proj;
    m4 View;
};

layout(push_constant) uniform model_constant
{
    m4 Model;
};

layout(location = 0) in v4f Position;
layout(location = 1) in v4f Normal;

layout(location = 0) out v3f ViewPosition;
layout(location = 1) out v3f ViewNormal;

void main()
{
    ViewPosition = v3f(View*Model*Position);
    ViewNormal = v3f(View*transpose(inverse(Model))*Normal);    
    gl_Position = VulkanTransform(Proj*v4f(ViewPosition, 1.0f));    
}

#endif

#ifdef FRAGMENT
layout(push_constant) uniform color_constant
{
    layout(offset = 64) c4 Color;
};

layout(location = 0) in v3f ViewPosition;
layout(location = 1) in v3f ViewNormal;

layout(location = 0) out c4 FragColor;
void main()
{
    c3 LightColor = v3f(1.0f, 1.0f, 1.0f);
    v3f L = v3f(0.0f, 0.0f, 1.0f);
    v3f V = -ViewPosition;
    v3f N = normalize(ViewNormal);    
    v3f H = normalize(L+V);
    int SpecularStrength = 8;
    
    f32 NDotL = max(dot(N, L), 0.0f);
    f32 NDotH = Saturate(dot(N, H));
    
    c3 Diffuse = c3(Color)*NDotL;
    c3 Specular = pow(NDotH, SpecularStrength)*LightColor*0.5f;
    
    c3 Color = Diffuse+Specular;
    FragColor = c4(Color, 1.0f);
}
#endif