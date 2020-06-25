global const char* GeometryShader_ShadowEmitToTextureArray = R"(

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform m4 LightViewProjection[6];
out v3f FragPosition;

void main()
{
    for(i32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
    {
        gl_Layer = FaceIndex; 
        for(i32 TriangleIndex = 0; TriangleIndex < 3; TriangleIndex++)
        {
            FragPosition = gl_in[TriangleIndex].gl_Position;
            gl_Position = LightViewProjection[FaceIndex]*v4f(FragPosition, 1.0f);
            EmitVertex();
        }
        EndPrimitive();
    }
}

)";