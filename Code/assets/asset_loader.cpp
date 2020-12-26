ak_file_handle* LoadAssetFile(ak_string AssetPath)
{
    ak_file_handle* FileHandle = AK_OpenFile(AssetPath, AK_FILE_ATTRIBUTES_READ);
    if(!FileHandle)
    {
        //TODO(JJ): Diagnostic and error logging
        return 0;
    }
    
    asset_header Header = {};
    AK_ReadFile(FileHandle, &Header, sizeof(Header));    
    
    if(!ValidateSignature(Header))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_CloseFile(FileHandle);
        return 0;
    }
    
    if(!ValidateVersion(Header))
    {
        //TODO(JJ): Diangostic and error logging
        AK_CloseFile(FileHandle);
        return 0;
    }
    
    if(Header.MeshCount != MESH_ASSET_COUNT)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_CloseFile(FileHandle);
        return 0;
    }
    
    if(Header.TextureCount != TEXTURE_ASSET_COUNT)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_CloseFile(FileHandle);
        return 0;
    }
    
    return FileHandle;    
}

ak_bool LoadMeshInfo(ak_arena* Arena, ak_file_handle* FileHandle, mesh_info* MeshInfo)
{
    //TODO(JJ): What constitutes a failure when loading mesh infos?
    
    AK_ReadFile(FileHandle, &MeshInfo->Header, sizeof(mesh_info_header));
    MeshInfo->Name = Arena->PushArray<ak_char>(MeshInfo->Header.NameLength+1);
    MeshInfo->ConvexHulls = Arena->PushArray<convex_hull>(MeshInfo->Header.ConvexHullCount);
    
    AK_ReadFile(FileHandle, MeshInfo->Name, MeshInfo->Header.NameLength*sizeof(char));
    MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
    
    for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        AK_ReadFile(FileHandle, &ConvexHull->Header, sizeof(ConvexHull->Header));
        
        ak_u32 ConvexHullDataSize = 0;
        ConvexHullDataSize += (ConvexHull->Header.VertexCount*sizeof(half_vertex));                
        ConvexHullDataSize += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
        ConvexHullDataSize += (ConvexHull->Header.FaceCount*sizeof(half_face));
        
        void* ConvexHullData = Arena->Push(ConvexHullDataSize);
        AK_ReadFile(FileHandle, ConvexHullData, ConvexHullDataSize);
        
        ConvexHull->Vertices = (half_vertex*)ConvexHullData;
        ConvexHull->Edges = (half_edge*)(ConvexHull->Vertices + ConvexHull->Header.VertexCount);
        ConvexHull->Faces = (half_face*)(ConvexHull->Edges + ConvexHull->Header.EdgeCount);                
    }
    
    return true;
}

ak_bool LoadMeshInfos(ak_arena* Arena, ak_file_handle* FileHandle, mesh_info* MeshInfos)
{
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = MeshInfos + MeshIndex;
        if(!LoadMeshInfo(Arena, FileHandle, MeshInfo))
        {
            //TODO(JJ): Output some diagnostics
            return false;
        }                
    }    
    return true;
}

ak_bool LoadTextureInfo(ak_arena* Arena, ak_file_handle* FileHandle, texture_info* TextureInfo)
{
    //TODO(JJ): What constitutes a failure when loading mesh infos?        
    AK_ReadFile(FileHandle, &TextureInfo->Header, sizeof(texture_info_header));
    
    TextureInfo->Name = Arena->PushArray<ak_char>(TextureInfo->Header.NameLength+1);
    AK_ReadFile(FileHandle, TextureInfo->Name, TextureInfo->Header.NameLength*sizeof(char));
    TextureInfo->Name[TextureInfo->Header.NameLength] = 0;
    
    return true;
}

ak_bool LoadTextureInfos(ak_arena* Arena, ak_file_handle* FileHandle, texture_info* TextureInfos)
{
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
    {
        texture_info* TextureInfo = TextureInfos + TextureIndex;
        if(!LoadTextureInfo(Arena, FileHandle, TextureInfo))
        {
            //TODO(JJ): Output some diagnostics
            return false;
        }
    }
    
    return true;
}

ak_bool LoadAssetInfos(assets* Assets, ak_arena* Arena)
{        
    if(!LoadMeshInfos(Arena, Assets->AssetFile, Assets->MeshInfos))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    if(!LoadTextureInfos(Arena, Assets->AssetFile, Assets->TextureInfos))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Assets->GraphicsMeshes[MeshIndex] = INVALID_GRAPHICS_MESH_ID;            
    
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
        Assets->GraphicsTextures[TextureIndex] = INVALID_GRAPHICS_TEXTURE_ID;
    
    return true;
}

mesh* LoadMesh(assets* Assets, mesh_asset_id ID)
{
    AK_Assert(ID != INVALID_MESH_ID, "Cannot load an invalid mesh");
    
    ak_file_handle* FileHandle = Assets->AssetFile;
    
    mesh_info* MeshInfo = Assets->MeshInfos + ID;            
    
    
    ak_u32 MeshSize = GetMeshDataSize(MeshInfo);
    ak_u32 AllocationSize = sizeof(mesh) + MeshSize + sizeof(ak_v3f)*MeshInfo->Header.VertexCount;
    mesh* Mesh = (mesh*)AK_Allocate(AllocationSize);
    
    ak_u32 VertexStride = GetVertexStride(MeshInfo);
    Mesh->Vertices = (void*)&Mesh[1];
    Mesh->Indices = (void*)((ak_u8*)Mesh->Vertices + VertexStride*MeshInfo->Header.VertexCount);
    Mesh->Positions = (ak_v3f*)((ak_u8*)Mesh->Indices + GetIndexStride(MeshInfo)*MeshInfo->Header.IndexCount);
    AK_ReadFile(FileHandle, Mesh->Vertices, MeshSize, MeshInfo->Header.OffsetToData);
    
    for(ak_u32 VertexIndex = 0; VertexIndex < MeshInfo->Header.VertexCount; VertexIndex++)
    {
        Mesh->Positions[VertexIndex] = *(ak_v3f*)((ak_u8*)Mesh->Vertices + VertexStride*VertexIndex);
    }
    
    Assets->Meshes[ID] = Mesh;
    
    return Mesh;
}

texture* LoadTexture(assets* Assets, texture_asset_id ID)
{
    AK_Assert(ID != INVALID_TEXTURE_ID, "Cannot load an invalid texture");
    
    ak_file_handle* FileHandle = Assets->AssetFile;
    
    texture_info* TextureInfo = Assets->TextureInfos + ID;
    
    ak_u32 TextureSize = GetTextureDataSize(TextureInfo);
    ak_u32 AllocationSize = sizeof(texture) + TextureSize;
    
    texture* Texture = (texture*)AK_Allocate(AllocationSize);    
    Texture->Texels = (void*)(Texture+1);
    
    AK_ReadFile(FileHandle, Texture->Texels, TextureSize, TextureInfo->Header.OffsetToData);
    
    Assets->Textures[ID] = Texture;
    
    return Texture;
}