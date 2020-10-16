#ifndef DEV_CONTEXT_H
#define DEV_CONTEXT_H

#define DEV_GIZMO_PLANE_DISTANCE 0.4f
#define DEV_POINT_LIGHT_RADIUS 0.1f
#define DEV_CAMERA_MIN_DISTANCE 0.1f

struct dev_input
{
    union
    {
        button Buttons[13];
        struct
        {
            button ToggleDevState;            
            button LMB;
            button MMB;
            button W;
            button E;
            button R;  
            button S;
            button L;
            button Z;
            button Y;
            button Delete;
            button Ctrl;
            button Alt;
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

enum dev_selected_object_type
{
    DEV_SELECTED_OBJECT_TYPE_NONE,
    DEV_SELECTED_OBJECT_TYPE_ENTITY,    
    DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD, 
    DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT
};

struct dev_selected_object
{
    dev_selected_object_type Type;
    union
    {        
        world_id PointLightID;        
        struct
        {
            world_id EntityID;
            material_context MaterialContext;
        };
        dual_world_id JumpingQuadIDs;
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

enum dev_object_edit_type
{
    DEV_OBJECT_EDIT_TYPE_TRANSFORM,
    DEV_OBJECT_EDIT_TYPE_DELETE,
    DEV_OBJECT_EDIT_TYPE_CREATE
};

struct dev_gizmo
{
    dev_mesh* Mesh;
    ak_sqtf Transform;
    ak_v3f IntersectionPlane;
    dev_gizmo_movement_direction MovementDirection;
};

struct gizmo_intersection_result
{
    ak_bool Hit;
    dev_gizmo* Gizmo;
    ak_v3f HitMousePosition;
};

struct dev_gizmo_state
{
    dev_gizmo_movement_type TransformMode;
    dev_gizmo Gizmos[6];    
    gizmo_intersection_result GizmoHit;
    ak_f32 GridDistance;
    ak_f32 ScaleSnap;
    ak_f32 RotationAngleSnap;
    ak_bool ShouldSnap;
};

struct dev_transform
{
    ak_v3f Translation;    
    ak_v3f Scale;    
    ak_v3f Euler;
};

struct dev_loaded_world
{
    ak_string LoadedWorldFile;    
};

struct dev_object_edit
{
    dev_object_edit_type ObjectEditType;
    entity Entity;
    material Material;
    dev_transform Transform;
    mesh_asset_id MeshID;
    ak_f32 Mass;
    ak_f32 Restitution;
};

struct dev_context
{
    ak_arena* DevStorage;        
    void* PlatformWindow;
    game* Game;
    graphics* Graphics;
    platform_development_update* PlatformUpdate;
    
    dev_input DevInput;
    dev_ui DevUI;
    
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
    
    camera Cameras[2];    
    ak_array<dev_transform> InitialTransforms[2];
    
    ak_string DefaultWorldFilePathName;
    
    dev_selected_object SelectedObject;
    dev_gizmo_state GizmoState;
    dev_loaded_world LoadedWorld;

    ak_array<dev_object_edit> UndoStack;
    ak_array<dev_object_edit> RedoStack;
};

void DevContext_Initialize(game* Game, graphics* Graphics, void* PlatformWindow, ak_string ProgramFilePath, platform_init_imgui* InitImGui, platform_development_update* PlatformUpdate);
void DevContext_DebugLog(const ak_char* Format, ...);
mesh_info DevContext_GetMeshInfoFromDevMesh(dev_mesh* DevMesh);
mesh DevContext_GetMeshFromDevMesh(dev_mesh* DevMesh);
#endif