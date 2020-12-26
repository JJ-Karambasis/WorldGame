#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 0.8f

void ResizeWorld(world* World, ak_u32 WorldIndex, ak_u32 Size)
{
    if(Size > World->PhysicsObjects[WorldIndex].Size) 
        World->PhysicsObjects[WorldIndex].Resize(Size*2);
    
    if(Size > World->GraphicsObjects[WorldIndex].Size)
        World->GraphicsObjects[WorldIndex].Resize(Size*2);
    
    if(Size > World->OldTransforms[WorldIndex].Size)
        World->OldTransforms[WorldIndex].Resize(Size*2);
    
    if(Size > World->ButtonStates[WorldIndex].Size)
        World->ButtonStates[WorldIndex].Resize(Size*2);
    
    if(Size > World->Movables[WorldIndex].Size)
        World->Movables[WorldIndex].Resize(Size*2);
}

void DeleteEntity(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_bool ProcessLink=true)
{
    world* World = Game->World;
    ak_u32 Index = AK_PoolIndex(ID);
    
    physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(Index);
    collision_volume* Volume = World->CollisionVolumeStorage.Get(PhysicsObject->CollisionVolumeID);
    while(Volume)
    {
        ak_u64 NextID = Volume->NextID;
        World->CollisionVolumeStorage.Free(Volume->ID);
        Volume = World->CollisionVolumeStorage.Get(NextID);
    }
    
    entity* Entity = World->EntityStorage[WorldIndex].Get(ID);
    
    if(ProcessLink)
    {
        if(Entity->LinkID)
            DeleteEntity(Game, !WorldIndex, Entity->LinkID, false);
    }
    
    World->EntityStorage[WorldIndex].Free(ID);
}

entity* CreateEntity(game* Game, ak_u32 WorldIndex, entity_type Type, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                     mesh_asset_id MeshID, material Material)
{
    world* World = Game->World;
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_u64 ID = EntityStorage->Allocate();
    ak_u32 Index = AK_PoolIndex(ID);
    
    ResizeWorld(World, WorldIndex, Index+1);
    
    entity* Entity = EntityStorage->GetByIndex(Index);
    Entity->Type = Type;
    Entity->ID =   ID;
    
    physics_object* PhysicsObject = &World->PhysicsObjects[WorldIndex][Index];
    *PhysicsObject = {};
    PhysicsObject->Position = Position;
    PhysicsObject->Orientation = Orientation;
    PhysicsObject->Scale = Scale;
    
    graphics_object* GraphicsObject = &World->GraphicsObjects[WorldIndex][Index];
    *GraphicsObject = {};
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

entity* CreateButtonEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Dimensions, 
                           material Material, ak_bool IsToggled)
{
    entity* Entity = CreateEntity(Game, WorldIndex, ENTITY_TYPE_BUTTON, Position, Dimensions, 
                                  AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_BUTTON, Material);
    button_state* ButtonState = Game->World->ButtonStates[WorldIndex].Get(AK_PoolIndex(Entity->ID));
    *ButtonState = {};
    ButtonState->IsToggled = IsToggled;
    return Entity;
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