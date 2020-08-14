#include "asset_loader.cpp"

b32 PopulateAssetMap(assets_2* Assets)
{
    for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = Assets->MeshInfos + MeshIndex;
        
        if(Assets->MeshNameMap.Find(MeshInfo->Name))
        {
            //TODO(JJ): Diagnostic and error logging. We found duplicate assets with the same key
            return false;
        }
        
        Assets->MeshNameMap.Insert(MeshInfo->Name, (mesh_asset_id)MeshIndex);        
    }
    
    return true;
}

b32 InitAssets(assets_2* Assets)
{
    Assets->AssetArena = CreateArena(MEGABYTE(1));    
    Assets->MeshNameMap = CreateHashMap<char*, mesh_asset_id>(MESH_ASSET_COUNT*10, &Assets->AssetArena);    
    
    if(!LoadAssetInfos(Assets))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    if(!PopulateAssetMap(Assets))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    return true;
}

inline mesh_info* 
GetMeshInfo(assets_2* Assets, mesh_asset_id ID)
{
    return Assets->MeshInfos + ID;    
}

inline u32
GetMeshIndexCount(assets_2* Assets, mesh_asset_id ID)
{
    return GetMeshInfo(Assets, ID)->Header.IndexCount;
}

inline mesh* 
GetMesh(assets_2* Assets, mesh_asset_id ID)
{
    return Assets->Meshes[ID];
}

inline graphics_mesh_id
GetGraphicsMeshID(assets_2* Assets, mesh_asset_id ID)
{
    return Assets->GraphicsMeshes[ID];
}
