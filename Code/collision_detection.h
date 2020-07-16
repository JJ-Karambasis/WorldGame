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
    v3f P0;
    v3f P1;
    f32 Radius;
};

struct collision_result
{
    b32 FoundCollision;    
    v3f ContactPoint;
    v3f Normal;    
    f32 t;
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

inline collision_result
InvalidCollisionResult()
{
    collision_result Result = {};
    Result.t = INFINITY;
    Result.ContactPoint = Result.Normal = InvalidV3();
    return Result;
}

#endif