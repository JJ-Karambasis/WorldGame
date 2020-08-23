inline entity* 
GetEntity(game* Game, entity_id ID)
{
    entity* Result = Game->EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
}

sim_state* GetSimState(game* Game, entity_id ID)
{
    u32 PoolIndex = Game->EntityStorage[ID.WorldIndex].GetIndex(ID.ID);
    sim_state* Result = &Game->SimStates[ID.WorldIndex][PoolIndex];
    return Result;
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
    sim_state* SimState = GetSimState(Game, ID);
    collision_volume* Volume = SimState->CollisionVolumes;
    while(Volume)
    {
        collision_volume* VolumeToFree = Volume;
        Volume = Volume->Next;        
        Game->CollisionVolumeStorage[ID.WorldIndex].Free(VolumeToFree);        
    }                    
    
    Game->EntityStorage[ID.WorldIndex].Free(ID.ID);    
}

entity_id
CreateEntity(game* Game, entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler,
             mesh_asset_id MeshID, material Material, b32 NoMeshColliders = false)
{
    entity_storage* EntityStorage = Game->EntityStorage + WorldIndex;
    entity_id Result = MakeEntityID(EntityStorage->Allocate(), WorldIndex);
    
    entity* Entity = EntityStorage->Get(Result.ID);
    
    Entity->Type = Type;
    Entity->State = ENTITY_STATE_NONE;
    Entity->ID = Result;
    Entity->LinkID = InvalidEntityID();    
    Entity->MeshID = MeshID; 
    Entity->Material = Material;
    
    sqt Transform = CreateSQT(Position, Scale, Euler);
    
    u32 PoolIndex = EntityStorage->GetIndex(Result.ID);
    Game->PrevTransforms[WorldIndex][PoolIndex]    = Transform;
    Game->CurrentTransforms[WorldIndex][PoolIndex] = Transform;
    
    if((MeshID != INVALID_MESH_ID) && !NoMeshColliders)
    {        
        sim_state* SimState  = GetSimState(Game, Result);        
        
        mesh_info* MeshInfo = GetMeshInfo(Game->Assets, MeshID);
        for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        {
            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;                        
            AddCollisionVolume(&Game->CollisionVolumeStorage[WorldIndex], SimState, ConvexHull);            
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
CreatePlayerEntity(game* Game, u32 WorldIndex, v3f Position, v3f Euler, material Material, capsule* Capsule)
{
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f), Euler, MESH_ASSET_ID_PLAYER, Material, true);
    AddCollisionVolume(&Game->CollisionVolumeStorage[WorldIndex], GetSimState(Game, Result), Capsule);
    return Result;
}

entity_id
CreateSphereRigidBody(game* Game, u32 WorldIndex, v3f Position, f32 Radius, f32 Mass, material Material)
{
    ASSERT(Mass != 0);
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f)*Radius, V3(), MESH_ASSET_ID_SPHERE, Material, true);
    
    sim_state* SimState = GetSimState(Game, Result);
    
    sphere Sphere = CreateSphere(V3(0.0f, 0.0f, 0.0f), 1.0f);    
    AddCollisionVolume(&Game->CollisionVolumeStorage[Result.WorldIndex], SimState, &Sphere);        
    
    f32 SphereRadius = Sphere.Radius*GetEntityTransform(Game, Result)->Scale.LargestComponent();
    
    SimState->InvMass = 1.0f/Mass;
    SimState->InvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);    
    
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