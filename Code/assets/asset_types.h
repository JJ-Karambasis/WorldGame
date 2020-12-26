#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

global const ak_char ASSET_SIGNATURE[] = "WGASSET";
global const ak_u16 ASSET_MAJOR_VERSION = 1;
global const ak_u16 ASSET_MINOR_VERSION = 1;

#pragma pack(push, 1)
struct asset_header
{
    ak_char Signature[AK_Count(ASSET_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u32 MeshCount;
    ak_u32 TextureCount;
};

struct asset_info_header
{    
    ak_u64 OffsetToData;    
};

struct mesh_info_header : public asset_info_header
{
    ak_bool IsSkeletalMesh;
    ak_bool IsIndexFormat32;
    ak_u32 VertexCount;
    ak_u32 IndexCount;    
    ak_u32 ConvexHullCount;
    ak_u32 NameLength;
};

struct convex_hull_header
{
    ak_sqtf  Transform;    
    ak_u32 VertexCount;
    ak_u32 EdgeCount;
    ak_u32 FaceCount;    
};

struct texture_info_header : public asset_info_header
{
    ak_u32 Width;
    ak_u32 Height;
    ak_u32 ComponentCount;    
    ak_bool IsSRGB;    
    ak_u32 NameLength;
};

#pragma pack(pop)

struct mesh_info
{
    mesh_info_header Header;
    ak_char* Name;
    struct convex_hull* ConvexHulls;
};

struct mesh
{
    void* Vertices;
    void* Indices;
    ak_v3f* Positions;
};

struct half_edge
{
    ak_i32 Vertex;
    ak_i32 EdgePair;
    ak_i32 Face;
    ak_i32 NextEdge;    
};

struct half_vertex
{
    ak_v3f V;
    ak_i32 Edge;
};

struct half_face
{
    ak_i32 Edge;
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
    ak_char* Name;
};

struct texture
{
    void* Texels;
};

inline ak_u32 
GetVertexStride(mesh_info* Info)
{
    ak_u32 Result = Info->Header.IsSkeletalMesh ? sizeof(ak_vertex_p3_n3_uv_w) : sizeof(ak_vertex_p3_n3_uv);
    return Result;
}

inline ak_u32 
GetIndexStride(mesh_info* Info)
{
    ak_u32 Result =  Info->Header.IsIndexFormat32 ? sizeof(ak_u32) : sizeof(ak_u16);
    return Result;
}

inline ak_u32
GetMeshDataSize(mesh_info* MeshInfo)
{
    ak_u32 Result = 0;
    
    Result += GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount;
    Result += GetIndexStride(MeshInfo)*MeshInfo->Header.IndexCount;
    return Result;
}

inline ak_u32 
GetTextureDataSize(texture_info* TextureInfo)
{
    ak_u32 Result = TextureInfo->Header.Width*TextureInfo->Header.Height*TextureInfo->Header.ComponentCount;
    return Result;
}

inline ak_bool 
ValidateVersion(asset_header Header)
{
    ak_bool Result = (Header.MajorVersion == ASSET_MAJOR_VERSION) && (Header.MinorVersion == ASSET_MINOR_VERSION);
    return Result;
}

inline ak_bool ValidateSignature(asset_header Header)
{
    ak_bool Result = AK_StringEquals(Header.Signature, ASSET_SIGNATURE);
    return Result;
}

inline ak_bool
operator!=(half_vertex& Left, half_vertex& Right)
{
    ak_bool Result = (Left.V != Right.V) || (Left.Edge != Right.Edge);
    return Result;
}

inline ak_bool 
operator!=(half_edge& Left, half_edge& Right)
{
    ak_bool Result = (Left.Vertex != Right.Vertex) || (Left.EdgePair != Right.EdgePair) || (Left.Face != Right.Face) || (Left.NextEdge != Right.NextEdge);
    return Result;
}

inline ak_bool 
operator!=(half_face& Left, half_face& Right)
{
    ak_bool Result = Left.Edge != Right.Edge;
    return Result;
}

#endif