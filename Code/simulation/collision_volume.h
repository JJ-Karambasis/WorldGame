#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

enum collision_volume_type
{    
    COLLISION_VOLUME_TYPE_SPHERE,
    COLLISION_VOLUME_TYPE_CAPSULE,
    COLLISION_VOLUME_TYPE_CONVEX_HULL    
};

struct sphere
{
    ak_v3f CenterP;
    ak_f32 Radius;
};

struct capsule
{
    union
    {
        ak_v3f P[2];
        struct
        {
            ak_v3f P0;
            ak_v3f P1;
        };
    };
    ak_f32 Radius;
    
    inline ak_f32 GetHeight() { return AK_Magnitude(P1-P0); }
    inline ak_v3f GetCenter() { return P0 + (P1-P0)*0.5f; }
    inline ak_v3f GetBottom() { return P0 - AK_Normalize(P1-P0)*Radius; }
};

struct collision_volume
{    
    collision_volume_type Type;    
    union
    { 
        sphere Sphere;
        capsule Capsule;        
        convex_hull* ConvexHull;        
    };    
    
    ak_u64 ID;
    ak_u64 NextID;
};

#endif