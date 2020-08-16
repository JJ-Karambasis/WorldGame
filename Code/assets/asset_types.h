#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

global const char ASSET_SIGNATURE[] = "WGASSET";
global const u16 ASSET_MAJOR_VERSION = 1;
global const u16 ASSET_MINOR_VERSION = 0;

#pragma pack(push, 1)
struct asset_header
{
    const char Signature[ARRAYCOUNT(ASSET_SIGNATURE)];
    u16 MajorVersion;
    u16 MinorVersion;
    u32 MeshCount;
};

struct asset_info_header
{    
    u64 OffsetToData;    
};

struct mesh_info_header : public asset_info_header
{
    b32 IsSkeletalMesh;
    b32 IsIndexFormat32;
    u32 VertexCount;
    u32 IndexCount;    
    u32 ConvexHullCount;
    u32 NameLength;
};

struct convex_hull_header
{
    sqt  Transform;    
    u32 VertexCount;
    u32 EdgeCount;
    u32 FaceCount;    
};

struct texture_info_header : public asset_info_header
{
    u32 Width;
    u32 Height;
    u32 ComponentCount;    
    b32 IsSRGB;    
    u32 NameLength;
};

#pragma pack(pop)

struct mesh_info
{
    mesh_info_header Header;
    char* Name;
    struct convex_hull* ConvexHulls;
};

struct mesh
{
    void* Vertices;
    void* Indices;
};

struct half_edge
{
    i32 Vertex;
    i32 EdgePair;
    i32 Face;
    i32 NextEdge;    
};

struct half_vertex
{
    v3f V;
    i32 Edge;
};

struct half_face
{
    i32 Edge;
};

struct convex_hull
{
    convex_hull_header Header;    
    half_vertex* Vertices;
    half_edge* Edges;
    half_face* Faces;        
};

struct texture_info
{
    texture_info_header Header;
    char* Name;
};

struct texture
{
    void* Texels;
};

inline u32 
GetVertexStride(mesh_info* Info)
{
    u32 Result = Info->Header.IsSkeletalMesh ? sizeof(vertex_p3_n3_t4_uv_weights) : sizeof(vertex_p3_n3_t4_uv);
    return Result;
}

inline u32 
GetIndexStride(mesh_info* Info)
{
    u32 Result =  Info->Header.IsIndexFormat32 ? sizeof(u32) : sizeof(u16);
    return Result;
}

inline u32
GetMeshDataSize(mesh_info* MeshInfo)
{
    u32 Result = 0;
    
    u32 VertexStride = MeshInfo->Header.IsSkeletalMesh ? sizeof(vertex_p3_n3_t4_uv_weights) : sizeof(vertex_p3_n3_t4_uv);
    u32 IndexStride =  MeshInfo->Header.IsIndexFormat32 ? sizeof(u32) : sizeof(u16);
    
    Result += GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount;
    Result += GetIndexStride(MeshInfo)*MeshInfo->Header.IndexCount;
    return Result;
}

inline u32 
GetTextureDataSize(texture_info* TextureInfo)
{
    u32 Result = TextureInfo->Header.Width*TextureInfo->Header.Height*TextureInfo->Header.ComponentCount;
    return Result;
}

#endif