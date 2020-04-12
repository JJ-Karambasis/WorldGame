#ifndef ASSETS_H
#define ASSETS_H

struct graphics_vertex
{
    v4f Position;
    v4f Normal;
};

struct graphics_vertex_array
{
    u32 Count;
    graphics_vertex* Ptr;
};

struct graphics_index_array
{
    u32 Count;
    u16* Ptr; //CONFIRM(JJ): Do we only need 16 bit indices?
};

struct graphics_mesh
{
    graphics_vertex_array Vertices;
    graphics_index_array Indices;    
};

inline ptr GetVerticesSizeInBytes(graphics_mesh* Mesh)
{
    ptr Result = Mesh->Vertices.Count*sizeof(graphics_vertex);
    return Result;
}

inline ptr GetIndicesSizeInBytes(graphics_mesh* Mesh)
{
    ptr Result = Mesh->Indices.Count*sizeof(u16);
    return Result;
}

inline graphics_vertex* GetVertices(graphics_mesh* Mesh)
{
    return Mesh->Vertices.Ptr;
}

inline u16* GetIndices(graphics_mesh* Mesh)
{
    return Mesh->Indices.Ptr;
}

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

struct assets
{
    arena Arena;
    
    graphics_mesh   BoxGraphicsMesh;
    triangle3D_mesh BoxTriangleMesh;
    audio TestAudio;
};

#endif