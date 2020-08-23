#ifndef SIMULATION_H
#define SIMULATION_H

struct sim_state;

#include "support.h"
#include "collision_volumes.h"
#include "penetration.h"
#include "continuous_collisions.h"
#include "rays.h"
#include "contacts.h"



struct sim_state
{
    v3f Velocity;            
    v3f Acceleration;
    
    v3f AngularVelocity;
    v3f AngularAcceleration;
    
    v3f MoveDelta;
    
    f32 InvMass;
    m3  InvInertiaTensor;            
    collision_volume* CollisionVolumes;
};

struct collision_event
{
    entity_id HitEntityID;
    penetration Penetration;
};

struct entity_collision_volume
{
    entity_id EntityID;
    collision_volume* Volume;
};

struct collision_pair
{    
    entity_collision_volume A;
    entity_collision_volume B;    
};

struct collision_pair_list
{
    collision_pair* Ptr;
    u32 Count;
    u32 Capacity;
};

struct collision_pair_check
{
    u64 ID;
    b32 Collided;
};

inline collision_event 
CreateCollisionEvent(entity_id HitEntityID, penetration Penetration)
{
    collision_event Result = {HitEntityID, Penetration};
    return Result;
}

#endif