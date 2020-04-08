//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"
#include "memory.h"
#include "string.h"
#include "error.h"
#include "riff.h"
#include "math.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
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
    
    triangle3D_mesh* Mesh;    
    entity* Next;
};

struct entity_list
{
    entity* Head;
};

struct game
{
    b32 Initialized;
    input* Input;  
    camera Camera;
    
    triangle3D_mesh BoxMesh;
    
    arena WorldStorage;

    player Player;
    entity_list  StaticEntities;        
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
#define DEBUG_START_TIMER(name)
#define DEBUG_END_TIMER(name)
#endif
