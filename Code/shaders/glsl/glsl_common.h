#ifndef COMMON_H
#define COMMON_H

#define f32 float
#define v2f vec2
#define v3f vec3
#define v4f vec4
#define c3 v3f
#define c4 v4f
#define m3 mat3
#define m4 mat4

//TODO(JJ): For opengl I think we need to use gl_VertexID instead of gl_VertexIndex
#define VertexIndex gl_VertexIndex

v4f VulkanTransform(v4f V)
{
    V.y = -V.y;
    V.z = (V.z + V.w) * 0.5f;
    return V;
}

void CreateBasis(v3f Z, out v3f X, out v3f Y)
{
    Z = normalize(Z);
    X = -cross(Z, v3f(0.0f, 1.0f, 0.0f));
    if((abs(X.x) < 1e-6f) && (abs(X.y) < 1e-6f) && (abs(X.z) < 1e-6f))
    {
        X = -cross(Z, v3f(0.0f, 0.0f, 1.0f));
    }    
    
    X = normalize(X);
    Y = cross(Z, X);
}

#ifdef GEOMETRY
void OutputVertex(v4f V)
{
    gl_Position = V;
    EmitVertex();
}

void OutputBoxResults(m4 Proj, m4 View, v4f V[8])
{
    m4 ViewProj = Proj*View;
    
    v4f Outputs[8];
    
    Outputs[0] = VulkanTransform(ViewProj*V[0]);
    Outputs[1] = VulkanTransform(ViewProj*V[1]);
    Outputs[2] = VulkanTransform(ViewProj*V[2]);
    Outputs[3] = VulkanTransform(ViewProj*V[3]);
    
    Outputs[4] = VulkanTransform(ViewProj*V[4]);
    Outputs[5] = VulkanTransform(ViewProj*V[5]);
    Outputs[6] = VulkanTransform(ViewProj*V[6]);
    Outputs[7] = VulkanTransform(ViewProj*V[7]);
    
    OutputVertex(Outputs[1]);
    OutputVertex(Outputs[2]);
    OutputVertex(Outputs[0]);
    OutputVertex(Outputs[3]);
    
    EndPrimitive();
    OutputVertex(Outputs[4]);
    OutputVertex(Outputs[7]);
    OutputVertex(Outputs[1]);
    OutputVertex(Outputs[2]);
    
    EndPrimitive();
    OutputVertex(Outputs[5]);
    OutputVertex(Outputs[6]);
    OutputVertex(Outputs[4]);
    OutputVertex(Outputs[7]);
    
    EndPrimitive();
    OutputVertex(Outputs[0]);
    OutputVertex(Outputs[3]);
    OutputVertex(Outputs[5]);
    OutputVertex(Outputs[6]);
    
    EndPrimitive();
    OutputVertex(Outputs[2]);
    OutputVertex(Outputs[7]);
    OutputVertex(Outputs[3]);
    OutputVertex(Outputs[6]);
    
    EndPrimitive();
    OutputVertex(Outputs[4]);
    OutputVertex(Outputs[1]);
    OutputVertex(Outputs[5]);
    OutputVertex(Outputs[0]);
}
#endif

#endif