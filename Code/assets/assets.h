#ifndef ASSETS_2_H
#define ASSETS_2_H
#include "asset_header.h"
#include "asset_types.h"

struct assets_2
{
    arena AssetArena;
    hash_map<char*, mesh_asset_id> MeshNameMap;    
        
    mesh_info MeshInfos[MESH_ASSET_COUNT];
    mesh* Meshes[MESH_ASSET_COUNT];    
    graphics_mesh_id GraphicsMeshes[MESH_ASSET_COUNT];    
};

#endif