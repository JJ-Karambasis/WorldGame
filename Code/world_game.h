//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"

#include "platform.h"
#include "input.h"
#include "geometry.h"
#include "graphics_2.h"
#include "assets.h"
#include "audio.h"
#include "world.h"

struct camera
{
    v3f Velocity;
    v3f Position;    
    v3f FocalPoint;
    m3 Orientation;
    v3f AngularVelocity;
    f32 Distance;
};

struct game
{
    b32 Initialized;
    assets* Assets;
    audio* Audio;
    input* Input;  
    camera Camera;
    
    arena GameStorage;
    
    f32 dt;
    u32 CurrentWorldIndex;
    
    world Worlds[2];        
    f32 PlayerRadius;
    f32 PlayerHeight;
};

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
#define DEVELOPER_INCREMENT_WALKING_TRIANGLE() __Internal_Developer_Game__->WalkingTriangleCount[Game->CurrentWorldIndex]++
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations) __Internal_Developer_Game__->MaxTimeIterations = MaximumI32(__Internal_Developer_Game__->MaxTimeIterations, Iterations)

global graphics* __Internal_Developer_Graphics__;
#define DEVELOPER_GRAPHICS(Graphics) __Internal_Developer_Graphics__ = Graphics
#define DRAW_POINT(position, size, color) __Internal_Developer_Graphics__->DEBUGDrawPoint(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color) __Internal_Developer_Graphics__->DEBUGDrawLine(position0, position1, width, height, color)

#define DRAW_TRIANGLE(triangle, color, width) \
do \
{ \
    DRAW_LINE(triangle.P[0], triangle.P[1], width, width, color); \
    DRAW_LINE(triangle.P[1], triangle.P[2], width, width, color);\
    DRAW_LINE(triangle.P[2], triangle.P[0], width, width, color);\
} while(0)

#else
#define DEVELOPER_GAME(Game)
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations)
#define DEVELOPER_INCREMENT_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations)
#define DEVELOPER_GRAPHICS(Graphics)
#define DRAW_POINT(position, size, color)
#define DRAW_LINE(position0, position1, width, height, color)
#define DRAW_TRIANGLE(p0, p1, p2, color, width)
#define DEBUG_START_TIMER(name)
#define DEBUG_END_TIMER(name)
#endif
