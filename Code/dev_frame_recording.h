#ifndef DEV_FRAME_RECORDING_H
#define DEV_FRAME_RECORDING_H

struct frame
{
    f32 dt;
    input Input;    
    v3f PlayerPosition[2];
    v3f PlayerVelocity[2];
    v3f CollidedNormal[2];
};

enum frame_recording_state
{
    FRAME_RECORDING_STATE_NONE,
    FRAME_RECORDING_STATE_RECORDING,
    FRAME_RECORDING_STATE_PLAYING,
    FRAME_RECORDING_STATE_INSPECT_FRAMES
};

struct frame_recording
{    
    string_storage RecordingPath;    
    frame_recording_state RecordingState;            
    dynamic_array<frame> RecordedFrames;    
    
    buffer RecordingBuffer;
    u32 TotalLoadedFrames;
    u32 CurrentFrameIndex;
};

void DevelopmentFrameRecording(dev_context* DevContext);


#endif