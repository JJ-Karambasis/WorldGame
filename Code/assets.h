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

struct assets
{
    arena Storage;
    graphics* Graphics;
    
    mesh BoxGraphicsMesh;
    mesh TestSkeletonMesh;
    skeleton TestSkeleton;
    
    animation_clip TestAnimation;
    
    //animation_clip IdleAnimation;
    //animation_clip MovingAnimation;
    
    triangle3D_mesh BoxTriangleMesh;
    audio TestAudio;
};



#endif