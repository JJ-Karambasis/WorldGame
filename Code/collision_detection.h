#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

enum collision_volume_type
{
    COLLISION_VOLUME_TYPE_NONE,
    COLLISION_VOLUME_TYPE_SPHERE,
    COLLISION_VOLUME_TYPE_CAPSULE,
    COLLISION_VOLUME_TYPE_CONVEX_HULL,    
    COLLISION_VOLUME_TYPE_TRIANGLE_MESH
};

struct sphere
{
    v3f CenterP;
    f32 Radius;
};

struct capsule
{
    v3f P0;
    v3f P1;
    f32 Radius;
};

struct collision_volume
{
    collision_volume_type Type;
    union
    { 
        sphere Sphere;
        capsule Capsule;
        convex_hull* ConvexHull;
        triangle_mesh* TriangleMesh;
    };
};

#endif