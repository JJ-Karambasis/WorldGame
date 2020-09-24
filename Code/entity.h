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

struct world_id
{
    ak_u64 ID;
    ak_u32 WorldIndex;    
    
    
    inline ak_bool IsValid()
    {
        ak_bool Result = (ID != 0) && ((WorldIndex == 0) || (WorldIndex == 1));
        return Result;
    }
};


inline world_id MakeWorldID(ak_u64 ID, ak_u32 WorldIndex) { world_id Result = {ID, WorldIndex}; return Result; }
inline world_id InvalidWorldID() { return MakeWorldID(0, (ak_u32)-1); }
inline ak_bool AreEqualIDs(world_id A, world_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

struct dual_world_id
{
    world_id EntityA;
    world_id EntityB;    
};

struct entity;

#define COLLISION_EVENT(name) void name(struct game* Game, entity* Entity, entity* CollidedEntity, ak_v3f Normal) 
typedef COLLISION_EVENT(collision_event_function);

struct entity
{    
    entity_type Type;    
    world_id ID;
    world_id LinkID;         
    sim_entity_id SimEntityID;                 
    ak_u64 GraphicsEntityID;    
    void* UserData;            
    
    collision_event_function* OnCollision;
};

enum player_state
{
    PLAYER_STATE_NONE,
    PLAYER_STATE_JUMPING,
    PLAYER_STATE_PUSHING
};

struct pushing_object
{
    ak_u64 ID;
    world_id PlayerID;
    ak_v2f Direction;
};

struct player
{    
    player_state State;        
};

COLLISION_EVENT(OnPlayerCollision);

#define GetUserData(Entity, type) ((type*)Entity->UserData)

COLLISION_EVENT(OnCollisionStub) { }

global ak_f32 Global_PlayerAcceleration = 20.0f;
global ak_f32 Global_PlayerDamping = 5.0f;
global ak_f32 Global_Gravity = 10.0f;

#include "player.h"

#endif