#ifndef DEV_FRAME_RECORDING_H
#define DEV_FRAME_RECORDING_H

global const ak_char FRAME_RECORDING_SIGNATURE[] = "FRAME_RECORDING";
global const ak_u16 FRAME_RECORDING_MAJOR_VERSION = 1;
global const ak_u16 FRAME_RECORDING_MINOR_VERSION = 0;

#pragma pack(push, 1)
struct frame_recording_header
{
    const ak_char Signature[AK_Count(FRAME_RECORDING_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u32 FrameCount;
};

struct frame_info
{
    ak_u32 OffsetToFrame;
    ak_u32 Size;
    ak_u32 RecordedEntityCount;
};

struct recording_entity_header
{
    entity_id ID;    
    ak_sqtf Transform;
    ak_v3f Velocity;
    ak_v3f Acceleration;
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
    
    ak_u32 FrameCount;    
    ak_buffer ReadRecordingBuffer;  
    ak_arena* RecordingArena;        
    ak_array<frame_info> WrittenFrameInfos;
    ak_array<frame_data> WrittenFrameDatas;
    
    void StartRecording();
    void RecordFrame(game* Game);
    void EndRecording(char* Path);
    
    ak_bool StartPlaying(ak_char* RecordingPath);
    void PlayFrame(game* Game, ak_u32 FrameIndex);
    void EndPlaying();
    
    void AddWrittenFrame(frame_info FrameInfo, frame_data FrameData);
    void Reset();
};

#endif