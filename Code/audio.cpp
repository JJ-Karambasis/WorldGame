inline audio_format
CreateAudioFormat(u16 ChannelCount, u32 SamplesPerSecond)
{
    audio_format Format = {ChannelCount, SamplesPerSecond};
    return Format;
}

inline ptr
GetBytesPerSecond(audio_format* Format)
{
    ptr Result = Format->ChannelCount*sizeof(i16)*Format->SamplesPerSecond;
    return Result;
}

inline ptr
GetAudioBufferSizeFromSeconds(audio_format* Format, ptr Seconds)
{
    ptr Result = GetBytesPerSecond(Format)*Seconds;
    return Result;
}

inline i16* GetSamples(i16* Samples, audio_format* Format, u32 SampleIndex)
{     
    i16* Result = Samples + SampleIndex+Format->ChannelCount*sizeof(i16);
    return Result;
}

inline i16* GetSamples(audio* Audio, u32 SampleIndex)
{ 
    i16* Result = GetSamples(Audio->Samples, &Audio->Format, SampleIndex);
    return Result;    
}