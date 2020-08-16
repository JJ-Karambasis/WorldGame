#include "asset_loader.cpp"

b32 PopulateAssetMap(assets* Assets)
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
    
    for(u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
    {
        texture_info* TextureInfo = Assets->TextureInfos + TextureIndex;
        if(Assets->TextureNameMap.Find(TextureInfo->Name))
        {            
            //TODO(JJ): Diagnostic and error logging
            return false;
        }
        
        Assets->TextureNameMap.Insert(TextureInfo->Name, (texture_asset_id)TextureIndex);
    }
    
    return true;
}

b32 InitAssets(assets* Assets)
{
    Assets->AssetArena = CreateArena(MEGABYTE(1));    
    Assets->MeshNameMap = CreateHashMap<char*, mesh_asset_id>(MESH_ASSET_COUNT*10, &Assets->AssetArena);    
    Assets->TextureNameMap = CreateHashMap<char*, texture_asset_id>(TEXTURE_ASSET_COUNT*10, &Assets->AssetArena);
    
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
GetMeshInfo(assets* Assets, mesh_asset_id ID)
{
    return Assets->MeshInfos + ID;    
}

inline u32
GetMeshIndexCount(assets* Assets, mesh_asset_id ID)
{
    return GetMeshInfo(Assets, ID)->Header.IndexCount;
}

inline mesh* 
GetMesh(assets* Assets, mesh_asset_id ID)
{
    return Assets->Meshes[ID];
}

inline graphics_mesh_id
GetGraphicsMeshID(assets* Assets, mesh_asset_id ID)
{
    return Assets->GraphicsMeshes[ID];
}

inline texture_info* 
GetTextureInfo(assets* Assets, texture_asset_id ID)
{
    return Assets->TextureInfos + ID;    
}

inline texture*
GetTexture(assets* Assets, texture_asset_id ID)
{
    return Assets->Textures[ID];
}

inline graphics_texture_id
GetGraphicsTextureID(assets* Assets, texture_asset_id ID)
{
    return Assets->GraphicsTextures[ID];
}