#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 0.8f

entity* CreateEntity(game* Game, ak_u32 WorldIndex, entity_type Type, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                     mesh_asset_id MeshID, material Material)
{
    world* World = Game->World;
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_u64 ID = EntityStorage->Allocate();
    ak_u32 Index = AK_PoolIndex(ID);
    
    entity* Entity = EntityStorage->GetByIndex(Index);
    Entity->Type = Type;
    Entity->ID =   ID;
    
    ak_u32 IndexPlusOne = Index+1;
    if(IndexPlusOne > World->PhysicsObjects[WorldIndex].Size)
        World->PhysicsObjects[WorldIndex].Resize(IndexPlusOne);
    
    if(IndexPlusOne > World->GraphicsObjects[WorldIndex].Size)
        World->GraphicsObjects[WorldIndex].Resize(IndexPlusOne);
    
    if(IndexPlusOne > World->OldTransforms[WorldIndex].Size)
        World->OldTransforms[WorldIndex].Resize(IndexPlusOne);
    
    physics_object* PhysicsObject = &World->PhysicsObjects[WorldIndex][Index];
    PhysicsObject->Position = Position;
    PhysicsObject->Orientation = Orientation;
    PhysicsObject->Scale = Scale;
    
    graphics_object* GraphicsObject = &World->GraphicsObjects[WorldIndex][Index];
    GraphicsObject->MeshID = MeshID;
    GraphicsObject->Material = Material;
    
    mesh_info* MeshInfo = GetMeshInfo(Game->Assets, MeshID);
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
        AddConvexHullVolume(PhysicsObject, &World->CollisionVolumeStorage, MeshInfo->ConvexHulls[ConvexHullIndex]);
    
    return Entity;
}

entity* CreatePlayerEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, material Material)
{
    capsule PlayerCapsule = CreateCapsule(AK_V3<ak_f32>(), PLAYER_HEIGHT, PLAYER_RADIUS);
    entity* Entity = CreateEntity(Game, WorldIndex, ENTITY_TYPE_PLAYER, Position, AK_V3(1.0f, 1.0f, 1.0f), AK_IdentityQuat<ak_f32>(), 
                                  MESH_ASSET_ID_PLAYER, Material);
    AddCapsuleVolume(&Game->World->PhysicsObjects[WorldIndex][AK_PoolIndex(Entity->ID)], &Game->World->CollisionVolumeStorage, PlayerCapsule);
    return Entity;
}

entity* CreateStaticEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, mesh_asset_id MeshID, material Material)
{
    return CreateEntity(Game, WorldIndex, ENTITY_TYPE_STATIC, Position, Scale, Orientation, MeshID, Material);
}

point_light* CreatePointLight(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    ak_pool<point_light>* PointLightStorage = &Game->World->PointLightStorage[WorldIndex];
    ak_u64 ID = PointLightStorage->Allocate();
    point_light* PointLight = PointLightStorage->Get(ID);
    
    PointLight->Position = Position;
    PointLight->Radius = Radius;
    PointLight->Color = Color;
    PointLight->Intensity = Intensity;
    PointLight->On = true;
    PointLight->ID = ID;
    return PointLight;
}