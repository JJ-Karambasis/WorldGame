inline entity* 
GetEntity(world* World, world_id ID)
{
    entity* Result = World->EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
}

inline entity* 
GetEntity(game* Game, world_id ID)
{
    entity* Result = Game->World.EntityStorage[ID.WorldIndex].Get(ID.ID);
    return Result;
}

inline sim_entity*
GetSimEntity(world* World, world_id ID)
{
    simulation* Simulation = &World->Simulations[ID.WorldIndex];
    return Simulation->GetSimEntity(GetEntity(World, ID)->SimEntityID);
}

inline rigid_body*
GetRigidBody(world* World, world_id ID)
{    
    return GetSimEntity(World, ID)->ToRigidBody();
}

inline sim_entity*
GetSimEntity(game* Game, world_id ID)
{
    return GetSimEntity(&Game->World, ID);
}

inline rigid_body*
GetRigidBody(game* Game, world_id ID)
{    
    return GetRigidBody(&Game->World, ID);
}

inline entity* 
GetEntity(entity_storage* EntityStorage, sim_entity* SimEntity)
{
    return EntityStorage->GetByIndex(UserDataToIndex(SimEntity->UserData));
}

template <typename type>
void AddCollisionVolume(game* Game, world_id EntityID, type* Collider)
{
    GetSimEntity(Game, EntityID)->AddCollisionVolume(GetSimulation(Game, EntityID), Collider);    
}

void DeleteWorld(world* World, graphics* Graphics)
{
    AK_DeletePool(&World->MovableStorage);
    AK_DeletePool(&World->ButtonStateStorage);
    AK_DeletePool(&World->JumpingQuadStorage[0]);
    AK_DeletePool(&World->JumpingQuadStorage[1]);
    AK_DeletePool(&World->EntityStorage[0]);
    AK_DeletePool(&World->EntityStorage[1]);
    AK_DeleteArray(&World->OldTransforms[0]);
    AK_DeleteArray(&World->OldTransforms[1]);
    AK_DeleteArray(&World->NewTransforms[0]);
    AK_DeleteArray(&World->NewTransforms[1]);    
    DeleteSimulation(&World->Simulations[0]);
    DeleteSimulation(&World->Simulations[1]);
    DeleteGraphicsState(Graphics, &World->GraphicsStates[0]);
    DeleteGraphicsState(Graphics, &World->GraphicsStates[1]);
}

ak_u32 CreateButtonState(world* World, ak_bool IsToggle)
{
    ak_u64 Result = World->ButtonStateStorage.Allocate();
    button_state* ButtonState = World->ButtonStateStorage.Get(Result);
    ButtonState->IsToggle = IsToggle;
    return AK_PoolIndex(Result);    
}

button_state* GetButtonState(world* World, entity* Entity)
{
    AK_Assert(Entity->Type == ENTITY_TYPE_BUTTON, "Cannot get a button state of an entity that is not a button");
    return World->ButtonStateStorage.GetByIndex(UserDataToIndex(Entity->UserData));
}

ak_u32 CreateMovable(world* World)
{
    ak_u64 Result = World->MovableStorage.Allocate();
    return AK_PoolIndex(Result);
}

ak_bool ValidateMovable(movable* Movable)
{
    if(Movable->ParentID.IsValid() && Movable->ChildID.IsValid())
        return !AreEqualIDs(Movable->ParentID, Movable->ChildID);
    return true;
}

movable* GetMovable(world* World, entity* Entity)
{
    AK_Assert(Entity->Type == ENTITY_TYPE_MOVABLE, "Cannot get a movable of an entity that is not a movable");
    
    movable* Movable = World->MovableStorage.GetByIndex(UserDataToIndex(Entity->UserData));
    AK_Assert(ValidateMovable(Movable), "Corrupted movable");
    return Movable;
}

inline world_id GetChildID(movable* Movable)
{
    return Movable ? Movable->ChildID : InvalidWorldID();
}

inline world_id GetParentID(movable* Movable)
{
    return Movable ? Movable->ParentID : InvalidWorldID();
}

void DeleteButtonState(world* World, ak_u32 Index)
{
    World->ButtonStateStorage.Free(World->ButtonStateStorage.IDs[Index]);
}

void FreeEntity(game* Game, world_id ID)
{       
    entity* Entity = Game->World.EntityStorage[ID.WorldIndex].Get(ID.ID);
    
    simulation* Simulation = &Game->World.Simulations[ID.WorldIndex];
    graphics_state* GraphicsState = &Game->World.GraphicsStates[ID.WorldIndex];
    
    Simulation->DeleteSimEntity(Entity->SimEntityID);            
    GraphicsState->GraphicsEntityStorage.Free(Entity->GraphicsEntityID);
    
    if(Entity->LinkID.IsValid())
    {
        entity* LinkEntity = Game->World.EntityStorage[Entity->LinkID.WorldIndex].Get(Entity->LinkID.ID);
        AK_Assert(AreEqualIDs(LinkEntity->LinkID, ID), "Linked entities do not have valid matching ids");
        LinkEntity->LinkID = InvalidWorldID();
    }
    
    if(Entity->Type == ENTITY_TYPE_BUTTON)
        DeleteButtonState(&Game->World, UserDataToIndex(Entity->UserData));        
    
    Game->World.EntityStorage[ID.WorldIndex].Free(ID.ID);    
}

void FreeJumpingQuad(world* World, world_id JumpingQuadID)
{        
    jumping_quad* Quad = World->JumpingQuadStorage[JumpingQuadID.WorldIndex].Get(JumpingQuadID.ID);
    World->JumpingQuadStorage[JumpingQuadID.WorldIndex].Free(Quad->OtherQuad.ID);
    World->JumpingQuadStorage[JumpingQuadID.WorldIndex].Free(JumpingQuadID.ID);        
}

world_id CreateJumpingQuad(world* World, ak_u32 WorldIndex, ak_v3f Translation, ak_v2f Dimension)
{
    ak_u64 ID = World->JumpingQuadStorage[WorldIndex].Allocate();
    world_id Result = {ID, WorldIndex};
    jumping_quad* Quad = World->JumpingQuadStorage[WorldIndex].Get(ID);
    Quad->ID = ID;
    Quad->Color = AK_Yellow3();
    Quad->CenterP = Translation;
    Quad->Dimensions = Dimension;
    return Result;
}

dual_world_id CreateJumpingQuads(world* World, ak_u32 WorldIndex, ak_v3f* Translations, ak_v2f Dimension)
{
    ak_u64 AID = World->JumpingQuadStorage[WorldIndex].Allocate();
    ak_u64 BID = World->JumpingQuadStorage[WorldIndex].Allocate();
    
    world_id A = {AID, WorldIndex};
    world_id B = {BID, WorldIndex};
    
    jumping_quad* JumpingQuadA = World->JumpingQuadStorage[WorldIndex].Get(A.ID);
    jumping_quad* JumpingQuadB = World->JumpingQuadStorage[WorldIndex].Get(B.ID);
    
    JumpingQuadA->ID = A.ID;
    JumpingQuadB->ID = B.ID;
    
    JumpingQuadA->Color = AK_Yellow3();
    JumpingQuadB->Color = AK_Yellow3();
    
    JumpingQuadA->OtherQuad = B;
    JumpingQuadB->OtherQuad = A;
    
    JumpingQuadA->CenterP = Translations[0];
    JumpingQuadB->CenterP = Translations[1];
    
    JumpingQuadA->Dimensions = Dimension;
    JumpingQuadB->Dimensions = Dimension;
    
    dual_world_id Result = {A, B};
    return Result;
}

world_id CreateEntity(world* World, assets* Assets, ak_u32 WorldIndex, 
                      entity_type Type, sim_entity_type SimType, 
                      ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                      mesh_asset_id MeshID, material Material)
{
    entity_storage* EntityStorage = &World->EntityStorage[WorldIndex];
    simulation* Simulation = &World->Simulations[WorldIndex];
    graphics_state* GraphicsState = &World->GraphicsStates[WorldIndex];
    
    world_id Result = MakeWorldID(EntityStorage->Allocate(), WorldIndex);    
    entity* Entity = EntityStorage->Get(Result.ID);
    
    ak_u32 Index = AK_PoolIndex(Result.ID);
    Entity->Type = Type;    
    Entity->ID = Result;
    Entity->SimEntityID = Simulation->CreateSimEntity(SimType);        
    Entity->LinkID = InvalidWorldID();    
    
    ak_sqtf Transform = AK_SQT(Position, Orientation, Scale);
    Entity->GraphicsEntityID = CreateGraphicsEntity(GraphicsState, MeshID, Material, Transform, IndexToUserData(Index));
    
    if(World->OldTransforms[WorldIndex].Size < (Index+1))
    {
        World->OldTransforms[WorldIndex].Resize(Index+1);
        World->NewTransforms[WorldIndex].Resize(Index+1);
    }
    
    World->OldTransforms[WorldIndex][Index]    = Transform;
    World->NewTransforms[WorldIndex][Index] = Transform;        
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    SimEntity->UserData = IndexToUserData(Index);
    
    SimEntity->Transform = Transform;        
    
    mesh_info* MeshInfo = GetMeshInfo(Assets, MeshID);
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        Simulation->AddCollisionVolume(SimEntity, ConvexHull);           
    }            
    
    return Result;
}

world_id CreatePlayerEntity(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, material Material, player* Player)
{    
    capsule PlayerCapsule = CreateCapsule(AK_V3<ak_f32>(), PLAYER_HEIGHT, PLAYER_RADIUS);
    
    world_id Result = CreateEntity(World, Assets, WorldIndex, ENTITY_TYPE_PLAYER, SIM_ENTITY_TYPE_RIGID_BODY, 
                                   Position, AK_V3(1.0f, 1.0f, 1.0f), AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_PLAYER, Material);
    entity* Entity = World->EntityStorage[WorldIndex].Get(Result.ID);
    Entity->UserData = Player;
    
    rigid_body* RigidBody = World->Simulations[WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    World->Simulations[WorldIndex].AddCollisionVolume(RigidBody, &PlayerCapsule);
    
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 0.0f;        
    return Result;
}

world_id CreateStaticEntity(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation,
                            mesh_asset_id Mesh, material Material)
{
    return CreateEntity(World, Assets, WorldIndex, ENTITY_TYPE_STATIC, SIM_ENTITY_TYPE_SIM_ENTITY, Position, Scale, Orientation, Mesh, Material);
}

world_id CreateMovableEntity(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                             ak_f32 Mass, material Material)
{
    AK_Assert(Mass > 0, "Mass cannot be zero");
    world_id Result = CreateEntity(World, Assets, WorldIndex, ENTITY_TYPE_MOVABLE, SIM_ENTITY_TYPE_RIGID_BODY, Position, Scale, Orientation,
                                   MESH_ASSET_ID_BOX, Material);
    entity* Entity = World->EntityStorage[WorldIndex].Get(Result.ID);    
    
    Entity->UserData = IndexToUserData(CreateMovable(World));
    
    rigid_body* RigidBody = World->Simulations[WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    RigidBody->Restitution = 0;
    RigidBody->InvMass = 0.0f;    
    return Result;
}

world_id CreateMovableEntity(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_f32 Mass, material Material)
{
    return CreateMovableEntity(World, Assets, WorldIndex, Position, Scale, AK_IdentityQuat<ak_f32>(), Mass, Material);
}

world_id CreateFloor(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, material Material)
{
    return CreateStaticEntity(World, Assets, WorldIndex, Position, Scale, AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_FLOOR, Material);
}

dual_world_id CreateFloorInBothWorlds(world* World, assets* Assets, ak_v3f Position, ak_v3f Scale, material Material)
{
    dual_world_id Result;
    Result.A = CreateFloor(World, Assets, 0, Position, Scale, Material);
    Result.B = CreateFloor(World, Assets, 1, Position, Scale, Material);
    return Result;
}

world_id CreateRamp(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, material Material)
{
    return CreateStaticEntity(World, Assets, WorldIndex, Position, Scale, Orientation, MESH_ASSET_ID_RAMP, Material);
}

dual_world_id CreateRampInBothWorlds(world* World, assets* Assets, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, material Material)
{
    dual_world_id Result;
    Result.A = CreateRamp(World, Assets, 0, Position, Scale, Orientation, Material);
    Result.B = CreateRamp(World, Assets, 1, Position, Scale, Orientation, Material);
    return Result;
}

world_id CreateSphereRigidBody(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                               ak_f32 Radius, ak_f32 Mass, ak_f32 Restitution, material Material)
{
    AK_Assert(Mass != 0, "Cannot have zero mass for a rigid body");    
    world_id Result = CreateEntity(World, Assets, WorldIndex, ENTITY_TYPE_RIGID_BODY, SIM_ENTITY_TYPE_RIGID_BODY, Position, Scale*Radius, Orientation, MESH_ASSET_ID_SPHERE, Material);
    ak_u32 Index = AK_PoolIndex(Result.ID);
    entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(Index);
    
    rigid_body* RigidBody = World->Simulations[WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
    
    sphere Sphere = CreateSphere(AK_V3<ak_f32>(), 1.0f);
    World->Simulations[WorldIndex].AddCollisionVolume(RigidBody, &Sphere);
    
    ak_f32 SphereRadius = Sphere.Radius*World->NewTransforms[WorldIndex][Index].Scale.LargestComp();
    RigidBody->Restitution = Restitution;
    RigidBody->InvMass = 1.0f/Mass;
    RigidBody->LocalInvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);
    RigidBody->LocalCenterOfMass = AK_V3<ak_f32>();
    
    return Result;
}

world_id CreateSphereRigidBody(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_f32 Radius, ak_f32 Mass, ak_f32 Restitution, 
                               material Material)
{
    return CreateSphereRigidBody(World, Assets, WorldIndex, Position, AK_V3(1.0f, 1.0f, 1.0f), AK_IdentityQuat<ak_f32>(), Radius, Mass, Restitution, Material);
}

world_id CreateButton(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Dimensions, ak_quatf Orientation, material Material, ak_bool IsToggle)
{
    world_id Result = CreateEntity(World, Assets, WorldIndex, ENTITY_TYPE_BUTTON, SIM_ENTITY_TYPE_SIM_ENTITY, Position, Dimensions, Orientation, MESH_ASSET_ID_BUTTON, Material);
    entity* Entity = World->EntityStorage[WorldIndex].Get(Result.ID);
    Entity->UserData = IndexToUserData(CreateButtonState(World, IsToggle));            
    return Result;
}

world_id CreateButton(world* World, assets* Assets, ak_u32 WorldIndex, ak_v3f Position, material Material, ak_bool IsToggle)
{
    return CreateButton(World, Assets, WorldIndex, Position, AK_V3(1.0f, 1.0f, 1.0f), AK_IdentityQuat<ak_f32>(), Material, IsToggle);
}

inline ak_bool
IsEntityType(entity* Entity, entity_type Type)
{
    ak_bool Result = Entity->Type == Type;
    return Result;
}

inline ak_bool CanBePushed(ak_v2f MoveDirection, ak_v2f ObjectDirection)
{            
    ak_bool Result = AK_Dot(ObjectDirection, MoveDirection) > 0.98f;
    return Result;
}

COLLISION_EVENT(OnButtonCollision)

{
    button_state* ButtonState = GetButtonState(&Game->World, Entity);
    ButtonState->Collided = true;
}