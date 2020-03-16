#ifndef RIFF_H
#define RIFF_H

struct riff_iterator
{
    u8* At;
    u8* End;
};

#pragma pack(push, 1)

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

struct riff_chunk_header
{
    u32 ID;
    u32 Size;
};

struct riff_header
{
    u32 RiffID;
    u32 Size;
    u32 FileTypeID;
};

enum
{
    RIFFID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
};

#pragma pack(pop)

inline riff_iterator
ParseChunkAt(void *At, void *End)
{
    riff_iterator Iter;
    
    Iter.At = (u8 *)At;
    Iter.End = (u8 *)End;
    
    return Iter;
}

inline riff_iterator 
IterateRIFF(buffer Buffer, riff_header *Header)
{
    riff_iterator Iter = {};
    if(Buffer.Size >= sizeof(riff_header))
    {
        *Header = *(riff_header *)Buffer.Data;
        u8 *DataStart = Buffer.Data + sizeof(riff_header);
        Iter = ParseChunkAt(DataStart, DataStart + Header->Size - 4);
    }
    else
    {
        ClearStruct(Header, riff_header);
    }
    
    return Iter;
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
    riff_chunk_header *Chunk = (riff_chunk_header *)Iter.At;
    u32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(riff_chunk_header) + Size;    
    return Iter;
}

inline b32
IsValid(riff_iterator Iter)
{    
    b32 Result = (Iter.At < Iter.End);    
    return Result;
}

inline void *
GetChunkData(riff_iterator Iter)
{
    void *Result = (Iter.At + sizeof(riff_chunk_header));    
    return Result;
}

inline u32
GetType(riff_iterator Iter)
{
    riff_chunk_header *Chunk = (riff_chunk_header *)Iter.At;
    u32 Result = Chunk->ID;    
    return Result;
}

inline u32
GetChunkDataSize(riff_iterator Iter)
{
    riff_chunk_header *Chunk = (riff_chunk_header *)Iter.At;
    u32 Result = Chunk->Size;
    
    return Result;
}

#endif