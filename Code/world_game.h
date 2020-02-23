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
struct development_game : public game
{
    b32 InDevelopmentMode;
    camera DevCamera;
};

global graphics* __Internal_Developer_Graphics__;
#define DEVELOPER_GRAPHICS(Graphics) __Internal_Developer_Graphics__ = Graphics
#define DRAW_POINT(position, size, color) __Internal_Developer_Graphics__->DEBUGDrawPoint(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color) __Internal_Developer_Graphics__->DEBUGDrawLine(position0, position1, width, height, color)
#else
#define DEVELOPER_GRAPHICS(Graphics)
#define DRAW_POINT(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color)
#endif