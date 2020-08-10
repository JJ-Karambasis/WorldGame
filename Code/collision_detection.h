#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

enum collision_volume_type
{
    COLLISION_VOLUME_TYPE_NONE,
    COLLISION_VOLUME_TYPE_SPHERE,
    COLLISION_VOLUME_TYPE_CAPSULE,
    COLLISION_VOLUME_TYPE_CONVEX_HULL    
};

struct sphere
{
    v3f CenterP;
    f32 Radius;
};

struct capsule
{
    union
    {
        v3f P[2];
        struct
        {
            v3f P0;
            v3f P1;
        };
    };
    f32 Radius;
};

struct ray_mesh_intersection_result
{
    b32 FoundCollision;
    f32 t;
    f32 u;
    f32 v;
};

struct penetration
{    
    v3f Normal;
    f32 Distance;
};

struct collision_result
{
    f32 t;
    struct world_entity* HitEntity;    
    penetration Penetration;    
};

struct collision_volume
{
    collision_volume_type Type;    
    union
    { 
        sphere Sphere;
        capsule Capsule;        
        struct
        {
            convex_hull* ConvexHull;        
            rigid_transform Transform;
        };
    };
};

#endif