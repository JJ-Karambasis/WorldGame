#ifndef FBX_H
#define FBX_H

#include "fbxsdk/fbxsdk.h"

global FbxManager* Global_FBXManager;

struct control_point_joint_data
{    
    u32 Count;
    f64 Weights[4];
    u8 Indices[4];
};

#define MAX_NODE_COUNT 1024
struct node_list
{        
    FbxNode* Ptr[MAX_NODE_COUNT];    
    u32 Count;
};

#define MAX_SKELETON_COUNT 64
struct skeleton_list
{
    u32 Count;
    struct fbx_skeleton* Ptr[MAX_SKELETON_COUNT];
};

struct fbx_skeleton
{
    u8 JointCount;
    FbxNode* Joints[256];
};

struct fbx_context
{
    node_list MeshNodes;
    skeleton_list Skeletons;
};

#endif