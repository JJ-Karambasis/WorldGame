#include "asset_loader.cpp"

ak_bool PopulateAssetMap(assets* Assets)
{
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = Assets->MeshInfos + MeshIndex;        
        if(Assets->MeshNameMap.Find(MeshInfo->Name))
        {
            //TODO(JJ): Diagnostic and error logging. We found duplicate assets with the same key
            return false;
        }
        
        Assets->MeshNameMap.Insert(MeshInfo->Name, (mesh_asset_id)MeshIndex);        
    }
    
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
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

assets* InitAssets(ak_arena* Storage, ak_string AssetPath)
{        
    ak_file_handle* AssetFile = LoadAssetFile(AssetPath);
    if(!AssetFile)
        return NULL;
    
    assets* Assets = Storage->Push<assets>();    
    Assets->AssetFile = AssetFile;
    
    Assets->MeshNameMap = AK_CreateHashMap<char*, mesh_asset_id>(8191);
    Assets->TextureNameMap = AK_CreateHashMap<char*, texture_asset_id>(8191);
    
    Assets->MeshInfos = Storage->PushArray<mesh_info>(MESH_ASSET_COUNT);
    Assets->Meshes = Storage->PushArray<mesh*>(MESH_ASSET_COUNT);
    Assets->GraphicsMeshes = Storage->PushArray<graphics_mesh_id>(MESH_ASSET_COUNT);
    
    Assets->TextureInfos = Storage->PushArray<texture_info>(TEXTURE_ASSET_COUNT);
    Assets->Textures = Storage->PushArray<texture*>(TEXTURE_ASSET_COUNT);
    Assets->GraphicsTextures = Storage->PushArray<graphics_texture_id>(TEXTURE_ASSET_COUNT);
    
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

inline ak_u32
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