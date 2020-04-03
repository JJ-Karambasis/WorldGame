triangle3D_mesh CreateBoxTriangleMesh(arena* Storage)
{
    triangle3D_mesh Result = {};
    
    Result.TriangleCount = 12;
    Result.Triangles = PushArray(Storage, 12, triangle3D, Clear, 0);
    
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
    
    Result.Triangles[0]  = CreateTriangle3D(Vertices[0], Vertices[1], Vertices[2]);
    Result.Triangles[1]  = CreateTriangle3D(Vertices[0], Vertices[2], Vertices[3]);
    Result.Triangles[2]  = CreateTriangle3D(Vertices[1], Vertices[4], Vertices[7]);
    Result.Triangles[3]  = CreateTriangle3D(Vertices[1], Vertices[7], Vertices[2]);
    Result.Triangles[4]  = CreateTriangle3D(Vertices[4], Vertices[5], Vertices[6]);
    Result.Triangles[5]  = CreateTriangle3D(Vertices[4], Vertices[6], Vertices[7]);
    Result.Triangles[6]  = CreateTriangle3D(Vertices[5], Vertices[0], Vertices[3]);
    Result.Triangles[7]  = CreateTriangle3D(Vertices[5], Vertices[3], Vertices[6]);
    Result.Triangles[8]  = CreateTriangle3D(Vertices[3], Vertices[2], Vertices[7]);
    Result.Triangles[9]  = CreateTriangle3D(Vertices[3], Vertices[7], Vertices[6]);
    Result.Triangles[10] = CreateTriangle3D(Vertices[4], Vertices[1], Vertices[0]);
    Result.Triangles[11] = CreateTriangle3D(Vertices[4], Vertices[0], Vertices[5]);
    
    return Result;    
}

f32 GetTriangleArea2D(v2f P0, v2f P1, v2f P2)
{
    f32 Result = Abs((P0.x*(P1.y-P2.y) + P1.x*(P2.y-P0.y) + P2.x*(P0.y-P1.y))*0.5f);
    return Result;
}

f32 GetTriangleArea2D(triangle2D Triangle)
{
    f32 Result = GetTriangleArea2D(Triangle.P[0], Triangle.P[1], Triangle.P[2]);
    return Result;
}

b32 IsPointInTriangle2D(v2f P0, v2f P1, v2f P2, v2f P)
{    
    f32 A = 0.5f * (-P1.y*P2.x + P0.y*(-P1.x+P2.x) + P0.x*(P1.y-P2.y) + P1.x*P2.y);
    f32 Sign = SIGN(A);
    
    f32 s = (P0.y*P2.x - P0.x*P2.y + (P2.y-P0.y)*P.x + (P0.x-P2.x)*P.y)*Sign;
    f32 t = (P0.x*P1.y - P0.y*P1.x + (P0.y-P1.y)*P.x + (P1.x-P0.x)*P.y)*Sign;
    
    b32 Result = (s > 0.0f) && (t > 0.0f) && (s+t) < 2.0f*A*Sign;
    return Result;
}

b32 IsPointInTriangle2D(triangle2D Triangle, v2f P)
{
    b32 Result = IsPointInTriangle2D(Triangle.P[0], Triangle.P[1], Triangle.P[2], P);
    return Result;
}

b32 IsPointInTriangle2D(triangle3D Triangle, v2f P)
{
    b32 Result = IsPointInTriangle2D(Triangle.P[0].xy, Triangle.P[1].xy, Triangle.P[2].xy, P);
    return Result;
}

inline f32 
FindTriangleZ(v3f P0, v3f P1, v3f P2, v2f P)
{
    f32 Determinant = (P1.x-P0.x)*(P2.y-P0.y) - (P2.x-P0.x)*(P1.y-P0.y);       
    if(Determinant == 0)
        return INFINITY;
    
    f32 InvDeterminant = 1.0f/Determinant;    
    
    f32 Diff20 = P2.z-P0.z;
    f32 Diff10 = P1.z-P0.z;
    
    f32 A = ((P1.x-P0.x)*Diff20 - (P2.x-P0.x)*Diff10)*InvDeterminant;
    f32 B = ((P1.y-P0.y)*Diff20 - (P2.y-P0.y)*Diff10)*InvDeterminant;
    
    f32 Result = P0.z + (A * (P.y-P0.y)) - (B * (P.x-P0.x));
    return Result;
}

inline f32
FindTriangleZ(triangle3D Triangle, v2f P)
{
    f32 Result = FindTriangleZ(Triangle.P[0], Triangle.P[1], Triangle.P[2], P);
    return Result;
}

v3f PointTriangleClosestPoint3D(v3f P0, v3f P1, v3f P2, v3f P)
{    
    v3f P01 = P1-P0;
    v3f P02 = P2-P0;
    v3f PP0 = P-P0;
    
    f32 d1 = Dot(P01, PP0);
    f32 d2 = Dot(P02, PP0);
    if(d1 <= 0.0f && d2 <= 0.0f) return P0;
    
    v3f bp = P-P1;
    f32 d3 = Dot(P01, bp);
    f32 d4 = Dot(P02, bp);
    if(d3 >= 0.0f && d4 <= d3) return P1;
    
    f32 vc = d1*d4 - d3*d2;
    if(vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        f32 v = d1/(d1-d3);
        return P0 + v*P01;
    }
    
    v3f cp = P-P2;
    f32 d5 = Dot(P01, cp);
    f32 d6 = Dot(P02, cp);
    if(d6 >= 0.0f && d5 <= d6) return P2;
    
    f32 vb = d5*d2 - d1*d6;
    if(vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        f32 w = d2/(d2-d6);
        return P0 + w*P02;
    }
    
    f32 va = d3*d6 - d5*d4;
    if(va <= 0.0f && (d4-d3) >= 0.0f && (d5-d6) >= 0.0f)
    {
        f32 w = (d4-d3)/((d4-d3)+(d5-d6));
        return P1 + w*(P2-P1);
    }
    
    f32 denom = 1.0f/(va+vb+vc);
    f32 v = vb*denom;
    f32 w = vc*denom;
    return P0 + P01*v + P02*w;
}

v3f PointTriangleClosestPoint3D(triangle3D Triangle, v3f P)
{
    v3f Result = PointTriangleClosestPoint3D(Triangle.P[0], Triangle.P[1], Triangle.P[2], P);
    return Result;
}

b32 RayToRayIntersection2D(v2f* Result, ray2D A, ray2D B)
{
    if(!IsValidRay2D(A) || !IsValidRay2D(B))
        return false;
    
    f32 Determinant = B.Direction.x*A.Direction.y - B.Direction.y*A.Direction.x;
    if(Determinant == 0.0f)
        return false;
    
    f32 InvDeterminant = 1.0f/Determinant;
    
    f32 dx = B.Origin.x - A.Origin.x;
    f32 dy = B.Origin.y - A.Origin.y;
    
    f32 u = (dy*B.Direction.x - dx*B.Direction.y) * InvDeterminant;
    f32 v = (dy*A.Direction.x - dx*A.Direction.y) * InvDeterminant;
    
    if(u >= 0 && v >= 0)
    {
        *Result = A.Origin + A.Direction*u;
        return true;
    }    
    
    return false;
}

b32 IsPointInCircle2D(v2f CenterP, f32 CircleR, v2f P)
{
    f32 XComp = Square(P.x - CenterP.x);
    f32 YComp = Square(P.y - CenterP.y);
    b32 Result = (XComp + YComp) < Square(CircleR);
    return Result;
}

b32 IsPointInSphere3D(v3f CenterP, f32 CircleR, v3f P)
{
    f32 XComp = Square(P.x - CenterP.x);
    f32 YComp = Square(P.y - CenterP.y);
    f32 ZComp = Square(P.z - CenterP.z);
    b32 Result = (XComp + YComp + ZComp) < Square(CircleR);
    return Result;
}

b32 LineIntersections2D(v2f P1, v2f P2, v2f P3, v2f P4, f32* t, f32* u)
{
    f32 P1P2x = P1.x-P2.x;
    f32 P1P2y = P1.y-P2.y;
    f32 P3P4x = P3.x-P4.x;
    f32 P3P4y = P3.y-P4.y;    
    
    f32 Determinant = P1P2x*P3P4y - P1P2y*P3P4x;
    if(Determinant == 0.0f)
        return false;
    
    f32 InvDeterminant = 1.0f/Determinant;
    
    f32 P1P3x = P1.x-P3.x;
    f32 P1P3y = P1.y-P3.y;
    
    *t = (P1P3x*P3P4y - P1P3y*P3P4x)*InvDeterminant;
    *u = -(P1P2x*P1P3y - P1P2y*P1P3x)*InvDeterminant;
    return true;
}


b32 EdgeToEdgeIntersection2D(v2f* Result, v2f P0, v2f P1, v2f P2, v2f P3)
{    
    f32 t, u;
    if(!LineIntersections2D(P0, P1, P2, P3, &t, &u))
        return false;
    
    if(t >= 0 && t <= 1.0f && u >= 0.0f && u <= 1.0f)
    {
        *Result = P0 + t*(P1-P0);
        return true;
    }
    
    return false;    
}

b32 EdgeToEdgeIntersection2D(v2f* Result, edge2D Edge0, edge2D Edge1)
{
    return EdgeToEdgeIntersection2D(Result, Edge0.P[0], Edge0.P[1], Edge1.P[0], Edge1.P[1]);
}

v2f PointLineSegmentClosestPoint2D(v2f P0, v2f P1, v2f P)
{
    v2f Edge = P1-P0;
    
    f32 SqrLength = SquareMagnitude(Edge);
    ASSERT(SqrLength > 1e-6f);    
    f32 t = SaturateF32(Dot(P-P0, Edge) / SqrLength);
    
    v2f Result = P0 + t*Edge;
    return Result;
}

b32 IsPointInCapsule2D(v2f P0, v2f P1, f32 Radius, v2f P)
{
    v2f ClosestPoint = PointLineSegmentClosestPoint2D(P0, P1, P);
    b32 Result = SquareMagnitude(P-ClosestPoint) <= Square(Radius);
    return Result;
}

b32 IsPointOnLine2D(v2f P0, v2f P1, v2f P)
{
    f32 Perp = (P0.x-P.x)*(P1.y-P.y) - (P0.y-P.y)*(P1.x-P.x);
    
    f32 dx = P1.x-P0.x;
    f32 dy = P1.y-P0.y;
    f32 Epsilon = 0.003f * (Square(dx) + Square(dy));
    
    b32 Result = Abs(Perp) < Epsilon;
    return Result;
}

b32 IsPointOnEdge2D(v2f P0, v2f P1, v2f P)
{
    if(!(((P0.x <= P.x) && (P.x <= P1.x)) || ((P1.x <= P.x) && (P.x <= P0.x))))
        return false;
    
    if(!(((P0.y <= P.y) && (P.y <= P1.y)) || ((P1.y <= P.y) && (P.y <= P0.y))))
        return false;
    
    return IsPointOnLine2D(P0, P1, P);
}

b32 IsPointOnEdge2D(edge2D Edge, v2f P)
{
    b32 Result = IsPointOnEdge2D(Edge.P[0], Edge.P[1], P);
    return Result;
}

b32 IsPointOnEdge2D(edge3D Edge, v2f P)
{
    b32 Result = IsPointOnEdge2D(Edge.P[0].xy, Edge.P[1].xy, P);
    return Result;
}

f32 FindLineZ(v3f P0, v3f P1, v2f P)
{   
    f32 t;
    
    f32 Determinant = P1.x-P0.x;
    if(Determinant == 0.0f)
    {   
        Determinant = P1.y-P0.y;
        if(Determinant == 0)
            return INFINITY;        
        t = (P.y-P0.y)/Determinant;        
    }
    else
        t = (P.x-P0.x)/Determinant;    
    
    f32 Result = P0.z + (P1.z-P0.z)*t;
    return Result;
}

f32 FindLineZ(edge3D Line, v2f P)
{
    f32 Result = FindLineZ(Line.P[0], Line.P[1], P);
    return Result;
}