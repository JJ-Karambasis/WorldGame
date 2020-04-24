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

#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations)
#define DEVELOPER_MAX_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations)
#define NOT_IN_DEVELOPMENT_MODE() !IsInDevelopmentMode((dev_context*)DevContext)

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

struct dev_context
{
    b32 InDevelopmentMode;    
    dev_input Input;    
    camera Camera;
    
    
    void* PlatformData;
    b32 Initialized;
};

void Platform_InitImGui(void* PlatformData);
void Platform_DevUpdate(void* PlatformData, v2i RenderDim, f32 dt);

inline b32 IsInDevelopmentMode(dev_context* Context)
{
    if(Context)
        return Context->InDevelopmentMode;    
    return false;
}

#else

#define DEVELOPER_MAX_GJK_ITERATIONS(Iterations)
#define DEVELOPER_MAX_WALKING_TRIANGLE()
#define DEVELOPER_MAX_TIME_ITERATIONS(Iterations)
#define NOT_IN_DEVELOPMENT_MODE() true

#endif

#endif
