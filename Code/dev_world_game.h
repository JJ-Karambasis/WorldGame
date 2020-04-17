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

struct frame_recording
{
    platform_file_handle* File;
    arena FrameStream;
    i32 MaxFrameCount;
    i32 FrameCount;
    ptr* FrameOffsets;    
    ptr NextFrameOffset;
    i32 CurrentFrameIndex;
    recording_state RecordingState;            
};

struct development_game : public game
{
    arena DevArena;        
    b32 InDevelopmentMode;
    camera DevCamera;      
    frame_recording FrameRecordings;
    u32 WalkingTriangleCount[2];
    u32 MaxGJKIterations;
    b32 DevInitialized;        
    bool TurnOnVolumeOutline;
    bool TurnAudioOn;
    bool TurnBlockerDrawingOn;
    
    f32 LastTickFrameTime;
};

#endif
