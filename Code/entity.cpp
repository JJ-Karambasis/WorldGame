inline entity* 
GetEntity(game* Game, entity_id ID)
{
    entity* Result = Game->EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
}

inline rigid_body*
GetRigidBody(game* Game, entity_id ID)
{
    entity* Entity = GetEntity(Game, ID);
    return GetSimulation(Game, ID)->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
}

template <typename type>
void AddCollisionVolume(game* Game, entity_id EntityID, type* Collider)
{
    GetSimEntity(Game, EntityID)->AddCollisionVolume(GetSimulation(Game, EntityID), Collider);    
}

inline ak_sqtf* GetEntityTransformOld(game* Game, entity_id ID)
{    
    ak_u32 Index = AK_PoolIndex(ID.ID);
    ak_sqtf* Result = &Game->PrevTransforms[ID.WorldIndex][Index];
    return Result;
}

inline ak_sqtf* GetEntityTransform(game* Game, entity_id ID)
{
    ak_u32 Index = AK_PoolIndex(ID.ID);
    ak_sqtf* Result = &Game->CurrentTransforms[ID.WorldIndex][Index];
    return Result;
}

inline ak_v3f GetEntityPositionOld(game* Game, entity_id ID)
{
    ak_v3f Result = GetEntityTransformOld(Game, ID)->Translation;
    return Result;
}

inline ak_v3f GetEntityPosition(game* Game, entity_id ID)
{
    ak_v3f Result = GetEntityTransform(Game, ID)->Translation;
    return Result;
}

void FreeEntity(game* Game, entity_id ID)
{    
    simulation* Simulation = GetSimulation(Game, ID.WorldIndex);    
    Simulation->DeleteSimEntity(GetEntity(Game, ID)->SimEntityID);        
    
    entity* Entity = Game->EntityStorage[ID.WorldIndex].Get(ID.ID);
    if(Entity->LinkID.IsValid())
    {
        entity* LinkEntity = Game->EntityStorage[Entity->LinkID.WorldIndex].Get(Entity->LinkID.ID);
        AK_Assert(AreEqualIDs(LinkEntity->LinkID, ID), "Linked entities do not have valid matching ids");
        LinkEntity->LinkID = InvalidEntityID();
    }
    
    Game->EntityStorage[ID.WorldIndex].Free(ID.ID);    
}

entity_id
CreateEntity(game* Game, entity_type Type, sim_entity_type SimType, ak_u32 WorldIndex, 
             ak_v3f Position, ak_v3f Scale, ak_quatf Orientation,
             mesh_asset_id MeshID, material Material, ak_bool NoMeshColliders = false)
{
    AK_Assert(WorldIndex < 2, "Index out of bounds");
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
    
    ak_sqtf Transform = AK_SQT(Position, Orientation, Scale);
    
    ak_u32 Index = AK_PoolIndex(Result.ID);
    if(Game->PrevTransforms[WorldIndex].Size < (Index+1))
    {
        Game->PrevTransforms[WorldIndex].Resize(Index+1);
        Game->CurrentTransforms[WorldIndex].Resize(Index+1);
    }
    
    Game->PrevTransforms[WorldIndex][Index]    = Transform;
    Game->CurrentTransforms[WorldIndex][Index] = Transform;        
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    SimEntity->UserData = Entity;
    
    SimEntity->Transform = Transform;        
    if((MeshID != INVALID_MESH_ID) && !NoMeshColliders)
    {                              
        mesh_info* MeshInfo = GetMeshInfo(Game->Assets, MeshID);
        for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        {
            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
            Simulation->AddCollisionVolume(SimEntity, ConvexHull);           
        }        
    }
    
    return Result;
}

entity_id
CreateStaticEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                   mesh_asset_id Mesh, material Material, ak_bool NoMeshColliders = false)
{    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_STATIC, SIM_ENTITY_TYPE_SIM_ENTITY, WorldIndex, Position, Scale, Orientation, Mesh, Material, NoMeshColliders);    
    return Result;
}

dual_entity_id
CreateDualStaticEntity(game* Game, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation,
                       mesh_asset_id Mesh, material Material, ak_bool NoMeshColliders = false)
{
    dual_entity_id Result;
    Result.EntityA = CreateStaticEntity(Game, 0, Position, Scale, Orientation, Mesh, Material, NoMeshColliders);
    Result.EntityB = CreateStaticEntity(Game, 1, Position, Scale, Orientation, Mesh, Material, NoMeshColliders);
    return Result;
}

entity_id 
CreatePlayerEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_quatf Orientation, ak_f32 Mass, material Material, capsule* Capsule)
{
    AK_Assert(Mass != 0, "Cannot have zero mass for the player");    
    
    Position.z += 0.01f;
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PLAYER, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, AK_V3(1.0f, 1.0f, 1.0f), Orientation, MESH_ASSET_ID_PLAYER, Material, true);            
    simulation* Simulation = GetSimulation(Game, WorldIndex);
    entity* Entity = GetEntity(Game, Result);                
    
    player* Player = Game->GameStorage->Push<player>();
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
CreateSphereRigidBody(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_f32 Radius, ak_f32 Mass, ak_f32 Restitution, material Material)
{
    AK_Assert(Mass != 0, "Cannot have zero mass for a rigid body");    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_RIGID_BODY, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, Position, AK_V3(1.0f, 1.0f, 1.0f)*Radius, AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_SPHERE, Material, true);            
    entity* Entity = GetEntity(Game, Result);    
    simulation* Simulation = GetSimulation(Game, WorldIndex);        
    
    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    
    sphere Sphere = CreateSphere(AK_V3(0.0f, 0.0f, 0.0f), 1.0f);    
    Simulation->AddCollisionVolume(RigidBody, &Sphere);
    
    ak_f32 SphereRadius = Sphere.Radius*GetEntityTransform(Game, Result)->Scale.LargestComp();
    
    RigidBody->Restitution = Restitution;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);
    RigidBody->LocalCenterOfMass = AK_V3<ak_f32>();
    
    return Result;
}

entity_id
CreatePushableBox(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_f32 Dimensions, ak_f32 Mass, material Material)
{
    AK_Assert(Mass != 0, "Cannot have zero mass for a pushable box body");    
    entity_id Result = CreateEntity(Game, ENTITY_TYPE_PUSHABLE, SIM_ENTITY_TYPE_RIGID_BODY, WorldIndex, 
                                    Position, AK_V3(Dimensions, Dimensions, Dimensions), AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_BOX, Material, false);        
    entity* Entity = GetEntity(Game, Result);
    Entity->UserData = Game->GameStorage->Push<pushing_object>();
    
    simulation* Simulation = GetSimulation(Game, WorldIndex);                
    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetBoxInvInertiaTensor(AK_V3(Dimensions, Dimensions, Dimensions), Mass);
    RigidBody->LocalCenterOfMass = AK_V3(0.0f, 0.0f, Dimensions*0.5f);
    
    return Result;
}

dual_entity_id
CreateDualPushableBox(game* Game, ak_v3f Position, ak_f32 Dimensions, ak_f32 Mass, material Material)
{
    AK_Assert(Mass != 0, "Cannot have zero mass for a pushable box body");    

    dual_entity_id Result;
    Result.EntityA = CreatePushableBox(Game, 0, Position, Dimensions, Mass, Material);
    Result.EntityB = CreatePushableBox(Game, 1, Position, Dimensions, Mass, Material);
    
    entity* EntityA = GetEntity(Game, Result.EntityA);
    entity* EntityB = GetEntity(Game, Result.EntityB);    
    
    EntityA->LinkID = EntityB->ID;
    EntityB->LinkID = EntityA->ID;
    
    return Result;
}

inline ak_bool
IsEntityType(entity* Entity, entity_type Type)
{
    ak_bool Result = Entity->Type == Type;
    return Result;
}

inline entity_type
GetEntityType(game* Game, entity_id ID)
{
    return Game->EntityStorage[ID.WorldIndex].Get(ID.ID)->Type;
}

#include "player.cpp"