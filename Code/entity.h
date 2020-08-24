#ifndef ENTITY_H
#define ENTITY_H

enum entity_type
{
    ENTITY_TYPE_STATIC,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_RIGID_BODY
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

inline entity_id MakeEntityID(u64 ID, u32 WorldIndex) { entity_id Result = {ID, WorldIndex}; return Result; }
inline entity_id InvalidEntityID() { return MakeEntityID(0, (u32)-1); }
inline b32 IsInvalidEntityID(entity_id ID) { return (ID.ID == 0) || ((ID.WorldIndex != 0) && (ID.WorldIndex != 1)); }
inline b32 AreEqualIDs(entity_id A, entity_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

struct entity
{
    entity_type Type;
    entity_state State;
    entity_id ID;
    entity_id LinkID;         
    u64 SimEntityID;            
    mesh_asset_id MeshID;
    material Material;    
};

typedef pool<entity> entity_storage;

global f32 Global_PlayerAcceleration = 20.0f;
global f32 Global_PlayerDamping = 5.0f;
global f32 Global_Gravity = 20.0f;

#endif