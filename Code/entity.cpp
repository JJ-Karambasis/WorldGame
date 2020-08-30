inline entity* 
GetEntity(game* Game, entity_id ID)
{
    entity* Result = Game->EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
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

inline v3f GetEntityPositionOld(game* Game, entity_id ID)
{
    v3f Result = GetEntityTransformOld(Game, ID)->Translation;
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
    Simulation->DeleteSimEntity(GetEntity(Game, ID)->SimEntityID);        
    Game->EntityStorage[ID.WorldIndex].Free(ID.ID);    
}

entity_id
CreateEntity(game* Game, entity_type Type, sim_entity_type SimType, u32 WorldIndex, 
             v3f Position, v3f Scale, v3f Euler,
             mesh_asset_id MeshID, material Material, b32 NoMeshColliders = false)
{
    entity_storage* EntityStorage = Game->EntityStorage + WorldIndex;
    simulation* Simulation = GetSimulation(Game, WorldIndex);        
    
    entity_id Result = MakeEntityID(EntityStorage->Allocate(), WorldIndex);
    
    entity* Entity = EntityStorage->Get(Result.ID);
    
    Entity->Type = Type;    
    Entity->ID = Result;
    Entity->SimEntityID = Simulation->CreateSimEntity(SimType);
    Entity->LinkID = InvalidEntityID();    
    Entity->MeshID = MeshID; 
    Entity->Material = Material;    
    
    sqt Transform = CreateSQT(Position, Scale, Euler);
    
    u32 PoolIndex = EntityStorage->GetIndex(Result.ID);
    Game->PrevTransforms[WorldIndex][PoolIndex]    = Transform;
    Game->CurrentTransforms[WorldIndex][PoolIndex] = Transform;        
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    SimEntity->UserData = Entity;
    
    SimEntity->Transform = Transform;        
    if((MeshID != INVALID_MESH_ID) && !NoMeshColliders)
    {                              
        mesh_info* MeshInfo = GetMeshInfo(Game->Assets, MeshID);
        for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        {
            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
            Simulation->AddCollisionVolume(SimEntity, ConvexHull);           
        }        
    }
    
    return Result;
}

entity_id
CreateStaticEntity(game* Game, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, 
                   mesh_asset_id Mesh, material Material, b32 NoMeshColliders = false)
{    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_STATIC, SIM_ENTITY_TYPE_SIM_ENTITY, WorldIndex, Position, Scale, Euler, Mesh, Material, NoMeshColliders);    
    return Result;
}

entity_id 
CreatePlayerEntity(game* Game, u32 WorldIndex, v3f Position, v3f Euler, f32 Mass, material Material, capsule* Capsule)
{
    Position.z += 0.01f;
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PLAYER, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f), Euler, MESH_ASSET_ID_PLAYER, Material, true);            
    simulation* Simulation = GetSimulation(Game, WorldIndex);
    entity* Entity = GetEntity(Game, Result);                
    
    player* Player = PushStruct(Game->GameStorage, player, Clear, 0);
    Entity->UserData = Player;
    
    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    Simulation->AddCollisionVolume(RigidBody, Capsule);    
    
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetCylinderInvInertiaTensor(Capsule->Radius, Capsule->GetHeight(), Mass);
    RigidBody->LocalCenterOfMass = Capsule->GetCenter();
    
    return Result;
}

entity_id
CreateSphereRigidBody(game* Game, u32 WorldIndex, v3f Position, f32 Radius, f32 Mass, f32 Restitution, material Material)
{
    ASSERT(Mass != 0);    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_RIGID_BODY, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, V3(1.0f, 1.0f, 1.0f)*Radius, V3(), MESH_ASSET_ID_SPHERE, Material, true);            
    entity* Entity = GetEntity(Game, Result);    
    simulation* Simulation = GetSimulation(Game, WorldIndex);        
    
    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    
    sphere Sphere = CreateSphere(V3(0.0f, 0.0f, 0.0f), 1.0f);    
    Simulation->AddCollisionVolume(RigidBody, &Sphere);
    
    f32 SphereRadius = Sphere.Radius*GetEntityTransform(Game, Result)->Scale.LargestComponent();
    
    RigidBody->Restitution = Restitution;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);
    RigidBody->LocalCenterOfMass = V3();
    
    return Result;
}

entity_id
CreatePushableBox(game* Game, u32 WorldIndex, v3f Position, f32 Dimensions, f32 Mass, material Material)
{
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PUSHABLE, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, V3(Dimensions, Dimensions, Dimensions), V3(), MESH_ASSET_ID_BOX, Material, false);        
    entity* Entity = GetEntity(Game, Result);
    Entity->UserData = PushStruct(Game->GameStorage, pushing_object, Clear, 0);
    
    simulation* Simulation = GetSimulation(Game, WorldIndex);                
    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetBoxInvInertiaTensor(V3(Dimensions, Dimensions, Dimensions), Mass);
    RigidBody->LocalCenterOfMass = V3(0.0f, 0.0f, Dimensions*0.5f);
    
    return Result;
}

inline b32 
IsEntityType(entity* Entity, entity_type Type)
{
    b32 Result = Entity->Type == Type;
    return Result;
}

#include "player.cpp"