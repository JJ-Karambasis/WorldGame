inline pushing_state
InitPushingState()
{
    pushing_state Result;
    Result.EntityID = InvalidEntityID();
    Result.Direction = {};
    return Result;
}

inline b32 
AreEqualIDs(world_entity_id AID, world_entity_id BID)
{
    b32 Result = (AID.ID == BID.ID) && (AID.WorldIndex == BID.WorldIndex);
    return Result;
}

inline world* 
GetWorld(game* Game, u32 WorldIndex)
{
    ASSERT((WorldIndex == 0) || (WorldIndex == 1));
    world* World = Game->Worlds + WorldIndex;
    return World;
}

inline world* 
GetWorld(game* Game, world_entity_id ID)
{    
    world* World = GetWorld(Game, ID.WorldIndex);
    return World;
}

inline world* 
GetCurrentWorld(game* Game)
{
    world* World = GetWorld(Game, Game->CurrentWorldIndex);
    return World;
}

inline world* 
GetNotCurrentWorld(game* Game)
{
    world* World = GetWorld(Game, !Game->CurrentWorldIndex);
    return World;
}

inline b32 
IsCurrentWorldIndex(game* Game, u32 WorldIndex)
{
    b32 Result = Game->CurrentWorldIndex == WorldIndex;
    return Result;
}

inline i64 
GetIndex(world_entity_pool* Pool, i64 ID)
{
    i64 Result = ID & 0xFFFFFFFF;
    ASSERT(Result <= Pool->MaxUsed);
    return Result;
}

inline world_entity* 
GetEntity(world_entity_pool* Pool, world_entity_id EntityID)
{
    world_entity* Result = GetByID(Pool, EntityID.ID);    
    return Result;
}

inline world_entity* 
GetEntity(world* World, world_entity_id EntityID)
{        
    world_entity* Result = GetEntity(&World->EntityPool, EntityID);
    return Result;
}

inline world_entity* 
GetEntity(game* Game, world_entity_id EntityID)
{    
    world_entity* Result = GetEntity(GetWorld(Game, EntityID.WorldIndex), EntityID);
    return Result;
}

inline world_entity* 
GetEntityOrNull(world_entity_pool* Pool, world_entity_id EntityID)
{
    if(IsInvalidEntityID(EntityID))
        return NULL;
    
    world_entity* Result = GetEntity(Pool, EntityID);
    return Result;
}

inline world_entity* 
GetEntityOrNull(world* World, world_entity_id EntityID)
{    
    world_entity* Result = GetEntityOrNull(&World->EntityPool, EntityID);
    return Result;    
}

inline world_entity* 
GetEntityOrNull(game* Game, world_entity_id EntityID)
{
    if(IsInvalidEntityID(EntityID))
        return NULL;
    
    world_entity* Result = GetEntityOrNull(GetWorld(Game, EntityID.WorldIndex), EntityID);
    return Result;    
}

inline v3f
GetPlayerPosition(game* Game, player* Player)
{
    v3f Result = GetEntity(Game, Player->EntityID)->Position;
    return Result;
}

inline v3f
GetPlayerVelocity(game* Game, player* Player)
{
    v3f Result = GetEntity(Game, Player->EntityID)->Velocity;
    return Result;
}

void FreeEntity(game* Game, world_entity_id ID)
{
    world* World = GetWorld(Game, ID);
    FreeFromPool(&World->EntityPool, ID.ID);
}

world_entity*
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Euler, graphics_material* Material, mesh* Mesh, collision_volume CollisionVolume, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    
    i64 EntityID = AllocateFromPool(&World->EntityPool);
    world_entity* Entity = GetByID(&World->EntityPool, EntityID);
    
    Entity->ID = MakeEntityID(EntityID, WorldIndex);
    
    Entity->Type = Type;
    Entity->Transform = CreateRigidTransform(Position, Euler);
    Entity->Material = Material;
    Entity->Mesh = Mesh;
    Entity->CollisionVolume = CollisionVolume;    
    Entity->LinkID = InvalidEntityID();
    
    return Entity;
}

world_entity*
CreateStaticEntity(game* Game, u32 WorldIndex, v3f Position, v3f Euler, graphics_material* Material, mesh* Mesh, collision_volume CollisionVolume, void* UserData=NULL)
{
    world_entity* Result = CreateEntity(Game, WORLD_ENTITY_TYPE_STATIC, WorldIndex, Position, Euler, Material, Mesh, CollisionVolume, UserData);
    return Result;
}

world_entity_id
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, graphics_material* Material, mesh* Mesh, walkable_mesh* WalkableMesh = NULL, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    
    i64 EntityID = AllocateFromPool(&World->EntityPool);
    world_entity* Entity = GetByID(&World->EntityPool, EntityID);
    
    Entity->ID = MakeEntityID(EntityID, WorldIndex);
    
    Entity->Type = Type;    
    Entity->Transform = CreateRigidTransform(Position, Euler);
    Entity->Material = Material;
    Entity->Mesh = Mesh;
    Entity->WalkableMesh = WalkableMesh;    
    Entity->LinkID = InvalidEntityID();
    
    return Entity->ID;
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, graphics_material* Material0, graphics_material* Material1, mesh* Mesh0, mesh* Mesh1, walkable_mesh* WalkableMesh0=NULL, walkable_mesh* WalkableMesh1=NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Material0, Mesh0, WalkableMesh0);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Material1, Mesh1, WalkableMesh1);    
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, graphics_material* Material0, graphics_material* Material1, mesh* Mesh, walkable_mesh* WalkableMesh = NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Material0, Mesh, WalkableMesh);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Material1, Mesh, WalkableMesh);    
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, graphics_material* Material, mesh* Mesh, walkable_mesh* WalkableMesh = NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Material, Mesh, WalkableMesh);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Material, Mesh, WalkableMesh);    
}

inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

void 
OnWorldSwitch(game* Game, u32 LastWorldIndex, u32 CurrentWorldIndex)
{    
    GetWorld(Game, LastWorldIndex)->PlayerEntity->Velocity = {};    
}

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