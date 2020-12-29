#ifndef UI_H
#define UI_H

global ak_u32 StaticEntity_HighestIndex;
global const char* StaticEntity_DefaultName = "Static_%d";

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

struct ui
{
    entity_spawner EntitySpawner;
    light_spawner LightSpawner;
    ak_bool ShowLightNameErrorText;
    ak_bool ShowLightNameNullErrorText;
    ak_bool ShowEntityNameErrorText;
    ak_bool ShowEntityNameNullErrorText;
    
    ak_bool GameDrawColliders;
    ak_bool GameUseDevCamera;
    ak_bool EditorDrawColliders;
    
    ak_u32 EditorGridSizeIndex;
    ak_u32 EditorScaleSnapIndex;
    ak_u32 EditorRotateSnapIndex;
    
    ak_bool EditorDrawGrid;
    ak_bool EditorDrawOtherWorld;
};


global ak_arena* Internal__LogArena;
global ak_array<ak_string> Internal__Logs;

#endif
