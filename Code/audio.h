#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_OUTPUT_CHANNEL_COUNT 2
#define AUDIO_OUTPUT_SAMPLES_PER_SECOND 48000
 
struct playing_audio
{
    audio* Audio;
    u64 PlayingSampleIndex;
    b32 IsFinishedPlaying;
    f32 Volume;
};

struct audio_output
{
    b32 Mute;
    pool<playing_audio> PlayingAudioPool;
};

#endif