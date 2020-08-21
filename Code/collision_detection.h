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

struct continuous_collision_result
{
    f32 t;
    world_entity_id HitEntityID;    
    penetration Penetration;    
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

typedef pool<collision_volume> collision_volume_pool;

struct collision_event
{
    world_entity_id HitEntityID;
    penetration     Penetration;
};

struct toi_result
{
    f32 t;
    world_entity_id HitEntityID;
    collision_volume* VolumeA;
    collision_volume* VolumeB;
};

inline collision_event CreateCollisionEvent(world_entity_id HitEntityID, penetration Penetration)
{
    collision_event Result;
    Result.HitEntityID = HitEntityID;
    Result.Penetration = Penetration;
    return Result;
}

struct sim_state
{
    v3f Velocity;    
    v3f MoveDelta;    
    v2f MoveDirection;    
    collision_volume* CollisionVolumes;
};

template <typename type> void AddCollisionVolume(collision_volume_pool* Pool, sim_state* Entity, type* Collider);

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

sim_state* GetSimState(game* Game, world_entity_id ID);

#endif
