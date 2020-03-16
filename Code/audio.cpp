inline audio_format
CreateAudioFormat(u16 BytesPerSample, u16 ChannelCount, u32 SamplesPerSecond)
{
    audio_format Format = {BytesPerSample, ChannelCount, SamplesPerSecond};
    return Format;
}

inline u16 
GetSampleCount(audio_format* Format)
{
    u16 Result = Format->BytesPerSample*Format->ChannelCount;
    return Result;
}

inline ptr
GetBytesPerSecond(audio_format* Format)
{
    ptr Result = GetSampleCount(Format)*Format->SamplesPerSecond;
    return Result;
}

inline ptr
GetAudioBufferSizeFromSeconds(audio_format* Format, ptr Seconds)
{
    ptr Result = GetBytesPerSecond(Format)*Seconds;
    return Result;
}

inline u8* GetSamples(u8* Samples, audio_format* Format, u32 SampleIndex)
{     
    u8* Result = Samples + SampleIndex+GetSampleCount(Format);
    return Result;
}

inline u8* GetSamples(audio* Audio, u32 SampleIndex)
{ 
    u8* Result = GetSamples((u8*)Audio->Samples, &Audio->Format, SampleIndex);
    return Result;    
}



audio 
LoadWAVFile(char* Path)
{
    audio Result = {};
    temp_arena Temp = BeginTemporaryMemory();
    
    buffer FileBuffer = Global_Platform->ReadEntireFile(Path);
    BOOL_CHECK_AND_HANDLE(FileBuffer.Data, "Failed to read the wav file: %s\n", Path);
    
    riff_header Header;
    riff_iterator Iter = IterateRIFF(FileBuffer, &Header); 
    
    BOOL_CHECK_AND_HANDLE((Header.RiffID == RIFFID_RIFF) && (Header.FileTypeID == WAVE_CHUNK_TYPE_WAVE),
                          "WAV File is not a valid riff file.");
    
    buffer SampleData = {};
    
    audio_format AudioFormat = {};
    while(IsValid(Iter))
    {
        switch(GetType(Iter))
        {
            case WAVE_CHUNK_TYPE_FMT:
            {
                wav_format* WAVFormat = (wav_format*)GetChunkData(Iter);
                BOOL_CHECK_AND_HANDLE(WAVFormat->wFormatTag == 1, "Format tag for wav files is not PCM");                
                AudioFormat = CreateAudioFormat(WAVFormat->wBitsPerSample/8, WAVFormat->nChannels, WAVFormat->nSamplesPerSec);                                
            } break;
            
            case WAVE_CHUNK_TYPE_DATA:
            {
                SampleData.Data = (u8*)GetChunkData(Iter);
                SampleData.Size = GetChunkDataSize(Iter);
            } break;
        }
        
        Iter = NextChunk(Iter);
    }
    
    BOOL_CHECK_AND_HANDLE(SampleData.Data, "Failed to find the data chunk for the wav file.");
    
    Result.Format = AudioFormat;
    Result.SampleCount = SampleData.Size / GetSampleCount(&AudioFormat);
    Result.Samples = Global_Platform->AllocateMemory(SampleData.Size);
    BOOL_CHECK_AND_HANDLE(Result.Samples, "Failed to allocate memory for the audio file.");
    
    for(u32 SampleIndex = 0; SampleIndex < Result.SampleCount; SampleIndex++)
    {
        u8* DstChannelSamples = GetSamples(&Result, SampleIndex);
        u8* SrcChannelSamples = GetSamples(SampleData.Data, &AudioFormat, SampleIndex);
        for(u32 ChannelIndex = 0; ChannelIndex < AudioFormat.ChannelCount; ChannelIndex++)
        {
            CopyMemory(DstChannelSamples, SrcChannelSamples, AudioFormat.BytesPerSample);
            DstChannelSamples += Result.Format.BytesPerSample;
            SrcChannelSamples += Result.Format.BytesPerSample;
        }
    }
    
    handle_error:
    EndTemporaryMemory(&Temp);
    return Result;
}