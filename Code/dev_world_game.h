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
#define DEBUG_DRAW_POINT(position, color) ASSERT(__Internal_Dev_Context__->DebugPointCount < ARRAYCOUNT(__Internal_Dev_Context__->DebugPoints)); __Internal_Dev_Context__->DebugPoints[__Internal_Dev_Context__->DebugPointCount++] = {position, color}
#define DEBUG_DRAW_DIRECTION_VECTOR(origin, direction, color) ASSERT(__Internal_Dev_Context__->DebugDirectionVectorsCount < ARRAYCOUNT(__Internal_Dev_Context__->DebugDirectionVectors)); __Internal_Dev_Context__->DebugDirectionVectors[__Internal_Dev_Context__->DebugDirectionVectorsCount++] = {origin, direction, color}

#include "imgui/imgui.h"

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

struct debug_direction_vector
{
    v3f Origin;
    v3f Direction;
    c4 Color;
};

struct frame
{
    f32 dt;
    input Input;    
    v3f PlayerPosition;
    v3f PlayerVelocity;
    v3f CollidedNormal;
};

struct frame_recording
{
    string_storage RecordingPath;
    buffer RecordingBuffer;
    b32 IsRecording;
    b32 IsPlaying;    
    
    u32 TotalFrames;    
    free_list<frame> Frames;    
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
    
    frame_recording FrameRecording;
    
    
    game_information GameInformation;
    
    dev_input Input;    
    camera Cameras[2];
    
    u32 ImGuiMeshCount;
    i64 ImGuiMeshes[MAX_IMGUI_MESHES];    
    
    dev_mesh LineBoxMesh;
    dev_mesh LineSphereMesh;
    
    dev_mesh TriangleBoxMesh;    
    
    u32 DebugPointCount;
    debug_point DebugPoints[2048];
    
    u32 DebugDirectionVectorsCount;
    debug_direction_vector DebugDirectionVectors[2048];        
    
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

#endif

#endif
