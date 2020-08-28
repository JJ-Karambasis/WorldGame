#ifndef DEV_FRAME_RECORDING_H
#define DEV_FRAME_RECORDING_H

global const char FRAME_RECORDING_SIGNATURE[] = "FRAME_RECORDING";
global const u16 FRAME_RECORDING_MAJOR_VERSION = 1;
global const u16 FRAME_RECORDING_MINOR_VERSION = 0;

#pragma pack(push, 1)
struct frame_recording_header
{
    const char Signature[ARRAYCOUNT(FRAME_RECORDING_SIGNATURE)];
    u16 MajorVersion;
    u16 MinorVersion;
    u32 FrameCount;
};

struct frame_info
{
    u32 OffsetToFrame;
    u32 Size;
    u32 RecordedEntityCount;
};

struct recording_entity_header
{
    entity_id ID;    
    sqt Transform;
    v3f Velocity;
    v3f Acceleration;
};

#pragma pack(pop)

struct frame_data
{
    input Input;
    void* EntityBuffer;
};

enum frame_recording_state
{
    FRAME_RECORDING_STATE_NONE,
    FRAME_RECORDING_STATE_RECORDING,
    FRAME_RECORDING_STATE_PLAYING
};

struct frame_recording
{   
    frame_recording_state RecordingState;
    
    u32 FrameCount;    
    buffer ReadRecordingBuffer;  
    arena RecordingArena;        
    dynamic_array<frame_info> WrittenFrameInfos;
    dynamic_array<frame_data> WrittenFrameDatas;
    
    void StartRecording();
    void RecordFrame(game* Game);
    void EndRecording(char* Path);
    
    b32 StartPlaying(char* RecordingPath);
    void PlayFrame(game* Game, u32 FrameIndex);
    void EndPlaying();
    
    void AddWrittenFrame(frame_info FrameInfo, frame_data FrameData);
    void Reset();
};

#endif