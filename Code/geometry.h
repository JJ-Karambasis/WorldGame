#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "gjk.h"

#define SUPPORT_FUNCTION(name) v3f name(void* Data, v3f Direction) 
typedef SUPPORT_FUNCTION(support_function);

//NOTE(EVERYONE): 2D structures
struct edge2D
{
    v2f P[2];
};

struct ray2D
{
    v2f Origin;
    v2f Direction;
};

struct circle2D
{
    v2f CenterP;
    f32 Radius;
};

struct rect2D
{
    v2f Min;
    v2f Max;
};

struct rect2D_center_dim
{
    v2f CenterP;
    v2f Dim;
};

struct triangle2D
{
    v2f P[3];
};

inline v2f GetEdgeDirection2D(edge2D Edge)
{
    v2f Result = Normalize(Edge.P[1]-Edge.P[0]);
    return Result;
}

inline edge2D CreateEdge2D(v2f P0, v2f P1)
{
    edge2D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
    return Result;
}

inline edge2D CreateEdge2D(v2f* P)
{
    edge2D Result = CreateEdge2D(P[0], P[1]);
    return Result;
}

inline edge2D CreateEdge2D(v3f P0, v3f P1)
{
    edge2D Result = CreateEdge2D(P0.xy, P1.xy);
    return Result;
}

//NOTE(EVERYONE): Inline 2D functions
inline ray2D CreateRay2D(edge2D Edge)
{
    ray2D Result = {Edge.P[0], GetEdgeDirection2D(Edge)};
    return Result;
}

inline ray2D CreateRay2D(v2f Origin, v2f Direction)
{
    ray2D Result;
    Result.Origin = Origin;
    Result.Direction = Normalize(Direction);
    return Result;
}

inline b32 IsValidRay2D(ray2D Ray)
{
    b32 Result = SquareMagnitude(Ray.Direction) > 0.0f;
    return Result;    
}

inline circle2D CreateCircle2D(v2f CenterP, f32 Radius)
{
    circle2D Result = {CenterP, Radius};
    return Result;
}

inline rect2D CreateRect2D(v2f Min, v2f Max)
{
    rect2D Result;
    Result.Min = Min;
    Result.Max = Max;
    return Result;
}

inline rect2D CreateRect2DCenterDim(v2f CenterP, v2f Dim)
{   
    v2f HalfDim = Dim*0.5f;
    rect2D Result = CreateRect2D(CenterP-HalfDim, CenterP+HalfDim);    
    return Result;
}

inline rect2D CreateRect2DCenterDim(rect2D_center_dim Rect)
{        
    rect2D Result = CreateRect2DCenterDim(Rect.CenterP, Rect.Dim);
    return Result;
}

inline triangle2D CreateTriangle2D(v2f P0, v2f P1, v2f P2)
{
    triangle2D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
    Result.P[2] = P2;
}

inline void GetTriangleEdgeDirections2D(v2f* Edges, v2f P0, v2f P1, v2f P2)
{
    Edges[0] = P1-P0;
    Edges[1] = P2-P1;
    Edges[2] = P0-P2;    
}

inline void GetTriangleEdgeDirections2D(v2f* E, edge2D* Edges)
{
    E[0] = Edges[0].P[1]-Edges[0].P[0];
    E[1] = Edges[1].P[1]-Edges[1].P[0];
    E[2] = Edges[2].P[1]-Edges[2].P[0];
}

inline void GetTriangleEdgeDirections2D(v2f* Edges, triangle2D Triangle)
{
    GetTriangleEdgeDirections2D(Edges, Triangle.P[0], Triangle.P[1], Triangle.P[2]);    
}

inline void GetTriangleNormals2D(v2f* Normals, v2f* E)
{    
    Normals[0] = Perp(E[0]);
    Normals[1] = Perp(E[1]);
    Normals[2] = Perp(E[2]);
    
    Normals[0] = Dot(Normals[0], E[2]) < 0.0f ? -Normals[0] : Normals[0];
    Normals[1] = Dot(Normals[1], E[0]) < 0.0f ? -Normals[1] : Normals[1];
    Normals[2] = Dot(Normals[2], E[1]) < 0.0f ? -Normals[2] : Normals[2];    
}

inline void GetTriangleNormals2D(v2f* Normals, edge2D* Edges)
{
    v2f E[3];
    GetTriangleEdgeDirections2D(E, Edges);    
    GetTriangleNormals2D(Normals, E);
}

//NOTE(EVERYONE): 3D structures
struct edge3D
{
    v3f P[2];
};

struct rect3D
{
    v3f Min;
    v3f Max;
};

struct rect3D_center_dim
{
    v3f CenterP;
    v3f Dim;
};

struct triangle3D
{
    v3f P[3];        
};

struct ellipsoid3D
{
    v3f CenterP;
    v3f Radius;
};

struct plane3D
{        
    v3f Normal;
    f32 D;
};

//NOTE(EVERYONE): Inline 3D functions

inline edge3D CreateEdge3D(v3f P0, v3f P1)
{
    edge3D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
    return Result;
}

inline rect3D CreateRect3D(v3f Min, v3f Max)
{
    rect3D Result;
    Result.Min = Min;
    Result.Max = Max;
    return Result;
}

inline rect3D CreateRect3DCenterDim(v3f Center, v3f Dim)
{    
    v3f HalfDim = Dim*0.5f;
    rect3D Result = CreateRect3D(Center-HalfDim, Center+HalfDim);
    return Result;
}

inline rect3D CreateRect3DCenterDim(rect3D_center_dim Rect)
{        
    rect3D Result = CreateRect3D(Rect.CenterP, Rect.Dim);
    return Result;
}

inline rect3D TransformAABB3D(rect3D Rect, sqt Transform)
{
    Transform.Orientation = IdentityQuaternion();
    
    rect3D Result;
    Result.Min = TransformV3(Rect.Min, Transform);
    Result.Max = TransformV3(Rect.Max, Transform);
    return Result;    
}

inline triangle3D CreateTriangle3D(v3f P0, v3f P1, v3f P2)
{
    triangle3D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
    Result.P[2] = P2;
    return Result;
}

inline plane3D InvalidPlane3D()
{
    plane3D Result;
    Result.Normal = InvalidV3();
    Result.D = INFINITY;
    return Result;
}

inline plane3D CreatePlane3D(v3f P0, v3f P1, v3f P2)
{
    plane3D Result;    
    Result.Normal = Normalize(Cross(P1-P0, P2-P0));    
    Result.D = -(Result.Normal.x*P0.x + Result.Normal.y*P0.y + Result.Normal.z*P0.z);
    return Result;
}

inline plane3D CreatePlane3D(v3f* P)
{
    plane3D Result = CreatePlane3D(P[0], P[1], P[2]);        
    return Result;
}

inline plane3D CreatePlane3D(triangle3D Triangle)
{
    plane3D Result = CreatePlane3D(Triangle.P);        
    return Result;
}

inline plane3D CreatePlane3D(v3f Origin, v3f Normal)
{
    plane3D Result;
    Result.Normal = Normalize(Normal);
    Result.D = -(Result.Normal.x*Origin.x + Result.Normal.y*Origin.y + Result.Normal.z*Origin.z);
    return Result;
}

inline triangle3D InvalidTriangle3D()
{
    triangle3D Result;
    Result.P[0] = InvalidV3();
    Result.P[1] = InvalidV3();
    Result.P[2] = InvalidV3();
    return Result;
}

inline b32 IsInvalidTriangle3D(triangle3D Triangle)
{
    b32 Result = IsInvalidV3(Triangle.P[0]) || IsInvalidV3(Triangle.P[1]) || IsInvalidV3(Triangle.P[2]);
    return Result;
}

inline triangle3D TransformTriangle3D(triangle3D Triangle, sqt Transform)
{
    triangle3D Result;
    Result.P[0] = TransformV3(Triangle.P[0], Transform);
    Result.P[1] = TransformV3(Triangle.P[1], Transform);
    Result.P[2] = TransformV3(Triangle.P[2], Transform);
    return Result;
}

inline void GetTriangleEdges3D(edge3D* Edges, triangle3D Triangle)
{
    Edges[0] = CreateEdge3D(Triangle.P[0], Triangle.P[1]);
    Edges[1] = CreateEdge3D(Triangle.P[1], Triangle.P[2]);
    Edges[2] = CreateEdge3D(Triangle.P[2], Triangle.P[0]);
}

inline void GetTriangleEdges2D(edge2D* Edges, triangle3D Triangle)
{
    Edges[0] = CreateEdge2D(Triangle.P[0], Triangle.P[1]);
    Edges[1] = CreateEdge2D(Triangle.P[1], Triangle.P[2]);
    Edges[2] = CreateEdge2D(Triangle.P[2], Triangle.P[0]);
}

//NOTE(EVERYONE): All other structures and inline functions
struct penetration_result
{
    b32 Hit;
    v3f Normal;
    f32 Distance;
};

struct penetration_result_2D
{
    b32 Hit;
    v2f Normal;
    f32 Distance;
};

struct time_result_2D
{    
    f32 Time;
    v2f ContactPoint;
    v2f Normal;
};

inline time_result_2D CreateTimeResult(penetration_result_2D Penetration, v2f P)
{
    time_result_2D Result = {};
    Result.Normal = Penetration.Normal;
    Result.ContactPoint = P + Penetration.Normal*Penetration.Distance;
    return Result;
}

inline time_result_2D CreateTimeResult(f32 Time)
{
    time_result_2D Result = {};
    Result.Time = Time;
    return Result;
}

inline time_result_2D InvalidTimeResult2D()
{
    time_result_2D Result = {};
    Result.Time = FLT_MAX;
    Result.ContactPoint = InvalidV2();
    return Result;
}

inline b32 IsInvalidTimeResult2D(time_result_2D TimeResult)
{
    b32 Result = (TimeResult.Time == FLT_MAX) || (TimeResult.ContactPoint == InvalidV2());
    return Result;
}

#endif