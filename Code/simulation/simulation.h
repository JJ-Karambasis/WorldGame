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

#define BAUMGARTE_CONSTANT 0.2f
#define PENETRATION_SLOP 0.01f

struct sim_entity
{
    sqt         Transform;           
    v3f         Velocity;        
    v3f         Acceleration;        
    v3f         MoveDelta;
    b32         DetectDuringCCD;    
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
    v3f LocalCenterOfMass;
    v3f WorldCenterOfMass;
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

struct sim_entity_rigid_body_volume_pair
{
    sim_entity* SimEntity;
    rigid_body* RigidBody;
    collision_volume* Volume;
};

struct contact_pair
{
    sim_entity_rigid_body_volume_pair A;
    sim_entity_rigid_body_volume_pair B;
};

struct contact_pair_list
{
    contact_pair* Ptr;
    u32 Count;
    u32 Capacity;
};

typedef pool<rigid_body> rigid_body_storage;
typedef pool<sim_entity> sim_entity_storage;

struct collision_event
{
    sim_entity* A;
    sim_entity* B;
    penetration Penetration;
};

struct collision_event_list
{    
    collision_event* Ptr;
    u32 Capacity;
    u32 Count;
};

struct simulation
{
    collision_volume_storage CollisionVolumeStorage;
    contact_storage          ContactStorage;
    manifold_storage         ManifoldStorage;
    rigid_body_storage       RigidBodyStorage;
    sim_entity_storage       SimEntityStorage;                          
    collision_event_list CollisionEvents;
    
    sim_entity* GetSimEntity(u64 ID);        
    void        FreeSimEntity(u64 ID);
    
    u64         AllocateSimEntity();        
    manifold*   GetManifold(rigid_body* A, rigid_body* B);
    
    continuous_collision DetectContinuousCollisions(sim_entity* Entity);        
    void GenerateContacts(contact_pair_list* RigidBodyPairs);
    void SolveConstraints(u32 MaxIterations, f32 dt);
    
    void AddCollisionEvent(sim_entity* A, sim_entity* B, penetration Penetration);
};

#endif