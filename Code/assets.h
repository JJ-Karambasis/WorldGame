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

struct walkable_mesh
{
    u32 TriangleCount;
    triangle3D* Triangles;
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
};



#endif