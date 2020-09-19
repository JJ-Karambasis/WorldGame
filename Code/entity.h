#ifndef ENTITY_H
#define ENTITY_H

struct sim_entity_base;

enum entity_type
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_STATIC,    
    ENTITY_TYPE_RIGID_BODY,
    ENTITY_TYPE_PUSHABLE,
    ENTITY_TYPE_COUNT
};

struct entity_id
{
    ak_u64 ID;
    ak_u32 WorldIndex;    
    
    inline ak_bool IsValid()
    {
        ak_bool Result = (ID != 0) && ((WorldIndex == 0) || (WorldIndex == 1));
        return Result;
    }
};

struct dual_entity_id
{
    entity_id EntityA;
    entity_id EntityB;    
};

inline entity_id MakeEntityID(ak_u64 ID, ak_u32 WorldIndex) { entity_id Result = {ID, WorldIndex}; return Result; }
inline entity_id InvalidEntityID() { return MakeEntityID(0, (ak_u32)-1); }
inline ak_bool AreEqualIDs(entity_id A, entity_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

struct entity;

#define COLLISION_EVENT(name) void name(struct game* Game, entity* Entity, entity* CollidedEntity, ak_v3f Normal) 
typedef COLLISION_EVENT(collision_event_function);

struct entity
{    
    entity_type Type;    
    entity_id ID;
    entity_id LinkID;         
    sim_entity_id SimEntityID;    
    mesh_asset_id MeshID;
    material Material;                
    void* UserData;            
    
    collision_event_function* OnCollision;
};

#define GetUserData(Entity, type) ((type*)Entity->UserData)

COLLISION_EVENT(OnCollisionStub) { }

global ak_f32 Global_PlayerAcceleration = 20.0f;
global ak_f32 Global_PlayerDamping = 5.0f;
global ak_f32 Global_Gravity = 10.0f;

#include "player.h"

#endif