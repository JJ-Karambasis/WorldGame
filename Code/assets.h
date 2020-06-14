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

struct assets
{
    arena Storage;
    graphics* Graphics;
    
    mesh BoxGraphicsMesh;
    walkable_mesh BoxWalkableMesh;
    
    mesh QuadGraphicsMesh;
    walkable_mesh QuadWalkableMesh;
    
    mesh PlayerMesh;
    
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