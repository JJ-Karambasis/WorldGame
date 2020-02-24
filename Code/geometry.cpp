triangle_mesh CreateBoxMesh(arena* Storage)
{
    triangle_mesh Result = {};
    
    v3f Vertices[8] = 
    {
        V3(-0.5f, -0.5f, 1.0f),    
        V3( 0.5f, -0.5f, 1.0f),
        V3( 0.5f,  0.5f, 1.0f),
        V3(-0.5f,  0.5f, 1.0f),
        
        V3( 0.5f, -0.5f, 0.0f),
        V3(-0.5f, -0.5f, 0.0f),
        V3(-0.5f,  0.5f, 0.0f),
        V3( 0.5f,  0.5f, 0.0f)
    };
    
    triangle* Triangles = PushArray(Storage, 12, triangle, Clear, 0);
    
    Triangles[0].P[0]  = Vertices[0]; 
    Triangles[0].P[1]  = Vertices[1]; 
    Triangles[0].P[2]  = Vertices[2];
    
    Triangles[1].P[0]  = Vertices[0];
    Triangles[1].P[1]  = Vertices[2];
    Triangles[1].P[2]  = Vertices[3];
    
    Triangles[2].P[0]  = Vertices[1];
    Triangles[2].P[1]  = Vertices[4];
    Triangles[2].P[2]  = Vertices[7];
    
    Triangles[3].P[0]  = Vertices[1];
    Triangles[3].P[1]  = Vertices[7];
    Triangles[3].P[2]  = Vertices[2];
    
    Triangles[4].P[0]  = Vertices[4];
    Triangles[4].P[1]  = Vertices[5];
    Triangles[4].P[2]  = Vertices[6];
    
    Triangles[5].P[0]  = Vertices[4];
    Triangles[5].P[1]  = Vertices[6];
    Triangles[5].P[2]  = Vertices[7];
    
    Triangles[6].P[0]  = Vertices[5];
    Triangles[6].P[1]  = Vertices[0];
    Triangles[6].P[2]  = Vertices[3];
    
    Triangles[7].P[0]  = Vertices[5];
    Triangles[7].P[1]  = Vertices[3];
    Triangles[7].P[2]  = Vertices[6];
    
    Triangles[8].P[0]  = Vertices[3];
    Triangles[8].P[1]  = Vertices[2];
    Triangles[8].P[2]  = Vertices[7];
    
    Triangles[9].P[0]  = Vertices[3];
    Triangles[9].P[1]  = Vertices[7];
    Triangles[9].P[2]  = Vertices[6];
    
    Triangles[10].P[0] = Vertices[4];
    Triangles[10].P[1] = Vertices[1];
    Triangles[10].P[2] = Vertices[0];
    
    Triangles[11].P[0] = Vertices[4];
    Triangles[11].P[1] = Vertices[0];
    Triangles[11].P[2] = Vertices[5];
    
    Triangles[0].AdjTriangles[0]  = &Triangles[1];
    Triangles[0].AdjTriangles[1]  = &Triangles[3];
    Triangles[0].AdjTriangles[2]  = &Triangles[10];
    
    Triangles[1].AdjTriangles[0]  = &Triangles[0];
    Triangles[1].AdjTriangles[1]  = &Triangles[6];
    Triangles[1].AdjTriangles[2]  = &Triangles[8];
    
    Triangles[2].AdjTriangles[0]  = &Triangles[3];
    Triangles[2].AdjTriangles[1]  = &Triangles[10];
    Triangles[2].AdjTriangles[2]  = &Triangles[5];
    
    Triangles[3].AdjTriangles[0]  = &Triangles[2];
    Triangles[3].AdjTriangles[1]  = &Triangles[0];
    Triangles[3].AdjTriangles[2]  = &Triangles[8];
    
    Triangles[4].AdjTriangles[0]  = &Triangles[11];
    Triangles[4].AdjTriangles[1]  = &Triangles[7];
    Triangles[4].AdjTriangles[2]  = &Triangles[5];
    
    Triangles[5].AdjTriangles[0]  = &Triangles[4];
    Triangles[5].AdjTriangles[1]  = &Triangles[9];
    Triangles[5].AdjTriangles[2]  = &Triangles[2];
    
    Triangles[6].AdjTriangles[0]  = &Triangles[7];
    Triangles[6].AdjTriangles[1]  = &Triangles[1];
    Triangles[6].AdjTriangles[2]  = &Triangles[11];
    
    Triangles[7].AdjTriangles[0]  = &Triangles[6];
    Triangles[7].AdjTriangles[1]  = &Triangles[4];
    Triangles[7].AdjTriangles[2]  = &Triangles[9];
    
    Triangles[8].AdjTriangles[0]  = &Triangles[1];
    Triangles[8].AdjTriangles[1]  = &Triangles[3];
    Triangles[8].AdjTriangles[2]  = &Triangles[9];
    
    Triangles[9].AdjTriangles[0]  = &Triangles[8];
    Triangles[9].AdjTriangles[1]  = &Triangles[7];
    Triangles[9].AdjTriangles[2]  = &Triangles[5];
    
    Triangles[10].AdjTriangles[0] = &Triangles[11];
    Triangles[10].AdjTriangles[1] = &Triangles[0];
    Triangles[10].AdjTriangles[2] = &Triangles[2];
    
    Triangles[11].AdjTriangles[0] = &Triangles[10];
    Triangles[11].AdjTriangles[1] = &Triangles[6];
    Triangles[11].AdjTriangles[2] = &Triangles[4];
    
    Result.TriangleCount = 12;
    Result.Triangles = Triangles;
    
    return Result;    
}

f32 GetTriangleArea2D(v2f p0, v2f p1, v2f p2)
{
    f32 Result = Abs((p0.x*(p1.y-p2.y) + p1.x*(p2.y-p0.y) + p2.x*(p0.y-p1.y))/2.0f);
    return Result;
}

b32 IsPointInTriangle2D(v2f p, v2f a, v2f b, v2f c)
{
    //NOTE(EVERYONE): Need to make sure that the input triangle is actually not a line
    ASSERT((a != b) && (a != c));
    
#if 0     
    f32 pab = Cross2D(p-a, b-a);
    f32 pbc = Cross2D(p-b, c-b);
    
    if(!SameSign(pab, pbc)) return false;
    
    f32 pca = Cross2D(p-c, a-c);
    if(!SameSign(pab, pca)) return false;
    
    return true;
#else
    f32 Area = GetTriangleArea2D(a, b, c);    
    f32 SubArea0 = GetTriangleArea2D(p, a, b);
    f32 SubArea1 = GetTriangleArea2D(p, b, c);
    f32 SubArea2 = GetTriangleArea2D(p, c, a);    
    f32 Diff = Abs(Area - (SubArea0+SubArea1+SubArea2));
    b32 Result = Diff < 1e-6f;
    return Result;
#endif
}

b32 IsDegenerateTriangle2D(v2f a, v2f b, v2f c, u32* LineIndices)
{
    b32 ABEqual = (a == b);
    b32 ACEqual = (a == c);
    b32 BCEqual = (b == c);
    
    b32 Result = ABEqual || ACEqual || BCEqual;
    
    if(Result && LineIndices)
    {
        if(ABEqual)
        {
            ASSERT(!ACEqual && !BCEqual);
            LineIndices[0] = 0;
            LineIndices[1] = 2;            
        }    
        else if(ACEqual)
        {
            ASSERT(!ABEqual && !BCEqual);
            LineIndices[0] = 0;
            LineIndices[1] = 1;
        }
        else if(BCEqual)
        {
            ASSERT(!ABEqual && !ACEqual);
            LineIndices[0] = 0;
            LineIndices[1] = 1;
        }
    }
    
    return Result;
}

inline f32 
FindTriangleZ(v3f P0, v3f P1, v3f P2, v2f P)
{
    f32 Determinant = (P1.x-P0.x)*(P2.y-P0.y) - (P2.x-P0.x)*(P1.y-P0.y);
    ASSERT(Abs(Determinant) > 1e-6f);
    
    f32 InvDeterminant = 1.0f/Determinant;
    
    f32 Diff20 = P2.z-P0.z;
    f32 Diff10 = P1.z-P0.z;
    
    f32 A = ((P1.x-P0.x)*Diff20 - (P2.x-P0.x)*Diff10)*InvDeterminant;
    f32 B = ((P1.y-P0.y)*Diff20 - (P2.y-P0.y)*Diff10)*InvDeterminant;
    
    f32 Result = P0.z + (A * (P.y-P0.y)) - (B * (P.x-P0.x));
    return Result;
}

v3f PointTriangleClosestPoint(v3f a, v3f b, v3f c, v3f p)
{    
    v3f ab = b-a;
    v3f ac = c-a;
    v3f ap = p-a;
    
    f32 d1 = Dot(ab, ap);
    f32 d2 = Dot(ac, ap);
    if(d1 <= 0.0f && d2 <= 0.0f) return a;
    
    v3f bp = p-b;
    f32 d3 = Dot(ab, bp);
    f32 d4 = Dot(ac, bp);
    if(d3 >= 0.0f && d4 <= d3) return b;
    
    f32 vc = d1*d4 - d3*d2;
    if(vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        f32 v = d1/(d1-d3);
        return a + v*ab;
    }
    
    v3f cp = p-c;
    f32 d5 = Dot(ab, cp);
    f32 d6 = Dot(ac, cp);
    if(d6 >= 0.0f && d5 <= d6) return c;
    
    f32 vb = d5*d2 - d1*d6;
    if(vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        f32 w = d2/(d2-d6);
        return a + w*ac;
    }
    
    f32 va = d3*d6 - d5*d4;
    if(va <= 0.0f && (d4-d3) >= 0.0f && (d5-d6) >= 0.0f)
    {
        f32 w = (d4-d3)/((d4-d3)+(d5-d6));
        return b + w*(c-b);
    }
    
    f32 denom = 1.0f/(va+vb+vc);
    f32 v = vb*denom;
    f32 w = vc*denom;
    return a + ab*v + ac*w;
}