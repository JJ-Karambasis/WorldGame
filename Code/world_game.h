//TODO(JJ): Should actually be changed into the actual game name
#define GAME_NAME "WorldGame"

#include "common.h"
#include "memory.h"
#include "string.h"
#include "error.h"
#include "math.h"

#include "platform.h"
#include "input.h"
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
};

struct game
{
    b32 Initialized;
    input* Input;  
    camera Camera;
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
#endif