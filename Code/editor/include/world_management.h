#ifndef WORLD_MANAGEMENT_H
#define WORLD_MANAGEMENT_H

struct dev_entity
{
    ak_string Name;
    ak_string LinkName;
    entity_type Type;
    ak_sqtf Transform;
    material Material;
    mesh_asset_id MeshID;
    ak_bool IsToggled;
};

struct dual_dev_entity
{
    dev_entity* EntityA;
    dev_entity* EntityB;
};

struct dev_point_light
{
    ak_string Name;
    graphics_point_light Light;
};

struct dual_dev_point_light
{
    dev_point_light* PointLightA;
    dev_point_light* PointLightB;
};

enum world_management_state 
{
    WORLD_MANAGEMENT_STATE_NONE,
    WORLD_MANAGEMENT_STATE_CREATE,
    WORLD_MANAGEMENT_STATE_LOAD,
    WORLD_MANAGEMENT_STATE_SAVE,
    WORLD_MANAGEMENT_STATE_DELETE
};

struct world_management
{
    ak_arena* StringArena;
    
    world_management_state OldState;
    world_management_state NewState;
    
    ak_string CurrentWorldName;
    ak_string CurrentWorldPath;
    
    ak_pool<dev_entity> DevEntities[2];
    ak_pool<dev_point_light> DevPointLights[2];
    
    ak_hash_map<ak_string, ak_u32> EntityIndices[2];
    ak_hash_map<ak_string, ak_u32> PointLightIndices[2];
    
    ak_hash_map<ak_string, ak_u64> EntityTables[2];
    ak_hash_map<ak_string, ak_u64> PointLightTables[2];
    
    dev_entity* AllocateDevEntity(ak_u32 WorldIndex);
    dev_point_light* AllocateDevPointLight(ak_u32 WorldIndex);
    
    dev_entity* CreateDevEntity(ak_u32 WorldIndex, ak_char* Name, entity_type Type, ak_v3f Position, 
                                ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID);
    
    dual_dev_entity CreateDevEntityInBothWorlds(ak_char* Name, entity_type Type, ak_v3f Position, 
                                                ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID);
    
    dev_point_light* CreateDevPointLight(ak_u32 WorldIndex, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity);
    
    dual_dev_point_light CreateDevPointLightInBothWorlds(ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity);
    
    dev_entity* CopyDevEntity(dev_entity* CopyDevEntity, ak_u32 WorldIndex);
    dev_point_light* CopyDevPointLight(dev_point_light* CopyPointLight, ak_u32 WorldIndex);
    
    void DeleteDevEntity(ak_u32 WorldIndex, ak_string Name, ak_bool ProcessLink=true);
    void DeleteDevPointLight(ak_u32 WorldIndex, ak_string Name);
    
    void SetState(world_management_state State);    void Update(editor* Editor, platform* Platform, dev_platform* DevPlatform, assets* Assets);
    
    ak_string BuildWorld(ak_arena* Scratch, dev_platform* DevPlatform, assets* Assets);
    ak_bool LoadWorld(ak_string WorldName, editor* Editor, assets* Assets, dev_platform* DevPlatform);
    
    ak_bool CreateWorld(ak_string WorldName, editor* Editor, dev_platform* DevPlatform, assets* Assets, platform* Platform);
    void DeleteWorld(ak_arena* Scratch, ak_string WorldName);
    
    dev_entity* DuplicateEntity(ak_arena* Scratch, dev_entity* Entity, ak_u32 WorldIndex);
    dev_point_light* DuplicatePointLight(ak_arena* Scratch, dev_point_light* PointLight, ak_u32 WorldIndex);
    void DeleteAll();
    void DeleteIndices();
};

#endif
