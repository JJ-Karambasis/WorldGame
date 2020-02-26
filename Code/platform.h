#ifndef PLATFORM_H
#define PLATFORM_H

struct file_results
{
    u8* Data;
    u32 Size;
};

struct platform_file_handle;
enum platform_file_attributes
{
    PLATFORM_FILE_ATTRIBUTES_READ,
    PLATFORM_FILE_ATTRIBUTES_WRITE
};

#define NO_OFFSET ((u64)-1)

#define PLATFORM_LOG(name) void name(char* Format, ...)
#define PLATFORM_READ_ENTIRE_FILE(name) file_results name(char* Path)
#define PLATFORM_WRITE_ENTIRE_FILE(name) void name(char* Path, void* Data, u32 Length)
#define PLATFORM_OPEN_FILE(name) platform_file_handle* name(char* Path, platform_file_attributes Attributes)
#define PLATFORM_READ_FILE(name) b32 name(platform_file_handle* File, void* Data, u32 ReadSize, u64 Offset)
#define PLATFORM_WRITE_FILE(name) b32 name(platform_file_handle* File, void* Data, u32 WriteSize, u64 Offset)
#define PLATFORM_CLOSE_FILE(name) void name(platform_file_handle* File)

typedef PLATFORM_LOG(platform_log);
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);
typedef PLATFORM_OPEN_FILE(platform_open_file);
typedef PLATFORM_READ_FILE(platform_read_file);
typedef PLATFORM_WRITE_FILE(platform_write_file);
typedef PLATFORM_CLOSE_FILE(platform_close_file);

struct platform
{   
    arena* TempArena;
    error_stream* ErrorStream;
    platform_log* Log;
    allocate_memory* AllocateMemory;
    free_memory* FreeMemory;        
    platform_read_entire_file* ReadEntireFile;
    platform_write_entire_file* WriteEntireFile;    
    platform_open_file* OpenFile;
    platform_read_file* ReadFile;
    platform_write_file* WriteFile;
    platform_close_file* CloseFile;    
};

global platform* Global_Platform;

#define CONSOLE_LOG(format, ...) Global_Platform->Log(format, __VA_ARGS__)

#endif