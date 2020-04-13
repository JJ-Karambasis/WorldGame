#include "gjk.cpp"

b32 IsTriangle2DClockwise(v2f P0, v2f P1, v2f P2)
{
    f32 Det = P0.x*(P1.y-P2.y) - P0.y*(P1.x-P2.x) + (P1.x*P2.y - P1.y*P2.x);
    ASSERT(Det != 0.0f);
    
    b32 Result = Det < 0.0f;
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

v2f PointLineSegmentClosestPoint2D(v2f P0, v2f P1, v2f P)
{
    v2f Edge = P1-P0;
    
    f32 SqrLength = SquareMagnitude(Edge);
    ASSERT(SqrLength != 0.0f);    
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

b32 IsPointBehindLine2D(v2f* Line, v2f N, v2f P)
{
    b32 Result = Dot(P-Line[0], N) < 0.0f;
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

SUPPORT_FUNCTION(AABB3DSupport)
{
    aabb3D* AABB = (aabb3D*)Data;
    
    v3f CenterPos;
    v3f Dim;
    GetAABB3DDimAndCenterPos(&CenterPos, &Dim, *AABB);
    Dim *= 0.5f;
    
    v3f Result = CenterPos + V3(SIGN(Direction.x)*Dim.x, 
                                SIGN(Direction.y)*Dim.y, 
                                SIGN(Direction.z)*Dim.z);
    return Result;
}

SUPPORT_FUNCTION(Line3DSupport)
{
    v3f* Line = (v3f*)Data;
    if(Dot(Direction, Line[0]) < Dot(Direction, Line[1]))
        return Line[1];    
    return Line[0];
}

penetration_result PenetrationTestAABB3D(aabb3D A, aabb3D B)
{
    v3f CenterPoint;
    v3f ADim;
    GetAABB3DDimAndCenterPos(&CenterPoint, &ADim, A);
    ADim *= 0.5f;
    
    B.Max += ADim;
    B.Min -= ADim;
    
    penetration_result Result;
    Result.Hit = false;
    Result.Distance = INFINITY;
    
    v3f XNormal = {};    
    f32 XDistance = 0.0f;
    {
        f32 Distance0 = MaximumF32(CenterPoint.x - B.Min.x, 0.0f);        
        f32 Distance1 = MaximumF32(B.Max.x - CenterPoint.x, 0.0f);
        
        if((Distance0 != 0.0f) || (Distance1 != 0.0f))
        {                        
            if(Distance0 < Distance1)
            {
                XDistance = Distance0;
                XNormal = V3(-1.0f, 0.0f, 0.0f);
            }
            else
            {
                XDistance = Distance1;
                XNormal = V3(1.0f, 0.0f, 0.0f);
            }
        }        
    }
    
    v3f YNormal = {};    
    f32 YDistance = 0.0f;
    {
        f32 Distance0 = MaximumF32(CenterPoint.y - B.Min.y, 0.0f);        
        f32 Distance1 = MaximumF32(B.Max.y - CenterPoint.y, 0.0f);
        
        if((Distance0 != 0.0f) || (Distance1 != 0.0f))
        {                        
            if(Distance0 < Distance1)
            {
                YDistance = Distance0;
                YNormal = V3(0.0f, -1.0f, 0.0f);
            }
            else
            {
                YDistance = Distance1;
                YNormal = V3(0.0f, 1.0f, 0.0f);
            }
        }        
    }
    
    v3f ZNormal = {};    
    f32 ZDistance = 0.0f;
    {
        f32 Distance0 = MaximumF32(CenterPoint.z - B.Min.z, 0.0f);        
        f32 Distance1 = MaximumF32(B.Max.z - CenterPoint.z, 0.0f);
        
        if((Distance0 != 0.0f) || (Distance1 != 0.0f))
        {                        
            if(Distance0 < Distance1)
            {
                ZDistance = Distance0;
                ZNormal = V3(0.0f, 0.0f, -1.0f);
            }
            else
            {
                ZDistance = Distance1;
                ZNormal = V3(0.0f, 0.0f, 1.0f);
            }
        }        
    }
    
    if((ZDistance != 0.0f) && (YDistance != 0.0f) && (XDistance != 0.0f))
    {
        Result.Hit = true;        
        
        if(ZDistance < YDistance)
        {
            if(ZDistance < XDistance)
            {
                Result.Distance = ZDistance;
                Result.Normal = ZNormal;
            }
            else
            {
                Result.Distance = XDistance;
                Result.Normal = XNormal;
            }
        }
        else if(YDistance < XDistance)
        {
            Result.Distance = YDistance;
            Result.Normal = YNormal;
        }
        else
        {
            Result.Distance = XDistance;
            Result.Normal = XNormal;
        }
    }
    
    return Result;
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

time_result_2D EdgeCapsuleIntersectionTime2D(v2f P0, v2f P1, v2f C0, v2f C1, f32 Radius, v2f* Normal)
{
    time_result_2D Result = InvalidTimeResult2D();
    
    v2f L = Normalize(C1-C0);
    v2f N = PerpCCW(L);
    
    v2f P[4] = 
    {
        C0 + N*Radius,
        C1 + N*Radius,
        C0 - N*Radius,
        C1 - N*Radius
    };
    
    f32 t[6] = {INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY};        
    EdgeIntersections2D(P0, P1, P[0], P[1], &t[0], NULL);
    EdgeIntersections2D(P0, P1, P[2], P[3], &t[1], NULL);
    EdgeCircleIntersections2D(P0, P1, C0, Radius, &t[2]);
    EdgeCircleIntersections2D(P0, P1, C1, Radius, &t[4]);    
    
    for(u32 i = 0; i < 6; i++)
    {        
        if(t[i] != INFINITY && t[i] < Result.Time)            
        {            
            Result.ContactPoint = P0 + t[i]*(P1-P0);
            
            switch(i)
            {
                case 0:
                {
                    *Normal = N;
                    break;
                } 
                
                case 1:
                {
                    *Normal = -N;
                    break;
                }
                
                case 2:
                case 3:
                {
                    *Normal = Normalize(Result.ContactPoint-C0);
                    break;
                }
                
                case 4:
                case 5:
                {
                    *Normal = Normalize(Result.ContactPoint-C1);
                    break;
                }
                
                INVALID_DEFAULT_CASE;
            }
            
            Result.Time = t[i];                    
        }
    }
    
    return Result;
}