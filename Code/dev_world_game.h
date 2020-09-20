/* Original Author: Armand (JJ) Karambasis */
#ifndef DEV_WORLD_GAME_H
#define DEV_WORLD_GAME_H

#if DEVELOPER_BUILD

#define CAMERA_ANGULAR_DAMPING 10.0f
#define CAMERA_ANGULAR_ACCELERATION 5.0f
#define CAMERA_LINEAR_DAMPING 10.0f     
#define CAMERA_LINEAR_ACCELERATION 7.5f
#define CAMERA_SCROLL_ACCELERATION 300.0f*5
#define CAMERA_SCROLL_DAMPING 7.5f
#define CAMERA_MIN_DISTANCE 0.1f
#define GIZMO_PLANE_DISTANCE 0.4f

global struct dev_context* __Internal_Dev_Context__;
#define SET_DEVELOPER_CONTEXT() __Internal_Dev_Context__ = (dev_context*)DevContext
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations) (__Internal_Dev_Context__->GameInformation.MaxGJKIterations = AK_Max(__Internal_Dev_Context__->GameInformation.MaxGJKIterations, Iterations))
#define DEVELOPER_MAX_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations) (__Internal_Dev_Context__->GameInformation.MaxTimeIterations = AK_Max(__Internal_Dev_Context__->GameInformation.MaxTimeIterations, Iterations))
#define NOT_IN_DEVELOPMENT_MODE() !IsInDevelopmentMode((dev_context*)DevContext)
#define DEBUG_DRAW_POINT(position, color) DebugDrawPoint(__Internal_Dev_Context__, position, color)
#define DEBUG_DRAW_EDGE(position0, position1, color) DebugDrawEdge(__Internal_Dev_Context__, position0, position1, color)
#define DEBUG_DRAW_QUAD(center, normal, dimensions, color) DebugDrawQuad(__Internal_Dev_Context__, center, normal, dimensions, color)
#define DEBUG_LOG(format, ...) DebugLog(__Internal_Dev_Context__, format, __VA_ARGS__)
#define IN_EDIT_MODE() (((dev_context*)DevContext)->EditMode)

#include "imgui/imgui.h"
#include "camera.h"
#include "input.h"
#include "assets/assets.h"
#include "simulation/simulation.h"
#include "entity.h"
#include "dev_frame_recording.h"

enum gizmo_movement_type
{
    GIZMO_MOVEMENT_TYPE_TRANSLATE,
    GIZMO_MOVEMENT_TYPE_SCALE,
    GIZMO_MOVEMENT_TYPE_ROTATE
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
    
    ak_v2i MouseDelta;
    ak_v2i MouseCoordinates;
    ak_f32 Scroll;
};

struct game_information
{
    ak_u64 MaxTimeIterations;
    ak_u64 MaxGJKIterations;
};

struct dev_mesh
{    
    ak_i64 MeshID;
    ak_u32 IndexCount;
    ak_u32 VertexCount;
    ak_u16 IndexOffSet;
    void* Vertices;
    void* Indices;    
};

struct dev_capsule_mesh
{    
    ak_i64 MeshID;
    ak_u32 CapIndexCount;
    ak_u32 CapVertexCount;
    ak_u32 BodyIndexCount;    
};

struct debug_point
{
    ak_v3f P;
    ak_color3f Color;
};

struct debug_edge
{
    ak_v3f P0;
    ak_v3f P1;
    ak_color3f Color;
};

struct debug_quad
{
    ak_v3f CenterP;
    ak_v3f Normal;
    ak_v2f Dim;
    ak_color3f Color;
};

struct gizmo
{
    dev_mesh* Mesh;
    ak_sqtf Transform;
    ak_v3f IntersectionPlane;
    gizmo_movement_direction MovementDirection;
};

struct gizmo_hit
{
    ak_v3f HitMousePosition;
    gizmo* Gizmo;
};

enum debug_primitive_type
{
    DEBUG_PRIMITIVE_TYPE_POINT,
    DEBUG_PRIMITIVE_TYPE_EDGE,
    DEBUG_PRIMITIVE_TYPE_QUAD
};

struct debug_primitive
{
    debug_primitive_type Type;
    union
    {
        debug_point Point;
        debug_edge  Edge;
        debug_quad  Quad;
    };
};

enum view_mode_type
{
    VIEW_MODE_TYPE_LIT,
    VIEW_MODE_TYPE_UNLIT,
    VIEW_MODE_TYPE_WIREFRAME,
    VIEW_MODE_TYPE_WIREFRAME_ON_LIT
};

struct mesh_convex_hull_gdi
{
    ak_u32 Count;
    graphics_mesh_id* Meshes;
};

enum frame_playback_state
{
    FRAME_PLAYBACK_STATE_NONE,
    FRAME_PLAYBACK_STATE_RECORDING,
    FRAME_PLAYBACK_STATE_PLAYING,
    FRAME_PLAYBACK_STATE_INSPECT_FRAMES
};

struct frame_playback
{
    ak_u32 MaxRecordingPathLength;
    ak_char* RecordingPath;
    frame_playback_state PlaybackState;    
    ak_u32 CurrentFrameIndex;    
    frame_recording Recording;    
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

#define MAX_IMGUI_MESHES 32
struct dev_context
{
    struct game* Game;
    graphics* Graphics;
    
    ak_arena* DevStorage;
    ak_bool InDevelopmentMode;    
    ak_bool UseDevCamera;        
    ak_bool DrawOtherWorld;
    ak_bool DrawColliders;
    view_mode_type ViewModeType;    
    
    ak_bool DrawGrid;
    ak_bool EditMode;
    ak_bool IsGizmoHit;
    
    entity_spawner EntitySpawner;
    
    ak_f32 tInterpolated;
    graphics_render_buffer* RenderBuffer;
    
    frame_playback FramePlayback;
    
    game_information GameInformation;
    
    dev_input Input;    
    camera Cameras[2];
    
    ak_u32 ImGuiMeshCount;
    ak_i64 ImGuiMeshes[MAX_IMGUI_MESHES];    
    
    dev_capsule_mesh LineCapsuleMesh;
    
    dev_mesh PlaneMesh;
    dev_mesh LineBoxMesh;
    dev_mesh LineSphereMesh;    
    dev_mesh TriangleBoxMesh;
    dev_mesh TriangleSphereMesh;
    dev_mesh TriangleCylinderMesh;
    dev_mesh TriangleConeMesh;
    dev_mesh TriangleArrowMesh;
    dev_mesh TriangleCircleMesh;
    dev_mesh TriangleScaleMesh;
    dev_mesh TriangleTorusMesh;
    
    ak_array<debug_primitive> DebugPrimitives;
    
    mesh_convex_hull_gdi MeshConvexHulls[MESH_ASSET_COUNT];
    
    ak_arena* LogStorage;
    ak_array<ak_string> Logs;
    
    void** PlatformData;
    ak_bool Initialized;
    entity_id SelectedObjectID;    
    ak_v3f InspectRay;
    ak_char DebugMessage[100];
    
    ak_array<ak_v3f> EntityRotations[2];
    gizmo Gizmo[6];
    gizmo_hit GizmoHit;
    gizmo_movement_type TransformationMode;
    ak_f32 GridDistance;
    ak_f32 ScaleSnapTo;
    ak_f32 RotationSnapTo;
};

void Platform_InitImGui(void* PlatformData);
void Platform_DevUpdate(void* PlatformData, ak_v2i RenderDim, ak_f32 dt, dev_context* DevContext);
ak_string Platform_FindNewFrameRecordingPath();

inline ak_bool IsInDevelopmentMode(dev_context* Context)
{
    if(Context)
        return Context->InDevelopmentMode;    
    return false;
}

inline void DebugDrawPoint(dev_context* DevContext, ak_v3f P, ak_color3f Color)
{    
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_POINT};
    Primitive.Point = {P, Color};    
    DevContext->DebugPrimitives.Add(Primitive);
}

inline void DebugDrawEdge(dev_context* DevContext, ak_v3f P0, ak_v3f P1, ak_color3f Color)
{   
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_EDGE};
    Primitive.Edge = {P0, P1, Color};
    DevContext->DebugPrimitives.Add(Primitive);    
}

inline void DebugDrawQuad(dev_context* DevContext, ak_v3f CenterP, ak_v3f Normal, ak_v2f Dim, ak_color3f Color)
{    
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_QUAD};
    Primitive.Quad = {CenterP, Normal, Dim, Color};
    DevContext->DebugPrimitives.Add(Primitive);    
}

inline void DebugLog(dev_context* DevContext, ak_char* Format, ...)
{    
    va_list Args;
    va_start(Args, Format);
    DevContext->Logs.Add(AK_FormatString(DevContext->LogStorage, Format, Args));    
    va_end(Args);    
}

inline ak_u32 
ConvexHullIndexCount(convex_hull* Hull)
{
    ak_u32 Result = Hull->Header.FaceCount*3*2;
    return Result;
}

inline mesh
GetMeshFromDevMesh(dev_mesh DevMesh)
{
    mesh Mesh;
    Mesh.Vertices = DevMesh.Vertices;
    Mesh.Indices = DevMesh.Indices;

    return Mesh;
}

inline mesh_info
GetMeshInfoFromDevMesh(dev_mesh DevMesh)
{
    mesh_info MeshInfo;
    MeshInfo.Header.VertexCount = DevMesh.VertexCount;
    MeshInfo.Header.IndexCount = DevMesh.IndexCount;
    MeshInfo.Header.IsIndexFormat32 = false;
    MeshInfo.Header.IsSkeletalMesh = false;

    return MeshInfo;
}

#else

#define SET_DEVELOPER_CONTEXT(context)
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations)
#define DEVELOPER_MAX_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations)
#define NOT_IN_DEVELOPMENT_MODE() true
#define DEBUG_DRAW_POINT(position, color)
#define DEBUG_DRAW_EDGE(position0, position1, color)
#define DEBUG_DRAW_DIRECTION_VECTOR(origin, direction, color)
#define DEBUG_DRAW_QUAD(center, normal, dimensions, color)
#define IN_EDIT_MODE() false

#endif

#endif
