#ifndef SIMULATION_H
#define SIMULATION_H

struct sim_entity;
struct simulation;


#include "support.h"
#include "collision_volumes.h"
#include "penetration.h"
#include "continuous_collisions.h"
#include "rays.h"
#include "contacts.h"

struct sim_entity
{
    sqt         Transform;           
    v3f         Velocity;        
    v3f         Acceleration;        
    v3f         MoveDelta;
    rigid_body* RigidBody;    
    collision_volume* CollisionVolumes;    
    
    void* UserData;
    
    template <typename type> void AddCollisionVolume(simulation* Simulation, type* Collider);
};

struct rigid_body
{
    sim_entity* SimEntity;      
    v3f AngularVelocity;    
    v3f AngularAcceleration;        
    f32 Restitution;
    f32 InvMass;
    m3  LocalInvInertiaTensor;
    m3  WorldInvInertiaTensor;    
};

struct sim_entity_volume_pair
{
    sim_entity*       SimEntity;
    collision_volume* Volume;    
};

struct collision_pair
{    
    sim_entity_volume_pair A;
    sim_entity_volume_pair B;    
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

typedef pool<rigid_body> rigid_body_storage;
typedef pool<sim_entity> sim_entity_storage;

struct simulation
{
    collision_volume_storage CollisionVolumeStorage;
    contact_storage          ContactStorage;
    manifold_storage         ManifoldStorage;
    rigid_body_storage       RigidBodyStorage;
    sim_entity_storage       SimEntityStorage;          
        
    sim_entity* GetSimEntity(u64 ID);        
    void        FreeSimEntity(u64 ID);
    
    u64         AllocateSimEntity();        
    manifold*   GetManifold(rigid_body* A, rigid_body* B);
    
    continuous_collision DetectStaticContinuousCollisions(sim_entity* Entity);        
    void GenerateContinuousContacts(collision_pair_list* RigidBodyPairs);
    void SolveConstraints(u32 MaxIterations);
};

#endif