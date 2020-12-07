#ifndef ENGINE_H
#define ENGINE_H

struct engine;
struct game;
struct world;

#include <ak_common.h>

#define ENGINE_INITIALIZE(name) engine* name(ak_string AssetPath)
#define ENGINE_TICK(name) void name(engine* Engine)

#define GAME_INITIALIZE(name) game* name(engine* Engine)
#define GAME_TICK(name) void name(game* Game)

typedef ENGINE_INITIALIZE(engine_initialize);
typedef ENGINE_TICK(engine_tick);

typedef GAME_INITIALIZE(game_initialize);
typedef GAME_TICK(game_tick);

struct engine
{
    ak_arena* Scratch;
    
    ak_v2i Resolution;
    ak_f32 dtFixed;
    
    //assets Assets;
    //input Input;
    
    game* Game;
};

struct game
{
    engine* Engine;
    world* World;
    game_tick* Tick;
};


#endif
