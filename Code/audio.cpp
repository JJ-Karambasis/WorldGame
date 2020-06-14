inline ptr
GetBytesPerSecond(u32 ChannelCount)
{
    ptr Result = ChannelCount*sizeof(i16)*AUDIO_OUTPUT_SAMPLES_PER_SECOND;
    return Result;
}

inline ptr
GetAudioBufferSizeFromSeconds(u32 ChannelCount, ptr Seconds)
{
    ptr Result = GetBytesPerSecond(ChannelCount)*Seconds;
    return Result;
}

inline i16* GetSamples(samples* Samples, u32 ChannelCount, u32 SampleIndex)
{     
    ASSERT(SampleIndex < Samples->Count);
    i16* Result = Samples->Data + (SampleIndex*ChannelCount);
    return Result;
}

inline i16* GetSamples(audio* Audio, u32 SampleIndex)
{         
    i16* Result = GetSamples(&Audio->Samples, Audio->ChannelCount, SampleIndex);
    return Result;    
}

void PlayAudio(audio_output* AudioOutput, audio* Audio)
{
    if(IsInitialized(&AudioOutput->PlayingAudioPool))
    {
        pool<playing_audio>* AudioPool = &AudioOutput->PlayingAudioPool;        
        playing_audio* PlayingAudio = GetByID(AudioPool, AllocateFromPool(AudioPool));        
        PlayingAudio->Audio = Audio;
    }
}

extern "C"
EXPORT GAME_OUTPUT_SOUND_SAMPLES(OutputSoundSamples)
{
    Global_Platform = Platform;
    
    audio_output* AudioOutput = Game->AudioOutput;        
}