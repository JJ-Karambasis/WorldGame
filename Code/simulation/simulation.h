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
#define MAX_CONSTRAINT_COUNT 64
#define MAX_COLLISION_EVENT_COUNT 64
#define SIMULATION_PAIR_CHECK_COUNT 1024        

struct contact_constraint
{
    ak_v3f WorldPosition;
    ak_v3f Normal;
    ak_f32 Penetration;
    
    ak_v3f LocalPositionA;            
    ak_v3f LocalPositionB;
    ak_f32 NormalMass;
    ak_f32 NormalLambda;
    
    ak_f32 Bias;
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
    ak_u32 Count;    
};

struct manifold
{
    rigid_body* RigidBodyA;
    rigid_body* RigidBodyB;        
    ak_u64 Contacts[4];    
    
    inline ak_u32 GetFreeContactIndex()
    {
        for(ak_u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)
        {
            if(Contacts[ContactIndex] == 0)
                return ContactIndex;
        }
        return (ak_u32)-1;
    }
};

struct ray_mesh_intersection_result
{
    ak_bool FoundCollision;
    ak_f32 t;
    ak_f32 u;
    ak_f32 v;
};

struct collision_event
{
    sim_entity* SimEntityA;
    sim_entity* SimEntityB;
    ak_v3f         Normal;
};

struct collision_event_list
{
    collision_event Ptr[MAX_COLLISION_EVENT_COUNT];
    ak_u32 Count;
};

typedef ak_pool<collision_volume> collision_volume_storage;
typedef ak_pool<sim_entity> sim_entity_storage;
typedef ak_pool<rigid_body> rigid_body_storage;
typedef ak_pool<manifold> manifold_storage;
typedef ak_pool<contact_constraint> contact_storage;
typedef ak_pool<constraint> constraint_storage;

struct simulation
{
    collision_volume_storage CollisionVolumeStorage;
    sim_entity_storage SimEntityStorage;
    rigid_body_storage RigidBodyStorage;
    manifold_storage   ManifoldStorage;
    contact_storage    ContactStorage;    
    constraint_list    Constraints;    
    collision_event_list CollisionEvents;
    ak_hash_map<ak_pair<ak_u32>, ak_bool> CollisionMap;
    
    void AddCollisionEvent(sim_entity* SimEntityA, sim_entity* SimEntityB, ak_v3f Normal);
    
    sim_entity_id CreateSimEntity(sim_entity_type Type);
    void          DeleteSimEntity(sim_entity_id ID);    
    sim_entity*   GetSimEntity(sim_entity_id ID);
    template <typename type> void AddCollisionVolume(sim_entity* SimEntity, type* Collider);        
    
    broad_phase_pair_list AllocatePairList(ak_u32 Capacity);                            
    broad_phase_pair_list FilterPairs(broad_phase_pair_list Pairs, broad_phase_pair_filter_func* FilterFunc, void* UserData = NULL);
    broad_phase_pair_list GetPairs(rigid_body* RigidBody, broad_phase_pair_filter_func* FilterFunc = NULL, void* UserData = NULL);
    broad_phase_pair_list GetAllPairs(broad_phase_pair_filter_func* FilterFunc = NULL, void* UserData = NULL);
    
    void Integrate(ak_f32 dt);
    void AddContactConstraints(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact_list Contacts);
    void AddContactConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact Contact);
    void AddConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, constraint_callback* Callback); 
    contact_list ComputeContacts(broad_phase_pair* Pair);
    contact* ComputeDeepestContact(broad_phase_pair* Pair);
    ak_bool  ComputeIntersection(broad_phase_pair* Pair);
    void SolveConstraints(ak_u32 MaxIterations, ak_f32 dt);        
    continuous_contact ComputeTOI(rigid_body* RigidBody, broad_phase_pair_list Pairs);
    continuous_contact ComputeTOI(rigid_body* RigidBodyA, sim_entity* SimEntityB);
};

#if DEVELOPER_BUILD
#define DEBUG_DRAW_CONTACT(Contact) DEBUG_DRAW_EDGE(Contact->Position, Contact->Position+Contact->Normal, White3()); DEBUG_DRAW_POINT(Contact->Position, Yellow3())
#else
#define DEBUG_DRAW_CONTACT(Contact)
#endif

#endif