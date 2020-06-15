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
#define DEVELOPER_MUTE __Internal_Dev_Context__->Mute

#define DEBUG_DRAW_POINT(position, color) \
if(!IsInitialized(&__Internal_Dev_Context__->DebugPoints)) \
__Internal_Dev_Context__->DebugPoints = CreateDynamicArray<debug_point>(2048); \
Append(&__Internal_Dev_Context__->DebugPoints, {position, color})

#define DEBUG_DRAW_EDGE(position0, position1, color) \
if(!IsInitialized(&__Internal_Dev_Context__->DebugEdges)) \
__Internal_Dev_Context__->DebugEdges = CreateDynamicArray<debug_edges>(1024); \
Append(&__Internal_Dev_Context__->DebugEdges, {position0, position1, color})

#define DEBUG_DRAW_DIRECTION_VECTOR(origin, direction, color) \
if(!IsInitialized(&__Internal_Dev_Context__->DebugDirectionVectors)) \
__Internal_Dev_Context__->DebugDirectionVectors = CreateDynamicArray<debug_direction_vector>(1024); \
if(direction != V3()) \
Append(&__Internal_Dev_Context__->DebugDirectionVectors, {origin, Normalize(direction), color})

#define DEBUG_DRAW_QUAD(center, normal, dimensions, color) \
if(!IsInitialized(&__Internal_Dev_Context__->DebugQuads)) \
__Internal_Dev_Context__->DebugQuads = CreateDynamicArray<debug_quad>(1024); \
Append(&__Internal_Dev_Context__->DebugQuads, {center, normal, dimensions, color})

#include "imgui/imgui.h"
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

struct debug_point
{
    v3f P;
    c4 Color;
};

struct debug_edges
{
    v3f P0;
    v3f P1;
    c4 Color;
};

struct debug_direction_vector
{
    v3f Origin;
    v3f Direction;
    c4 Color;
};

struct debug_quad
{
    v3f CenterP;
    v3f Normal;
    v2f Dim;
    c4 Color;
};

enum shading_type
{
    SHADING_TYPE_NORMAL,
    SHADING_TYPE_WIREFRAME,
    SHADING_TYPE_WIREFRAME_ON_NORMAL
};

#define MAX_IMGUI_MESHES 32
struct dev_context
{
    game* Game;
    graphics* Graphics;
    
    arena DevStorage;
    b32 InDevelopmentMode;    
    b32 UseDevCamera;        
    b32 DrawOtherWorld;    
    b32 DrawFrames;
    shading_type ShadingType;
    
    frame_recording FrameRecording;
        
    game_information GameInformation;
    
    dev_input Input;    
    camera Cameras[2];
    
    u32 ImGuiMeshCount;
    i64 ImGuiMeshes[MAX_IMGUI_MESHES];    
    
    dev_mesh LineBoxMesh;
    dev_mesh LineSphereMesh;    
    dev_mesh TriangleBoxMesh;
    dev_mesh TriangleSphereMesh;
    dev_mesh TriangleCylinderMesh;
    dev_mesh TriangleConeMesh;
    dev_mesh TriangleArrowMesh;
    
    dynamic_array<debug_point> DebugPoints;
    dynamic_array<debug_edges> DebugEdges;
    dynamic_array<debug_direction_vector> DebugDirectionVectors;
    dynamic_array<debug_quad> DebugQuads;
    
    void* PlatformData;
    b32 Initialized;
};

void Platform_InitImGui(void* PlatformData);
void Platform_DevUpdate(void* PlatformData, v2i RenderDim, f32 dt);
string Platform_OpenFileDialog(char* Extension);
string Platform_FindNewFrameRecordingPath();

inline b32 IsInDevelopmentMode(dev_context* Context)
{
    if(Context)
        return Context->InDevelopmentMode;    
    return false;
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

#endif

#endif
