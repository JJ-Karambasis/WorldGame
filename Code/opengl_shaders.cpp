global const char* Shader_Header = R"(
#version 330
#define v4f vec4
#define v3f vec3
#define v2f vec2
#define m4 mat4
#define m3 mat3
#define c4 v4f
#define c3 v3f
#define f32 float
#define f64 double
)";

global const char* VertexShader_StandardWorldSpaceToClipSpace = R"(
layout (location = 0) in v3f P;
layout (location = 1) in v3f N;

out v3f ViewP;
out v3f ViewN;

uniform m4 Projection;
uniform m4 View;
uniform m4 Model;

void main()
{
    ViewP = v3f(View*Model*v4f(P, 1.0f));
    ViewN = m3(transpose(inverse(View*Model)))*N;
    gl_Position = Projection*v4f(ViewP, 1.0f);
}

)";

global const char* FragmentShader_StandardPhongShading = R"(

in v3f ViewP;
in v3f ViewN;

out c4 FragColor;

uniform c4 Color;

void main()
{
    c3 LightColor = v3f(1.0f, 1.0f, 1.0f);
    v3f L = v3f(0.0f, 0.0f, 1.0f);
    v3f V = -ViewP;
    v3f N = normalize(ViewN);    
    v3f H = normalize(L+V);
    int SpecularStrength = 8;
    
    f32 NDotL = max(dot(N, L), 0.0f);
    f32 NDotH = max(dot(N, H), 0.0f);
    
    c3 Diffuse = c3(Color)*NDotL;
    c3 Specular = pow(NDotH, SpecularStrength)*LightColor*0.5f;
    
    c3 FinalColor = Diffuse+Specular;
    
    FragColor = c4(FinalColor, 1.0f);
}

)";