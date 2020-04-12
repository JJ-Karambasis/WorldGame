/* Original Author: Armand (JJ) Karambasis */
#ifndef GJK_H
#define GJK_H

#define GJK_MAX_ITERATIONS 30
#define GJK_EPSILON 1.19209290E-07F

struct gjk_simplex_vertex
{
    v3f ASupportPos;
    v3f BSupportPos;
    v3f CsoD;
};

struct gjk_simplex
{
    f32 Barycentric[4];
    gjk_simplex_vertex Vertices[4];
    u32 VertexCount;        
    f32 ClosestDistance;
};

struct gjk_result
{
    b32 Intersected;    
    v3f ClosestPoints[2];
};

#endif
