#ifndef GEOMETRY_H
#define GEOMETRY_H

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

struct triangle2D
{
    v2f P[3];
};

inline edge2D CreateEdge2D(v2f P0, v2f P1)
{
    edge2D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
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
    ray2D Result = {Edge.P[0], Normalize(Edge.P[1]-Edge.P[0])};
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

inline triangle2D CreateTriangle2D(v2f P0, v2f P1, v2f P2)
{
    triangle2D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
    Result.P[2] = P2;
}

//NOTE(EVERYONE): 3D structures
struct edge3D
{
    v3f P[2];
};

struct triangle3D
{
    v3f P[3];        
};

struct triangle3D_mesh
{
    u32 TriangleCount;
    triangle3D* Triangles;
};

//NOTE(EVERYONE): Inline 3D functions

inline edge3D CreateEdge3D(v3f P0, v3f P1)
{
    edge3D Result;
    Result.P[0] = P0;
    Result.P[1] = P1;
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

inline triangle3D TransformTriangle3D(triangle3D Triangle, sqt Transform)
{
    triangle3D Result;
    Result.P[0] = TransformV3(Triangle.P[0], Transform);
    Result.P[1] = TransformV3(Triangle.P[1], Transform);
    Result.P[2] = TransformV3(Triangle.P[2], Transform);
    return Result;
}

inline void GetTriangleEdges2D(edge2D* Edges, triangle3D Triangle)
{
    Edges[0] = CreateEdge2D(Triangle.P[0], Triangle.P[1]);
    Edges[1] = CreateEdge2D(Triangle.P[1], Triangle.P[2]);
    Edges[2] = CreateEdge2D(Triangle.P[2], Triangle.P[0]);
}

inline void GetTriangleEdges3D(edge3D* Edges, triangle3D Triangle)
{
    Edges[0] = CreateEdge3D(Triangle.P[0], Triangle.P[1]);
    Edges[1] = CreateEdge3D(Triangle.P[1], Triangle.P[2]);
    Edges[2] = CreateEdge3D(Triangle.P[2], Triangle.P[0]);
}

#endif