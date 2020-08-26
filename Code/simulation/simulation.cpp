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

void simulation::GenerateContacts(collision_pair_list* RigidBodyPairs)
{
    for(u32 Index = 0; Index < RigidBodyPairs->Count; Index++)
        AddContacts(this, &RigidBodyPairs->Ptr[Index].A, &RigidBodyPairs->Ptr[Index].B);
}

f32 Baumgarte(f32 D, f32 dt)
{
    f32 Result = -BAUMGARTE_CONSTANT * (1.0f/dt) * MinimumF32(0.0f, D+PENETRATION_SLOP);
    return Result;
}

f32 MixRestitution(rigid_body* A, rigid_body* B)
{
    f32 Result = A->Restitution;
    if(B) Result = MaximumF32(Result, B->Restitution);
    return Result;
}

void simulation::SolveConstraints(u32 MaxIterations, f32 dt)
{
    FOR_EACH(Manifold, &ManifoldStorage)
    {        
        rigid_body* BodyA = Manifold->BodyA;        
        rigid_body* BodyB = Manifold->BodyB;
        
        sim_entity* SimEntityA = BodyA->SimEntity;
        sim_entity* SimEntityB = BodyB ? BodyB->SimEntity : NULL;
        
        v3f vA = SimEntityA->Velocity;
        v3f wA = BodyA->AngularVelocity;
        v3f vB = SimEntityB ? SimEntityB->Velocity : V3();
        v3f wB = BodyB ? BodyB->AngularVelocity : V3();
        
        f32 Restitution = MixRestitution(BodyA, BodyB);
        
        FOR_EACH(Contact, &Manifold->Contacts)
        {            
            if(BodyB)
            {                
                Contact->LocalPositionA = Contact->WorldPosition - BodyA->WorldCenterOfMass;
                Contact->LocalPositionB = Contact->WorldPosition - BodyB->WorldCenterOfMass;
                
                v3f ARotAxis = Cross(Contact->LocalPositionA, Contact->Normal);
                v3f BRotAxis = Cross(Contact->LocalPositionB, Contact->Normal);
                
                Contact->NormalMass = BodyA->InvMass + BodyB->InvMass;
                Contact->NormalMass += (Dot(ARotAxis, ARotAxis*BodyA->WorldInvInertiaTensor) +
                                        Dot(BRotAxis, BRotAxis*BodyB->WorldInvInertiaTensor));
                Contact->NormalMass = SafeInverse(Contact->NormalMass);
                
                Contact->Bias = Baumgarte(-Contact->Penetration, dt);
                
                f32 RelVelocityProjected = Dot(vB + Cross(wB, Contact->LocalPositionB) - vA - Cross(wA, Contact->LocalPositionA), Contact->Normal);                                               
                if(RelVelocityProjected < -1)
                    Contact->Bias += -(Restitution)*RelVelocityProjected;
                
            }
            else
            {
                Contact->LocalPositionA = Contact->WorldPosition - BodyA->WorldCenterOfMass;                
                v3f ARotAxis = Cross(Contact->LocalPositionA, Contact->Normal);
                
                Contact->NormalMass = BodyA->InvMass;
                Contact->NormalMass += (Dot(ARotAxis, ARotAxis*BodyA->WorldInvInertiaTensor));
                Contact->NormalMass = SafeInverse(Contact->NormalMass);
                
                Contact->Bias = Baumgarte(Contact->Penetration, dt);
                
                f32 RelVelocityProjected = Dot(-vA - Cross(wA, Contact->LocalPositionA), Contact->Normal);
                if(RelVelocityProjected < -1)
                    Contact->Bias += -(Restitution)*RelVelocityProjected;
            }
        }        
    }
    
    for(u32 Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        FOR_EACH(Manifold, &ManifoldStorage)
        {
            rigid_body* BodyA = Manifold->BodyA;        
            rigid_body* BodyB = Manifold->BodyB;
            
            sim_entity* SimEntityA = BodyA->SimEntity;
            sim_entity* SimEntityB = BodyB ? BodyB->SimEntity : NULL;
            
            v3f vA = SimEntityA->Velocity;
            v3f wA = BodyA->AngularVelocity;
            v3f vB = SimEntityB ? SimEntityB->Velocity : V3();
            v3f wB = BodyB ? BodyB->AngularVelocity : V3();
            
            FOR_EACH(Contact, &Manifold->Contacts)
            {
                if(BodyB)
                {
                    v3f RelVelocity = vB + Cross(wB, Contact->LocalPositionB) - vA - Cross(wA, Contact->LocalPositionA);
                    f32 RelVelocityProj = Dot(RelVelocity, Contact->Normal);
                    
                    f32 Lambda = Contact->NormalMass * (-RelVelocityProj + Contact->Bias);
                    
                    f32 TempLambda = Contact->NormalLambda;
                    Contact->NormalLambda = MaximumF32(TempLambda+Lambda, 0.0f);
                    Lambda = Contact->NormalLambda - TempLambda;
                    
                    v3f Impulse = Contact->Normal*Lambda;
                    
                    vA -= Impulse*BodyA->InvMass;
                    wA -= Cross(Contact->LocalPositionA, Impulse)*BodyA->WorldInvInertiaTensor;
                    
                    vB += Impulse*BodyB->InvMass;
                    wB += Cross(Contact->LocalPositionB, Impulse)*BodyB->WorldInvInertiaTensor;                    
                }
                else
                {
                    v3f RelVelocity = -vA - Cross(wA, Contact->LocalPositionA);
                    f32 RelVelocityProj = Dot(RelVelocity, Contact->Normal);
                    
                    f32 Lambda = Contact->NormalMass * (-RelVelocityProj + Contact->Bias);
                    
                    f32 TempLambda = Contact->NormalLambda;
                    Contact->NormalLambda = MaximumF32(TempLambda+Lambda, 0.0f);
                    Lambda = Contact->NormalLambda - TempLambda;
                    
                    v3f Impulse = Contact->Normal*Lambda;
                    
                    vA -= Impulse*BodyA->InvMass;
                    wA -= Cross(Contact->LocalPositionA, Impulse)*BodyA->WorldInvInertiaTensor;
                }
                
            }
            
            SimEntityA->Velocity   = vA;
            BodyA->AngularVelocity = wA;
            
            if(SimEntityB && BodyB)
            {
                SimEntityB->Velocity = vB;
                BodyB->AngularVelocity = wB;
            }
        }
    }
}

inline 
m3 GetWorldInvInertiaTensor(m3 LocalTensor, quaternion Orientation)
{
    m3 OrientationMatrix = ToMatrix3(Orientation);    
    m3 Result = Transpose(OrientationMatrix)*LocalTensor*OrientationMatrix;    
    return Result;
}