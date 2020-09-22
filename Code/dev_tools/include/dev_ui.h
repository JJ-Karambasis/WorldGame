#ifndef DEV_UI_H
#define DEV_UI_H

#include "imgui/imgui.h"

enum view_mode_type
{
    VIEW_MODE_TYPE_LIT,
    VIEW_MODE_TYPE_UNLIT,
    VIEW_MODE_TYPE_WIREFRAME,
    VIEW_MODE_TYPE_WIREFRAME_ON_LIT
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

struct light_spawner
{   
    ak_bool Init;    
    ak_v3f Translation;
    ak_f32 Radius;
    ak_u32 WorldIndex;
    ak_f32 Intensity;
    ak_color3f Color;
};

struct dev_ui
{
    graphics_render_buffer* UIRenderBuffer;
    ak_arena* LogArena;
    ak_array<graphics_mesh_id> ImGuiMeshes;
    ak_array<ak_string> Logs;        
    entity_spawner EntitySpawner;
    light_spawner LightSpawner;
    
    ak_bool DevToolsOpen;
    ak_bool EntitySpawnerOpen;
    ak_bool LightSpawnerOpen;
    ak_bool PlayGame;            
    view_mode_type ViewModeType;
};

void DevUI_Initialize(dev_ui* UI, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui);
void DevUI_Update(dev_context* DevContext, dev_ui* UI);
void DevUI_Render(graphics* Graphics, dev_ui* UI, graphics_render_buffer* MergeRenderBuffer);

#endif