#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_OUTPUT_CHANNEL_COUNT 2
#define AUDIO_OUTPUT_SAMPLES_PER_SECOND 48000
 
struct playing_audio
{
    audio* Audio;
    ak_u64 PlayingSampleIndex;
    ak_bool IsFinishedPlaying;
    ak_f32 Volume;
};

struct audio_output
{
    ak_arena* AudioThreadArena;
    ak_bool Mute;
    ak_pool<playing_audio> PlayingAudioStorage;    
};

#endif