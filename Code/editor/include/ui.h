#ifndef UI_H
#define UI_H

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
    ak_quatf Orientation;
    ak_f32 Radius;
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

enum render_mode_type
{
    RENDER_MODE_TYPE_LIT,
    RENDER_MODE_TYPE_UNLIT,
    RENDER_MODE_TYPE_WIREFRAME,
    RENDER_MODE_TYPE_LIT_WIREFRAME,
    RENDER_MODE_TYPE_COUNT
};

enum view_mode_type
{
    VIEW_MODE_TYPE_PERSPECTIVE, 
    VIEW_MODE_TYPE_TOP, 
    VIEW_MODE_TYPE_BOTTOM, 
    VIEW_MODE_TYPE_LEFT, 
    VIEW_MODE_TYPE_RIGHT, 
    VIEW_MODE_TYPE_NEAR, 
    VIEW_MODE_TYPE_FAR
};

struct temp_object
{
    object_type Type;
    union
    {
        dev_entity Entity;				
        dev_point_light PointLight;
    };
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
    ak_bool EditorOverlayOtherWorld;
    
    ak_bool RenameModalState;
    
    ak_bool EntitySpawnerOpen;
    ak_bool LightSpawnerOpen;
    
    render_mode_type RenderModeType;
    view_mode_type ViewModeType;
    
    temp_object TempObject;
    material_context TempContext;
};


global ak_arena* Internal__LogArena;
global ak_array<ak_string> Internal__Logs;

#endif
