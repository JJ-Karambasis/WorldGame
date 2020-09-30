#ifndef DEV_FRAME_RECORDING_H
#define DEV_FRAME_RECORDING_H

global const ak_char FRAME_RECORDING_SIGNATURE[] = "FRAME_RECORDING";
global const ak_u16 FRAME_RECORDING_MAJOR_VERSION = 1;
global const ak_u16 FRAME_RECORDING_MINOR_VERSION = 0;

#pragma pack(push, 1)
struct dev_frame_recording_header
{
    const ak_char Signature[AK_Count(FRAME_RECORDING_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u32 FrameCount;
    ak_u32 WorldPathCount;
};

struct dev_frame_info
{
    ak_u32 OffsetToFrame;
    ak_u32 Size;
    ak_u32 RecordedEntityCount;
};

struct dev_recording_entity_header
{
    world_id ID;    
    ak_sqtf Transform;
    ak_v3f Velocity;
    ak_v3f Acceleration;
};

#pragma pack(pop)

struct dev_frame_data
{
    input Input;
    void* EntityBuffer;
};

enum dev_frame_recording_state
{
    DEV_FRAME_RECORDING_STATE_NONE,
    DEV_FRAME_RECORDING_STATE_RECORDING,
    DEV_FRAME_RECORDING_STATE_PLAYING
};

struct dev_frame_recording
{   
    dev_frame_recording_state RecordingState;
    
    ak_u32 FrameCount;    
    ak_buffer ReadRecordingBuffer;  
    ak_arena* RecordingArena;        
    ak_array<dev_frame_info> WrittenFrameInfos;
    ak_array<dev_frame_data> WrittenFrameDatas;
    
    void StartRecording();
    void RecordFrame(game* Game);
    void EndRecording(ak_char* Path, ak_string WorldFilePath);
    
    ak_bool StartPlaying(ak_char* RecordingPath);
    void PlayFrame(game* Game, ak_u32 FrameIndex);
    void EndPlaying();
    
    void AddWrittenFrame(dev_frame_info FrameInfo, dev_frame_data FrameData);
    void Reset();
};

#endif