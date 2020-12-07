#include "support.cpp"
#include "gjk.cpp"
#include "epa.cpp"

#include "collision_volume.cpp"
#include "collision_tables.cpp"
#include "broad_phase.cpp"
#include "collision_detection.cpp"

void simulation::AddCollisionEvent(sim_entity* SimEntityA, sim_entity* SimEntityB, ak_v3f Normal)
{
    AK_Assert(CollisionEvents.Count < MAX_COLLISION_EVENT_COUNT, "Index out of bounds");
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
    
    collision_volume* Volume = CollisionVolumeStorage.Get(SimEntity->CollisionVolumeID);
    while(Volume)
    {
        collision_volume* VolumeToFree = Volume;
        Volume = CollisionVolumeStorage.Get(Volume->NextID);        
        CollisionVolumeStorage.Free(VolumeToFree->ID);
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
    ak_u64 ID = CollisionVolumeStorage.Allocate();
    collision_volume* Volume = CollisionVolumeStorage.Get(ID);
    AttachToCollisionVolume(Volume, Collider);
    
    Volume->ID = ID;
    Volume->NextID = SimEntity->CollisionVolumeID;
    SimEntity->CollisionVolumeID = ID;    
}

broad_phase_pair_list simulation::AllocatePairList(ak_u32 Capacity)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    
    broad_phase_pair_list Result = {};
    Result.Capacity = Capacity;
    Result.Ptr = GlobalArena->PushArray<broad_phase_pair>(Result.Capacity);
    return Result;
}

broad_phase_pair_list simulation::FilterPairs(broad_phase_pair_list Pairs, broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    broad_phase_pair_list Result = AllocatePairList(Pairs.Count);
    for(ak_u32 PairIndex = 0; PairIndex < Pairs.Count; PairIndex++)
    {
        broad_phase_pair* Pair = Pairs.Ptr + PairIndex;
        if(FilterFunc(Pair, UserData))
            Result.AddPair(Pair->SimEntityA, Pair->SimEntityB, Pair->AVolumeID, Pair->BVolumeID);
    }    
    return Result;
}

broad_phase_pair_list simulation::GetPairs(rigid_body* RigidBody, broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    if(FilterFunc == NULL)
        FilterFunc = FilterFuncPass;
    
    broad_phase_pair_list Result = AllocatePairList(SIMULATION_PAIR_CHECK_COUNT);    
    AK_ForEach(SimEntity, &SimEntityStorage) { Result.AddPassedVolumes(this, RigidBody, SimEntity, FilterFunc, UserData); }
    AK_ForEach(TestRigidBody, &RigidBodyStorage) { if(RigidBody != TestRigidBody) Result.AddPassedVolumes(this, RigidBody, TestRigidBody, FilterFunc, UserData); }
    return Result;    
}

broad_phase_pair_list simulation::GetAllPairs(broad_phase_pair_filter_func* FilterFunc, void* UserData)
{   
    if(FilterFunc == NULL)
        FilterFunc = FilterFuncPass;    
    
    if(!CollisionMap.Slots)
        CollisionMap = AK_CreateHashMap<ak_pair<ak_u32>, ak_bool>(8191);    
    
    broad_phase_pair_list Result = AllocatePairList(SIMULATION_PAIR_CHECK_COUNT);
    
    AK_ForEach(RigidBody, &RigidBodyStorage)
    {
        AK_ForEach(TestRigidBody, &RigidBodyStorage)
        {
            if(RigidBody != TestRigidBody)
            {
                ak_u32 AIndex = (ak_u32)(RigidBody->ID.ID & 0xFFFFFFFF);
                ak_u32 BIndex = (ak_u32)(TestRigidBody->ID.ID & 0xFFFFFFFF);
                
                ak_pair<ak_u32> Pair = {AIndex, BIndex};
                if(!CollisionMap.Find(Pair))
                {
                    CollisionMap.Insert(Pair, true);
                    Result.AddPassedVolumes(this, RigidBody, TestRigidBody, FilterFunc, UserData);
                }                
            }
        }
        
        AK_ForEach(SimEntity, &SimEntityStorage) { Result.AddPassedVolumes(this, RigidBody, SimEntity, FilterFunc, UserData); }
    }
    
    CollisionMap.Reset();
    
    return Result;
}

void simulation::Integrate(ak_f32 dt)
{
    AK_ForEach(RigidBody, &RigidBodyStorage)
    {                
        if(RigidBody->InvMass != 0.0f)
        {
            RigidBody->WorldInvInertiaTensor = RigidBody->GetWorldInvInertiaTensor();
            RigidBody->WorldCenterOfMass = AK_Transform(RigidBody->LocalCenterOfMass, RigidBody->Transform);            
            RigidBody->Acceleration = (RigidBody->Force * RigidBody->InvMass);
            RigidBody->AngularAcceleration = (RigidBody->Torque*RigidBody->WorldInvInertiaTensor);
        }
        
        RigidBody->IntegrateVelocity(dt);
        RigidBody->IntegrateAngVelocity(dt);
        
        RigidBody->ClearForce();
        RigidBody->ClearTorque();                
    }
}

void simulation::AddContactConstraints(rigid_body* RigidBodyA, rigid_body* RigidBodyB, contact_list Contacts)
{
    AK_Assert(RigidBodyA || RigidBodyB, "Must have a valid rigid body to add contact constraints");
    if(!RigidBodyA)
    {
        RigidBodyA = RigidBodyB;
        RigidBodyB = NULL;
        Contacts.FlipNormals();        
    }
        
    manifold* Manifold = NULL;
    
    AK_ForEach(ManifoldEntry, &ManifoldStorage)
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
    
    for(ak_u32 ContactIndex = 0; ContactIndex < Contacts.Count; ContactIndex++)
    {
        contact* Contact = Contacts.Ptr + ContactIndex;
        
        ak_u64 ContactID = ContactStorage.Allocate();        
        contact_constraint* Constraint = ContactStorage.Get(ContactID);        
        
        Constraint->WorldPosition = Contact->Position;
        Constraint->Normal = Contact->Normal;
        Constraint->Penetration = Contact->Penetration;
        
        ak_u32 FreeContactIndex = Manifold->GetFreeContactIndex();
        AK_Assert(FreeContactIndex != (ak_u32)-1, "No free contacts");        
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
    AK_Assert(Constraints.Count < MAX_CONSTRAINT_COUNT, "Index out of bounds");
    constraint* Constraint = Constraints.Ptr + Constraints.Count++;
    Constraint->RigidBodyA = RigidBodyA;
    Constraint->RigidBodyB = RigidBodyB;
    Constraint->Callback   = Callback;
}

contact_list simulation::ComputeContacts(broad_phase_pair* Pair)
{
    collision_volume* VolumeA = CollisionVolumeStorage.Get(Pair->AVolumeID);
    collision_volume* VolumeB = CollisionVolumeStorage.Get(Pair->BVolumeID);
    
    contact_function* ContactFunction = ContactFunctions[VolumeA->Type][VolumeB->Type];
    contact_list ContactList = ContactFunction(Pair->SimEntityA, Pair->SimEntityB, VolumeA, VolumeB);            
    if(ContactList.Count > 0)    
        AddCollisionEvent(Pair->SimEntityA, Pair->SimEntityB, ContactList.GetDeepestContact()->Normal);            
    
    return ContactList;
}

contact* simulation::ComputeDeepestContact(broad_phase_pair* Pair)
{
    collision_volume* VolumeA = CollisionVolumeStorage.Get(Pair->AVolumeID);
    collision_volume* VolumeB = CollisionVolumeStorage.Get(Pair->BVolumeID);
    
    deepest_contact_function* DeepestContactFunction = DeepestContactFunctions[VolumeA->Type][VolumeB->Type];
    
    contact Result;
    if(DeepestContactFunction(&Result, Pair->SimEntityA, Pair->SimEntityB, VolumeA, VolumeB))
    {
        AddCollisionEvent(Pair->SimEntityA, Pair->SimEntityB, Result.Normal);
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        contact* pResult = GlobalArena->Push<contact>();
        *pResult = Result;
        return pResult;
    }    
    return NULL;
}

ak_bool simulation::ComputeIntersection(broad_phase_pair* Pair)
{
    collision_volume* VolumeA = CollisionVolumeStorage.Get(Pair->AVolumeID);
    collision_volume* VolumeB = CollisionVolumeStorage.Get(Pair->BVolumeID);    
    intersection_function* IntersectionFunction = IntersectionFunctions[VolumeA->Type][VolumeB->Type];
    ak_bool Result = IntersectionFunction(Pair->SimEntityA, Pair->SimEntityB, VolumeA, VolumeB);
    if(Result)
        AddCollisionEvent(Pair->SimEntityA, Pair->SimEntityB, AK_V3<ak_f32>());
    return Result;
}

inline ak_f32 
Baumgarte(ak_f32 D, ak_f32 dt)
{
    ak_f32 Result = BAUMGARTE_CONSTANT * (1.0f/dt) * AK_Max(0.0f, D-PENETRATION_SLOP);
    return Result;
}

inline ak_f32 
MixRestitution(rigid_body* A, rigid_body* B)
{
    ak_f32 Result = A->Restitution;
    if(B) Result = AK_Max(Result, B->Restitution);
    return Result;
}

void simulation::SolveConstraints(ak_u32 MaxIterations, ak_f32 dt)
{
    AK_ForEach(Manifold, &ManifoldStorage)
    {
        rigid_body* RigidBodyA = Manifold->RigidBodyA;        
        rigid_body* RigidBodyB = Manifold->RigidBodyB;
        
        ak_v3f vA = RigidBodyA->Velocity;
        ak_v3f wA = RigidBodyA->AngularVelocity;
        ak_v3f vB = RigidBodyB ? RigidBodyB->Velocity : AK_V3<ak_f32>();
        ak_v3f wB = RigidBodyB ? RigidBodyB->AngularVelocity : AK_V3<ak_f32>();
        
        ak_f32 Restitution = MixRestitution(RigidBodyA, RigidBodyB);
        
        for(ak_u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)        
        {   
            ak_u64 ContactID = Manifold->Contacts[ContactIndex];
            if(ContactID == 0)
                continue;
            
            contact_constraint* Constraint = ContactStorage.Get(ContactID);
            if(RigidBodyB)
            {                
                Constraint->LocalPositionA = Constraint->WorldPosition - RigidBodyA->WorldCenterOfMass;
                Constraint->LocalPositionB = Constraint->WorldPosition - RigidBodyB->WorldCenterOfMass;
                
                ak_v3f ARotAxis = AK_Cross(Constraint->LocalPositionA, Constraint->Normal);
                ak_v3f BRotAxis = AK_Cross(Constraint->LocalPositionB, Constraint->Normal);
                
                Constraint->NormalMass = RigidBodyA->InvMass + RigidBodyB->InvMass;
                Constraint->NormalMass += (AK_Dot(ARotAxis, ARotAxis*RigidBodyA->WorldInvInertiaTensor) +
                                           AK_Dot(BRotAxis, BRotAxis*RigidBodyB->WorldInvInertiaTensor));
                Constraint->NormalMass = AK_SafeInverse(Constraint->NormalMass);
                
                Constraint->Bias = Baumgarte(Constraint->Penetration, dt);
                
                ak_f32 RelVelocityProjected = AK_Dot(vB + AK_Cross(wB, Constraint->LocalPositionB) - vA - AK_Cross(wA, Constraint->LocalPositionA), Constraint->Normal);                                               
                if(RelVelocityProjected < -1)
                    Constraint->Bias += -(Restitution)*RelVelocityProjected;
                
            }
            else
            {
                Constraint->LocalPositionA = Constraint->WorldPosition - RigidBodyA->WorldCenterOfMass;                
                ak_v3f ARotAxis = AK_Cross(Constraint->LocalPositionA, Constraint->Normal);
                
                Constraint->NormalMass = RigidBodyA->InvMass;
                Constraint->NormalMass += (AK_Dot(ARotAxis, ARotAxis*RigidBodyA->WorldInvInertiaTensor));
                Constraint->NormalMass = AK_SafeInverse(Constraint->NormalMass);
                
                Constraint->Bias = Baumgarte(Constraint->Penetration, dt);
                
                ak_f32 RelVelocityProjected = AK_Dot(-vA - AK_Cross(wA, Constraint->LocalPositionA), Constraint->Normal);
                if(RelVelocityProjected < -1)
                    Constraint->Bias += -(Restitution)*RelVelocityProjected;
            }
        }        
    }
    
    for(ak_u32 Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        AK_ForEach(Manifold, &ManifoldStorage)
        {
            rigid_body* RigidBodyA = Manifold->RigidBodyA;        
            rigid_body* RigidBodyB = Manifold->RigidBodyB;
            
            ak_v3f vA = RigidBodyA->Velocity;
            ak_v3f wA = RigidBodyA->AngularVelocity;
            ak_v3f vB = RigidBodyB ? RigidBodyB->Velocity : AK_V3<ak_f32>();
            ak_v3f wB = RigidBodyB ? RigidBodyB->AngularVelocity : AK_V3<ak_f32>();
            
            for(ak_u32 ContactIndex = 0; ContactIndex < 4; ContactIndex++)        
            {   
                ak_u64 ContactID = Manifold->Contacts[ContactIndex];
                if(ContactID == 0)
                    continue;
                
                contact_constraint* Constraint = ContactStorage.Get(ContactID);                                
                if(RigidBodyB)
                {
                    ak_v3f RelVelocity = vB + AK_Cross(wB, Constraint->LocalPositionB) - vA - AK_Cross(wA, Constraint->LocalPositionA);
                    ak_f32 RelVelocityProj = AK_Dot(RelVelocity, Constraint->Normal);
                    
                    ak_f32 Lambda = Constraint->NormalMass * (-RelVelocityProj + Constraint->Bias);
                    
                    ak_f32 TempLambda = Constraint->NormalLambda;
                    Constraint->NormalLambda = AK_Max(TempLambda+Lambda, 0.0f);
                    Lambda = Constraint->NormalLambda - TempLambda;
                    
                    ak_v3f Impulse = Constraint->Normal*Lambda;
                    
                    vA -= Impulse*RigidBodyA->InvMass;
                    wA -= AK_Cross(Constraint->LocalPositionA, Impulse)*RigidBodyA->WorldInvInertiaTensor;
                    
                    vB += Impulse*RigidBodyB->InvMass;
                    wB += AK_Cross(Constraint->LocalPositionB, Impulse)*RigidBodyB->WorldInvInertiaTensor;                    
                }
                else
                {
                    ak_v3f RelVelocity = -vA - AK_Cross(wA, Constraint->LocalPositionA);
                    ak_f32 RelVelocityProj = AK_Dot(RelVelocity, Constraint->Normal);
                    
                    ak_f32 Lambda = Constraint->NormalMass * (-RelVelocityProj + Constraint->Bias);
                    
                    ak_f32 TempLambda = Constraint->NormalLambda;
                    Constraint->NormalLambda = AK_Max(TempLambda+Lambda, 0.0f);
                    Lambda = Constraint->NormalLambda - TempLambda;
                    
                    ak_v3f Impulse = Constraint->Normal*Lambda;
                    
                    vA -= Impulse*RigidBodyA->InvMass;
                    wA -= AK_Cross(Constraint->LocalPositionA, Impulse)*RigidBodyA->WorldInvInertiaTensor;
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
    
    for(ak_u32 Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        for(ak_u32 ConstraintIndex = 0; ConstraintIndex < Constraints.Count; ConstraintIndex++)
        {
            constraint* Constraint = Constraints.Ptr + ConstraintIndex;            
            Constraint->Callback(Constraint->RigidBodyA, Constraint->RigidBodyB);
        }
    }
    
    AK_ForEach(RigidBody, &RigidBodyStorage)
    {
        ak_quatf Delta = AK_RotQuat(RigidBody->AngularVelocity*dt, 0.0f)*RigidBody->Transform.Orientation;
        RigidBody->Transform.Orientation = AK_Normalize(RigidBody->Transform.Orientation + (Delta*0.5f));
        RigidBody->MoveDelta = RigidBody->Velocity*dt;
    }
    
    ContactStorage.FreeAll();
    ManifoldStorage.FreeAll();    
    Constraints.Count = 0;    
}

#define UPDATE_HIT(SimEntityB) \
if((t != INFINITY) && (t < TOIResult.t)) \
{ \
    TOIResult.HitEntity = SimEntityB; \
    TOIResult.t = t; \
    TOIResult.VolumeA = VolumeA; \
    TOIResult.VolumeB = VolumeB; \
}

continuous_contact simulation::ComputeTOI(rigid_body* RigidBody, broad_phase_pair_list Pairs)
{
    
    toi_result TOIResult = InvalidTOIResult();
    for(ak_u32 PairIndex = 0; PairIndex < Pairs.Count; PairIndex++)
    {
        broad_phase_pair* Pair = Pairs.Ptr + PairIndex;
        collision_volume* VolumeA = CollisionVolumeStorage.Get(Pair->AVolumeID);
        collision_volume* VolumeB = CollisionVolumeStorage.Get(Pair->BVolumeID);
        
        ccd_function* CCDFunction = CCDFunctions[VolumeA->Type][VolumeB->Type];        
        ak_f32 t = CCDFunction(Pair->SimEntityA, Pair->SimEntityB, VolumeA, VolumeB);
        UPDATE_HIT(Pair->SimEntityB);
    }
    
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

continuous_contact simulation::ComputeTOI(rigid_body* RigidBodyA, sim_entity* SimEntityB)
{
    toi_result TOIResult = InvalidTOIResult();
    collision_volume* VolumeA = CollisionVolumeStorage.Get(RigidBodyA->CollisionVolumeID);
    while(VolumeA)
    {
        collision_volume* VolumeB = CollisionVolumeStorage.Get(SimEntityB->CollisionVolumeID);
        while(VolumeB)
        {
            ccd_function* CCDFunction = CCDFunctions[VolumeA->Type][VolumeB->Type];
            ak_f32 t = CCDFunction(RigidBodyA, SimEntityB, VolumeA, VolumeB);
            UPDATE_HIT(SimEntityB);            
            VolumeB = CollisionVolumeStorage.Get(VolumeB->NextID);
        }        
        VolumeA = CollisionVolumeStorage.Get(VolumeA->NextID);
    }   
                
    continuous_contact Result = {};
    Result.t = TOIResult.t;    
    if(Result.t != INFINITY)    
    {
        Result.HitEntity = TOIResult.HitEntity;        
        ccd_contact_function* CCDContactFunction = CCDContactFunctions[TOIResult.VolumeA->Type][TOIResult.VolumeB->Type];
        Result.Contact = CCDContactFunction(RigidBodyA, TOIResult.HitEntity, TOIResult.VolumeA, TOIResult.VolumeB, Result.t);                        
        AddCollisionEvent(RigidBodyA, TOIResult.HitEntity, Result.Contact.Normal);
    }
    
    return Result;
}

#undef UPDATE_HIT

CONSTRAINT_CALLBACK(LockConstraint)
{
    AK_Assert(!RigidBodyB, "Lock constraint does not use the second rigid body");
    
    ak_v4f PLock[3] = 
    {
        AK_V4(0.0f, 1.0f, 0.0f, 0.0f),
        AK_V4(0.0f, 0.0f, 1.0f, 0.0f),
        AK_V4(0.0f, 0.0f, 0.0f, 1.0f)
    };    
    
    ak_quatf Orient = RigidBodyA->Transform.Orientation;                
    
    ak_v4f Eqt[3];        
    Eqt[0] = AK_V4(-Orient.v.x,  Orient.s,   -Orient.v.z,  Orient.v.y);
    Eqt[1] = AK_V4(-Orient.v.y,  Orient.v.z,  Orient.s,   -Orient.v.x);
    Eqt[2] = AK_V4(-Orient.v.z, -Orient.v.y,  Orient.v.x,  Orient.s);
    
    ak_v3f Jacobian[3] = 
    {
        -0.5f*AK_V3(AK_Dot(PLock[0], Eqt[0]), AK_Dot(PLock[1], Eqt[0]), AK_Dot(PLock[2], Eqt[0])),
        -0.5f*AK_V3(AK_Dot(PLock[0], Eqt[1]), AK_Dot(PLock[1], Eqt[1]), AK_Dot(PLock[2], Eqt[1])),
        -0.5f*AK_V3(AK_Dot(PLock[0], Eqt[2]), AK_Dot(PLock[1], Eqt[2]), AK_Dot(PLock[2], Eqt[2]))
    };
    
    ak_v3f DeltaVel = AK_V3(AK_Dot(Jacobian[0], RigidBodyA->AngularVelocity), 
                            AK_Dot(Jacobian[1], RigidBodyA->AngularVelocity),
                            AK_Dot(Jacobian[2], RigidBodyA->AngularVelocity));
    
    RigidBodyA->AngularVelocity = DeltaVel;                
    
}

simulation CreateSimulation()
{
    simulation Simulation = {};                    
    Simulation.CollisionMap = AK_CreateHashMap<ak_pair<ak_u32>, ak_bool>(8191);    
    return Simulation;
}

void DeleteSimulation(simulation* Simulation)
{
    AK_DeleteHashMap(&Simulation->CollisionMap);
    AK_DeletePool(&Simulation->ContactStorage);
    AK_DeletePool(&Simulation->ManifoldStorage);
    AK_DeletePool(&Simulation->RigidBodyStorage);
    AK_DeletePool(&Simulation->SimEntityStorage);
    AK_DeletePool(&Simulation->CollisionVolumeStorage);
}