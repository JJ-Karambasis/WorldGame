#ifndef GEOMETRY_H
#define GEOMETRY_H

struct edge2D
{
    v2f P[2];
};

struct edge3D
{
    v3f P[2];
};

struct triangle
{
    v3f P[3];    
    triangle* AdjTriangles[3];
};

struct triangle_mesh
{
    u32 TriangleCount;
    triangle* Triangles;
};

#endif