#ifndef SIMULATION_H
#define SIMULATION_H

#include "collision_volume.h"
#include "sim_entity.h"
#include "broad_phase.h"
#include "collision_detection.h"

#define BAUMGARTE_CONSTANT 0.2f
#define PENETRATION_SLOP 0.01f

#define MAX_COLLISION_VOLUME_COUNT 1024
#define MAX_SIM_ENTITY_COUNT 512
#define MAX_RIGID_BODY_COUNT 128
#define MAX_CONTACT_COUNT 32
#define MAX_MANIFOLD_COUNT 8
#define MAX_CONSTRAINT_COUNT 8
#define MAX_COLLISION_EVENT_COUNT 64

struct contact_constraint
{
    v3f WorldPosition;
    v3f Normal;
    f32 Penetration;
    
    v3f LocalPositionA;            
    v3f LocalPositionB;
    f32 NormalMass;
    f32 NormalLambda;
    
    f32 Bias;
};

#define CONSTRAINT_CALLBACK(name) void name(rigid_body* RigidBodyA, rigid_body* RigidBodyB)
typedef CONSTRAINT_CALLBACK(constraint_callback);

struct constraint
{
    rigid_body* RigidBodyA;
    rigid_body* RigidBodyB;    
    constraint_callback* Callback;
};

struct constraint_list
{
    constraint Ptr[MAX_CONSTRAINT_COUNT];
    u32 Count;    
};

struct manifold
{
    rigid_body* RigidBodyA;
    rigid_body* RigidBodyB;        
    u64 Contacts[4];    
    
    inline u32 GetFreeContactIndex()
    {
        for(u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)
        {
            if(Contacts[ContactIndex] == 0)
                return ContactIndex;
        }
        return (u32)-1;
    }
};

struct ray_mesh_intersection_result
{
    b32 FoundCollision;
    f32 t;
    f32 u;
    f32 v;
};

struct collision_event
{
    sim_entity* SimEntityA;
    sim_entity* SimEntityB;
    v3f         Normal;
};

struct collision_event_list
{
    collision_event Ptr[MAX_COLLISION_EVENT_COUNT];
    u32 Count;
};

typedef pool<collision_volume> collision_volume_storage;
typedef pool<sim_entity> sim_entity_storage;
typedef pool<rigid_body> rigid_body_storage;
typedef pool<manifold> manifold_storage;
typedef pool<contact_constraint> contact_storage;
typedef pool<constraint> constraint_storage;

#define BROAD_PHASE_PAIR_FILTER_FUNC(name) b32 name(broad_phase_pair* Pair)
typedef BROAD_PHASE_PAIR_FILTER_FUNC(broad_phase_pair_filter_func);

struct simulation
{
    collision_volume_storage CollisionVolumeStorage;
    sim_entity_storage SimEntityStorage;
    rigid_body_storage RigidBodyStorage;
    manifold_storage   ManifoldStorage;
    contact_storage    ContactStorage;    
    constraint_list    Constraints;    
    collision_event_list CollisionEvents;
    
    void AddCollisionEvent(sim_entity* SimEntityA, sim_entity* SimEntityB, v3f Normal);
    
    sim_entity_id CreateSimEntity(sim_entity_type Type);
    void          DeleteSimEntity(sim_entity_id ID);    
    sim_entity*   GetSimEntity(sim_entity_id ID);
    template <typename type> void AddCollisionVolume(sim_entity* SimEntity, type* Collider);        
    
    broad_phase_pair_list AllocatePairList(u32 Capacity);                    
    broad_phase_pair_list GetAllPairs();
    broad_phase_pair_list GetAllPairs(rigid_body* RigidBody);
    broad_phase_pair_list GetSimEntityOnlyPairs(rigid_body* RigidBody);
    broad_phase_pair_list FilterPairs(broad_phase_pair_list Pairs, broad_phase_pair_filter_func* FilterFunc);
    
    void Integrate(f32 dt);
    void AddContactConstraints(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact_list Contacts);
    void AddContactConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact Contact);
    void AddConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, constraint_callback* Callback); 
    contact_list ComputeContacts(broad_phase_pair* Pair);
    void SolveConstraints(u32 MaxIterations, f32 dt);        
    continuous_contact ComputeTOI(rigid_body* RigidBody, broad_phase_pair_list Pairs);
};

#if DEVELOPER_BUILD
#define DEBUG_DRAW_CONTACT(Contact) DEBUG_DRAW_EDGE(Contact->Position, Contact->Position+Contact->Normal, White3()); DEBUG_DRAW_POINT(Contact->Position, Yellow3())
#else
#define DEBUG_DRAW_CONTACT(Contact)
#endif

#endif