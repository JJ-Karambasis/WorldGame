#ifndef WORLD_H
#define WORLD_H

#define VERY_CLOSE_DISTANCE 0.0001f

enum world_entity_type
{
    WORLD_ENTITY_TYPE_STATIC,
    WORLD_ENTITY_TYPE_PLAYER        
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

inline world_entity_id MakeEntityID(u64 ID, u32 WorldIndex) { world_entity_id Result = {ID, WorldIndex}; return Result; }
inline world_entity_id InvalidEntityID() { return MakeEntityID(0, (u32)-1); }
inline b32 IsInvalidEntityID(world_entity_id ID) { return (ID.ID == 0) || ((ID.WorldIndex != 0) && (ID.WorldIndex != 1)); }

struct world_entity
{
    world_entity_type Type;            
    
    union
    {
        sqt Transform;
        struct
        {
            quaternion Orientation;
            v3f Position;
            v3f Scale;            
        };
    };        
    v3f Velocity;            
    v3f MoveDelta;            
    world_entity_state State;    
    
    collision_volume* CollisionVolumes;    
    mesh_asset_id MeshID;        
    material* Material;    
    
    world_entity_id ID;
    world_entity_id LinkID;                
};

typedef pool<world_entity> world_entity_pool;

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

enum player_state
{
    PLAYER_STATE_DEFAULT,
    PLAYER_STATE_PUSHING
};

struct pushing_state
{
    world_entity_id EntityID;    
    v2f Direction;
};

#include "player.h"

struct world
{   
    u32 WorldIndex;
    world_entity_pool EntityPool;
    game_camera Camera;    
    world_entity* PlayerEntity;        
    jumping_quad JumpingQuads[2];        
};



#endif