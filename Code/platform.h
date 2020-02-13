#ifndef PLATFORM_H
#define PLATFORM_H

struct file_results
{
    u8* Data;
    u32 Size;
};

#define PLATFORM_LOG(name) void name(char* Format, ...)
#define PLATFORM_READ_ENTIRE_FILE(name) file_results name(char* Path)
#define PLATFORM_WRITE_ENTIRE_FILE(name) void name(char* Path, void* Data, u32 Length)

typedef PLATFORM_LOG(platform_log);
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

struct platform
{   
    arena* TempArena;
    error_stream* ErrorStream;
    platform_log* Log;
    allocate_memory* AllocateMemory;
    free_memory* FreeMemory;        
    platform_read_entire_file* ReadEntireFile;
    platform_write_entire_file* WriteEntireFile;    
};

global platform* Global_Platform;

#define CONSOLE_LOG(format, ...) Global_Platform->Log(format, __VA_ARGS__)

#endif