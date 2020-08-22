#ifndef WORLD_H
#define WORLD_H

#define VERY_CLOSE_DISTANCE 0.0001f

enum world_entity_type
{
    WORLD_ENTITY_TYPE_STATIC,
    WORLD_ENTITY_TYPE_PLAYER,
    WORLD_ENTITY_TYPE_PUSHABLE
};

enum world_entity_state
{
    WORLD_ENTITY_STATE_NOTHING,
    WORLD_ENTITY_STATE_JUMPING
};

struct world_entity_id
{
    u64 ID;
    u32 WorldIndex;
};

struct dual_world_entity_id
{
    world_entity_id EntityA;
    world_entity_id EntityB;
};

inline world_entity_id MakeEntityID(u64 ID, u32 WorldIndex) { world_entity_id Result = {ID, WorldIndex}; return Result; }
inline world_entity_id InvalidEntityID() { return MakeEntityID(0, (u32)-1); }
inline b32 IsInvalidEntityID(world_entity_id ID) { return (ID.ID == 0) || ((ID.WorldIndex != 0) && (ID.WorldIndex != 1)); }
inline b32 AreEqualIDs(world_entity_id A, world_entity_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

struct world_entity
{
    world_entity_type Type;   
    world_entity_state State;        
    world_entity_id ID;
    world_entity_id LinkID;                
    
    mesh_asset_id MeshID;        
    material Material;        
};

typedef pool<world_entity> world_entity_pool;

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

#endif