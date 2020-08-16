#ifndef FBX_H
#define FBX_H

#include "fbxsdk/fbxsdk.h"

#define FBX_HASH_SIZE 8101
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
    FbxScene* Scene;
    node_list MeshNodes;
    node_list ConvexHullNodes;
    skeleton_list Skeletons;    
};

struct vertex_p3_n3_index
{
    vertex_p3_n3 Vertex;
    u32 Index;
};

struct vertex_p3_n3_uv_index
{
    vertex_p3_n3_uv Vertex;
    u32 Index;
};

inline u64 Hash(vertex_p3_n3_index Data, u64 TableSize)
{
    u64 Result = Hash(Data.Vertex, TableSize);    
    return Result;
}

inline u64 Hash(vertex_p3_n3_uv_index Data, u64 TableSize)
{
    u64 Result = Hash(Data.Vertex, TableSize);
    return Result;
}

inline b32 operator!=(vertex_p3_n3_index Left, vertex_p3_n3_index Right)
{
    b32 Result = Left.Vertex != Right.Vertex;    
    return Result;
}

inline b32 operator!=(vertex_p3_n3_uv_index Left, vertex_p3_n3_uv_index Right)
{
    b32 Result = Left.Vertex != Right.Vertex;
    return Result;
}

struct edge_triangle_entry
{
    u32 EdgeIndex;
    walkable_triangle* Triangle;
};

#endif