#ifndef EDITOR_H
#define EDITOR_H

#include <game.h>
#include <imgui.h>

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

#define GIZMO_PLANE_DISTANCE 0.4f
#define POINT_LIGHT_RADIUS 0.1f

#include "include/dev_input.h"
#include "include/ui.h"
#include "include/world_management.h"
#include "include/dev_mesh.h"
#include "include/gizmo.h"
#include "include/frame_playback.h"

#define EDITOR_RUN(name) ak_i32 name(graphics* Graphics, platform* Platform, dev_platform* DevPlatform, ImGuiContext* Context)
typedef EDITOR_RUN(editor_run);

#define DEV_PLATFORM_UPDATE(name) ak_bool name(editor* Editor, ak_f32 dt)
typedef DEV_PLATFORM_UPDATE(dev_platform_update);

#define DEV_BUILD_WORLD(name) ak_bool name(ak_string WorldName)
typedef DEV_BUILD_WORLD(dev_build_world);

#define DEV_DELETE_WORLD_FILES(name) void name(ak_string WorldName)
typedef DEV_DELETE_WORLD_FILES(dev_delete_world_files);

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
    dev_delete_world_files* DeleteWorldFiles;
};

#define MAX_WORLD_NAME 256
#define MAX_MESH_NAME 256

enum selected_object_type
{
    SELECTED_OBJECT_TYPE_ENTITY, 
    SELECTED_OBJECT_TYPE_LIGHT
};

enum selector_transform_mode
{
    SELECTOR_TRANSFORM_MODE_TRANSLATE,
    SELECTOR_TRANSFORM_MODE_SCALE,
    SELECTOR_TRANSFORM_MODE_ROTATE
};

struct selected_object
{
    selected_object_type Type;
    ak_u64 ID;
    
    dev_entity* GetEntity(world_management* WorldManagement, ak_u32 WorldIndex);
    dev_point_light* GetPointLight(world_management* WorldManagement, ak_u32 WorldIndex);
    ak_v3f GetPosition(world_management* WorldManagement, ak_u32 WorldIndex);
};

struct gizmo_selected_object
{
    ak_bool IsSelected;
    selected_object SelectedObject;
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
    
    gizmo_selected_object SelectedObject;
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
    
    game* Game;
    ak_array<ak_char[MAX_OBJECT_NAME_LENGTH]> GameEntityNames[2];
    ak_hash_map<ak_char*, ak_u64> GameEntityNameHash[2];
    
    ak_u32 CurrentWorldIndex;
    
    ak_v2f ListerDim;
    ak_v2f DetailsDim;
    
    graphics_camera Cameras[2];
    graphics_camera OldCameras[2];
    
    ak_bool ShowCloseButtonInModals;
    
    ak_u32 WorldSelectedIndex;
};

#define EDITOR_CREATE_NEW_WORLD_MODAL(name) void name(editor* Editor, dev_platform* DevPlatform, \
assets* Assets, \
ak_bool HasCloseButton,\
ak_bool HasLoadWorldButton)
typedef EDITOR_CREATE_NEW_WORLD_MODAL(editor_create_new_world_modal);
EDITOR_CREATE_NEW_WORLD_MODAL(CreateNewWorldModal_Stub) {}
EDITOR_CREATE_NEW_WORLD_MODAL(CreateNewWorldModal);

#define EDITOR_LOAD_WORLD_MODAL(name) void name(editor* Editor, dev_platform* DevPlatform, \
assets* Assets, \
ak_bool HasCloseButton)

typedef EDITOR_LOAD_WORLD_MODAL(editor_load_world_modal);
EDITOR_LOAD_WORLD_MODAL(LoadWorldModal_Stub) {}
EDITOR_LOAD_WORLD_MODAL(LoadWorldModal);

#define EDITOR_DELETE_WORLD_MODAL(name) void name(editor* Editor, dev_platform* DevPlatform)
typedef EDITOR_DELETE_WORLD_MODAL(editor_delete_world_modal);
EDITOR_DELETE_WORLD_MODAL(DeleteWorldModal_Stub) {}
EDITOR_DELETE_WORLD_MODAL(DeleteWorldModal);

selected_object* Editor_GetSelectedObject(editor* Editor);
ak_quatf Editor_GetOrientationDiff(ak_m3f OriginalRotation, ak_v3f SelectorDiff);
void Editor_StopGame(editor* Editor, platform* Platform);
ak_bool Editor_PlayGame(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform);

#endif //EDITOR_H
