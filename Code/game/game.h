#ifndef GAME_H
#define GAME_H

#define GAME_NAME AK_Stringify(_GAME_NAME_)
#define WORLDS_PATH AK_Stringify(_WORLDS_PATH_ ## \\)

#define MAX_OBJECT_NAME_LENGTH 128

#define TARGET_DT (1.0f/60.0f)

struct platform;
struct game;
struct world;

#if DEVELOPER_BUILD
#define AK_DEVELOPER_BUILD
#endif

#include <ak_common.h>
#include <include/graphics.h>
#include <assets.h>
#include "game_common_include.h"
#include <include/graphics_state.h>

#define GAME_STARTUP(name) game* name(graphics* Graphics, assets* Assets)
#define GAME_UPDATE(name) void name(game* Game)
#define GAME_SHUTDOWN(name) void name(game* Game)

#define WORLD_STARTUP(name) ak_bool name(game* Game)
#define WORLD_UPDATE(name) void name(game* Game)
#define WORLD_SHUTDOWN(name) void name(game* Game)

#define PLATFORM_PROCESS_MESSAGES(name) ak_bool name(input* Input)
#define PLATFORM_LOAD_GAME_CODE(name) game_startup* name()
#define PLATFORM_LOAD_WORLD_CODE(name) world_startup* name(ak_string WorldPath, ak_string WorldName)
#define PLATFORM_UNLOAD_CODE(name) void name()
#define PLATFORM_GET_RESOLUTION(name) ak_v2i name()

typedef GAME_STARTUP(game_startup);
typedef GAME_UPDATE(game_update); 
typedef GAME_SHUTDOWN(game_shutdown);

typedef WORLD_STARTUP(world_startup);
typedef WORLD_UPDATE(world_update);
typedef WORLD_SHUTDOWN(world_shutdown);

typedef PLATFORM_PROCESS_MESSAGES(platform_process_messages);
typedef PLATFORM_LOAD_GAME_CODE(platform_load_game_code);
typedef PLATFORM_LOAD_WORLD_CODE(platform_load_world_code);
typedef PLATFORM_UNLOAD_CODE(platform_unload_code);
typedef PLATFORM_GET_RESOLUTION(platform_get_resolution);

struct platform
{
    ak_string ProgramPath;
    ak_string AssetPath;
    platform_process_messages* ProcessMessages;
    platform_load_game_code* LoadGameCode;
    platform_load_world_code* LoadWorldCode;
    platform_unload_code* UnloadWorldCode;
    platform_unload_code*  UnloadGameCode;
    platform_get_resolution* GetResolution;
    execute_render_commands* ExecuteRenderCommands;
};

struct game
{
    ak_arena* Scratch;
    ak_f32 dtFixed;
    
    input Input;
    assets* Assets;
    graphics* Graphics;
    graphics_render_buffer* RenderBuffer;
    world* World;
    
    game_loop_accum LoopAccum;
    
    graphics_camera Cameras[2];
    
    game_update* Update;
    game_shutdown* Shutdown;
    world_shutdown* WorldShutdownCommon;
    
    ak_u32 CurrentWorldIndex;
};

struct toi_normal
{
    ak_bool Intersected;
    ak_v3f  Normal;
    ak_f32  tHit;
};

struct world
{
    ak_pool<entity> EntityStorage[2];
    ak_pool<point_light> PointLightStorage[2];
    ak_array<ak_sqtf> OldTransforms[2];
    ak_array<physics_object> PhysicsObjects[2];
    ak_array<graphics_object> GraphicsObjects[2];
    ak_array<button_state> ButtonStates[2];
    ak_array<movable> Movables[2];
    ak_pool<collision_volume> CollisionVolumeStorage;
    
    player Players[2];
    
    world_update* Update;
    world_shutdown* Shutdown;
};

extern "C" AK_EXPORT GAME_STARTUP(Game_Startup);
extern "C" AK_EXPORT GAME_UPDATE(Game_Update);
extern "C" AK_EXPORT GAME_SHUTDOWN(Game_Shutdown);
WORLD_SHUTDOWN(Game_WorldShutdownCommon);

#if defined(GAME_COMPILED) && defined(DEVELOPER_BUILD)
#include <editor.h>

extern "C"
{
    AK_EXPORT editor* Internal__Editor;
}

#define Debug_Log(format, ...) Internal__Editor->DebugLog(format, __VA_ARGS__)
#define Debug_DrawPoint(position, size, color) Internal__Editor->DrawPoint(Internal__Editor, position, size, color)
#define Debug_DrawSegment(position0, position1, size, color) Internal__Editor->DrawSegment(Internal__Editor, position0, position1, size, color)
#define Debug_AddEntity(world_index, id, name) Internal__Editor->AddEntity(Internal__Editor, world_index, id, name)
#else
#define Debug_Log(format, ...)
#define Debug_DrawPoint(position, size, color)
#define Debug_DrawSegment(position0, position1, size, color)
#define Debug_AddEntity(world_index, id, name)
#endif


inline toi_normal MakeTOINormal(ak_f32 tHit, ak_v3f Normal)
{
    toi_normal Result = {};
    Result.Intersected = true;
    Result.Normal = Normal;
    Result.tHit = tHit;
    return Result;
}

#endif