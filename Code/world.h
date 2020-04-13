#ifndef WORLD_H
#define WORLD_H

#include "player.h"

struct entity
{    
    b32 Simulate;   
    triangle3D_mesh* WalkableMesh;    
    aabb3D AABB;        
    
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
    
    entity* Link;
    
    entity* Next;
    entity* Prev;
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
    entity* First;
    entity* Last;
    u32 Count;
};

struct world
{
    player Player;
    entity_list Entities;
    blocker_list Blockers;
};

#endif