#ifndef DEV_CONTEXT_H
#define DEV_CONTEXT_H

#define DEV_GIZMO_PLANE_DISTANCE 0.4f
#define DEV_POINT_LIGHT_RADIUS 0.1f

struct dev_input
{
    union
    {
        button Buttons[9];
        struct
        {
            button ToggleDevState;
            button Alt;
            button LMB;
            button MMB;
            button W;
            button E;
            button R;            
            button Delete;
            button Ctl;
        };
    };
    
    ak_v2i LastMouseCoordinates;
    ak_v2i MouseCoordinates;
    ak_f32 Scroll;
};

struct dev_mesh
{    
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
    ak_u32 VertexCount;
    ak_u16 IndexOffSet;
    void* Vertices;
    void* Indices;    
};

struct dev_slim_mesh
{
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
};

struct dev_capsule_mesh
{    
    graphics_mesh_id MeshID;
    ak_u32 CapIndexCount;
    ak_u32 CapVertexCount;
    ak_u32 BodyIndexCount;    
};

struct dev_entity
{
    entity_type Type;
    world_id ID;
    world_id LinkID;
    ak_sqtf Transform;
    mesh_asset_id MeshID;
    material Material;
};

struct dev_point_light : public point_light
{
    world_id ID;
};

struct dev_camera
{    
    ak_v3f   Target;
    ak_v3f   SphericalCoordinates;    
};

enum dev_selected_object_type
{
    DEV_SELECTED_OBJECT_TYPE_NONE,
    DEV_SELECTED_OBJECT_TYPE_ENTITY,
    DEV_SELECTED_OBJECT_TYPE_PLAYER_CAPSULE,
    DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT
};

struct dev_selected_object
{
    dev_selected_object_type Type;
    union
    {
        capsule* PlayerCapsule;
        world_id PointLightID;
        
        struct
        {
            world_id EntityID;
            material_context MaterialContext;
        };
    };
};

enum dev_gizmo_movement_type
{
    DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE,
    DEV_GIZMO_MOVEMENT_TYPE_SCALE,
    DEV_GIZMO_MOVEMENT_TYPE_ROTATE
};

enum dev_gizmo_movement_direction
{
    DEV_GIZMO_MOVEMENT_DIRECTION_X,
    DEV_GIZMO_MOVEMENT_DIRECTION_Y,
    DEV_GIZMO_MOVEMENT_DIRECTION_Z,
    DEV_GIZMO_MOVEMENT_DIRECTION_XY,
    DEV_GIZMO_MOVEMENT_DIRECTION_XZ,
    DEV_GIZMO_MOVEMENT_DIRECTION_YZ
};

struct dev_gizmo
{
    dev_mesh* Mesh;
    ak_sqtf Transform;
    ak_v3f IntersectionPlane;
    dev_gizmo_movement_direction MovementDirection;
};

struct dev_gizmo_state
{
    dev_gizmo_movement_type TransformMode;
    dev_gizmo Gizmos[6];    
    ak_f32 GridDistance;
    ak_f32 ScaleSnap;
    ak_f32 RotationAngleSnap;
    ak_bool ShouldSnap;
};

struct dev_context
{
    ak_arena* DevStorage;
    game* Game;
    graphics* Graphics;
    void* PlatformWindow;
    platform_development_update* PlatformUpdate;
    
    dev_input DevInput;
    dev_ui DevUI;
    
    dev_capsule_mesh LineCapsuleMesh;                
    dev_slim_mesh    LineBoxMesh;
    dev_slim_mesh    LineSphereMesh;    
    dev_slim_mesh    TriangleBoxMesh;
    dev_slim_mesh    TriangleSphereMesh;
    dev_slim_mesh    TriangleCylinderMesh;
    dev_slim_mesh    TriangleConeMesh;    
    dev_mesh         TriangleArrowMesh;
    dev_mesh         TriangleCircleMesh;
    dev_mesh         TriangleScaleMesh;
    dev_mesh         TriangleTorusMesh;    
    dev_mesh         TrianglePlaneMesh;    
    
    graphics_render_buffer* DevRenderBuffer;
    dev_camera DevCameras[2];
    capsule InitialPlayerCapsules[2];
    ak_pool<dev_entity> InitialEntityStorage[2];    
    ak_pool<dev_point_light> InitialPointLights[2];    
    ak_array<ak_v3f> InitialRotations[2];
    
    dev_selected_object SelectedObject;
    dev_gizmo_state GizmoState;
};

void DevContext_Initialize(game* Game, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui, platform_development_update* PlatformUpdate);
void DevContext_DebugLog(const ak_char* Format, ...);

#endif