#ifndef ASSETS_H
#define ASSETS_H

//NOTE(EVERYONE): We only support pcm integer formats for now
struct audio_format
{    
    u16 ChannelCount;
    u32 SamplesPerSecond;
};

struct audio
{
    audio_format Format;
    u64 SampleCount;    
    i16* Samples;
};

struct mesh
{
    vertex_p3_n3* Vertices;
    u32* Indices;
    u32 VertexCount;
    u32 IndexCount;    
    i64 GDIHandle;
};

struct assets
{
    arena Arena;
    graphics* Graphics;
    
    mesh FBXBoxMesh;
    mesh BoxGraphicsMesh;
        
    triangle3D_mesh BoxTriangleMesh;
    audio TestAudio;
};



#endif