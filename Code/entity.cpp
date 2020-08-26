inline entity* 
GetEntity(game* Game, entity_id ID)
{
    entity* Result = Game->EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
}

inline sim_entity*
GetSimEntity(game* Game, entity_id ID)
{
    simulation* Simulation = GetSimulation(Game, ID);
    entity* Entity = GetEntity(Game, ID);            
    return Simulation->GetSimEntity(Entity->SimEntityID);
}

template <typename type>
void AddCollisionVolume(game* Game, entity_id EntityID, type* Collider)
{
    GetSimEntity(Game, EntityID)->AddCollisionVolume(GetSimulation(Game, EntityID), Collider);    
}

inline sqt* GetEntityTransformOld(game* Game, entity_id ID)
{
    u32 PoolIndex = Game->EntityStorage[ID.WorldIndex].GetIndex(ID.ID);
    sqt* Result = &Game->PrevTransforms[ID.WorldIndex][PoolIndex];
    return Result;
}

inline sqt* GetEntityTransform(game* Game, entity_id ID)
{
    u32 PoolIndex = Game->EntityStorage[ID.WorldIndex].GetIndex(ID.ID);
    sqt* Result = &Game->CurrentTransforms[ID.WorldIndex][PoolIndex];
    return Result;
}

inline v3f GetEntityPosition(game* Game, entity_id ID)
{
    v3f Result = GetEntityTransform(Game, ID)->Translation;
    return Result;
}

void FreeEntity(game* Game, entity_id ID)
{    
    simulation* Simulation = GetSimulation(Game, ID.WorldIndex);            
    Simulation->FreeSimEntity(GetEntity(Game, ID)->SimEntityID);        
    Game->EntityStorage[ID.WorldIndex].Free(ID.ID);    
}

entity_id
CreateEntity(game* Game, entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler,
             mesh_asset_id MeshID, material Material, b32 NoMeshColliders = false)
{
    entity_storage* EntityStorage = Game->EntityStorage + WorldIndex;
    simulation* Simulation = GetSimulation(Game, WorldIndex);        
    
    entity_id Result = MakeEntityID(EntityStorage->Allocate(), WorldIndex);
    
    entity* Entity = EntityStorage->Get(Result.ID);
    
    Entity->Type = Type;
    Entity->State = ENTITY_STATE_NONE;
    Entity->ID = Result;
    Entity->SimEntityID = Simulation->AllocateSimEntity();
    Entity->LinkID = InvalidEntityID();    
    Entity->MeshID = MeshID; 
    Entity->Material = Material;
    
    sqt Transform = CreateSQT(Position, Scale, Euler);
    
    u32 PoolIndex = EntityStorage->GetIndex(Result.ID);
    Game->PrevTransforms[WorldIndex][PoolIndex]    = Transform;
    Game->CurrentTransforms[WorldIndex][PoolIndex] = Transform;    
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    SimEntity->Transform = Transform;
    SimEntity->UserData = Entity;
    
    if((MeshID != INVALID_MESH_ID) && !NoMeshColliders)
    {                              
        mesh_info* MeshInfo = GetMeshInfo(Game->Assets, MeshID);
        for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        {
            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
            SimEntity->AddCollisionVolume(Simulation, ConvexHull);                        
        }        
    }
    
    return Result;
}

entity_id
CreateStaticEntity(game* Game, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, 
                   mesh_asset_id Mesh, material Material, b32 NoMeshColliders = false)
{
    return CreateEntity(Game, ENTITY_TYPE_STATIC, WorldIndex, Position, Scale, Euler, Mesh, Material, NoMeshColliders);
}

entity_id 
CreatePlayerEntity(game* Game, u32 WorldIndex, v3f Position, v3f Euler, f32 Mass, material Material, capsule* Capsule)
{
    Position.z += 0.01f;
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f), Euler, MESH_ASSET_ID_PLAYER, Material, true);    
    
    simulation* Simulation = GetSimulation(Game, WorldIndex);
    sim_entity* SimEntity = Simulation->GetSimEntity(GetEntity(Game, Result)->SimEntityID);
    
    SimEntity->AddCollisionVolume(Simulation, Capsule);
    
    rigid_body* RigidBody = Simulation->RigidBodyStorage.Get(Simulation->RigidBodyStorage.Allocate());
    RigidBody->SimEntity = SimEntity;
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetCylinderInvInertiaTensor(Capsule->Radius, GetCapsuleHeight(Capsule), Mass);
    RigidBody->LocalCenterOfMass = GetCapsuleCenter(Capsule);
    
    SimEntity->RigidBody = RigidBody;
    
    return Result;
}

entity_id
CreateSphereRigidBody(game* Game, u32 WorldIndex, v3f Position, f32 Radius, f32 Mass, f32 Restitution, material Material)
{
    ASSERT(Mass != 0);    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f)*Radius, V3(), MESH_ASSET_ID_SPHERE, Material, true);            
    
    simulation* Simulation = GetSimulation(Game, WorldIndex);
    
    u64 SimEntityID = GetEntity(Game, Result)->SimEntityID;
    sim_entity* SimEntity = Simulation->GetSimEntity(SimEntityID);
    
    sphere Sphere = CreateSphere(V3(0.0f, 0.0f, 0.0f), 1.0f);    
    SimEntity->AddCollisionVolume(Simulation, &Sphere);
    
    f32 SphereRadius = Sphere.Radius*GetEntityTransform(Game, Result)->Scale.LargestComponent();
    
    SimEntity->RigidBody = Simulation->RigidBodyStorage.Get(Simulation->RigidBodyStorage.Allocate());
    rigid_body* RigidBody = SimEntity->RigidBody;
    RigidBody->SimEntity = SimEntity;
    RigidBody->Restitution = Restitution;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);
    RigidBody->LocalCenterOfMass = V3();
    
    return Result;
}

inline b32 
IsEntityType(entity* Entity, entity_type Type)
{
    b32 Result = Entity->Type == Type;
    return Result;
}

inline b32
IsEntityState(entity* Entity, entity_state State)
{
    b32 Result = Entity->State == State;
    return Result;
}

inline void
SetEntityState(entity* Entity, entity_state State)
{
    ASSERT(Entity->State != State);    
    Entity->State = State;
}