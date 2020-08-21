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

global struct dev_context* __Internal_Dev_Context__;
#define SET_DEVELOPER_CONTEXT(context) __Internal_Dev_Context__ = (dev_context*)context
#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations) (__Internal_Dev_Context__->GameInformation.MaxGJKIterations = MaximumU64(__Internal_Dev_Context__->GameInformation.MaxGJKIterations, Iterations))
#define DEVELOPER_MAX_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations) (__Internal_Dev_Context__->GameInformation.MaxTimeIterations = MaximumU64(__Internal_Dev_Context__->GameInformation.MaxTimeIterations, Iterations))
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
#include "dev_frame_recording.h"

struct dev_input
{
    union
    {
        button Buttons[4];
        struct
        {
            button ToggleDevState;
            button Alt;
            button LMB;
            button MMB;            
        };
    };
    v2i MouseDelta;
    v2i MouseCoordinates;
    f32 Scroll;
};

struct game_information
{
    u64 MaxTimeIterations;
    u64 MaxGJKIterations;
};

struct dev_mesh
{
    i64 MeshID;
    u32 IndexCount;
};

struct dev_capsule_mesh
{    
    i64 MeshID;
    u32 CapIndexCount;
    u32 CapVertexCount;
    u32 BodyIndexCount;    
};

struct debug_point
{
    v3f P;
    c3 Color;
};

struct debug_edge
{
    v3f P0;
    v3f P1;
    c3 Color;
};

struct debug_quad
{
    v3f CenterP;
    v3f Normal;
    v2f Dim;
    c3 Color;
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
    u32 Count;
    graphics_mesh_id* Meshes;
};

#define MAX_IMGUI_MESHES 32
struct dev_context
{
    struct game* Game;
    graphics* Graphics;
    
    arena DevStorage;
    b32 InDevelopmentMode;    
    b32 UseDevCamera;        
    b32 DrawOtherWorld;
    b32 DrawColliders;
    view_mode_type ViewModeType;    
    b32 DrawGrid;
    b32 EditMode;
    
    graphics_render_buffer* RenderBuffer;
    
    frame_recording FrameRecording;
    
    game_information GameInformation;
    
    dev_input Input;    
    camera Cameras[2];
    
    u32 ImGuiMeshCount;
    i64 ImGuiMeshes[MAX_IMGUI_MESHES];    
    
    dev_capsule_mesh LineCapsuleMesh;
    
    dev_mesh LineBoxMesh;
    dev_mesh LineSphereMesh;    
    dev_mesh TriangleBoxMesh;
    dev_mesh TriangleSphereMesh;
    dev_mesh TriangleCylinderMesh;
    dev_mesh TriangleConeMesh;
    dev_mesh TriangleArrowMesh;
        
    dynamic_array<debug_primitive> DebugPrimitives;
    
    mesh_convex_hull_gdi MeshConvexHulls[MESH_ASSET_COUNT];
    
    arena LogStorage;
    dynamic_array<string> Logs;
    
    void** PlatformData;
    b32 Initialized;
    struct world_entity* SelectedObject;
    v3f InspectRay;
    char DebugMessage[100];
    
    v3f* EntityRotations[2];
};

void Platform_InitImGui(void* PlatformData);
void Platform_DevUpdate(void* PlatformData, v2i RenderDim, f32 dt, dev_context* DevContext);
string Platform_OpenFileDialog(char* Extension);
string Platform_FindNewFrameRecordingPath();

inline b32 IsInDevelopmentMode(dev_context* Context)
{
    if(Context)
        return Context->InDevelopmentMode;    
    return false;
}

inline void CheckPrimitivesAreAllocated(dev_context* DevContext)
{
    if(!IsInitialized(&DevContext->DebugPrimitives))
        DevContext->DebugPrimitives = CreateDynamicArray<debug_primitive>(2048);
}

inline void DebugDrawPoint(dev_context* DevContext, v3f P, c3 Color)
{
    CheckPrimitivesAreAllocated(DevContext);
    
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_POINT};
    Primitive.Point = {P, Color};    
    Append(&DevContext->DebugPrimitives, Primitive);
}

inline void DebugDrawEdge(dev_context* DevContext, v3f P0, v3f P1, c3 Color)
{
    CheckPrimitivesAreAllocated(DevContext);
    
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_EDGE};
    Primitive.Edge = {P0, P1, Color};
    Append(&DevContext->DebugPrimitives, Primitive);
}

inline void DebugDrawQuad(dev_context* DevContext, v3f CenterP, v3f Normal, v2f Dim, c3 Color)
{
    CheckPrimitivesAreAllocated(DevContext);
    
    debug_primitive Primitive = {DEBUG_PRIMITIVE_TYPE_QUAD};
    Primitive.Quad = {CenterP, Normal, Dim, Color};
    Append(&DevContext->DebugPrimitives, Primitive);
}

inline void DebugLog(dev_context* DevContext, char* Format, ...)
{
    if(!IsInitialized(&DevContext->Logs))
        DevContext->Logs = CreateDynamicArray<string>(16);    
           
    va_list Args;
    va_start(Args, Format);
    Append(&DevContext->Logs, FormatString(Format, Args, &DevContext->LogStorage));    
    va_end(Args);
    CONSOLE_LOG(DevContext->Logs[DevContext->Logs.Size-1].Data);
}

inline u32 
ConvexHullIndexCount(convex_hull* Hull)
{
    u32 Result = Hull->Header.FaceCount*3*2;
    return Result;
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
