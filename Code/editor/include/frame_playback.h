#ifndef FRAME_PLAYBACK_H
#define FRAME_PLAYBACK_H

#define MAX_RECORDING_NAME 128

enum frame_playback_state
{
    FRAME_PLAYBACK_STATE_NONE,
    FRAME_PLAYBACK_STATE_CREATE,
    FRAME_PLAYBACK_STATE_LOAD,
    FRAME_PLAYBACK_STATE_DELETE, 
    FRAME_PLAYBACK_STATE_PLAYING,
    FRAME_PLAYBACK_STATE_INSPECTING,
    FRAME_PLAYBACK_STATE_RECORDING
};

global const ak_char RECORDING_FILE_SIGNATURE[] = "WGRECORDING";
global const ak_u16 RECORDING_FILE_MAJOR_VERSION = 1;
global const ak_u16 RECORDING_FILE_MINOR_VERSION = 0;
global const ak_char RECORDING_FILE_CHECKSUM[] = "B9EF68B868F378990F272322F2683D13";

struct recording_file_header
{
    ak_char Signature[AK_Count(RECORDING_FILE_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u32 FrameCount;
};

struct frame_header
{
    input Input;
    ak_u32 EntityCount[2];
    ak_u32 DataSize;
    ak_u32 OffsetToData;
};

struct frame_writer
{
    ak_binary_builder HeaderBuilder;
    ak_binary_builder DataBuilder;
    
    recording_file_header* FileHeader;
};

struct frame_reader
{
    ak_arena* Arena;
    ak_stream Stream;
    recording_file_header* FileHeader;
    ak_u32 CurrentFrameIndex;
    frame_header* FrameHeaders;
    
    inline void Clear()
    {
        Arena->Clear();
        CurrentFrameIndex = 0;
        FileHeader = NULL;
        Stream = {};
    }
};

struct frame_playback
{
    frame_playback_state OldState;
    frame_playback_state NewState;
    ak_string CurrentRecordingName;
    
    frame_writer Writer;
    frame_reader Reader;
    
    void Update(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform, dev_platform* DevPlatform);
    
    void StopRecording(editor* Editor);
    void StopPlaying(editor* Editor);
    
    void PlayFrame(editor* Editor);
    void RecordFrame(editor* Editor);
};

#endif
