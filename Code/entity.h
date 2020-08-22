#ifndef ENTITY_H
#define ENTITY_H

enum entity_type
{
    ENTITY_TYPE_STATIC,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_PUSHABLE
};

enum entity_state
{
    ENTITY_STATE_NONE,
    ENTITY_STATE_JUMPING
};

struct entity_id
{
    u64 ID;
    u32 WorldIndex;    
};

struct dual_entity_id
{
    entity_id EntityA;
    entity_id EntityB;    
};

inline world_entity_id MakeEntityID(u64 ID, u32 WorldIndex) { world_entity_id Result = {ID, WorldIndex}; return Result; }
inline world_entity_id InvalidEntityID() { return MakeEntityID(0, (u32)-1); }
inline b32 IsInvalidEntityID(world_entity_id ID) { return (ID.ID == 0) || ((ID.WorldIndex != 0) && (ID.WorldIndex != 1)); }
inline b32 AreEqualIDs(world_entity_id A, world_entity_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

struct entity
{
    entity_type Type;
    entity_state State;
    entity_id ID;
    entity_id LinkID;    
    mesh_asset_id MeshID;
    material Material;
};

#endif