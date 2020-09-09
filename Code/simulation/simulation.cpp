#include "collision_volume.cpp"
#include "collision_tables.cpp"
#include "collision_detection.cpp"

void simulation::AddCollisionEvent(sim_entity* SimEntityA, sim_entity* SimEntityB, v3f Normal)
{
    ASSERT(CollisionEvents.Count < MAX_COLLISION_EVENT_COUNT);
    CollisionEvents.Ptr[CollisionEvents.Count++] = {SimEntityA, SimEntityB, Normal};
}

sim_entity_id simulation::CreateSimEntity(sim_entity_type Type)
{
    sim_entity_id Result = {};
    
    sim_entity* Entity = NULL;
    switch(Type)
    {
        case SIM_ENTITY_TYPE_SIM_ENTITY:
        {
            Result.ID = SimEntityStorage.Allocate();
            Entity = SimEntityStorage.Get(Result.ID);
        } break;
        
        case SIM_ENTITY_TYPE_RIGID_BODY:
        {
            Result.ID = RigidBodyStorage.Allocate();
            Entity = RigidBodyStorage.Get(Result.ID);
        } break;
    }
    
    Result.Type = Type; 
    Entity->ID = Result;           
    return Result;
}

void simulation::DeleteSimEntity(sim_entity_id ID)
{
    sim_entity* SimEntity = GetSimEntity(ID);
    
    collision_volume* Volume = SimEntity->CollisionVolumes;
    while(Volume)
    {
        collision_volume* VolumeToFree = Volume;
        Volume = Volume->Next;
        CollisionVolumeStorage.Free(VolumeToFree);
    }
    
    switch(ID.Type)
    {
        case SIM_ENTITY_TYPE_SIM_ENTITY:
        {
            SimEntityStorage.Free(ID.ID);
        } break;
        
        case SIM_ENTITY_TYPE_RIGID_BODY:
        {
            RigidBodyStorage.Free(ID.ID);
        } break;        
    }
}

sim_entity* simulation::GetSimEntity(sim_entity_id ID)
{
    switch(ID.Type)
    {
        case SIM_ENTITY_TYPE_SIM_ENTITY:
        {
            return SimEntityStorage.Get(ID.ID);
        } break;
        
        case SIM_ENTITY_TYPE_RIGID_BODY:
        {
            return RigidBodyStorage.Get(ID.ID);
        } break;                
    }
    
    return NULL;
}

template <typename type> 
void simulation::AddCollisionVolume(sim_entity* SimEntity, type* Collider)
{
    collision_volume* Volume = CollisionVolumeStorage.Get(CollisionVolumeStorage.Allocate());
    AttachToCollisionVolume(Volume, Collider);
    
    Volume->Next = SimEntity->CollisionVolumes;
    SimEntity->CollisionVolumes = Volume;
}

broad_phase_pair_list simulation::AllocatePairList(u32 Capacity)
{
    broad_phase_pair_list Result = {};
    Result.Capacity = Capacity;
    Result.Ptr = PushArray(Result.Capacity, broad_phase_pair, Clear, 0);
    return Result;
}

broad_phase_pair_list simulation::GetAllPairs()
{    
#define PAIR_CHECK_COUNT 1024    
    struct collision_pair_check
    {
        u64 ID;
        b32 Collided;
    } CollisionPairChecks[PAIR_CHECK_COUNT] = {};
    
    broad_phase_pair_list Result = AllocatePairList(PAIR_CHECK_COUNT);
    
    FOR_EACH(RigidBody, &RigidBodyStorage)
    {
        FOR_EACH(TestRigidBody, &RigidBodyStorage)
        {
            if(RigidBody != TestRigidBody)
            {
                u32 AIndex = RigidBodyStorage.GetIndex(RigidBody);
                u32 BIndex = RigidBodyStorage.GetIndex(TestRigidBody);            
                
                if(BIndex < AIndex)
                    SWAP(AIndex, BIndex);
                
                u64 ID = BijectiveMap(AIndex, BIndex);
                u64 Index = ID % PAIR_CHECK_COUNT;
                
                collision_pair_check* Check = CollisionPairChecks + Index++;
                
                b32 AlreadyCollided = false;
                while(Check->Collided)
                {
                    if(Check->ID == ID)
                    {
                        AlreadyCollided = true;
                        break;
                    }                        
                    Check = CollisionPairChecks + Index++;
                    ASSERT(Index < PAIR_CHECK_COUNT);
                }
                
                if(!AlreadyCollided)
                {
                    Check->Collided = true;
                    Check->ID = ID;        
                    Result.AddAllVolumes(RigidBody, TestRigidBody);                                
                }
            }
        }
        
        FOR_EACH(SimEntity, &SimEntityStorage) { Result.AddAllVolumes(RigidBody, SimEntity); }
    }
    
    return Result;
}

broad_phase_pair_list simulation::GetAllPairs(rigid_body* RigidBody)
{
    broad_phase_pair_list Result = AllocatePairList(PAIR_CHECK_COUNT);    
    FOR_EACH(SimEntity, &SimEntityStorage) { Result.AddAllVolumes(RigidBody, SimEntity); }
    FOR_EACH(TestRigidBody, &RigidBodyStorage) { if(RigidBody != TestRigidBody) Result.AddAllVolumes(RigidBody, TestRigidBody); }
    return Result;
}

broad_phase_pair_list simulation::GetSimEntityOnlyPairs(rigid_body* RigidBody)
{
    broad_phase_pair_list Result = AllocatePairList(PAIR_CHECK_COUNT);    
    FOR_EACH(SimEntity, &SimEntityStorage) { Result.AddAllVolumes(RigidBody, SimEntity); }
    return Result;
}

broad_phase_pair_list simulation::FilterPairs(broad_phase_pair_list Pairs, broad_phase_pair_filter_func* FilterFunc)
{
    broad_phase_pair_list Result = AllocatePairList(Pairs.Count);
    for(u32 PairIndex = 0; PairIndex < Pairs.Count; PairIndex++)
    {
        broad_phase_pair* Pair = Pairs.Ptr + PairIndex;
        if(FilterFunc(Pair))
            Result.AddPair(Pair->SimEntityA, Pair->SimEntityB, Pair->VolumeA, Pair->VolumeB);
    }    
    return Result;
}

void simulation::Integrate(f32 dt)
{
    FOR_EACH(RigidBody, &RigidBodyStorage)
    {
        RigidBody->WorldInvInertiaTensor = RigidBody->GetWorldInvInertiaTensor();
        RigidBody->WorldCenterOfMass = TransformV3(RigidBody->LocalCenterOfMass, RigidBody->Transform);
        
        RigidBody->Acceleration = (RigidBody->Force * RigidBody->InvMass);
        RigidBody->AngularAcceleration = (RigidBody->Torque*RigidBody->WorldInvInertiaTensor);
        
        RigidBody->Velocity += (RigidBody->Acceleration*dt);
        RigidBody->AngularVelocity += (RigidBody->AngularAcceleration*dt);
        
        RigidBody->ApplyLinearDamp(dt);
        RigidBody->ApplyAngularDamp(dt);
        
        //RigidBody->ClearForce();
        //RigidBody->ClearTorque();        
    }
}

void simulation::AddContactConstraints(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact_list Contacts)
{
    ASSERT(RigidBodyA || RigidBodyB);
    if(!RigidBodyA)
    {
        RigidBodyA = RigidBodyB;
        RigidBodyB = NULL;
        Contacts.FlipNormals();        
    }
    
    manifold* Manifold = NULL;
    
    FOR_EACH(ManifoldEntry, &ManifoldStorage)
    {
        if((ManifoldEntry->RigidBodyA == RigidBodyA) && (ManifoldEntry->RigidBodyB == RigidBodyB))
        {
            Manifold = ManifoldEntry;
            break;
        }
    }
    
    if(!Manifold)
    {
        Manifold = ManifoldStorage.Get(ManifoldStorage.Allocate());
        Manifold->RigidBodyA = RigidBodyA;
        Manifold->RigidBodyB = RigidBodyB;
    }
    
    for(u32 ContactIndex = 0; ContactIndex < Contacts.Count; ContactIndex++)
    {
        contact* Contact = Contacts.Ptr + ContactIndex;
        
        u64 ContactID = ContactStorage.Allocate();        
        contact_constraint* Constraint = ContactStorage.Get(ContactID);        
        
        Constraint->WorldPosition = Contact->Position;
        Constraint->Normal = Contact->Normal;
        Constraint->Penetration = Contact->Penetration;
        
        u32 FreeContactIndex = Manifold->GetFreeContactIndex();
        ASSERT(FreeContactIndex != (u32)-1);        
        Manifold->Contacts[FreeContactIndex] = ContactID;        
    }
}

void simulation::AddContactConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact Contact)
{
    contact_list ContactList = {};
    ContactList.Ptr = &Contact;    
    ContactList.Count = 1;
    AddContactConstraints(RigidBodyA, RigidBodyB, ContactList);
}

void simulation::AddConstraint(rigid_body* RigidBodyA, rigid_body* RigidBodyB, constraint_callback* Callback)
{
    ASSERT(Constraints.Count < MAX_CONSTRAINT_COUNT);
    constraint* Constraint = Constraints.Ptr + Constraints.Count++;
    Constraint->RigidBodyA = RigidBodyA;
    Constraint->RigidBodyB = RigidBodyB;
    Constraint->Callback   = Callback;
}

contact_list simulation::ComputeContacts(broad_phase_pair* Pair)
{
    contact_function* ContactFunction = ContactFunctions[Pair->VolumeA->Type][Pair->VolumeB->Type];
    contact_list ContactList = ContactFunction(Pair->SimEntityA, Pair->SimEntityB, Pair->VolumeA, Pair->VolumeB);            
    if(ContactList.Count > 0)    
        AddCollisionEvent(Pair->SimEntityA, Pair->SimEntityB, ContactList.GetDeepestContact()->Normal);            
    
    return ContactList;
}

inline f32 
Baumgarte(f32 D, f32 dt)
{
    f32 Result = BAUMGARTE_CONSTANT * (1.0f/dt) * MaximumF32(0.0f, D-PENETRATION_SLOP);
    return Result;
}

inline f32 
MixRestitution(rigid_body* A, rigid_body* B)
{
    f32 Result = A->Restitution;
    if(B) Result = MaximumF32(Result, B->Restitution);
    return Result;
}

void simulation::SolveConstraints(u32 MaxIterations, f32 dt)
{
    FOR_EACH(Manifold, &ManifoldStorage)
    {
        rigid_body* RigidBodyA = Manifold->RigidBodyA;        
        rigid_body* RigidBodyB = Manifold->RigidBodyB;
        
        v3f vA = RigidBodyA->Velocity;
        v3f wA = RigidBodyA->AngularVelocity;
        v3f vB = RigidBodyB ? RigidBodyB->Velocity : V3();
        v3f wB = RigidBodyB ? RigidBodyB->AngularVelocity : V3();
        
        f32 Restitution = MixRestitution(RigidBodyA, RigidBodyB);
        
        for(u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)        
        {   
            u64 ContactID = Manifold->Contacts[ContactIndex];
            if(ContactID == 0)
                continue;
            
            contact_constraint* Constraint = ContactStorage.Get(ContactID);
            if(RigidBodyB)
            {                
                Constraint->LocalPositionA = Constraint->WorldPosition - RigidBodyA->WorldCenterOfMass;
                Constraint->LocalPositionB = Constraint->WorldPosition - RigidBodyB->WorldCenterOfMass;
                
                v3f ARotAxis = Cross(Constraint->LocalPositionA, Constraint->Normal);
                v3f BRotAxis = Cross(Constraint->LocalPositionB, Constraint->Normal);
                
                Constraint->NormalMass = RigidBodyA->InvMass + RigidBodyB->InvMass;
                Constraint->NormalMass += (Dot(ARotAxis, ARotAxis*RigidBodyA->WorldInvInertiaTensor) +
                                           Dot(BRotAxis, BRotAxis*RigidBodyB->WorldInvInertiaTensor));
                Constraint->NormalMass = SafeInverse(Constraint->NormalMass);
                
                Constraint->Bias = Baumgarte(Constraint->Penetration, dt);
                
                f32 RelVelocityProjected = Dot(vB + Cross(wB, Constraint->LocalPositionB) - vA - Cross(wA, Constraint->LocalPositionA), Constraint->Normal);                                               
                if(RelVelocityProjected < -1)
                    Constraint->Bias += -(Restitution)*RelVelocityProjected;
                
            }
            else
            {
                Constraint->LocalPositionA = Constraint->WorldPosition - RigidBodyA->WorldCenterOfMass;                
                v3f ARotAxis = Cross(Constraint->LocalPositionA, Constraint->Normal);
                
                Constraint->NormalMass = RigidBodyA->InvMass;
                Constraint->NormalMass += (Dot(ARotAxis, ARotAxis*RigidBodyA->WorldInvInertiaTensor));
                Constraint->NormalMass = SafeInverse(Constraint->NormalMass);
                
                Constraint->Bias = Baumgarte(Constraint->Penetration, dt);
                
                f32 RelVelocityProjected = Dot(-vA - Cross(wA, Constraint->LocalPositionA), Constraint->Normal);
                if(RelVelocityProjected < -1)
                    Constraint->Bias += -(Restitution)*RelVelocityProjected;
            }
        }        
    }
    
    for(u32 Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        FOR_EACH(Manifold, &ManifoldStorage)
        {
            rigid_body* RigidBodyA = Manifold->RigidBodyA;        
            rigid_body* RigidBodyB = Manifold->RigidBodyB;
            
            v3f vA = RigidBodyA->Velocity;
            v3f wA = RigidBodyA->AngularVelocity;
            v3f vB = RigidBodyB ? RigidBodyB->Velocity : V3();
            v3f wB = RigidBodyB ? RigidBodyB->AngularVelocity : V3();
            
            for(u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)        
            {   
                u64 ContactID = Manifold->Contacts[ContactIndex];
                if(ContactID == 0)
                    continue;
                
                contact_constraint* Constraint = ContactStorage.Get(ContactID);                                
                if(RigidBodyB)
                {
                    v3f RelVelocity = vB + Cross(wB, Constraint->LocalPositionB) - vA - Cross(wA, Constraint->LocalPositionA);
                    f32 RelVelocityProj = Dot(RelVelocity, Constraint->Normal);
                    
                    f32 Lambda = Constraint->NormalMass * (-RelVelocityProj + Constraint->Bias);
                    
                    f32 TempLambda = Constraint->NormalLambda;
                    Constraint->NormalLambda = MaximumF32(TempLambda+Lambda, 0.0f);
                    Lambda = Constraint->NormalLambda - TempLambda;
                    
                    v3f Impulse = Constraint->Normal*Lambda;
                    
                    vA -= Impulse*RigidBodyA->InvMass;
                    wA -= Cross(Constraint->LocalPositionA, Impulse)*RigidBodyA->WorldInvInertiaTensor;
                    
                    vB += Impulse*RigidBodyB->InvMass;
                    wB += Cross(Constraint->LocalPositionB, Impulse)*RigidBodyB->WorldInvInertiaTensor;                    
                }
                else
                {
                    v3f RelVelocity = -vA - Cross(wA, Constraint->LocalPositionA);
                    f32 RelVelocityProj = Dot(RelVelocity, Constraint->Normal);
                    
                    f32 Lambda = Constraint->NormalMass * (-RelVelocityProj + Constraint->Bias);
                    
                    f32 TempLambda = Constraint->NormalLambda;
                    Constraint->NormalLambda = MaximumF32(TempLambda+Lambda, 0.0f);
                    Lambda = Constraint->NormalLambda - TempLambda;
                    
                    v3f Impulse = Constraint->Normal*Lambda;
                    
                    vA -= Impulse*RigidBodyA->InvMass;
                    wA -= Cross(Constraint->LocalPositionA, Impulse)*RigidBodyA->WorldInvInertiaTensor;
                }
                
            }
            
            RigidBodyA->Velocity   = vA;
            RigidBodyA->AngularVelocity = wA;
            
            if(RigidBodyB)
            {
                RigidBodyB->Velocity = vB;
                RigidBodyB->AngularVelocity = wB;
            }
        }
    }
    
    for(u32 Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        for(u32 ConstraintIndex = 0; ConstraintIndex < Constraints.Count; ConstraintIndex++)
        {
            constraint* Constraint = Constraints.Ptr + ConstraintIndex;            
            Constraint->Callback(Constraint->RigidBodyA, Constraint->RigidBodyB);
        }
    }
    
    FOR_EACH(RigidBody, &RigidBodyStorage)
    {
        quaternion Delta = RotQuat(RigidBody->AngularVelocity*dt, 0.0f)*RigidBody->Transform.Orientation;
        RigidBody->Transform.Orientation = Normalize(RigidBody->Transform.Orientation + (Delta*0.5f));
        RigidBody->MoveDelta = RigidBody->Velocity*dt;
    }
    
    ContactStorage.FreeAll();
    ManifoldStorage.FreeAll();    
    Constraints.Count = 0;    
}

continuous_contact simulation::ComputeTOI(rigid_body* RigidBody, broad_phase_pair_list Pairs)
{
    
#define UPDATE_HIT() \
    if((t != INFINITY) && (t < TOIResult.t)) \
    { \
        TOIResult.HitEntity = Pair->SimEntityB; \
        TOIResult.t = MaximumF32(0.0f, t-1e-3f); \
        TOIResult.VolumeA = Pair->VolumeA; \
        TOIResult.VolumeB = Pair->VolumeB; \
    }
    
    toi_result TOIResult = InvalidTOIResult();
    for(u32 PairIndex = 0; PairIndex < Pairs.Count; PairIndex++)
    {
        broad_phase_pair* Pair = Pairs.Ptr + PairIndex;
        
        ccd_function* CCDFunction = CCDFunctions[Pair->VolumeA->Type][Pair->VolumeB->Type];        
        f32 t = CCDFunction(Pair->SimEntityA, Pair->SimEntityB, Pair->VolumeA, Pair->VolumeB);
        UPDATE_HIT();
    }
    
#undef UPDATE_HIT
    
    continuous_contact Result = {};
    Result.t = TOIResult.t;    
    if(Result.t != INFINITY)    
    {
        Result.HitEntity = TOIResult.HitEntity;        
        ccd_contact_function* CCDContactFunction = CCDContactFunctions[TOIResult.VolumeA->Type][TOIResult.VolumeB->Type];
        Result.Contact = CCDContactFunction(RigidBody, TOIResult.HitEntity, TOIResult.VolumeA, TOIResult.VolumeB, Result.t);                        
        AddCollisionEvent(RigidBody, TOIResult.HitEntity, Result.Contact.Normal);
    }
    
    return Result;
}

CONSTRAINT_CALLBACK(LockConstraint)
{
    ASSERT(!RigidBodyB);
    
    v4f PLock[3] = 
    {
        V4(0.0f, 1.0f, 0.0f, 0.0f),
        V4(0.0f, 0.0f, 1.0f, 0.0f),
        V4(0.0f, 0.0f, 0.0f, 1.0f)
    };    
    
    quaternion Orient = RigidBodyA->Transform.Orientation;                
    
    v4f Eqt[3];        
    Eqt[0] = V4(-Orient.v.x,  Orient.s,   -Orient.v.z,  Orient.v.y);
    Eqt[1] = V4(-Orient.v.y,  Orient.v.z,  Orient.s,   -Orient.v.x);
    Eqt[2] = V4(-Orient.v.z, -Orient.v.y,  Orient.v.x,  Orient.s);
    
    v3f Jacobian[3] = 
    {
        -0.5f*V3(Dot(PLock[0], Eqt[0]), Dot(PLock[1], Eqt[0]), Dot(PLock[2], Eqt[0])),
        -0.5f*V3(Dot(PLock[0], Eqt[1]), Dot(PLock[1], Eqt[1]), Dot(PLock[2], Eqt[1])),
        -0.5f*V3(Dot(PLock[0], Eqt[2]), Dot(PLock[1], Eqt[2]), Dot(PLock[2], Eqt[2]))
    };
    
    v3f DeltaVel = V3(Dot(Jacobian[0], RigidBodyA->AngularVelocity), 
                      Dot(Jacobian[1], RigidBodyA->AngularVelocity),
                      Dot(Jacobian[2], RigidBodyA->AngularVelocity));
    
    RigidBodyA->AngularVelocity = DeltaVel;                
    
}

simulation CreateSimulation(arena* Arena)
{
    simulation Simulation = {};
    Simulation.CollisionVolumeStorage = CreatePool<collision_volume>(Arena, MAX_COLLISION_VOLUME_COUNT);    
    Simulation.SimEntityStorage = CreatePool<sim_entity>(Arena, MAX_SIM_ENTITY_COUNT);    
    Simulation.RigidBodyStorage = CreatePool<rigid_body>(Arena, MAX_RIGID_BODY_COUNT);    
    Simulation.ContactStorage = CreatePool<contact_constraint>(Arena, MAX_CONTACT_COUNT);
    Simulation.ManifoldStorage = CreatePool<manifold>(Arena, MAX_MANIFOLD_COUNT);    
    
    return Simulation;
}