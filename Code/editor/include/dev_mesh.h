#ifndef DEV_MESH_H
#define DEV_MESH_H

struct dev_mesh
{
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
    ak_u32 VertexCount;
    void* Vertices;
    void* Indices;    
};

struct dev_slim_mesh
{
    graphics_mesh_id MeshID;
    ak_u32 IndexCount;
};

struct dev_capsule_mesh
{    
    graphics_mesh_id MeshID;
    ak_u32 CapIndexCount;
    ak_u32 CapVertexCount;
    ak_u32 BodyIndexCount;    
};


#endif
