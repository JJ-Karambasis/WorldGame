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

assets* InitAssets(arena* Storage, string AssetPath)
{    
    assets* Assets = PushStruct(Storage, assets, Clear, 0);
    
    Assets->AssetFile = LoadAssetFile(AssetPath);
    if(!Assets->AssetFile.IsValid())
        return NULL;
    
    Assets->MeshNameMap = CreateHashMap<char*, mesh_asset_id>(MESH_ASSET_COUNT*10, StringEquals, Storage);    
    Assets->TextureNameMap = CreateHashMap<char*, texture_asset_id>(TEXTURE_ASSET_COUNT*10, StringEquals, Storage);        
    
    Assets->MeshInfos = PushArray(Storage, MESH_ASSET_COUNT, mesh_info, Clear, 0);
    Assets->Meshes = PushArray(Storage, MESH_ASSET_COUNT, mesh*, Clear, 0);
    Assets->GraphicsMeshes = PushArray(Storage, MESH_ASSET_COUNT, graphics_mesh_id, Clear, 0);
    
    Assets->TextureInfos = PushArray(Storage, TEXTURE_ASSET_COUNT, texture_info, Clear, 0);
    Assets->Textures = PushArray(Storage, TEXTURE_ASSET_COUNT, texture*, Clear, 0);
    Assets->GraphicsTextures = PushArray(Storage, TEXTURE_ASSET_COUNT, graphics_texture_id, Clear, 0);
    
    if(!LoadAssetInfos(Assets, Storage))
    {
        //TODO(JJ): Output some diagnostics
        return NULL;
    }
    
    if(!PopulateAssetMap(Assets))
    {
        //TODO(JJ): Output some diagnostics
        return NULL;
    }
    
    return Assets;
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