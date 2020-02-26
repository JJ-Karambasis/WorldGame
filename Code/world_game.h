//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"
#include "memory.h"
#include "string.h"
#include "error.h"
#include "math.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "graphics.h"

struct entity
{
    b32 IsBlocker;
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
    
    triangle_mesh* Mesh;
    
    entity* Prev;
    entity* Next;
};

struct entity_list
{
    entity* First;
    entity* Last;
    u32 Count;
};

struct walkable_pole
{
    union
    {
        v3f IntersectionPoint;
        struct
        {
            v2f Position2D;
            f32 ZIntersection;
        };
    };    
    b32 HitWalkable;
    entity* HitEntity;
};

struct walkable_grid
{
    v2f Min;
    v2i CellCount;
    v2i PoleCount;
    walkable_pole* Poles;
};

struct triangle_array
{
    u32 Count;
    v3f* Ptr;
};

struct walkable_triangle_ring
{ 
    v3f P[3];    
    walkable_triangle_ring* Next;
};

struct walkable_triangle_ring_list
{
    walkable_triangle_ring* Head;
};

struct game
{
    b32 Initialized;
    input* Input;  
    camera Camera;
    
    triangle_mesh BoxMesh;
    
    arena WorldStorage;
    
    entity_list AllocatedEntities;
    entity_list FreeEntities;
    entity* Player;
};

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform)
typedef GAME_TICK(game_tick);

GAME_TICK(Game_TickStub)
{
}

#if DEVELOPER_BUILD

#include "dev_world_game.h"

global graphics* __Internal_Developer_Graphics__;
#define DEVELOPER_GRAPHICS(Graphics) __Internal_Developer_Graphics__ = Graphics
#define DRAW_POINT(position, size, color) __Internal_Developer_Graphics__->DEBUGDrawPoint(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color) __Internal_Developer_Graphics__->DEBUGDrawLine(position0, position1, width, height, color)

#else
#define DEVELOPER_GRAPHICS(Graphics)
#define DRAW_POINT(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color)
#endif

inline entity* 
CreateEntity(game* Game, v3f Position, v3f Scale, v3f Euler, c4 Color, b32 IsBlocker, triangle_mesh* Mesh)
{
    entity* Result = NULL;
    if(Game->FreeEntities.Count > 0)    
        Result = RemoveEndOfList<entity_list, entity>(&Game->FreeEntities);    
    else
        Result = PushStruct(&Game->WorldStorage, entity, Clear, 0);
    
    Result->Transform = CreateSQT(Position, Scale, Euler);
    Result->Color = Color;
    Result->Mesh = Mesh;
    AddToList(&Game->AllocatedEntities, Result);
    return Result;
}

inline void
FreeEntity(game* Game, entity* Entity)
{
    RemoveFromList(&Game->AllocatedEntities, Entity);
    ClearStruct(Entity, entity);
    AddToList(&Game->FreeEntities, Entity);
}

inline void
FreeAllEntities(game* Game)
{
    entity* Entity = Game->AllocatedEntities.First;    
    while(Entity)
    {
        entity* Remove = Entity;
        Entity = Entity->Next;
        FreeEntity(Game, Remove);        
    }
    ASSERT(!Game->AllocatedEntities.Count);
}