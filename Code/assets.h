#ifndef ASSETS_H
#define ASSETS_H

struct samples
{
    u64 Count;
    i16* Data;
};

struct audio
{
    u32 ChannelCount;
    samples Samples;    
};

inline b32 IsAssetValid(audio* Audio)
{
    b32 Result = (Audio->ChannelCount > 0 &&
                  Audio->Samples.Count > 0 &&
                  Audio->Samples.Data);
    return Result;
}

struct mesh
{
    void* Vertices;
    void* Indices;
    graphics_vertex_format VertexFormat;
    graphics_index_format IndexFormat;    
    u32 VertexCount;
    u32 IndexCount;    
    i64 GDIHandle;
};

inline b32 IsAssetValid(mesh* Mesh)
{
    b32 Result = (Mesh->Vertices && Mesh->Indices && 
                  (Mesh->VertexFormat != GRAPHICS_VERTEX_FORMAT_UNKNOWN) && 
                  (Mesh->IndexFormat != GRAPHICS_INDEX_FORMAT_UNKNOWN) &&
                  Mesh->VertexCount && Mesh->IndexCount);
    return Result;
}

struct walkable_triangle
{
    v3f P[3];    
    walkable_triangle* AdjTriangles[3];
};

struct walkable_mesh
{
    u32 TriangleCount;
    walkable_triangle* Triangles;
};

inline b32 IsAssetValid(walkable_mesh* Mesh)
{
    b32 Result = (Mesh->TriangleCount && Mesh->Triangles);
    return Result;
}

#define NO_PARENT_JOINT (u8)-1
struct joint
{
    u8 ParentIndex; //No parent is 255 or (u8)-1
    m4 ModelToJoint;
};

struct skeleton
{    
    joint* Joints;
    u32 JointCount;
};

inline b32 IsAssetValid(skeleton* Skeleton)
{
    b32 Result = (Skeleton->JointCount && Skeleton->Joints);
    return Result;
}

struct joint_pose
{
    quaternion Orientation;
    v3f Translation;
};

struct animation_frame
{
    joint_pose* JointPoses;
};

#define ANIMATION_HZ 0.03333333333333f
#define ANIMATION_FPS 30
struct animation_clip
{    
    u32 JointCount; 
    u32 FrameCount;
    animation_frame* Frames;        
};

inline b32 IsAssetValid(animation_clip* Clip)
{
    b32 Result = (Clip->JointCount && Clip->FrameCount && Clip->Frames);
    return Result;
}

struct texture
{
    void* Texels;
    v2i Dimensions;
    graphics_texture_format Format;
    graphics_texture_id GDIHandle;
};

inline b32 IsAssetValid(texture* Texture)
{
    b32 Result = (Texture->Texels && Texture->Dimensions.width && Texture->Dimensions.height);
    return Result;
}

struct convex_vertex;
struct convex_face;

struct convex_edge
{
    i32 Vertex;
    i32 EdgePair;
    i32 Face;
    i32 NextEdge;    
};

struct convex_vertex
{    
    v3f V;
    i32 Edge;    
};

struct convex_face
{
    i32 Edge;    
};

struct convex_hull
{
    u32 VertexCount;
    u32 EdgeCount;
    u32 FaceCount;
    
    convex_vertex* Vertices;
    convex_edge* Edges;
    convex_face* Faces;    
        
#if DEVELOPER_BUILD
    i64 GDIHandle;
#endif
};

inline b32 IsAssetValid(convex_hull* Hull)
{
    b32 Result = Hull->VertexCount && Hull->EdgeCount && Hull->FaceCount;
    return Result;
}

struct assets
{
    arena Storage;
    graphics* Graphics;
    
    mesh BoxMesh;
    walkable_mesh BoxWalkableMesh;
    
    mesh QuadGraphicsMesh;
    walkable_mesh QuadWalkableMesh;
    
    mesh FloorMesh;
    walkable_mesh FloorWalkableMesh;
    
    mesh TorusMesh;
    
    mesh PlayerMesh;
    
    texture TestMaterial0_Diffuse;    
    texture TestMaterial0_Normal;
    texture TestMaterial0_Specular;
    
    texture TestMaterial1_Diffuse;
    texture TestMaterial1_Normal;
    texture TestMaterial1_Specular;
    
    graphics_material Material_DiffuseC;
    graphics_material Material_DiffuseT;            
    graphics_material Material_DiffuseC_SpecularC;
    graphics_material Material_DiffuseC_SpecularT;
    graphics_material Material_DiffuseC_Normal;
    graphics_material Material_DiffuseT_SpecularC;
    graphics_material Material_DiffuseT_SpecularT;
    graphics_material Material_DiffuseT_Normal;
    graphics_material Material_DiffuseC_SpecularC_Normal;
    graphics_material Material_DiffuseT_SpecularT_Normal;
    graphics_material Material_DiffuseT_SpecularT_Normal_2;
    
    convex_hull BoxConvexHull;
    
#if 0 
    mesh TestSkeletonMesh;
    skeleton TestSkeleton;    
    animation_clip TestAnimation;
#endif
    //animation_clip IdleAnimation;
    //animation_clip MovingAnimation;
        
    audio TestAudio;
    audio TestAudio2;
};

#endif