#include "gjk.cpp"
#include "epa.cpp"
#include "collision_volumes.cpp"
#include "closest_points.cpp"
#include "rays.cpp"
#include "continuous_collisions.cpp"
#include "penetration.cpp"
#include "contacts.cpp"

template <typename type>
void sim_entity::AddCollisionVolume(simulation* Simulation, type* Collider)
{
    collision_volume* Volume = Simulation->CollisionVolumeStorage.Get(Simulation->CollisionVolumeStorage.Allocate());
    AttachToCollisionVolume(Volume, Collider);
    
    Volume->Next = CollisionVolumes;
    CollisionVolumes = Volume;
}

sim_entity* simulation::GetSimEntity(u64 ID)
{
    sim_entity* Result = SimEntityStorage.Get(ID);
    return Result;
}

void simulation::FreeSimEntity(u64 ID)
{
    sim_entity* SimEntity = GetSimEntity(ID);
    if(SimEntity->RigidBody)        
        RigidBodyStorage.Free(SimEntity->RigidBody);
    
    collision_volume* Volume = SimEntity->CollisionVolumes;
    while(Volume)
    {
        collision_volume* VolumeToFree = Volume;
        Volume = Volume->Next;
        CollisionVolumeStorage.Free(VolumeToFree);
    }
    
    SimEntityStorage.Free(ID);
}

u64 simulation::AllocateSimEntity()
{
    u64 Result = SimEntityStorage.Allocate();
    return Result;
}

manifold* simulation::GetManifold(rigid_body* A, rigid_body* B)
{
    ASSERT(A);
    FOR_EACH(Manifold, &ManifoldStorage)
    {
        if((Manifold->BodyA == A) && (Manifold->BodyB == B))
            return Manifold;            
    }
    
    manifold* Manifold = ManifoldStorage.Get(ManifoldStorage.Allocate());
    Manifold->BodyA = A;
    Manifold->BodyB = B;
    return Manifold;
}

continuous_collision simulation::DetectStaticContinuousCollisions(sim_entity* Entity)
{
    continuous_collision Result = {};
    Result.t = INFINITY;
    
    toi_result TOIResult = FindStaticTOI(this, Entity);
    if(TOIResult.HitEntity)
    {
        Result.t = TOIResult.t;
        Result.HitEntity = TOIResult.HitEntity;
        Result.Penetration = GetPenetration(Entity, TOIResult.HitEntity, TOIResult.VolumeA, TOIResult.VolumeB, TOIResult.t);
    }    
    
    return Result;
}

void simulation::GenerateContinuousContacts(collision_pair_list* RigidBodyPairs)
{
    for(u32 Index = 0; Index < RigidBodyPairs->Count; Index++)
        AddContinuousContacts(this, &RigidBodyPairs->Ptr[Index].A, &RigidBodyPairs->Ptr[Index].B);
}

void simulation::SolveConstraints(u32 MaxIterations)
{
}

inline 
m3 GetWorldInvInertiaTensor(m3 LocalTensor, quaternion Orientation)
{
    m3 OrientationMatrix = ToMatrix3(Orientation);    
    m3 Result = Transpose(OrientationMatrix)*LocalTensor*OrientationMatrix;    
    return Result;
}