platform_file_handle LoadAssetFile(string AssetPath)
{
    platform_file_handle FileHandle = FileOpen(AssetPath, PLATFORM_FILE_ATTRIBUTES_READ);
    if(!FileHandle.IsValid())
    {
        //TODO(JJ): Diagnostic and error logging
        return {};
    }
    
    asset_header Header = {};
    FileRead(&FileHandle, &Header, sizeof(Header), NO_OFFSET);
    
    BOOL_CHECK_AND_HANDLE(ValidateSignature(Header), "Asset file header signature does not match. Cannot load asset file!");
    BOOL_CHECK_AND_HANDLE(ValidateVersion(Header), "Asset file is not the right version. Found %d.%d when engine supports %d.%d", Header.MajorVersion, Header.MinorVersion, ASSET_MAJOR_VERSION, ASSET_MINOR_VERSION); 
    BOOL_CHECK_AND_HANDLE(Header.MeshCount == MESH_ASSET_COUNT, "Mesh count does not match the asset header's mesh count. Found %d, when engine expected %d", Header.MeshCount, MESH_ASSET_COUNT);
    BOOL_CHECK_AND_HANDLE(Header.TextureCount == TEXTURE_ASSET_COUNT, "Texture count does not match the asset header's texture count. Found %d, when engine expected %d", Header.TextureCount, TEXTURE_ASSET_COUNT);
    
    return FileHandle;
    
    handle_error:
    FileClose(&FileHandle);
    return {};
}

b32 LoadMeshInfo(arena* Arena, platform_file_handle* File, mesh_info* MeshInfo)
{
    //TODO(JJ): What constitutes a failure when loading mesh infos?
    
    FileRead(File, &MeshInfo->Header, sizeof(mesh_info_header), NO_OFFSET); 
    MeshInfo->Name = PushArray(Arena, MeshInfo->Header.NameLength+1, char, Clear, 0);
    MeshInfo->ConvexHulls = PushArray(Arena, MeshInfo->Header.ConvexHullCount, convex_hull, Clear, 0);
    
    FileRead(File, MeshInfo->Name, MeshInfo->Header.NameLength*sizeof(char), NO_OFFSET);
    MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
    
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        FileRead(File, &ConvexHull->Header, sizeof(ConvexHull->Header), NO_OFFSET);
        
        u32 ConvexHullDataSize = 0;
        ConvexHullDataSize += (ConvexHull->Header.VertexCount*sizeof(half_vertex));                
        ConvexHullDataSize += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
        ConvexHullDataSize += (ConvexHull->Header.FaceCount*sizeof(half_face));
        
        void* ConvexHullData = PushSize(Arena, ConvexHullDataSize, Clear, 0);
        FileRead(File, ConvexHullData, ConvexHullDataSize, NO_OFFSET);
        
        ConvexHull->Vertices = (half_vertex*)ConvexHullData;
        ConvexHull->Edges = (half_edge*)(ConvexHull->Vertices + ConvexHull->Header.VertexCount);
        ConvexHull->Faces = (half_face*)(ConvexHull->Edges + ConvexHull->Header.EdgeCount);                
    }
    
    return true;
}

b32 LoadMeshInfos(arena* Arena, platform_file_handle* File, mesh_info* MeshInfos)
{
    for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = MeshInfos + MeshIndex;
        if(!LoadMeshInfo(Arena, File, MeshInfo))
        {
            //TODO(JJ): Output some diagnostics
            return false;
        }                
    }    
    return true;
}

b32 LoadTextureInfo(arena* Arena, platform_file_handle* File, texture_info* TextureInfo)
{
    //TODO(JJ): What constitutes a failure when loading mesh infos?        
    FileRead(File, &TextureInfo->Header, sizeof(texture_info_header), NO_OFFSET);
    
    TextureInfo->Name = PushArray(Arena, TextureInfo->Header.NameLength+1, char, Clear, 0);
    FileRead(File, TextureInfo->Name, TextureInfo->Header.NameLength*sizeof(char), NO_OFFSET);
    TextureInfo->Name[TextureInfo->Header.NameLength] = 0;
    
    return true;
}

b32 LoadTextureInfos(arena* Arena, platform_file_handle* File, texture_info* TextureInfos)
{
    for(u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
    {
        texture_info* TextureInfo = TextureInfos + TextureIndex;
        if(!LoadTextureInfo(Arena, File, TextureInfo))
        {
            //TODO(JJ): Output some diagnostics
            return false;
        }
    }
    
    return true;
}

b32 LoadAssetInfos(assets* Assets, arena* Arena)
{
    
    
    if(!LoadMeshInfos(Arena, &Assets->AssetFile, Assets->MeshInfos))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    if(!LoadTextureInfos(Arena, &Assets->AssetFile, Assets->TextureInfos))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Assets->GraphicsMeshes[MeshIndex] = INVALID_GRAPHICS_MESH_ID;            
    
    for(u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)
        Assets->GraphicsTextures[TextureIndex] = INVALID_GRAPHICS_TEXTURE_ID;
    
    return true;
}

mesh* LoadMesh(assets* Assets, mesh_asset_id ID)
{
    ASSERT(ID != INVALID_MESH_ID);
    
    platform_file_handle* File = &Assets->AssetFile;
    
    mesh_info* MeshInfo = Assets->MeshInfos + ID;            
    
    u32 MeshSize = GetMeshDataSize(MeshInfo);
    u32 AllocationSize = sizeof(mesh) + MeshSize;
    mesh* Mesh = (mesh*)AllocateMemory(AllocationSize);
    
    Mesh->Vertices = (void*)&Mesh[1];
    Mesh->Indices = (void*)((u8*)Mesh->Vertices + GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount);
    
    FileRead(File, Mesh->Vertices, MeshSize, MeshInfo->Header.OffsetToData);
    
    Assets->Meshes[ID] = Mesh;
    
    return Mesh;
}

texture* LoadTexture(assets* Assets, texture_asset_id ID)
{
    ASSERT(ID != INVALID_TEXTURE_ID);
    
    platform_file_handle* File = &Assets->AssetFile;
    
    texture_info* TextureInfo = Assets->TextureInfos + ID;
    
    u32 TextureSize = GetTextureDataSize(TextureInfo);
    u32 AllocationSize = sizeof(texture) + TextureSize;
    
    texture* Texture = (texture*)AllocateMemory(AllocationSize);    
    Texture->Texels = (void*)(Texture+1);
    
    FileRead(File, Texture->Texels, TextureSize, TextureInfo->Header.OffsetToData);
    
    Assets->Textures[ID] = Texture;
    
    return Texture;
}