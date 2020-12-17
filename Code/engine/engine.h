#ifndef ENGINE_H
#define ENGINE_H

#define GAME_NAME AK_Stringify(_GAME_NAME_)
#define WORLDS_PATH AK_Stringify(_WORLDS_PATH_ ## \\)

struct platform;
struct engine;
struct game;
struct world;

#if DEVELOPER_BUILD
#define AK_DEVELOPER_BUILD
#endif

#include <ak_common.h>

#define GAME_NAME_DLL AK_Cat(GAME_NAME, ".dll")

#include "include/input.h"
#include <include/graphics.h>
#include "include/assets.h"
#include "include/graphics_state.h"
#include "include/game_loop_accum.h"

#define GAME_STARTUP(name) ak_bool name(engine* Engine)
#define GAME_UPDATE(name) void name(game* Game)
#define GAME_SHUTDOWN(name) void name(game* Game)
#define GAME_GET_GRAPHICS_STATE(name) graphics_state name(game* Game, ak_f32 tInterpolated)

typedef GAME_STARTUP(game_startup);
typedef GAME_UPDATE(game_update);
typedef GAME_SHUTDOWN(game_shutdown);
typedef GAME_GET_GRAPHICS_STATE(game_get_graphics_state);

struct game_code
{
    game_startup* Startup;
    game_update*  Update;
    game_shutdown* Shutdown;
    game_get_graphics_state* GetGraphicsState;
};

#define WORLD_STARTUP(name) ak_bool name(game* Game)
#define WORLD_UPDATE(name) void name(game* Game)
#define WORLD_SHUTDOWN(name) void name(game* Game)

typedef WORLD_STARTUP(world_startup);
typedef WORLD_UPDATE(world_update);
typedef WORLD_SHUTDOWN(world_shutdown);

struct world_code
{
    world_startup* Startup;
    world_update* Update;
    world_shutdown* Shutdown;
};

#define PLATFORM_PROCESS_MESSAGES(name) ak_bool name(input* Input)
#define PLATFORM_LOAD_GAME_CODE(name) ak_bool name(game_code* GameCode)
#define PLATFORM_LOAD_WORLD_CODE(name) ak_bool name(world_code* WorldCode, ak_string WorldName)
#define PLATFORM_GET_RESOLUTION(name) ak_v2i name()

typedef PLATFORM_PROCESS_MESSAGES(platform_process_messages);
typedef PLATFORM_LOAD_GAME_CODE(platform_load_game_code);
typedef PLATFORM_LOAD_WORLD_CODE(platform_load_world_code);
typedef PLATFORM_GET_RESOLUTION(platform_get_resolution);

struct platform
{
    ak_string ProgramPath;
    ak_string AssetPath;
    platform_process_messages* ProcessMessages;
    platform_load_game_code* LoadGameCode;
    platform_load_world_code* LoadWorldCode;
    platform_get_resolution* GetResolution;
    execute_render_commands* ExecuteRenderCommands;
};

#define ENGINE_RUN(name) ak_i32 name(platform* Platform, graphics* Graphics)
typedef ENGINE_RUN(engine_run);

#define ENGINE_LOAD_ASSETS(name) assets* name(ak_string AssetPath)
typedef ENGINE_LOAD_ASSETS(engine_load_assets);

struct engine_code
{
    engine_run* EngineRun;
    engine_load_assets* LoadAssets;
};

struct engine
{
    ak_arena* Scratch;
    ak_f32 dtFixed;
    
    input Input;
    
    
    assets* Assets;
    graphics* Graphics;
    graphics_render_buffer* RenderBuffer;
    
    game* Game;
};

struct game
{
    engine* Engine;
    world* World;
};

#endif
