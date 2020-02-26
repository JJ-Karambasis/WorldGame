/* Original Author: Armand (JJ) Karambasis */
#ifndef DEV_WORLD_GAME_H
#define DEV_WORLD_GAME_H

#define MAX_FRAME_RECORDINGS 

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
    f32 Scroll;
};

struct frame_offset_array
{
    u32 Count;
    ptr* Ptr;
};

struct frame_recording
{
    platform_file_handle* File;
    arena FrameStream;
    u32 MaxFrameCount;
    frame_offset_array FrameOffsets;
    ptr NextFrameOffset;
    u32 CurrentFrameIndex;
    recording_state RecordingState;
};

struct development_game : public game
{
    arena DevArena;        
    b32 InDevelopmentMode;
    camera DevCamera;      
    frame_recording FrameRecordings;
};

#endif
