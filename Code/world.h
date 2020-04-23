#ifndef WORLD_H
#define WORLD_H

#include "player.h"

enum world_entity_type
{
    WORLD_ENTITY_TYPE_STATIC,
    WORLD_ENTITY_TYPE_PLAYER,
    WORLD_ENTITY_TYPE_WALKABLE,
    WORLD_ENTITY_TYPE_PUSHABLE
};

enum collider_type 
{
    COLLIDER_TYPE_UNKNOWN,
    COLLIDER_TYPE_ALIGNED_BOX,
    COLLIDER_TYPE_VERTICAL_CAPSULE
};

struct vertical_capsule
{
    v3f P;            
    f32 Radius;
    f32 Height;
};

struct aligned_box
{
    v3f CenterP;
    v3f Dim;
};

struct collider
{
    collider_type Type;
    union
    {
        aligned_box AlignedBox;
        vertical_capsule VerticalCapsule;        
    };
};

struct world_entity
{
    world_entity_type Type;    
    u32 WorldIndex;
    
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
    
    c4 Color;
    v3f Velocity;        
    
    collider Collider;
    
    i64 ID;
    i64 LinkID;
    
    void* UserData;
};

#define MAX_WORLD_ENTITIES 512
struct world_entity_pool
{
    world_entity Entities[MAX_WORLD_ENTITIES];
    i32 MaxUsed;
    i64 NextKey;
    i32 FreeHead;    
};

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

enum player_state
{
    PLAYER_STATE_DEFAULT,
    PLAYER_STATE_PUSHING
};

struct pushing_state
{
    i64 EntityID;
    v2f Direction;
};

struct player
{
    i64 EntityID;    
    player_state State;
    pushing_state Pushing;
};

struct blocker
{
    v3f P0;
    f32 Height0;
    v3f P1;
    f32 Height1;
    
    blocker* Next;
    blocker* Prev;
};

struct blocker_list
{
    blocker* First;
    blocker* Last;
    u32 Count;
};

struct world
{    
    world_entity_pool EntityPool;
    player Player;
    
    blocker_list Blockers;
};

#endif