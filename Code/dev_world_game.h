/* Original Author: Armand (JJ) Karambasis */
#ifndef DEV_WORLD_GAME_H
#define DEV_WORLD_GAME_H

#define MAX_FRAME_RECORDINGS 

#include "imgui/imgui.h"

enum recording_state
{
    RECORDING_STATE_NONE,
    RECORDING_STATE_RECORDING,    
    RECORDING_STATE_PLAYBACK,
    RECORDING_STATE_CYCLE
};

struct development_input : public input
{
    union
    {
        button DevButtons[8];
        struct
        {
            button Alt;
            button LMB;
            button MMB;
            button RecordButton;
            button PlaybackButton;
            button CycleButton;
            button CycleLeft;
            button CycleRight;
        };
    };
    v2i MouseDelta;
    v2i MouseCoordinates;
    f32 Scroll;
};

enum walking_event_type
{
    WALKING_EVENT_TYPE_UNKNOWN,
    WALKING_EVENT_TYPE_DRAW_POINT,
    WALKING_EVENT_TYPE_DRAW_EDGE
};

struct walking_event
{
    walking_event_type Type;
    union
    {
        struct
        {
            v3f Point;            
            v3f Padding;
            c4 Color;
        } DebugPoint;                
        
        struct
        {
            v3f Point0;
            v3f Point1;
            c4 Color;
        } DebugEdge;
    };
};

#define MAX_EVENTS 8096

struct walking_event_recording
{
    bool ShouldRender;
    walking_event* Events;
    u32 EventCount;
};

struct walking_system_recording
{
    union
    {
        walking_event_recording EventRecordings[5];
        struct
        {
            walking_event_recording PoleTestingRecording;
            walking_event_recording EdgeTestingRecording;
            walking_event_recording RadiusTestingRecording;
            walking_event_recording RingBuildingTestingRecording;
            walking_event_recording RingTraversalTestingRecording;
        };
    };
};

struct frame_recording
{
    platform_file_handle* File;
    arena FrameStream;
    u32 MaxFrameCount;
    u32 FrameCount;
    ptr* FrameOffsets;    
    ptr NextFrameOffset;
    u32 CurrentFrameIndex;
    recording_state RecordingState;        
    walking_system_recording WalkingSystemRecording;
};

struct development_game : public game
{
    arena DevArena;        
    b32 InDevelopmentMode;
    camera DevCamera;      
    frame_recording FrameRecordings;
    b32 DevInitialized;        
};

local walking_system_recording* __Internal_Recording__;
#define BEGIN_WALKING_SYSTEM(Game) __Internal_Recording__ = &((development_game*)Game)->FrameRecordings.WalkingSystemRecording;
#define END_WALKING_SYSTEM() __Internal_Recording__ = NULL

local walking_event_recording* __Internal_Event_Recording__;
#define BEGIN_POLE_TESTING() __Internal_Event_Recording__ = &__Internal_Recording__->PoleTestingRecording; __Internal_Event_Recording__->EventCount = 0
#define END_POLE_TESTING() ASSERT(__Internal_Event_Recording__ == &__Internal_Recording__->PoleTestingRecording); __Internal_Event_Recording__ = NULL

#define BEGIN_POLE_EDGE_TESTING() __Internal_Event_Recording__ = &__Internal_Recording__->EdgeTestingRecording; __Internal_Event_Recording__->EventCount = 0
#define END_POLE_EDGE_TESTING() ASSERT(__Internal_Event_Recording__ == &__Internal_Recording__->EdgeTestingRecording);__Internal_Event_Recording__ = NULL

#define BEGIN_RADIUS_TESTING() __Internal_Event_Recording__ = &__Internal_Recording__->RadiusTestingRecording; __Internal_Event_Recording__->EventCount = 0
#define END_RADIUS_TESTING() ASSERT(__Internal_Event_Recording__ == &__Internal_Recording__->RadiusTestingRecording); __Internal_Event_Recording__ = NULL

#define BEGIN_RING_BUILDING_TESTING() __Internal_Event_Recording__ = &__Internal_Recording__->RingBuildingTestingRecording; __Internal_Event_Recording__->EventCount = 0
#define END_RING_BUILDING_TESTING() ASSERT(__Internal_Event_Recording__ = &__Internal_Recording__->RingBuildingTestingRecording); __Internal_Event_Recording__ = NULL

#define BEGIN_RING_TRAVERSAL_TESTING() __Internal_Event_Recording__ = &__Internal_Recording__->RingTraversalTestingRecording; __Internal_Event_Recording__->EventCount = 0
#define END_RING_TRAVERSAL_TESTING() ASSERT(__Internal_Event_Recording__ = &__Internal_Recording__->RingTraversalTestingRecording); __Internal_Recording__ = NULL;

#define WALKING_SYSTEM_EVENT_DRAW_POINT(point, color) \
do \
{ \
    ASSERT(__Internal_Event_Recording__->EventCount < MAX_EVENTS); \
    walking_event* Event = &__Internal_Event_Recording__->Events[__Internal_Event_Recording__->EventCount++]; \
    Event->Type = WALKING_EVENT_TYPE_DRAW_POINT; \
    Event->DebugPoint.Point = point; \
    Event->DebugPoint.Color = color; \
} while(0)

#define WALKING_SYSTEM_EVENT_DRAW_EDGE(point0, point1, color) \
do \
{ \
    ASSERT(__Internal_Event_Recording__->EventCount < MAX_EVENTS); \
    walking_event* Event = &__Internal_Event_Recording__->Events[__Internal_Event_Recording__->EventCount++]; \
    Event->Type = WALKING_EVENT_TYPE_DRAW_EDGE; \
    Event->DebugEdge.Point0 = point0; \
    Event->DebugEdge.Point1 = point1; \
    Event->DebugEdge.Color = color; \
} while(0)

#endif
