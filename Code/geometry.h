#ifndef GEOMETRY_H
#define GEOMETRY_H

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