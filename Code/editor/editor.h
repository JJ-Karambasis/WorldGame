#ifndef EDITOR_H
#define EDITOR_H

#include <game.h>
#include <imgui.h>

#include "include/dev_input.h"

struct dev_platform;
struct editor;

#define EDITOR_RUN(name) ak_i32 name(graphics* Graphics, platform* Platform, dev_platform* DevPlatform, ImGuiContext* Context)
typedef EDITOR_RUN(editor_run);

#define DEV_PLATFORM_UPDATE(name) ak_bool name(editor* Editor, game* Game, ak_f32 dt)
typedef DEV_PLATFORM_UPDATE(dev_platform_update);

#define DEV_BUILD_WORLD(name) ak_bool name(ak_string WorldName)
typedef DEV_BUILD_WORLD(dev_build_world);

struct dev_platform
{
    dev_platform_update* Update;
    dev_build_world* BuildWorld;
};

#define MAX_OBJECT_NAME_LENGTH 128

struct dev_entity
{
    ak_char Name[MAX_OBJECT_NAME_LENGTH];
    entity_type Type;
    ak_u64 ID;
    ak_u64 LinkID;
    ak_sqtf Transform;
    material Material;
    mesh_asset_id MeshID;
};

struct dual_dev_entity
{
    dev_entity* EntityA;
    dev_entity* EntityB;
};

struct dev_point_light
{
    ak_char Name[MAX_OBJECT_NAME_LENGTH];
    graphics_point_light Light;
};

struct dual_dev_point_light
{
    dev_point_light* PointLightA;
    dev_point_light* PointLightB;
};

struct material_context
{
    ak_bool DiffuseIsTexture;
    texture_asset_id DiffuseID;
    ak_color3f Diffuse;
    ak_bool SpecularInUse;
    ak_bool SpecularIsTexture;
    texture_asset_id SpecularID;
    ak_f32 Specular;
    ak_i32 Shininess;
    ak_bool NormalInUse;
    texture_asset_id NormalID;
};

struct entity_spawner
{
    ak_bool Init;
    ak_char Name[MAX_OBJECT_NAME_LENGTH];
    entity_type EntityType;
    ak_v3f Translation;
    ak_v3f Scale;
    ak_f32 Radius;
    ak_f32 Restitution;
    ak_v3f Axis;
    ak_f32 Angle;
    ak_u32 WorldIndex;    
    mesh_asset_id MeshID;
    material_context MaterialContext;
    ak_f32 Mass;
};

struct light_spawner
{   
    ak_bool Init;    
    ak_char Name[MAX_OBJECT_NAME_LENGTH];
    ak_v3f Translation;
    ak_f32 Radius;
    ak_u32 WorldIndex;
    ak_f32 Intensity;
    ak_color3f Color;
};

#define MAX_WORLD_NAME 128

struct world_context
{
    ak_bool WorldLoaded;
    ak_string WorldPath;
    ak_string WorldName;
};

struct editor
{
    ak_arena* Scratch;
    dev_input Input;
    
    ak_array<graphics_mesh_id> ImGuiMeshes;
    graphics_render_buffer* RenderBuffer;
    
    entity_spawner EntitySpawner;
    light_spawner  LightSpawner;
    
    ak_bool ShowEmptyWorldNameErrorText;
    ak_bool ShowDuplicateWorldErrorText;
    
    ak_bool ShowEntityNameErrorText;
    ak_bool ShowEntityNameNullErrorText;
    ak_hash_map<ak_char*, ak_bool> EntityNameCollisionMap[2]; 
    
    ak_bool ShowLightNameErrorText;
    ak_bool ShowLightNameNullErrorText;
    ak_hash_map<ak_char*, ak_bool> LightNameCollisionMap[2];
    
    ak_pool<dev_entity> DevEntities[2];
    ak_pool<dev_point_light> DevPointLights[2];
    
    ak_hash_map<ak_char*, ak_u32> EntityIndices[2];
    ak_hash_map<ak_char*, ak_u32> PointLightIndices[2];
    
    ak_u32 CurrentWorldIndex;
    ak_f32 ListerWidth;
    
    graphics_camera Cameras[2];
    
    ak_bool ShowCloseButtonInModals;
    world_context WorldContext;
    
    ak_bool IsGamePlaying;
};

#define EDITOR_CREATE_NEW_WORLD_MODAL(name) void name(editor* Editor, dev_platform* DevPlatform, ak_bool HasCloseButton)
typedef EDITOR_CREATE_NEW_WORLD_MODAL(editor_create_new_world_modal);
EDITOR_CREATE_NEW_WORLD_MODAL(CreateNewWorldModal_Stub) {}

#endif //EDITOR_H
