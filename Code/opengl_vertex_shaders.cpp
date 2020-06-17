global const char* VertexShader_ImGui = R"(
out v2f FragUV;
out c4  FragColor;

uniform m4 Projection;

void main()
{
    FragUV = UV;
    FragColor = Color;
    gl_Position = Projection*v4f(Position2D, 0, 1);
}

)";

global const char* VertexShader_LocalToClip = R"(
#define HAS_SKINNING %d
#define HAS_NORMALS  %d

uniform m4 Projection;
uniform m4 View;
uniform m4 Model;

#if HAS_SKINNING
#define MAX_JOINT_COUNT %d
layout (std140) uniform SkinningBuffer
{
    m4 Joints[MAX_JOINT_COUNT];    
};
#endif

#if HAS_NORMALS
out v3f ViewP;
out v3f ViewN;
#endif

void main()
{
#if HAS_SKINNING
    u32 I0 = (JointI >> 0)  & u32(0x000000FF);
    u32 I1 = (JointI >> 8)  & u32(0x000000FF);
    u32 I2 = (JointI >> 16) & u32(0x000000FF);
    u32 I3 = (JointI >> 24) & u32(0x000000FF);

    f32 W0 = JointW[0];
    f32 W1 = JointW[1];
    f32 W2 = JointW[2];
    f32 W3 = JointW[3];

    v4f VertexP = v4f(0);
    VertexP += W0*(Joints[I0]*v4f(Position, 1.0f));
    VertexP += W1*(Joints[I1]*v4f(Position, 1.0f));
    VertexP += W2*(Joints[I2]*v4f(Position, 1.0f));
    VertexP += W3*(Joints[I3]*v4f(Position, 1.0f));

#if HAS_NORMALS
    v3f VertexN = v3f(0);
    VertexN += W0*(m3(Joints[I0])*Normal);
    VertexN += W1*(m3(Joints[I1])*Normal);
    VertexN += W2*(m3(Joints[I2])*Normal);
    VertexN += W3*(m3(Joints[I3])*Normal);
#endif

#else
    
    v4f VertexP = v4f(Position, 1.0f);

#if HAS_NORMALS
    v3f VertexN = Normal;
#endif

#endif

    m4 ModelView = View*Model;
    
#if HAS_NORMALS
    ViewP = v3f(ModelView*VertexP);
    ViewN = m3(transpose(inverse(ModelView)))*VertexN;
#endif

    gl_Position = Projection*ModelView*VertexP;    
}

)";