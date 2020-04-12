//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"
#include "memory.h"
#include "list.h"
#include "hash_table.h"
#include "string.h"
#include "error.h"
#include "riff.h"
#include "math.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "assets.h"
#include "audio.h"
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
    
    entity* Next;
    entity* Prev;
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
    assets* Assets;
    audio* Audio;
    input* Input;  
    camera Camera;
    
    arena WorldStorage;
    
    player Player;
    
    entity_list Entities;    
};

#define MOVE_ACCELERATION 20.0f
#define MOVE_DAMPING 5.0f

#define GAME_TICK(name) void name(game* Game, graphics* Graphics, platform* Platform)
typedef GAME_TICK(game_tick);

GAME_TICK(Game_TickStub)
{
}

#if DEVELOPER_BUILD

#include "dev_world_game.h"

global development_game* __Internal_Developer_Game__;
#define DEVELOPER_GAME(Game) __Internal_Developer_Game__ = (development_game*)Game
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations) __Internal_Developer_Game__->MaxGJKIterations = MaximumI32(__Internal_Developer_Game__->MaxGJKIterations, Iterations)

global graphics* __Internal_Developer_Graphics__;
#define DEVELOPER_GRAPHICS(Graphics) __Internal_Developer_Graphics__ = Graphics
#define DRAW_POINT(position, size, color) __Internal_Developer_Graphics__->DEBUGDrawPoint(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color) __Internal_Developer_Graphics__->DEBUGDrawLine(position0, position1, width, height, color)

#define DRAW_TRIANGLE(p0, p1, p2, color, width) \
do \
{ \
    DRAW_LINE(p0, p1, width, width, color); \
    DRAW_LINE(p1, p2, width, width, color);\
    DRAW_LINE(p2, p0, width, width, color);\
} while(0)

#else
#define DEVELOPER_GAME(Game)
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations)

#define DEVELOPER_GRAPHICS(Graphics)
#define DRAW_POINT(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color)
#define DRAW_TRIANGLE(p0, p1, p2, color, width)
#define DEBUG_START_TIMER(name)
#define DEBUG_END_TIMER(name)
#endif
