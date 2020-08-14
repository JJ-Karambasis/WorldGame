#ifndef PLATFORM_H
#define PLATFORM_H

struct platform_file_handle;
enum platform_file_attributes
{
    PLATFORM_FILE_ATTRIBUTES_READ,
    PLATFORM_FILE_ATTRIBUTES_WRITE
};

#define NO_OFFSET ((u64)-1)

#ifdef OS_WINDOWS
typedef u64 platform_time;
#endif

#define PLATFORM_LOG(name) void name(char* Format, ...)
#define PLATFORM_READ_ENTIRE_FILE(name) buffer name(char* Path)
#define PLATFORM_WRITE_ENTIRE_FILE(name) void name(char* Path, void* Data, u32 Length)
#define PLATFORM_FREE_FILE_MEMORY(name) void name(buffer* Buffer)
#define PLATFORM_OPEN_FILE(name) platform_file_handle* name(char* Path, platform_file_attributes Attributes)
#define PLATFORM_READ_FILE(name) b32 name(platform_file_handle* File, void* Data, u32 ReadSize, u64 Offset)
#define PLATFORM_WRITE_FILE(name) b32 name(platform_file_handle* File, void* Data, u32 WriteSize, u64 Offset)
#define PLATFORM_CLOSE_FILE(name) void name(platform_file_handle* File)
#define PLATFORM_CLOCK(name) platform_time name()
#define PLATFORM_ELAPSED_TIME(name) f64 name(platform_time End, platform_time Start)
#define PLATFORM_TOGGLE_AUDIO(name) void name(struct audio_output* Audio, b32 State) 

typedef PLATFORM_LOG(platform_log);
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);
typedef PLATFORM_OPEN_FILE(platform_open_file);
typedef PLATFORM_READ_FILE(platform_read_file);
typedef PLATFORM_WRITE_FILE(platform_write_file);
typedef PLATFORM_CLOSE_FILE(platform_close_file);
typedef PLATFORM_CLOCK(platform_clock);
typedef PLATFORM_ELAPSED_TIME(platform_elapsed_time);
typedef PLATFORM_TOGGLE_AUDIO(platform_toggle_audio);

struct platform
{   
    arena* TempArena;
    error_stream* ErrorStream;
    platform_log* Log;
    allocate_memory* AllocateMemory;
    free_memory* FreeMemory;        
    platform_read_entire_file* ReadEntireFile;
    platform_write_entire_file* WriteEntireFile;    
    platform_free_file_memory* FreeFileMemory;
    platform_open_file* OpenFile;
    platform_read_file* ReadFile;
    platform_write_file* WriteFile;
    platform_close_file* CloseFile;    
    platform_clock* Clock;
    platform_elapsed_time* ElapsedTime;
    platform_toggle_audio* ToggleAudio;
    
    platform_file_handle* AssetFile;
};

global platform* Global_Platform;

#define CONSOLE_LOG(format, ...) Global_Platform->Log(format, __VA_ARGS__)

#endif