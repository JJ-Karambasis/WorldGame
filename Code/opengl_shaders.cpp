global const char* Shader_Header = R"(
#version 330 core
#define v4f vec4
#define v3f vec3
#define v2f vec2
#define m4 mat4
#define m3 mat3
#define c4 v4f
#define c3 v3f
#define f32 float
#define f64 double
#define u32 uint
#define i32 int


)";

global const char* VertexShader_StandardWorldSpaceToClipSpace = R"(

#define LIGHT_SHADING %d
#define POSITION_SHADING %d
#define SKELETON_LIGHT_SHADING %d

layout (location = 0) in v3f P;

uniform m4 Projection;
uniform m4 View;
uniform m4 Model;

#if SKELETON_LIGHT_SHADING || LIGHT_SHADING
layout (location = 1) in v3f N;
#endif

#if SKELETON_LIGHT_SHADING
layout (location = 2) in u32 JointI;
layout (location = 3) in v4f JointW;

#define MAX_JOINT_COUNT %d

layout (std140) uniform SkinningBuffer
{
    m4 Joints[MAX_JOINT_COUNT];    
};

#endif

#if LIGHT_SHADING || SKELETON_LIGHT_SHADING
out v3f ViewP;
out v3f ViewN;

void main()
{
    v4f VertexP = v4f(P, 1.0f);
    v3f VertexN = N;

    #if SKELETON_LIGHT_SHADING

    u32 I0 = (JointI >> 0)  & u32(0x000000FF);
    u32 I1 = (JointI >> 8)  & u32(0x000000FF);
    u32 I2 = (JointI >> 16) & u32(0x000000FF);
    u32 I3 = (JointI >> 24) & u32(0x000000FF);

    f32 W0 = JointW[0];
    f32 W1 = JointW[1];
    f32 W2 = JointW[2];
    f32 W3 = JointW[3];

    VertexP = v4f(0);
    VertexP += W0*(Joints[I0]*v4f(P, 1.0f));
    VertexP += W1*(Joints[I1]*v4f(P, 1.0f));
    VertexP += W2*(Joints[I2]*v4f(P, 1.0f));
    VertexP += W3*(Joints[I3]*v4f(P, 1.0f));

    VertexN = v3f(0);
    VertexN += W0*(m3(Joints[I0])*N);
    VertexN += W1*(m3(Joints[I1])*N);
    VertexN += W2*(m3(Joints[I2])*N);
    VertexN += W3*(m3(Joints[I3])*N);

    #endif


    m4 ViewModel = View*Model;
    ViewP = v3f(ViewModel*VertexP);
    ViewN = m3(transpose(inverse(ViewModel)))*VertexN;
    gl_Position = Projection*v4f(ViewP, 1.0f);
}
#endif

#if POSITION_SHADING
void main()
{
    gl_Position = Projection*View*Model*v4f(P, 1.0f);
}
#endif

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
    i32 SpecularStrength = 8;
    
    f32 NDotL = max(dot(N, L), 0.0f);
    f32 NDotH = max(dot(N, H), 0.0f);
    
    c3 Diffuse = c3(Color)*NDotL;
    c3 Specular = pow(NDotH, SpecularStrength)*LightColor*0.5f;
    
    c3 FinalColor = Diffuse+Specular;
    
    FragColor = c4(FinalColor, 1.0f);
}

)";

global const char* VertexShader_ImGui = R"(

layout (location = 0) in v2f P;
layout (location = 1) in v2f UV;
layout (location = 2) in c4  C;

out v2f FragUV;
out c4  FragColor;

uniform m4 Projection;

void main()
{
    FragUV = UV;
    FragColor = C;
    gl_Position = Projection*v4f(P, 0, 1);
}

)";

global const char* FragmentShader_ImGui = R"(

in v2f FragUV;
in c4 FragColor;

out c4 FinalColor;

uniform sampler2D Texture;

void main()
{
    FinalColor = FragColor * texture(Texture, FragUV.st);
}

)";

global const char* VertexShader_Quad = R"(

uniform m4 Projection;
uniform m4 View;
uniform v3f Positions[4];

u32 Indices[6] = u32[6](0, 1, 2, 0, 2, 3);

void main()
{
    gl_Position = Projection*View*v4f(Positions[Indices[gl_VertexID]], 1.0f);
}

)";

global const char* FragmentShader_SimpleColor = R"(

out c4 FinalColor;

uniform c4 Color;

void main()
{
    FinalColor = Color;
}

)";