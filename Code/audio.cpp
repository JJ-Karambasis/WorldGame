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

inline i16* GetSamples(samples* Samples, u32 ChannelCount, u64 SampleIndex)
{     
    ASSERT(SampleIndex < Samples->Count);
    i16* Result = Samples->Data + (SampleIndex*ChannelCount);
    return Result;
}

inline i16* GetSamples(audio* Audio, u64 SampleIndex)
{         
    i16* Result = GetSamples(&Audio->Samples, Audio->ChannelCount, SampleIndex);
    return Result;    
}

void PlayAudio(audio_output* AudioOutput, audio* Audio, f32 Volume)
{
    pool<playing_audio>* AudioPool = &AudioOutput->PlayingAudioPool;        
    if(AudioPool->IsInitialized() && !AudioOutput->Mute)
    {        
        playing_audio* PlayingAudio = AudioPool->Get(AudioPool->Allocate());
        PlayingAudio->Audio = Audio;
        PlayingAudio->Volume = Volume;
    }
}

void PlayAudio(game* Game, audio* Audio, f32 Volume)
{
    PlayAudio(Game->AudioOutput, Audio, Volume);
}

f32 ConvertSamplesI16ToF32(i16 Sample)
{
    f32 Result = (f32)Sample / 32768.0f;
    return Result;
}

i16 ConvertSamplesF32ToI16(f32 Sample)
{
    i16 Result = (i16)(Sample*32768.0f);
    return Result;
}

extern "C"
EXPORT GAME_OUTPUT_SOUND_SAMPLES(OutputSoundSamples)
{    
    audio_output* AudioOutput = Game->AudioOutput;        
    
    f32* RealDest = PushArray(TempArena, OutputSamples->Count*AUDIO_OUTPUT_CHANNEL_COUNT, f32, Clear, 0);    
    if(!AudioOutput->Mute)
    {        
        FOR_EACH(PlayingAudio, &AudioOutput->PlayingAudioPool)
        {
            if(!PlayingAudio->IsFinishedPlaying)
            {
                audio* Audio = PlayingAudio->Audio;            
                f32* RealDestAt = RealDest;
                
                u64 SampleCount = Audio->Samples.Count - PlayingAudio->PlayingSampleIndex;
                if(SampleCount > OutputSamples->Count)
                    SampleCount = OutputSamples->Count;
                
                for(u64 SampleIndex = 0; SampleIndex < SampleCount; SampleIndex++)
                {
                    i16* SrcSamples = GetSamples(Audio, PlayingAudio->PlayingSampleIndex+SampleIndex);                
                    if(Audio->ChannelCount == 1)
                    {                    
                        f32 RealSrcSamples = ConvertSamplesI16ToF32(SrcSamples[0])*PlayingAudio->Volume;                   
                        *RealDestAt++ += RealSrcSamples;
                        *RealDestAt++ += RealSrcSamples;                                                                                
                    }
                    else if(Audio->ChannelCount == 2)
                    {
                        f32 RealSrcSamples0 = ConvertSamplesI16ToF32(SrcSamples[0])*PlayingAudio->Volume;
                        f32 RealSrcSamples1 = ConvertSamplesI16ToF32(SrcSamples[1])*PlayingAudio->Volume;
                        *RealDestAt++ += RealSrcSamples0;
                        *RealDestAt++ += RealSrcSamples1;
                    }
                }                        
                
                
                PlayingAudio->PlayingSampleIndex += SampleCount;            
                ASSERT(PlayingAudio->PlayingSampleIndex <= Audio->Samples.Count);
                if(PlayingAudio->PlayingSampleIndex == Audio->Samples.Count)            
                    PlayingAudio->IsFinishedPlaying = true;            
            }
        }
    }    
    
    f32* SrcAt = RealDest;
    i16* DestAt = OutputSamples->Data;
    for(u32 OutputSampleIndex = 0; OutputSampleIndex < OutputSamples->Count; OutputSampleIndex++)
    {
        for(u32 ChannelIndex = 0; ChannelIndex < AUDIO_OUTPUT_CHANNEL_COUNT; ChannelIndex++)
        {
            *DestAt++ = ConvertSamplesF32ToI16(*SrcAt++);                        
        }
    }
    
}