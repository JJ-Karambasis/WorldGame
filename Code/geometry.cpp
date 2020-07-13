#include "gjk.cpp"

f32 SignedDistance(v3f P, plane3D Plane)
{
    f32 Result = Dot(P, Plane.Normal) + Plane.D;
    return Result;
}

b32 IsTriangle2DCW(v2f P0, v2f P1, v2f P2)
{
    f32 Det = P0.x*(P1.y-P2.y) - P0.y*(P1.x-P2.x) + (P1.x*P2.y - P1.y*P2.x);        
    b32 Result = Det < 0.0f;
    return Result;    
}

b32 IsTriangle2DCCW(v2f P0, v2f P1, v2f P2)
{
    f32 Det = P0.x*(P1.y-P2.y) - P0.y*(P1.x-P2.x) + (P1.x*P2.y - P1.y*P2.x);        
    b32 Result = Det > 0.0f;
    return Result;    
}

b32 IsTriangle3DCW(v3f P0, v3f P1, v3f P2)
{
    f32 Det = Determinant(P0, P1, P2);
    b32 Result = Det < 0.0f;
    return Result;
}

b32 IsTriangle3DCCW(v3f P0, v3f P1, v3f P2)
{
    f32 Det = Determinant(P0, P1, P2);
    b32 Result = Det > 0.0f;
    return Result;
}

b32 IsTriangle3DCW(v3f* P)
{
    b32 Result = IsTriangle3DCW(P[0], P[1], P[2]);    
    return Result;
}

b32 IsTriangle3DCCW(v3f* P)
{
    b32 Result = IsTriangle3DCCW(P[0], P[1], P[2]);    
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
    
    b32 Result = (s >= 0.0f) && (t >= 0.0f) && (s+t) <= 2.0f*A*Sign;
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

b32 IsPointInTriangle3D(v3f P0, v3f P1, v3f P2, v3f P)
{
    f32 TotalAngles = 0.0f;
        
    v3f V1 = Normalize(P-P1);
    v3f V2 = Normalize(P-P2);
    v3f V3 = Normalize(P-P0);
    
    f32 Dot1 = ClampF32(Dot(V1, V2), -1, 1);
    f32 Dot2 = ClampF32(Dot(V2, V3), -1, 1);
    f32 Dot3 = ClampF32(Dot(V3, V1), -1, 1); 
    
    f32 Angle = ACos(Dot1) + ACos(Dot2) + ACos(Dot3);
    
    if(Abs(Angle-2*PI) <= 0.005f)
        return true;
    
    return false;
}

b32 IsPointInTriangle3D(v3f* Triangle, v3f P)
{
    b32 Result = IsPointInTriangle3D(Triangle[0], Triangle[1], Triangle[2], P);
    return Result;
}

b32 IsPointProjectedInTriangle3D(v3f P0, v3f P1, v3f P2, v3f P)
{
    //NOTE(EVERYONE): Got this weird ass function from https://www.peroxide.dk/papers/collision/collision.pdf in Appendix C
    
#define WEIRD_DEFINE(a) ((u32&)a)
    
    v3f E1 = P1-P0;
    v3f E2 = P2-P0;
    
    f32 a = SquareMagnitude(E1);
    f32 b = Dot(E1, E2);
    f32 c = SquareMagnitude(E2);
    
    f32 ac_bb = (a*c)-(b*b);
    
    v3f V = V3(P.x-P0.x, P.y-P0.y, P.z-P0.z);
    
    f32 d = Dot(V, E1);
    f32 e = Dot(V, E2);
    
    f32 x = (d*c)-(e*b);
    f32 y = (e*a)-(d*b);
    f32 z = x+y - ac_bb;
    
    b32 Result = ((WEIRD_DEFINE(z)& ~(WEIRD_DEFINE(x)|WEIRD_DEFINE(y))) & 0x80000000);
    return Result;
    
#undef WEIRD_DEFINE
}

b32 IsPointProjectedInTriangle3D(v3f* Triangle, v3f P)
{
#if 1
    b32 Result = IsPointProjectedInTriangle3D(Triangle[0], Triangle[1], Triangle[2], P);
#else
    b32 Result = IsPointInTriangle3D(Triangle[0], Triangle[1], Triangle[2], P);
#endif
    return Result;
}

b32 IsPointProjectedInTriangle3D(triangle3D Triangle, v3f P)
{    
    b32 Result = IsPointProjectedInTriangle3D(Triangle.P, P);            
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
    
    if(t)
        *t = (P1P3x*P3P4y - P1P3y*P3P4x)*InvDeterminant;
    
    if(u)
        *u = -(P1P2x*P1P3y - P1P2y*P1P3x)*InvDeterminant;
    return true;
}

b32 EdgeIntersections2D(v2f P0, v2f P1, v2f P2, v2f P3, f32* t, f32* u)
{
    f32 t0, u0;
    if(!LineIntersections2D(P0, P1, P2, P3, &t0, &u0))
        return false;
    
    if((t0 >= 0.0f) && (t0 <= 1.0f) && (u0 >= 0.0f) && (u0 <= 1.0f))
    {
        if(t) *t = t0;
        if(u) *u = u0;
        return true;
    }  
    
    return false;
}

b32 EdgeCircleIntersections2D(v2f P0, v2f P1, v2f CenterP, f32 Radius, f32* t)
{
    v2f R = P1-P0;
    v2f F = P0-CenterP;
    
    f32 A = SquareMagnitude(R);
    f32 B = 2.0f*Dot(F, R);
    f32 C = SquareMagnitude(F) - Square(Radius);
    
    f32 Disc = B*B-4*A*C;
    if(Disc < 0.0f)    
        return false;
    
    f32 Root = Sqrt(Disc);
    
    f32 t0 = (-B - Root)/(2*A);
    f32 t1 = (-B + Root)/(2*A);
    
    b32 Result = false;
    if(t0 > 0.0f && t0 < 1.0f)    
    {
        t[0] = t0;        
        Result = true;
    }    
    
    if(t1 > 0.0f && t1 < 1.0f)
    {
        t[1] = t1;        
        Result = true;
    }
    
    return Result;
}

b32 CircleRectangleOverlap2D(v2f CenterP, f32 Radius, v2f Min, v2f Max)
{
    v2f HalfDim = (Max-Min)*0.5f;
    v2f Center = Min+HalfDim;
    v2f CircleDistance = Abs2f(CenterP-Min);
    
    if(CircleDistance.x > (Center.x + Radius)) return false;
    if(CircleDistance.y > (Center.y + Radius)) return false;
    
    if(CircleDistance.x <= Center.x) return true;
    if(CircleDistance.y <= Center.y) return true;
    
    f32 DistanceSqr = Square(CircleDistance.x - Center.x) + Square(CircleDistance.y - Center.y);
    return DistanceSqr <= Square(Radius);
    
}

v2f PointLineSegmentClosestPoint2D(v2f P0, v2f P1, v2f P)
{
    v2f Edge = P1-P0;
    
    f32 SqrLength = SquareMagnitude(Edge);
    ASSERT(SqrLength != 0.0f);    
    f32 t = SaturateF32(Dot(P-P0, Edge) / SqrLength);
    
    v2f Result = P0 + t*Edge;
    return Result;
}

b32 CircleEdgeOverlap2D(v2f CenterP, f32 Radius, v2f E0, v2f E1)
{
    v2f ClosestPoint = PointLineSegmentClosestPoint2D(E0, E1, CenterP);    
    b32 Result = SquareMagnitude(ClosestPoint-CenterP) <= Square(Radius);
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
    f32 Epsilon = 1e-5f * (Square(dx) + Square(dy));
    
    b32 Result = Abs(Perp) < Epsilon;
    return Result;
}

b32 IsPointOnLine2D(v2f* Line, v2f P)
{
    b32 Result = IsPointOnLine2D(Line[0], Line[1], P);
    return Result;
}

b32 IsPointOnEdge2D(v2f P0, v2f P1, v2f P)
{
    if(AreEqual(P, P1, 1e-5f) || (AreEqual(P, P0, 1e-5f)))
        return true;
    
    if(!(((P0.x <= P.x) && (P.x <= P1.x)) || ((P1.x <= P.x) && (P.x <= P0.x))))
        return false;
    
    if(!(((P0.y <= P.y) && (P.y <= P1.y)) || ((P1.y <= P.y) && (P.y <= P0.y))))
        return false;
    
    return IsPointOnLine2D(P0, P1, P);
}

b32 IsPointOnEdge2D(v2f* Edge, v2f P)
{
    b32 Result = IsPointOnEdge2D(Edge[0], Edge[1], P);
    return Result;
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

b32 IsPointBehindLine2D(v2f LineP, v2f N, v2f P)
{
    b32 Result = Dot(P-LineP, N) < 0.0f;
    return Result;
}

b32 IsPointBehindLine2D(v2f* Line, v2f N, v2f P)
{
    b32 Result = IsPointBehindLine2D(Line[0], N, P);
    return Result;
}

b32 IsPointOnOrBehindLine2D(v2f* Line, v2f N, v2f P)
{
    b32 Result = IsPointOnLine2D(Line, P) || IsPointBehindLine2D(Line, N, P);
    return Result;
}

v2f PointTriangleClosestPoint2D(v2f P0, v2f P1, v2f P2, v2f P)
{
    v2f P01 = P1-P0;
    v2f P02 = P2-P0;
    v2f PP0 = P-P0;
    
    f32 Proj01 = Dot(PP0, P01);
    f32 Proj02 = Dot(PP0, P02);
    if(Proj01 <= 0.0f && Proj02 <= 0.0f) return P0;
    
    v2f PP1 = P-P1;
    f32 Proj03 = Dot(PP1, P01);
    f32 Proj04 = Dot(PP1, P02);
    if((Proj03 >= 0.0f) && (Proj04 <= Proj03)) return P1;
    
    f32 Proj0 = Proj01*Proj04 - Proj03*Proj02;
    if((Proj0 <= 0.0f) && (Proj01 >= 0.0f) && (Proj03 <= 0.0f))
    {
        f32 t = Proj01 / (Proj01-Proj03);
        v2f Result = P0 + t*P01;
        return Result;
    }
    
    v2f PP2 = P-P2;
    f32 Proj05 = Dot(PP2, P01);
    f32 Proj06 = Dot(PP2, P02);
    if((Proj06 >= 0.0f) && (Proj05 <= Proj06)) return P2;
    
    f32 Proj1 = Proj05*Proj02 - Proj01*Proj06;
    if((Proj1 <= 0.0f) && (Proj02 >= 0.0f) && (Proj06 <= 0.0f))
    {
        f32 t = Proj02 / (Proj02 - Proj06);
        v2f Result = P0 + t*P02;
        return Result;
    }
    
    f32 Proj2 = Proj03*Proj06 - Proj05*Proj04;
    if((Proj2 <= 0.0f) && ((Proj04-Proj03) >= 0.0f) && ((Proj05-Proj06) >= 0.0f))
    {
        f32 t = (Proj04-Proj03) / ((Proj04-Proj03) + (Proj05-Proj06));
        v2f Result = P1 + t * (P2-P1);     
        return Result;
    }
    
    f32 Denominator = SafeInverse(Proj01+Proj02+Proj03);
    f32 u = Proj02*Denominator;
    f32 v = Proj01*Denominator;
    v2f Result = P0+(P01*u)+(P02*v);
    return Result;
}

v2f PointTriangleClosestPoint2D(triangle2D Triangle, v2f P)
{
    v2f Result = PointTriangleClosestPoint2D(Triangle.P[0], Triangle.P[1], Triangle.P[2], P);
    return Result;
}

v2f PointTriangleClosestPoint2D(triangle3D Triangle, v2f P)
{
    v2f Result = PointTriangleClosestPoint2D(Triangle.P[0].xy, Triangle.P[1].xy, Triangle.P[2].xy, P);
    return Result;
}

b32 TriangleCircleOverlap2D(v2f P0, v2f P1, v2f P2, v2f CenterP, f32 Radius)
{
    if(!IsPointInTriangle2D(P0, P1, P2, CenterP))
    {        
        v2f ClosestPoint = PointTriangleClosestPoint2D(P0, P1, P2, CenterP);
        b32 Result = SquareMagnitude(ClosestPoint-CenterP) <= Square(Radius);
        return Result;
    }    
    return true;
}

b32 TriangleCircleOverlap2D(triangle2D Triangle, v2f CenterP, f32 Radius)
{
    b32 Result = TriangleCircleOverlap2D(Triangle.P[0], Triangle.P[1], Triangle.P[2], CenterP, Radius);    
    return Result;
}

b32 TriangleCircleOverlap2D(triangle3D Triangle, v2f CenterP, f32 Radius)
{
    b32 Result = TriangleCircleOverlap2D(Triangle.P[0].xy, Triangle.P[1].xy, Triangle.P[2].xy, CenterP, Radius);
    return Result;    
}

b32 TriangleCircleOverlap2D(triangle3D Triangle, circle2D Circle)
{
    b32 Result = TriangleCircleOverlap2D(Triangle.P[0].xy, Triangle.P[1].xy, Triangle.P[2].xy, Circle.CenterP, Circle.Radius);
    return Result;    
}

b32 IsDegenerateTriangle2D(v2f P0, v2f P1, v2f P2)
{
    b32 Result = AreEqual(P0, P1, 1e-6f) || AreEqual(P0, P2, 1e-6f) || AreEqual(P1, P2, 1e-6f);
    return Result;
}

b32 IsDegenerateTriangle2D(triangle2D Triangle)
{
    b32 Result = IsDegenerateTriangle2D(Triangle.P[0], Triangle.P[1], Triangle.P[2]);
    return Result;
}

b32 IsDegenerateTriangle2D(triangle3D Triangle)
{
    b32 Result = IsDegenerateTriangle2D(Triangle.P[0].xy, Triangle.P[1].xy, Triangle.P[2].xy);
    return Result;
}

SUPPORT_FUNCTION(Line3DSupport)
{
    v3f* Line = (v3f*)Data;
    if(Dot(Direction, Line[0]) < Dot(Direction, Line[1]))
        return Line[1];    
    return Line[0];
}

circle2D CombineCircles(v2f CenterP0, f32 Radius0, v2f CenterP1, f32 Radius1)
{
    v2f Offset = CenterP1-CenterP0;
    f32 SqrDistance = SquareMagnitude(Offset);    
    f32 RadiusDiff = Radius1-Radius0;
    
    circle2D Result = {};   
    if(Square(RadiusDiff) >= SqrDistance)
    {
        if(Radius0 > Radius1)
        {
            Result.CenterP = CenterP0;
            Result.Radius = Radius0;            
        }
        else
        {
            Result.CenterP = CenterP1;
            Result.Radius = Radius1;
        }
    }
    else
    {
        f32 Distance = Sqrt(SqrDistance);
        Result.Radius = (Distance+Radius0+Radius1)*0.5f;
        Result.CenterP = CenterP0;
        if(Distance > 0)
            Result.CenterP += Offset*((Result.Radius-Radius0)/Distance);
    }
    
    return Result;
}


penetration_result_2D CommonCirclePenetrationResult2D(v2f CenterP, f32 Radius, v2f ClosestPoint)
{
    penetration_result_2D Result = {};
    Result.Hit = true;
    Result.Normal = Normalize(CenterP - ClosestPoint);
    v2f PenetrationPoint = CenterP-Result.Normal*Radius;
    Result.Distance = Magnitude(ClosestPoint-PenetrationPoint);
    return Result;
}

penetration_result_2D CircleEdgePenetrationResult2D(v2f CenterP, f32 Radius, v2f E0, v2f E1)
{
    penetration_result_2D Result = {};
    
    v2f ClosestPoint = PointLineSegmentClosestPoint2D(E0, E1, CenterP);    
    
    if(SquareMagnitude(ClosestPoint-CenterP) < Square(Radius))
    {   
        Result = CommonCirclePenetrationResult2D(CenterP, Radius, ClosestPoint);        
        return Result;
    }
    
    return Result;
}

penetration_result_2D CircleRectanglePenetrationResult2D(v2f CenterP, f32 Radius, v2f Min, v2f Max)
{
    penetration_result_2D Result = {};
    f32 SmallestDistance = FLT_MAX;
    
    //NOTE(EVERYONE): First detect the closest point between all the edges
    
    v2f Vertices[4]
    {
        Min, 
        V2(Min.x, Max.y),
        Max,
        V2(Max.x, Min.y)
    };
    
    v2f ClosestPoints[4] = 
    {
        PointLineSegmentClosestPoint2D(Vertices[0], Vertices[1], CenterP),
        PointLineSegmentClosestPoint2D(Vertices[1], Vertices[2], CenterP),
        PointLineSegmentClosestPoint2D(Vertices[2], Vertices[3], CenterP),
        PointLineSegmentClosestPoint2D(Vertices[3], Vertices[0], CenterP)
    };    
    
    f32 SquareDistances[4] = 
    {
        SquareMagnitude(ClosestPoints[0]-CenterP),
        SquareMagnitude(ClosestPoints[1]-CenterP),
        SquareMagnitude(ClosestPoints[2]-CenterP),
        SquareMagnitude(ClosestPoints[3]-CenterP)
    };
    
    for(u32 EdgeIndex = 0; EdgeIndex < 4; EdgeIndex++)
    {
        if(SquareDistances[EdgeIndex] < Square(Radius))
        {
            if(SquareDistances[EdgeIndex] < SmallestDistance)                
                Result = CommonCirclePenetrationResult2D(CenterP, Radius, ClosestPoints[EdgeIndex]);                            
        }
    }
    
    return Result;
}

time_result_2D MovingCircleEdgeIntersectionTime2D(v2f P0, v2f P1, f32 Radius, v2f E0, v2f E1)
{
    time_result_2D Result = InvalidTimeResult2D();
    
    penetration_result_2D InitialPenetration = CircleEdgePenetrationResult2D(P0, Radius, E0, E1);
    if(InitialPenetration.Hit)
    {
        Result = CreateTimeResult(InitialPenetration, P0);        
        return Result;
    }
    
    v2f L = Normalize(E1-E0);
    v2f N = PerpCCW(L);
    
    v2f P[4] = 
    {
        E0 + N*Radius,
        E1 + N*Radius,
        E0 - N*Radius,
        E1 - N*Radius
    };
    
    f32 t[6] = {INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY};        
    EdgeIntersections2D(P0, P1, P[0], P[1], &t[0], NULL);
    EdgeIntersections2D(P0, P1, P[2], P[3], &t[1], NULL);
    EdgeCircleIntersections2D(P0, P1, E0, Radius, &t[2]);
    EdgeCircleIntersections2D(P0, P1, E1, Radius, &t[4]);    
    
    for(u32 i = 0; i < 6; i++)
    {        
        if(t[i] != INFINITY && t[i] < Result.Time)            
        {               
            Result.ContactPoint = P0 + t[i]*(P1-P0);
            
            switch(i)
            {
                case 0:
                {
                    Result.Normal = N;
                    break;
                } 
                
                case 1:
                {
                    Result.Normal = -N;
                    break;
                }
                
                case 2:
                case 3:
                {
                    Result.Normal = Normalize(Result.ContactPoint-E0);
                    break;
                }
                
                case 4:
                case 5:
                {
                    Result.Normal = Normalize(Result.ContactPoint-E1);
                    break;
                }
                
                INVALID_DEFAULT_CASE;
            }
            
            
            Result.Time = MaximumF32(0.0f, t[i]);
        }
    }
    
    return Result;
}

time_result_2D MovingCircleRectangleIntersectionTimeNoInitialPenetration2D(v2f CenterP0, v2f CenterP1, f32 Radius, v2f RectMin, v2f RectMax)
{
    time_result_2D Result = InvalidTimeResult2D();
    
    v2f Edges[4][2] = 
    {
        {RectMin, V2(RectMin.x, RectMax.y)},
        {Edges[0][1], RectMax},
        {RectMax, V2(RectMax.x, RectMin.y)},
        {Edges[2][1], RectMin}
    };
    
    for(u32 EdgeIndex = 0; EdgeIndex < 4; EdgeIndex++)
    {        
        time_result_2D TimeResult = MovingCircleEdgeIntersectionTime2D(CenterP0, CenterP1, Radius, Edges[EdgeIndex][0], Edges[EdgeIndex][1]);
        if(!IsInvalidTimeResult2D(TimeResult))
        {
            if(TimeResult.Time < Result.Time)            
                Result = TimeResult;                            
        }        
    }
    
    return Result;
}

time_result_2D MovingCircleRectangleIntersectionTime2D(v2f CenterP0, v2f CenterP1, f32 Radius, 
                                                       v2f RectMin, v2f RectMax)
{            
    time_result_2D Result = InvalidTimeResult2D();    
    
    penetration_result_2D InitialPenetration = CircleRectanglePenetrationResult2D(CenterP0, Radius, RectMin, RectMax);
    if(InitialPenetration.Hit)
    {
        Result = CreateTimeResult(InitialPenetration, CenterP0);        
        return Result;
    }
    
    Result = MovingCircleRectangleIntersectionTimeNoInitialPenetration2D(CenterP0, CenterP1, Radius, RectMin, RectMax);    
    return Result;
}

time_result_2D MovingRectangleCircleIntersectionTime2D(v2f CenterP0, v2f CenterP1, v2f Dim, v2f CircleP, f32 CircleR)
{    
    rect2D Rect = CreateRect2DCenterDim(CenterP0, Dim);
    
    time_result_2D Result = InvalidTimeResult2D();
    
    penetration_result_2D InitialPenetration = CircleRectanglePenetrationResult2D(CircleP, CircleR, Rect.Min, Rect.Max);
    if(InitialPenetration.Hit)
    {
        InitialPenetration.Normal = -InitialPenetration.Normal;
        Result = CreateTimeResult(InitialPenetration, CenterP0); 
        return Result;
    }
    
    v2f Delta = CenterP0-CenterP1;    
    Result = MovingCircleRectangleIntersectionTimeNoInitialPenetration2D(CircleP, CircleP+Delta, CircleR, Rect.Min, Rect.Max);
    if(!IsInvalidTimeResult2D(Result))
    {
        Result.Normal = -Result.Normal;
        Result.ContactPoint = CenterP0 + Result.Time*(CenterP1-CenterP0);        
    }
    
    return Result;
}

u32 FindRectangleSupportIndex(v2f Direction)
{    
    if(SIGN(Direction.x) >= 0.0f)
    {
        if(SIGN(Direction.y) >= 0.0f)
            return 3;
        else
            return 1;
    }
    else
    {
        if(SIGN(Direction.y) >= 0.0f)
            return 2;
        else
            return 0;
    }    
}

u32 FindAdjVerticalSupportIndex(u32 SupportIndex)
{
    if(SupportIndex == 0)
        return 2;
    
    if(SupportIndex == 1)
        return 3;
    
    if(SupportIndex == 2)
        return 0;
    
    if(SupportIndex == 3)
        return 1;
    
    INVALID_CODE;
    return (u32)-1;
}

u32 FindAdjHorizontalSupportIndex(u32 SupportIndex)
{
    if(SupportIndex == 0)
        return 1;
    
    if(SupportIndex == 1)
        return 0;
    
    if(SupportIndex == 2)
        return 3;
    
    if(SupportIndex == 3)
        return 2;
    
    INVALID_CODE;
    return (u32)-1;
}

penetration_result_2D RectangleEdgePenetrationResult2D(v2f CenterP, v2f HalfDim, v2f E0, v2f E1)
{
    penetration_result_2D Result = {};
    
    v2f ClosestPoint = PointLineSegmentClosestPoint2D(E0, E1, CenterP);
    v2f Normal = Normalize(CenterP-ClosestPoint);
    
    v2f Offsets[4] = 
    {
        -HalfDim, 
        V2(HalfDim.x, -HalfDim.y),
        V2(-HalfDim.x, HalfDim.y),
        HalfDim
    };
    
    f32 BestDistance = -FLT_MAX;
        
    for(u32 PointIndex = 0; PointIndex < 4; PointIndex++)
    {
        v2f TestP = CenterP+Offsets[PointIndex];
        
        //NOTE(EVERYONE): It must be penetrating to return results. If it is on the line skip since the distance is so small
        if(IsPointOnLine2D(E0, E1, TestP))
            continue;
        
        if(IsPointBehindLine2D(E0, Normal, TestP))
        {                    
            v2f P = PointLineSegmentClosestPoint2D(E0, E1, TestP);                                
            f32 Distance = SquareMagnitude(P-TestP);
            if(Distance > BestDistance)            
                BestDistance = Distance;       
        }
    }
    
    if(BestDistance != -FLT_MAX)
    {
        Result.Hit = true;
        Result.Normal = Normal;
        Result.Distance = Sqrt(BestDistance);
    }
    
    return Result;
}

time_result_2D MovingRectangleEdgeIntersectionTime2D(v2f CenterP0, v2f CenterP1, v2f Dim, v2f E0, v2f E1)
{    
    v2f HalfDim = Dim*0.5f;    
    time_result_2D Result = InvalidTimeResult2D();
    
    penetration_result_2D InitialPenetration = RectangleEdgePenetrationResult2D(CenterP0, HalfDim, E0, E1);
    if(InitialPenetration.Hit)
    {
        Result = CreateTimeResult(InitialPenetration, CenterP0);
        return Result;
    }
    
    //0 bottom left, 1 bottom right, 2 top left, 3 top right    
    v2f Direction = E0-E1;    
    u32 SupportIndex0 = FindRectangleSupportIndex(Direction);
    u32 AdjVerticalSupportIndex0 = FindAdjVerticalSupportIndex(SupportIndex0);
    u32 AdjHorizontalSupportIndex0 = FindAdjHorizontalSupportIndex(SupportIndex0);
    
    u32 SupportIndex1 = FindRectangleSupportIndex(-Direction);
    u32 AdjVerticalSupportIndex1 = FindAdjVerticalSupportIndex(SupportIndex1);
    u32 AdjHorizontalSupportIndex1 = FindAdjHorizontalSupportIndex(SupportIndex1);
    
    //TODO(JJ): We need to handle this case
    ASSERT((SupportIndex1 == AdjVerticalSupportIndex0) || (SupportIndex1 == AdjHorizontalSupportIndex0));
    
    v2f Offsets[4] = 
    {
        -HalfDim, 
        V2(HalfDim.x, -HalfDim.y),
        V2(-HalfDim.x, HalfDim.y),
        HalfDim
    };    
    
    v2f Edges[6][2] = 
    {
        {E0 + Offsets[SupportIndex0], E0 + Offsets[AdjHorizontalSupportIndex0]},
        {E0 + Offsets[SupportIndex0], E0 + Offsets[AdjVerticalSupportIndex0]},
        {E1 + Offsets[SupportIndex1], E1 + Offsets[AdjHorizontalSupportIndex1]},
        {E1 + Offsets[SupportIndex1], E1 + Offsets[AdjVerticalSupportIndex1]},
        {Edges[1][1], Edges[3][1]},
        {Edges[0][1], Edges[2][1]}
    };        
        
    for(u32 EdgeIndex = 0; EdgeIndex < 6; EdgeIndex++)
    {
        f32 t;
        if(EdgeIntersections2D(CenterP0, CenterP1, Edges[EdgeIndex][0], Edges[EdgeIndex][1], &t, NULL) && (t < Result.Time))
        {
            v2f CenterP10 = CenterP1-CenterP0;
            Result.ContactPoint = CenterP0 + t*CenterP10;
            
            Result.Normal = Normalize(PerpCCW(Edges[EdgeIndex][1] - Edges[EdgeIndex][0]));
            if(Dot(Result.Normal, CenterP10) > 0)
                Result.Normal = -Result.Normal;            
            
            Result.Time = t;
        }
    }
    
    return Result;
}

penetration_result_2D PenetrationTestAABB2D(v2f CenterA, v2f DimA, v2f CenterB, v2f DimB)
{
    f32 XDistance = INFINITY;
    f32 YDistance = INFINITY;
    
    v2f T = CenterB-CenterA;
    
    v2f HalfDimA = DimA*0.5f;
    v2f HalfDimB = DimB*0.5f;
    
    f32 XOverlap = (HalfDimA.x + HalfDimB.x) - Abs(T.x);
    f32 YOverlap = (HalfDimA.y + HalfDimB.y) - Abs(T.y);
    
    penetration_result_2D Result = {};        
    Result.Hit = (XOverlap > 0) && (YOverlap > 0);
    if(Result.Hit)
    {        
        if(XOverlap < YOverlap)
        {
            Result.Distance = XOverlap;
            Result.Normal = T.x < 0 ? V2(1, 0) : V2(-1, 0);                
        }
        else
        {
            Result.Distance = YOverlap;
            Result.Normal = T.y < 0 ? V2(0, 1) : V2(0, -1);
        }
    }
    
    return Result;
}

time_result_2D MovingRectangleRectangleIntersectionTime2D(v2f CenterP0, v2f CenterP1, v2f DimP,
                                                          v2f CenterS, v2f DimS)
{    
    time_result_2D Result = InvalidTimeResult2D();
    
    penetration_result_2D InitialPenetration = PenetrationTestAABB2D(CenterP0, DimP, CenterS, DimS);
    if(InitialPenetration.Hit)
    {
        Result = CreateTimeResult(InitialPenetration, CenterP0);        
        return Result;
    }
    
    v2f MinkowskiDim = (DimS+DimP)*0.5f;
    
    v2f P[4] = 
    {
        -MinkowskiDim,
        V2(-MinkowskiDim.x, MinkowskiDim.y),
        MinkowskiDim,
        V2(MinkowskiDim.x, -MinkowskiDim.y)
    };
    
    P[0] += CenterS;
    P[1] += CenterS;
    P[2] += CenterS;
    P[3] += CenterS;
    
    v2f CenterP10 = CenterP1-CenterP0;
    
    f32 t0;        
    if(EdgeIntersections2D(CenterP0, CenterP1, P[0], P[1], &t0, NULL) && (t0 < Result.Time))
    {
        Result.Time = t0;
        Result.Normal = V2(-1.0f, 0.0f);
        Result.ContactPoint = CenterP0 + CenterP10*t0;
    }
    
    f32 t1;
    if(EdgeIntersections2D(CenterP0, CenterP1, P[1], P[2], &t1, NULL) && (t1 < Result.Time))
    {
        Result.Time = t1;
        Result.Normal = V2(0.0f, 1.0f);
        Result.ContactPoint = CenterP0 + CenterP10*t1;
    }
    
    f32 t2;
    if(EdgeIntersections2D(CenterP0, CenterP1, P[2], P[3], &t2, NULL) && (t2 < Result.Time))
    {
        Result.Time = t2;
        Result.Normal = V2(1.0f, 0.0f);
        Result.ContactPoint = CenterP0 + CenterP10*t2;
    }
    
    f32 t3;
    if(EdgeIntersections2D(CenterP0, CenterP1, P[3], P[0], &t3, NULL) && (t3 < Result.Time))
    {
        Result.Time = t3;
        Result.Normal = V2(0.0f, -1.0f);
        Result.ContactPoint = CenterP0 + CenterP10*t3;
    }
    
    return Result;
    
}

void GetBoxVerticesFromDimAndCenterP(v3f* Vertices, v3f CenterP, v3f Dim)
{    
    v3f HalfDim = Dim*0.5f;
    Vertices[0] = V3(CenterP.xy - HalfDim.xy, CenterP.z + HalfDim.z);
    Vertices[1] = V3(CenterP.x + HalfDim.x, CenterP.y - HalfDim.y, CenterP.z + HalfDim.z);
    Vertices[2] = CenterP + HalfDim;
    Vertices[3] = V3(CenterP.x - HalfDim.x, CenterP.yz + HalfDim.yz);
    Vertices[4] = V3(CenterP.x + HalfDim.x, CenterP.yz - HalfDim.yz);
    Vertices[5] = CenterP - HalfDim;
    Vertices[6] = V3(CenterP.x - HalfDim.x, CenterP.y + HalfDim.y, CenterP.z - HalfDim.z);
    Vertices[7] = V3(CenterP.xy + HalfDim.xy, CenterP.z - HalfDim.z);            
}

//NOTE(EVERYONE): Is RectA fully contained in RectB
b32 IsRectFullyContainedInRect3D(v3f MinA, v3f MaxA, v3f MinB, v3f MaxB)
{    
    b32 Result = (MinA >= MinB) && (MaxB >= MaxA);
    return Result;    
}



b32 IsLineIntersectingTriangle3D(v3f LineOrigin, v3f LineDirection, v3f Vertex0, v3f Vertex1, v3f Vertex2, f32* t, f32* u, f32* v)
{
    v3f edge1;
    v3f edge2;
    v3f tvec;
    v3f pvec;
    v3f qvec;
    f32 det;
    f32 inv_det;

    edge1 = Vertex1 - Vertex0;
    edge2 = Vertex2 - Vertex0;

    pvec = Cross(LineDirection, edge2);

    det = Dot(edge1, pvec);

    if(det == 0)
        return false;
    
    tvec = LineOrigin - Vertex0;

    *u = Dot(tvec, pvec);
    if(*u < 0.0f || *u > det)
        return false;

    qvec = Cross(tvec, edge1);

    *v = Dot(LineDirection, qvec);
    if(*v < 0.0f || *u + *v > det)
        return false;

    *t = Dot(edge2, qvec);
    inv_det = 1.0f / det;
    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;

    return true;
}

b32 IsRayIntersectingTriangle3D(ray3D Ray, triangle3D Triangle, f32* t, f32* u, f32* v)
{
    return IsLineIntersectingTriangle3D(Ray.Origin, Ray.Direction, Triangle.P[0], Triangle.P[1], Triangle.P[2], t, u, v);
}

b32 IsRayIntersectingEntity(v3f RayOrigin, v3f RayDirection, world_entity* Entity, f32* t, f32* u, f32* v)
{
    b32 RayIntersected = false;
    ptr MyVertexSize = GetVertexBufferSize(Entity->Mesh->VertexFormat, Entity->Mesh->VertexCount);
    ptr MyVertexStride = GetVertexStride(Entity->Mesh->VertexFormat);
    ptr IndexSize = GetIndexBufferSize(Entity->Mesh->IndexFormat, Entity->Mesh->IndexCount);
    ptr MyIndexStride = GetIndexSize(Entity->Mesh->IndexFormat);

    u32 i = 0;
    while(i*MyIndexStride < IndexSize)
    {
        u32 Index1 = 0;
        u32 Index2 = 0;
        u32 Index3 = 0;
        v3f Vertex1 = V3(0,0,0);
        v3f Vertex2 = V3(0,0,0);
        v3f Vertex3 = V3(0,0,0);
        if(Entity->Mesh->IndexFormat == GRAPHICS_INDEX_FORMAT_16_BIT)
        {
            Index1 = *(u16*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
            Index2 = *(u16*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
            Index3 = *(u16*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
        }
        else if(Entity->Mesh->IndexFormat == GRAPHICS_INDEX_FORMAT_32_BIT)
        {
            Index1 = *(u32*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
            Index2 = *(u32*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
            Index3 = *(u32*)((u8*)Entity->Mesh->Indices + i*MyIndexStride);
            i++;
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P3)
        {
            vertex_p3 TVertex1 = *(vertex_p3*)((u8*)Entity->Mesh->Vertices + (Index1*MyVertexStride));
            vertex_p3 TVertex2 = *(vertex_p3*)((u8*)Entity->Mesh->Vertices + (Index2*MyVertexStride));
            vertex_p3 TVertex3 = *(vertex_p3*)((u8*)Entity->Mesh->Vertices + (Index3*MyVertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P3_N3)
        {
            vertex_p3_n3 TVertex1 = *(vertex_p3_n3*)((u8*)Entity->Mesh->Vertices + (Index1*MyVertexStride));
            vertex_p3_n3 TVertex2 = *(vertex_p3_n3*)((u8*)Entity->Mesh->Vertices + (Index2*MyVertexStride));
            vertex_p3_n3 TVertex3 = *(vertex_p3_n3*)((u8*)Entity->Mesh->Vertices + (Index3*MyVertexStride));
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P3_N3_WEIGHTS)
        {
            vertex_p3_n3_weights TVertex1 = *(vertex_p3_n3_weights*)((u8*)Entity->Mesh->Vertices + (Index1*MyVertexStride));
            vertex_p3_n3_weights TVertex2 = *(vertex_p3_n3_weights*)((u8*)Entity->Mesh->Vertices + (Index2*MyVertexStride));
            vertex_p3_n3_weights TVertex3 = *(vertex_p3_n3_weights*)((u8*)Entity->Mesh->Vertices + (Index3*MyVertexStride));
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P2_UV_C)
        {
            ASSERT(false);
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV)
        {
            vertex_p3_n3_t4_uv TVertex1 = *(vertex_p3_n3_t4_uv*)((u8*)Entity->Mesh->Vertices + (Index1*MyVertexStride));
            vertex_p3_n3_t4_uv TVertex2 = *(vertex_p3_n3_t4_uv*)((u8*)Entity->Mesh->Vertices + (Index2*MyVertexStride));
            vertex_p3_n3_t4_uv TVertex3 = *(vertex_p3_n3_t4_uv*)((u8*)Entity->Mesh->Vertices + (Index3*MyVertexStride));
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        if(Entity->Mesh->VertexFormat == GRAPHICS_VERTEX_FORMAT_P3_N3_T4_UV_WEIGHTS)
        {
            vertex_p3_n3_t4_uv_weights TVertex1 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Entity->Mesh->Vertices + (Index1*MyVertexStride));
            vertex_p3_n3_t4_uv_weights TVertex2 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Entity->Mesh->Vertices + (Index2*MyVertexStride));
            vertex_p3_n3_t4_uv_weights TVertex3 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Entity->Mesh->Vertices + (Index3*MyVertexStride));
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }

        triangle3D Triangle = CreateTriangle3D(Vertex1, Vertex2, Vertex3);
        ray3D Ray;
        Ray.Origin = RayOrigin;
        Ray.Direction = RayDirection;
        Triangle = TransformTriangle3D(Triangle, Entity->Transform);
        f32 tempt, tempu, tempv;
        if(IsRayIntersectingTriangle3D(Ray, Triangle, &tempt, &tempu, &tempv))
        {
            if(tempt < *t)
            {
                *t = tempt;
                *u = tempu;
                *v = tempv;
            }
            RayIntersected = true;
        }

    }
    return RayIntersected;
}