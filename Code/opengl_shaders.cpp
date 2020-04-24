global const char* Shader_Header = R"(
#version 130
#define v4f vec4
#define v3f vec3
#define v2f vec2
#define m4 mat4
#define m3 mat3
#define c4 v4f
#define c3 v3f
)";

global const char* VertexShader_StandardWorldSpaceToClipSpace = R"(
in v3f P;
in v3f N;

out v3f ResultP;
out v3f ResultN;

uniform m4 Projection;
uniform m4 View;
uniform m4 Model;

void main()
{
    ResultP = v3f(View*Model*v4f(P, 1.0f));
    ResultN = v3f(View*transpose(inverse(Model))*v4f(N, 0.0f));
    gl_Position = Proj*v4f(ResultP, 1.0f);
}

)";

global const char* PixelShader_StandardPhongShading = R"(

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
    f32 NDotH = Saturate(dot(N, H));
    
    c3 Diffuse = c3(Color)*NDotL;
    c3 Specular = pow(NDotH, SpecularStrength)*LightColor*0.5f;
    
    c3 FinalColor = Diffuse+Specular;
    FragColor = c4(FinalColor, 1.0f);
}

)";