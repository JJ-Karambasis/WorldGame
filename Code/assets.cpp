mesh LoadGraphicsMesh(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));
    
    fbx_context FBX = FBX_LoadFile(File);
    mesh Result = FBX_LoadFirstMesh(&FBX, &Assets->Storage);    
    if(Result.Vertices && Result.Indices)
    {        
        Result.GDIHandle = Assets->Graphics->AllocateMesh(Assets->Graphics, Result.Vertices, GetVertexBufferSize(Result.VertexFormat, Result.VertexCount), Result.VertexFormat,
                                                          Result.Indices, GetIndexBufferSize(Result.IndexFormat, Result.IndexCount), Result.IndexFormat);
    }    
    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

walkable_mesh LoadWalkableMesh(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    walkable_mesh Result = FBX_LoadFirstWalkableMesh(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

skeleton LoadSkeleton(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    skeleton Result = FBX_LoadFirstSkeleton(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

animation_clip LoadAnimation(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    animation_clip Result = FBX_LoadFirstAnimation(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

audio 
DEBUGLoadWAVFile(char* Path)
{
    audio Result = {};
    
    buffer FileBuffer = Global_Platform->ReadEntireFile(Path);
    BOOL_CHECK_AND_HANDLE(FileBuffer.Data, "Failed to read the wav file: %s\n", Path);
    
    riff_header Header;
    riff_iterator Iter = IterateRIFF(FileBuffer, &Header); 
    
    BOOL_CHECK_AND_HANDLE((Header.RiffID == RIFFID_RIFF) && (Header.FileTypeID == WAVE_CHUNK_TYPE_WAVE),
                          "WAV File is not a valid riff file.");
    
    buffer SampleData = {};
    
    wav_format* WAVFormat = NULL;
    while(IsValid(Iter))
    {
        switch(GetType(Iter))
        {
            case WAVE_CHUNK_TYPE_FMT:
            {
                WAVFormat = (wav_format*)GetChunkData(Iter);
                BOOL_CHECK_AND_HANDLE(WAVFormat->wFormatTag == 1, "Format tag for wav files is not PCM");                
                BOOL_CHECK_AND_HANDLE(WAVFormat->wBitsPerSample == 16, "Bits per sample must be 16");
                BOOL_CHECK_AND_HANDLE(WAVFormat->nSamplesPerSec == 48000, "Samples per second needs to be 48000");                
                BOOL_CHECK_AND_HANDLE((WAVFormat->nChannels == 1) || (WAVFormat->nChannels == 2), "WAV format needs to have 1 (mono) or 2 (stereo) channels.");                
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
    
    Result.ChannelCount = WAVFormat->nChannels;
    
    samples Samples = {};
    Samples.Count = SampleData.Size / (Result.ChannelCount*sizeof(i16));        
    Samples.Data = (i16*)Global_Platform->AllocateMemory(SampleData.Size);
    BOOL_CHECK_AND_HANDLE(Samples.Data, "Failed to allocate memory for the audio file.");
    
    for(u32 SampleIndex = 0; SampleIndex < Samples.Count; SampleIndex++)
    {
        i16* DstChannelSamples = GetSamples(&Result.Samples, Result.ChannelCount, SampleIndex);
        i16* SrcChannelSamples = GetSamples(&Samples, Result.ChannelCount, SampleIndex);
        for(u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ChannelIndex++)        
            *DstChannelSamples++ = *SrcChannelSamples++;                    
    }
    
    handle_error:
    Global_Platform->FreeFileMemory(&FileBuffer);
    return Result;
}