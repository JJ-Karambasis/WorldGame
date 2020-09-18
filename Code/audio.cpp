inline ak_u32
GetBytesPerSecond(ak_u32 ChannelCount)
{
    ak_u32 Result = ChannelCount*sizeof(ak_i16)*AUDIO_OUTPUT_SAMPLES_PER_SECOND;
    return Result;
}

inline ak_u64
GetAudioBufferSizeFromSeconds(ak_u32 ChannelCount, ak_u64 Seconds)
{
    ak_u64 Result = GetBytesPerSecond(ChannelCount)*Seconds;
    return Result;
}

inline ak_i16* GetSamples(samples* Samples, ak_u32 ChannelCount, ak_u64 SampleIndex)
{     
    AK_Assert(SampleIndex < Samples->Count, "Index out of bounds");
    ak_i16* Result = Samples->Data + (SampleIndex*ChannelCount);
    return Result;
}

inline ak_i16* GetSamples(audio* Audio, ak_u64 SampleIndex)
{         
    ak_i16* Result = GetSamples(&Audio->Samples, Audio->ChannelCount, SampleIndex);
    return Result;    
}

void PlayAudio(audio_output* AudioOutput, audio* Audio, ak_f32 Volume)
{
    ak_pool<playing_audio>* AudioPool = &AudioOutput->PlayingAudioStorage;        
    if(!AudioOutput->Mute)
    {                
        playing_audio* PlayingAudio = AudioPool->Get(AudioPool->Allocate());
        PlayingAudio->Audio = Audio;
        PlayingAudio->Volume = Volume;
    }
}

void PlayAudio(game* Game, audio* Audio, ak_f32 Volume)
{
    PlayAudio(Game->AudioOutput, Audio, Volume);
}

ak_f32 ConvertSamplesI16ToF32(ak_i16 Sample)
{
    ak_f32 Result = (ak_f32)Sample / 32768.0f;
    return Result;
}

ak_i16 ConvertSamplesF32ToI16(ak_f32 Sample)
{
    ak_i16 Result = (ak_i16)(Sample*32768.0f);
    return Result;
}

extern "C"
AK_EXPORT GAME_OUTPUT_SOUND_SAMPLES(OutputSoundSamples)
{    
    audio_output* AudioOutput = Game->AudioOutput;        
    
    ak_f32* RealDest = AudioOutput->AudioThreadArena->PushArray<ak_f32>(AK_SafeU32(Samples->Count)*AUDIO_OUTPUT_CHANNEL_COUNT);
    if(!AudioOutput->Mute)
    {        
        AK_ForEach(PlayingAudio, &AudioOutput->PlayingAudioStorage)
        {
            if(!PlayingAudio->IsFinishedPlaying)
            {
                audio* Audio = PlayingAudio->Audio;            
                ak_f32* RealDestAt = RealDest;
                
                ak_u64 SampleCount = Audio->Samples.Count - PlayingAudio->PlayingSampleIndex;
                if(SampleCount > Samples->Count)
                    SampleCount = Samples->Count;
                
                for(ak_u64 SampleIndex = 0; SampleIndex < SampleCount; SampleIndex++)
                {
                    ak_i16* SrcSamples = GetSamples(Audio, PlayingAudio->PlayingSampleIndex+SampleIndex);                
                    if(Audio->ChannelCount == 1)
                    {                    
                        ak_f32 RealSrcSamples = ConvertSamplesI16ToF32(SrcSamples[0])*PlayingAudio->Volume;                   
                        *RealDestAt++ += RealSrcSamples;
                        *RealDestAt++ += RealSrcSamples;                                                                                
                    }
                    else if(Audio->ChannelCount == 2)
                    {
                        ak_f32 RealSrcSamples0 = ConvertSamplesI16ToF32(SrcSamples[0])*PlayingAudio->Volume;
                        ak_f32 RealSrcSamples1 = ConvertSamplesI16ToF32(SrcSamples[1])*PlayingAudio->Volume;
                        *RealDestAt++ += RealSrcSamples0;
                        *RealDestAt++ += RealSrcSamples1;
                    }
                }                        
                
                
                PlayingAudio->PlayingSampleIndex += SampleCount;            
                AK_Assert(PlayingAudio->PlayingSampleIndex <= Audio->Samples.Count, "Index out of bounds");
                if(PlayingAudio->PlayingSampleIndex == Audio->Samples.Count)            
                    PlayingAudio->IsFinishedPlaying = true;            
            }
        }
    }    
    
    ak_f32* SrcAt = RealDest;
    ak_i16* DestAt = Samples->Data;
    for(ak_u32 OutputSampleIndex = 0; OutputSampleIndex < Samples->Count; OutputSampleIndex++)
    {
        for(ak_u32 ChannelIndex = 0; ChannelIndex < AUDIO_OUTPUT_CHANNEL_COUNT; ChannelIndex++)
        {
            *DestAt++ = ConvertSamplesF32ToI16(*SrcAt++);                        
        }
    }
    
}