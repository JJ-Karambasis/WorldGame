#ifndef COLLISION_VOLUMES_H
#define COLLISION_VOLUMES_H

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

struct collision_volume
{
    collision_volume_type Type;    
    union
    { 
        sphere Sphere;
        capsule Capsule;        
        convex_hull* ConvexHull;        
    };
    
    collision_volume* Next;
};

inline sphere 
CreateSphere(v3f CenterP, f32 Radius) 
{
    sphere Sphere = {CenterP, Radius};
    return Sphere;
}


inline capsule 
CreateCapsule(v3f P0, v3f P1, f32 Radius)
{
    capsule Capsule = {P0, P1, Radius};
    return Capsule;
}

inline capsule 
CreateCapsule(v3f Bottom, f32 Height, f32 Radius)
{
    v3f P0 = Bottom + Global_WorldZAxis*Radius;
    return CreateCapsule(P0, P0+Global_WorldZAxis*Height, Radius);
}

struct collision_volume_iter
{
    collision_volume* First;
    collision_volume* Current;
};

collision_volume_iter BeginIter(collision_volume* First)
{
    collision_volume_iter Result = {};
    Result.First = First;
    return Result;
}

collision_volume* GetFirst(collision_volume_iter* Iter)
{
    ASSERT(!Iter->Current);
    Iter->Current = Iter->First;
    return Iter->Current;
}

collision_volume* GetNext(collision_volume_iter* Iter)
{
    collision_volume* Result = Iter->Current->Next;
    Iter->Current = Result;
    return Result;
}

typedef pool<collision_volume> collision_volume_storage;

template <typename type> void AddCollisionVolume(collision_volume_storage* Pool, sim_state* Entity, type* Collider);
template <typename type> void AddCollisionVolume(game* Game, entity_id EntityID, type* Collider);
m3 GetSphereInvInertiaTensor(f32 Radius, f32 Mass);

#endif