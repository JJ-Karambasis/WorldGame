#ifndef DEV_UI_H
#define DEV_UI_H

#include "imgui/imgui.h"

enum gizmo_movement_type
{
    GIZMO_MOVEMENT_TYPE_TRANSLATE,
    GIZMO_MOVEMENT_TYPE_SCALE,
    GIZMO_MOVEMENT_TYPE_ROTATE
};

enum view_mode_type
{
    VIEW_MODE_TYPE_LIT,
    VIEW_MODE_TYPE_UNLIT,
    VIEW_MODE_TYPE_WIREFRAME,
    VIEW_MODE_TYPE_WIREFRAME_ON_LIT
};

struct dev_ui_transform_state
{
    gizmo_movement_type TransformMode;
    ak_f32 GridDistance;
    ak_f32 ScaleSnap;
    ak_f32 RotationAngleSnap;
};

struct entity_spawner
{
    ak_bool Init;
    entity_type EntityType;
    ak_v3f Translation;
    ak_v3f Scale;
    ak_f32 Radius;
    ak_f32 Restitution;
    ak_v3f Axis;
    ak_f32 Angle;
    ak_u32 WorldIndex;    
    mesh_asset_id MeshID;
    material Material;
    ak_f32 Mass;
};

struct dev_ui
{
    graphics_render_buffer* UIRenderBuffer;
    ak_arena* LogArena;
    ak_array<graphics_mesh_id> ImGuiMeshes;
    ak_array<ak_string> Logs;        
    entity_spawner Spawner;
    
    ak_bool DevToolsOpen;
    ak_bool EntitySpawnerOpen;
    ak_bool PlayGame;        
    dev_ui_transform_state TransformState;
    view_mode_type ViewModeType;
};

void DevUI_Initialize(dev_ui* UI, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui);
void DevUI_Update(dev_ui* UI, game* Game);
void DevUI_Render(graphics* Graphics, game* Game, dev_ui* UI);

#endif