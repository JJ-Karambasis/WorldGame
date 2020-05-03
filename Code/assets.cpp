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
    return Result;
}

skeleton LoadSkeleton(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));
    
    fbx_context FBX = FBX_LoadFile(File);
    skeleton Result = FBX_LoadFirstSkeleton(&FBX, &Assets->Storage);
    return Result;
}

triangle3D_mesh CreateBoxTriangleMesh(arena* Storage)
{
    triangle3D_mesh Result = {};
    
    Result.TriangleCount = 12;
    Result.Triangles = PushArray(Storage, 12, triangle3D, Clear, 0);
    
    v3f Vertices[8] = 
    {
        V3(-0.5f, -0.5f, 1.0f),    
        V3( 0.5f, -0.5f, 1.0f),
        V3( 0.5f,  0.5f, 1.0f),
        V3(-0.5f,  0.5f, 1.0f),
        
        V3( 0.5f, -0.5f, 0.0f),
        V3(-0.5f, -0.5f, 0.0f),
        V3(-0.5f,  0.5f, 0.0f),
        V3( 0.5f,  0.5f, 0.0f)
    };
    
    Result.Triangles[0]  = CreateTriangle3D(Vertices[0], Vertices[1], Vertices[2]);
    Result.Triangles[1]  = CreateTriangle3D(Vertices[0], Vertices[2], Vertices[3]);
    Result.Triangles[2]  = CreateTriangle3D(Vertices[1], Vertices[4], Vertices[7]);
    Result.Triangles[3]  = CreateTriangle3D(Vertices[1], Vertices[7], Vertices[2]);
    Result.Triangles[4]  = CreateTriangle3D(Vertices[4], Vertices[5], Vertices[6]);
    Result.Triangles[5]  = CreateTriangle3D(Vertices[4], Vertices[6], Vertices[7]);
    Result.Triangles[6]  = CreateTriangle3D(Vertices[5], Vertices[0], Vertices[3]);
    Result.Triangles[7]  = CreateTriangle3D(Vertices[5], Vertices[3], Vertices[6]);
    Result.Triangles[8]  = CreateTriangle3D(Vertices[3], Vertices[2], Vertices[7]);
    Result.Triangles[9]  = CreateTriangle3D(Vertices[3], Vertices[7], Vertices[6]);
    Result.Triangles[10] = CreateTriangle3D(Vertices[4], Vertices[1], Vertices[0]);
    Result.Triangles[11] = CreateTriangle3D(Vertices[4], Vertices[0], Vertices[5]);
    
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