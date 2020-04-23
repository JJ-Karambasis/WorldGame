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
    COLLIDER_TYPE_CAPSULE
};

struct collider
{
    collider_type Type;
    union
    {
        struct
        {
            v3f CenterP;
            v3f Dim;
        } AlignedBox;
        
        struct
        {
            v3f P0;
            v3f P1;
            f32 Radius;
        } Capsule;
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
};

#define MAX_WORLD_ENTITIES 512
struct world_entity_pool
{
    world_entity Entities[MAX_WORLD_ENTITIES];
    i32 MaxUsed;
    i64 NextKey;
    i32 FreeHead;    
};

enum box_entity_type
{
    BOX_ENTITY_TYPE_DEFAULT,
    BOX_ENTITY_TYPE_WALKABLE,
    BOX_ENTITY_TYPE_PUSHABLE
};

struct box_entity
{    
    box_entity_type Type;    
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
    
    box_entity* Link;
    
    box_entity* Next;
    box_entity* Prev;
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

struct entity_list
{
    box_entity* First;
    box_entity* Last;
    u32 Count;
};

struct world
{
    world_entity_pool EntityPool;
    
    player Player;
    entity_list Entities;
    blocker_list Blockers;
};

#endif