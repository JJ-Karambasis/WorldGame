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
                BOOL_CHECK_AND_HANDLE(WAVFormat->wBitsPerSample == 16, "Bits per sample must be 16");
                AudioFormat = CreateAudioFormat(WAVFormat->nChannels, WAVFormat->nSamplesPerSec);                                
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
    Result.SampleCount = SampleData.Size / (AudioFormat.ChannelCount*sizeof(i16));
    Result.Samples = (i16*)Global_Platform->AllocateMemory(SampleData.Size);
    BOOL_CHECK_AND_HANDLE(Result.Samples, "Failed to allocate memory for the audio file.");
    
    for(u32 SampleIndex = 0; SampleIndex < Result.SampleCount; SampleIndex++)
    {
        i16* DstChannelSamples = GetSamples(&Result, SampleIndex);
        i16* SrcChannelSamples = GetSamples((i16*)SampleData.Data, &AudioFormat, SampleIndex);
        for(u32 ChannelIndex = 0; ChannelIndex < AudioFormat.ChannelCount; ChannelIndex++)        
            *DstChannelSamples++ = *SrcChannelSamples++;                    
    }
    
    handle_error:
    EndTemporaryMemory(&Temp);
    return Result;
}