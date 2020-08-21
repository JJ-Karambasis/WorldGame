#ifndef PLATFORM_H
#define PLATFORM_H

#define PLATFORM_LOG(name) void name(char* Format, ...)
#define PLATFORM_TOGGLE_AUDIO(name) void name(struct audio_output* Audio, b32 State) 

typedef PLATFORM_LOG(platform_log);
typedef PLATFORM_TOGGLE_AUDIO(platform_toggle_audio);

struct platform
{   
    arena* TempArena;
    error_stream* ErrorStream;
    platform_log* Log;                        
    platform_toggle_audio* ToggleAudio;    
    platform_file_handle AssetFile;
};

global platform* Global_Platform;

#define CONSOLE_LOG(format, ...) Global_Platform->Log(format, __VA_ARGS__)

#endif