global const char* VertexShader_ImGui = R"(
out v2f PixelUV;
out c4  PixelColor;

uniform m4 Projection;

void main()
{
    PixelUV = UV;
    PixelColor = Color;
    PixelColor.rgb = pow(PixelColor.rgb, v3f(2.2f));
    gl_Position = Projection*v4f(Position2D, 0, 1);
}

)";

global const char* VertexShader_LocalToClip = R"(
uniform m4 Model;
uniform m4 ViewProjection;

#ifdef HAS_SKINNING
layout (std140) uniform SkinningBuffer
{
    m4 Joints[MAX_JOINT_COUNT];    
};
#endif

#ifdef SHADOW_POSITION_OMNI
out v3f WorldPosition;
#endif

#ifdef HAS_LIGHTING
out v3f PixelWorldPosition;
out v3f PixelWorldNormal;

out v4f PixelLightPositions[MAX_DIRECTIONAL_LIGHT_COUNT];
layout (std140) uniform LightViewProjectionBuffer
{
    m4 LightViewProjection[MAX_DIRECTIONAL_LIGHT_COUNT];
};
#endif

#ifdef HAS_TEXTURES
out v2f PixelUV;
#endif

void main()
{
#ifdef HAS_SKINNING
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

#ifdef HAS_LIGHTING
    v3f VertexN = v3f(0);
    VertexN += W0*(m3(Joints[I0])*Normal);
    VertexN += W1*(m3(Joints[I1])*Normal);
    VertexN += W2*(m3(Joints[I2])*Normal);
    VertexN += W3*(m3(Joints[I3])*Normal);
#endif

#else    
    v4f VertexP = v4f(Position, 1.0f);

#ifdef HAS_LIGHTING
    v3f VertexN = Normal;
#endif

#endif    
    
    v3f WorldSpacePosition = v3f(Model*VertexP);
#ifdef HAS_LIGHTING
    PixelWorldPosition = WorldSpacePosition;
    PixelWorldNormal = m3(transpose(inverse(Model)))*VertexN;

    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < MAX_DIRECTIONAL_LIGHT_COUNT; DirectionalLightIndex++)    
        PixelLightPositions[DirectionalLightIndex] = LightViewProjection[DirectionalLightIndex]*v4f(WorldSpacePosition, 1.0f);            
#endif

#ifdef HAS_TEXTURES
    PixelUV = UV;
#endif

#ifdef SHADOW_OUTPUT_OMNI
    PixelWorldPosition = WorldSpacePosition;
    gl_Position = v4f(WorldSpacePosition, 1.0f);
#else
    gl_Position = ViewProjection*v4f(WorldSpacePosition, 1.0f);
#endif
}

)";