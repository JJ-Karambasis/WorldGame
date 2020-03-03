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

struct player
{
    v3f Position;
    f32 Radius;
    f32 Height;
    v2f FacingDirection;
    c4 Color;
    v3f Velocity;
};

struct static_entity
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
    static_entity* Next;
};

struct static_entity_list
{
    static_entity* Head;
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
    static_entity* HitEntity;
    triangle* HitTriangle;
    b32 HitWalkable;
};

struct walkable_grid
{
    v2f Min;
    v2i CellCount;
    v2i PoleCount;
    walkable_pole* Poles;
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

    player Player;
    static_entity_list StaticEntities;
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
