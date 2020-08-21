inline world_entity* 
GetEntity(game* Game, world_entity_id EntityID)
{    
    world_entity* Result = TryAndGetByID(&Game->EntityStorage[EntityID.WorldIndex], EntityID.ID);
    return Result;
}

void FreeEntity(game* Game, world_entity_id ID)
{
    sim_state* SimState = GetSimState(Game, ID);
    collision_volume* Volume = SimState->CollisionVolumes;
    while(Volume)
    {
        collision_volume* VolumeToFree = Volume;
        Volume = Volume->Next;        
        FreeFromPool(&Game->CollisionVolumeStorage[ID.WorldIndex], VolumeToFree);        
    }                    
    
    FreeFromPool(&Game->EntityStorage[ID.WorldIndex], ID.ID);
}

inline sqt* GetEntityTransformOld(game* Game, world_entity_id ID)
{
    u32 PoolIndex = GetPoolIndex(ID.ID);
    sqt* Result = &Game->PrevTransforms[ID.WorldIndex][PoolIndex];
    return Result;
}

inline sqt* GetEntityTransform(game* Game, world_entity_id ID)
{
    u32 PoolIndex = GetPoolIndex(ID.ID);
    sqt* Result = &Game->CurrentTransforms[ID.WorldIndex][PoolIndex];
    return Result;
}

inline v3f GetEntityPosition(game* Game, world_entity_id ID)
{
    v3f Result = GetEntityTransform(Game, ID)->Translation;
    return Result;
}

world_entity_id
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, mesh_asset_id MeshID, material Material, b32 NoMeshColliders = false)
{   
    world_entity_pool* EntityStorage = Game->EntityStorage + WorldIndex;
    i64 EntityID = AllocateFromPool(EntityStorage);    
    world_entity_id Result = MakeEntityID(EntityID, WorldIndex);
    
    u32 PoolIndex = GetPoolIndex(EntityID);
    world_entity* Entity = GetByID(EntityStorage, EntityID);
        
    Entity->Type = Type;    
    Entity->State = WORLD_ENTITY_STATE_NOTHING;
    Entity->ID = Result;    
    Entity->LinkID = InvalidEntityID();    
    Entity->MeshID = MeshID;            
    Entity->Material = Material;    
    
    sqt Transform = CreateSQT(Position, Scale, Euler);    
           
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

world_entity_id
CreateStaticEntity(game* Game, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, mesh_asset_id Mesh, material Material, b32 NoMeshColliders = false)
{
    world_entity_id Result = CreateEntity(Game, WORLD_ENTITY_TYPE_STATIC, WorldIndex, Position, Scale, Euler, Mesh, Material, NoMeshColliders);
    return Result;
}

inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

inline b32 
IsEntityType(world_entity* Entity, world_entity_type Type)
{
    b32 Result = Entity->Type == Type;
    return Result;
}

#if 0 
void 
UpdateWorld(game* Game)
{                
    world* World = GetWorld(Game, Game->CurrentWorldIndex);         
    
    DEBUG_DRAW_QUAD(World->JumpingQuads[0].CenterP, Global_WorldZAxis, World->JumpingQuads[0].Dimensions, Yellow3());    
    DEBUG_DRAW_QUAD(World->JumpingQuads[1].CenterP, Global_WorldZAxis, World->JumpingQuads[1].Dimensions, Yellow3());
    
    FOR_EACH(Entity, &World->EntityPool)
    {        
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
            {                
                UpdatePlayer(Game, Entity);                
            } break;                        
        }                        
    }    
    
    world_entity* PlayerEntity = World->PlayerEntity;
    
    game_camera* Camera = &World->Camera;    
    Camera->Target = PlayerEntity->Position;        
}
#endif