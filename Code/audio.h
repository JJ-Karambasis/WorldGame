#ifndef AUDIO_H
#define AUDIO_H

struct playing_audio
{
    audio* Audio;
    u32 PlayingSampleIndex;
};

#pragma pack(push, 1)
struct wav_format
{
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
    u16 wValidBitsPerSample;
    u32 dwChannelMask;
    u8 SubFormat[16];
};
#pragma pack(pop)

enum 
{
    WAVE_CHUNK_TYPE_FMT = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_CHUNK_TYPE_DATA = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_CHUNK_TYPE_WAVE = RIFF_CODE('W', 'A', 'V', 'E')
};

#endif