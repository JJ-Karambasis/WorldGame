#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

struct indices_uint_pair
{
    uint_pair Pair;
    u32 VertexIndex;
};

inline u64 Hash(indices_uint_pair Pair, u64 TableSize)
{
    u64 Result = Hash(Pair.Pair, TableSize);    
    return Result;
}

inline b32 operator!=(indices_uint_pair Left, indices_uint_pair Right)
{
    b32 Result = Left.Pair != Right.Pair;    
    return Result;
}

mesh LoadGraphicsMesh(assets* Assets, char* File)
{
    buffer Buffer = Global_Platform->ReadEntireFile(File);
    
    tinyobj_attrib_t Attributes;
    tinyobj_shape_t* Shapes;
    size_t ShapeCount;
    tinyobj_material_t* Materials;
    size_t MaterialCount;
    int Code = tinyobj_parse_obj(&Attributes, &Shapes, &ShapeCount, &Materials, &MaterialCount, (const char*)Buffer.Data, Buffer.Size, TINYOBJ_FLAG_TRIANGULATE);
    
    ASSERT(Code == TINYOBJ_SUCCESS);
    
    hash_table<indices_uint_pair> HashTable = CreateHashTable<indices_uint_pair>(8096);     
    
    mesh Result = {};
    Result.IndexCount = Attributes.num_face_num_verts*3;
    Result.Indices = PushArray(&Assets->Arena, Result.IndexCount, u32, Clear, 0);
    
    vertex_p3_n3* VertexData = PushArray(Result.IndexCount, vertex_p3_n3, Clear, 0);
    
    u32* Indices = (u32*)Result.Indices;
    
    u32 NextIndex = 0;
    for(u32 FaceIndex = 0; FaceIndex < Attributes.num_face_num_verts; FaceIndex++)
    {
        tinyobj_vertex_index_t* Face = Attributes.faces + (FaceIndex*3);
        ASSERT(Attributes.face_num_verts[FaceIndex] == 3);
        
        for(u32 FaceVertexIndex = 0; FaceVertexIndex < 3; FaceVertexIndex++)
        {   
            u32 VertexIndex = (FaceIndex*3)+FaceVertexIndex;
            
            u32 Index = Result.VertexCount;
            indices_uint_pair Pair = {{(u32)Face->v_idx, (u32)Face->vn_idx}, Index};
            if(HashTable[Pair])
            {
                Indices[VertexIndex] = HashTable.Table[HashTable.GetHashIndex(Pair)].Key.VertexIndex;
            }
            else
            {                
                VertexData[Index] = 
                {
                    V3(Attributes.vertices+(Face->v_idx*3)),
                    V3(Attributes.normals+(Face->vn_idx*3))
                };
                
                Result.VertexCount++;
                Indices[VertexIndex] = Index;                
            }
            
            Face++;
        }
    }
    
    Result.Vertices = PushWriteArray(&Assets->Arena, VertexData, Result.VertexCount, vertex_p3_n3, 0);
    Result.GDIHandle = Assets->Graphics->AllocateMesh(Assets->Graphics, Result.Vertices, Result.VertexCount*sizeof(vertex_p3_n3), GRAPHICS_VERTEX_FORMAT_P3_N3, 
                                                      Result.Indices, Result.IndexCount*sizeof(u32), GRAPHICS_INDEX_FORMAT_32_BIT);    
    ASSERT(Result.GDIHandle);
    
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