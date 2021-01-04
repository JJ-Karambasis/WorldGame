#ifndef EDITOR_H
#define EDITOR_H

#ifndef GAME_COMPILED
#include <game.h>
#endif

#define BUILD_WORLD_LOG_FILE "build_world_log.txt"

#ifdef _RECORDINGS_PATH_
#define RECORDINGS_PATH AK_Stringify(_RECORDINGS_PATH_ ## \\)
#endif

global const ak_char WORLD_FILE_SIGNATURE[] = "WGWORLD";
global const ak_u16 WORLD_FILE_MAJOR_VERSION = 1;
global const ak_u16 WORLD_FILE_MINOR_VERSION = 0;
global const ak_char WORLD_FILE_CHECKSUM[] = "BA9018D12F370C08092345BE9E5BAB61";

struct dev_platform;
struct editor;
struct world_management;
struct dev_entity;
struct dev_point_light;
struct ImGuiContext;

#define GIZMO_PLANE_DISTANCE 0.4f
#define POINT_LIGHT_RADIUS 0.1f

#define EDITOR_RUN(name) ak_i32 name(graphics* Graphics, platform* Platform, dev_platform* DevPlatform, ImGuiContext* Context)
typedef EDITOR_RUN(editor_run);

#define DEV_PLATFORM_UPDATE(name) ak_bool name(editor* Editor, ak_f32 dt)
typedef DEV_PLATFORM_UPDATE(dev_platform_update);

#define DEV_BUILD_WORLD(name) ak_bool name(ak_string WorldPath)
typedef DEV_BUILD_WORLD(dev_build_world);

#define DEV_SET_GAME_DEBUG_EDITOR(name) ak_bool name(editor* Editor)
typedef DEV_SET_GAME_DEBUG_EDITOR(dev_set_game_debug_editor);

#define DEV_HANDLE_HOT_RELOAD(name) void name(editor* Editor)
typedef DEV_HANDLE_HOT_RELOAD(dev_handle_hot_reload);

#define EDITOR_DEBUG_LOG(name) void name(const ak_char* Format, ...)
typedef EDITOR_DEBUG_LOG(editor_debug_log);

#define EDITOR_DRAW_POINT(name) void name(editor* Editor, ak_v3f Position, ak_f32 Size, ak_color3f Color)
typedef EDITOR_DRAW_POINT(editor_draw_point);

#define EDITOR_DRAW_SEGMENT(name) void name(editor* Editor, ak_v3f P0, ak_v3f P1, ak_f32 Size, ak_color3f Color)
typedef EDITOR_DRAW_SEGMENT(editor_draw_segment);

#define EDITOR_ADD_ENTITY(name) void name(editor* Editor, ak_u32 WorldIndex, ak_u64 ID, const ak_char* Name)
typedef EDITOR_ADD_ENTITY(editor_add_entity);

struct world_file_header
{
    ak_char Signature[AK_Count(WORLD_FILE_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u16 EntityCount[2];
    ak_u16 PointLightCount[2];
};

struct dev_platform
{
    dev_platform_update* Update;
    dev_build_world* BuildWorld;
    dev_set_game_debug_editor* SetGameDebugEditor;
    dev_handle_hot_reload* HandleHotReload;
};

#define MAX_WORLD_NAME 256
#define MAX_MESH_NAME 256

enum object_type
{
    OBJECT_TYPE_ENTITY,
    OBJECT_TYPE_LIGHT
};

struct object
{
    object_type Type;
    ak_u64      ID;
    
    dev_entity* GetEntity(world_management* WorldManagement, ak_u32 WorldIndex);
    dev_point_light* GetPointLight(world_management* WorldManagement, ak_u32 WorldIndex);
    ak_v3f GetPosition(world_management* WorldManagement, ak_u32 WorldIndex);
    
    ak_bool IsAlive(world_management* WorldManagement, ak_u32 WorldIndex);
};

#include "include/dev_input.h"
#include "include/world_management.h"
#include "include/ui.h"
#include "include/dev_mesh.h"
#include "include/frame_playback.h"
#include "include/generated_string_templates.h"
#include "include/edit_recordings.h"

enum selector_transform_mode
{
    SELECTOR_TRANSFORM_MODE_TRANSLATE,
    SELECTOR_TRANSFORM_MODE_SCALE,
    SELECTOR_TRANSFORM_MODE_ROTATE
};

struct gizmo_selected_object
{
    ak_bool IsSelected;
    object SelectedObject;
};

enum gizmo_movement_direction
{
    GIZMO_MOVEMENT_DIRECTION_X,
    GIZMO_MOVEMENT_DIRECTION_Y,
    GIZMO_MOVEMENT_DIRECTION_Z,
    GIZMO_MOVEMENT_DIRECTION_XY,
    GIZMO_MOVEMENT_DIRECTION_XZ,
    GIZMO_MOVEMENT_DIRECTION_YZ
};

struct gizmo
{
    dev_mesh* Mesh;
    ak_sqtf Transform;
    ak_v3f IntersectionPlane;
    gizmo_movement_direction MovementDirection;
    ak_bool IsHighLighted;
};

struct gizmo_intersection_result
{
    ak_bool Hit;
    gizmo* Gizmo;
    ak_v3f HitMousePosition;
};

struct euler_transform
{
    ak_sqtf Transform;
    ak_v3f Euler;
};

struct gizmo_state
{
    selector_transform_mode TransformMode;
    gizmo Gizmos[6];    
    gizmo_intersection_result GizmoHit;
    ak_f32 GridDistance;
    ak_f32 ScaleSnap;
    ak_f32 RotationAngleSnap;
    ak_bool ShouldSnap;
    ak_m3f OriginalRotation;
    ak_bool UseLocalTransforms;
    
    euler_transform OriginalTransform;
    gizmo_selected_object SelectedObject;
};

enum render_primitive_type
{
    RENDER_PRIMITIVE_TYPE_POINT,
    RENDER_PRIMITIVE_TYPE_SEGMENT
};

struct render_point
{
    ak_v3f P;
    ak_f32 Size;
    ak_color3f Color;    
};

struct render_segment
{
    ak_v3f P0;
    ak_v3f P1;
    ak_f32 Size;
    ak_color3f Color;
};

struct render_primitive
{
    render_primitive_type Type;
    union
    {
        render_point Point;
        render_segment Segment;
    };
};

struct game_context
{
    game* Game;
    ak_array<ak_string> GameEntityNames[2];
    ak_hash_map<ak_string, ak_u64> GameEntityNameHash[2];
};

enum timed_block_entry
{
    TIMED_BLOCK_ENTRY_Game_Update, 
    TIMED_BLOCK_ENTRY_Game_GravityMovementUpdate, 
    TIMED_BLOCK_ENTRY_GravityUpdate_Iterations, 
    TIMED_BLOCK_ENTRY_CCD,
    TIMED_BLOCK_ENTRY_COUNT
};

struct timed_entry
{
    const ak_char* Name;
    ak_u64 Cycles;
    ak_f64 ElapsedTime;
    ak_u64 Count;
};

struct editor
{
    ak_arena* Scratch;
    dev_input Input;
    
    ak_array<graphics_mesh_id> ImGuiMeshes;
    graphics_render_buffer* RenderBuffers[2];
    
    ui UI;
    world_management WorldManagement;
    gizmo_state GizmoState;
    frame_playback FramePlayback;
    edit_recordings EditRecordings;
    
    dev_capsule_mesh  LineCapsuleMesh;                
    dev_slim_mesh     LineBoxMesh;
    dev_slim_mesh     LineSphereMesh;    
    dev_slim_mesh     TriangleBoxMesh;
    dev_slim_mesh     TriangleSphereMesh;
    dev_slim_mesh     TriangleCylinderMesh;
    dev_slim_mesh     TriangleConeMesh;    
    dev_mesh          TriangleArrowMesh;
    dev_mesh          TriangleCircleMesh;
    dev_mesh          TriangleScaleMesh;
    dev_mesh          TriangleTorusMesh;    
    dev_mesh          TrianglePlaneMesh;
    dev_slim_mesh*    ConvexHullMeshes[MESH_ASSET_COUNT];
    
    ak_u32 CurrentWorldIndex;
    
    game_context GameContext;
    
    ak_v2f ListerDim;
    ak_v2f DetailsDim;
    
    graphics_camera Cameras[2];
    graphics_camera OldCameras[2];
    
    ak_bool ShowCloseButtonInModals;
    
    ak_u32 WorldSelectedIndex;
    
    editor_draw_point* DrawPoint;
    editor_draw_segment* DrawSegment;
    editor_add_entity* AddEntity;
    editor_debug_log* DebugLog;
    
    timed_entry TimedBlockEntries[TIMED_BLOCK_ENTRY_COUNT];
    
    ak_array<render_primitive> RenderPrimitives;
};

void Editor_StopGame(editor* Editor, platform* Platform);
ak_bool Editor_PlayGame(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform, dev_platform* DevPlatform);

#endif //EDITOR_H
