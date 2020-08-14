inline b32 
ValidateVersion(asset_header Header)
{
    b32 Result = (Header.MajorVersion == ASSET_MAJOR_VERSION) && (Header.MinorVersion == ASSET_MINOR_VERSION);
    return Result;
}

platform_file_handle* LoadAssetFile(string AssetPath)
{
    platform_file_handle* FileHandle = Global_Platform->OpenFile(AssetPath.Data, PLATFORM_FILE_ATTRIBUTES_READ);
    if(!FileHandle)
    {
        //TODO(JJ): Diagnostic and error logging
        return NULL;
    }
    
    asset_header Header = {};
    Global_Platform->ReadFile(FileHandle, &Header, sizeof(Header), NO_OFFSET);
    
    BOOL_CHECK_AND_HANDLE(StringEquals(Header.Signature, ASSET_SIGNATURE), "Asset file header signature does not match. Cannot load asset file!");
    BOOL_CHECK_AND_HANDLE(ValidateVersion(Header), "Asset file is not the right version. Found %d.%d when engine supports %d.%d", Header.MajorVersion, Header.MinorVersion, ASSET_MAJOR_VERSION, ASSET_MINOR_VERSION); 
    BOOL_CHECK_AND_HANDLE(Header.MeshCount == MESH_ASSET_COUNT, "Mesh count does not match the asset header's mesh count. Found %d, when engine expected %d", Header.MeshCount, MESH_ASSET_COUNT);
    
    return FileHandle;
    
    handle_error:
    Global_Platform->CloseFile(FileHandle);
    return NULL;
}

platform_file_handle* GetAssetFile()
{
    return Global_Platform->AssetFile;
}

b32 LoadMeshInfo(arena* Arena, mesh_info* MeshInfo)
{
    //TODO(JJ): What constitutes a failure when loading mesh infos?
    
    platform_file_handle* File = GetAssetFile();
    Global_Platform->ReadFile(File, &MeshInfo->Header, sizeof(mesh_info_header), NO_OFFSET); 
    MeshInfo->Name = PushArray(Arena, MeshInfo->Header.NameLength+1, char, Clear, 0);
    MeshInfo->ConvexHulls = PushArray(Arena, MeshInfo->Header.ConvexHullCount, convex_hull, Clear, 0);
    
    Global_Platform->ReadFile(File, MeshInfo->Name, MeshInfo->Header.NameLength*sizeof(char), NO_OFFSET);
    MeshInfo->Name[MeshInfo->Header.NameLength] = 0;
    
    for(u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
    {
        convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;
        Global_Platform->ReadFile(File, &ConvexHull->Header, sizeof(ConvexHull->Header), NO_OFFSET);
        
        u32 ConvexHullDataSize = 0;
        ConvexHullDataSize += (ConvexHull->Header.VertexCount*sizeof(half_vertex));                
        ConvexHullDataSize += (ConvexHull->Header.EdgeCount*sizeof(half_edge));
        ConvexHullDataSize += (ConvexHull->Header.FaceCount*sizeof(half_face));
        
        void* ConvexHullData = PushSize(Arena, ConvexHullDataSize, Clear, 0);
        Global_Platform->ReadFile(File, ConvexHullData, ConvexHullDataSize, NO_OFFSET);
        
        ConvexHull->Vertices = (half_vertex*)ConvexHullData;
        ConvexHull->Edges = (half_edge*)(ConvexHull->Vertices + ConvexHull->Header.VertexCount);
        ConvexHull->Faces = (half_face*)(ConvexHull->Edges + ConvexHull->Header.EdgeCount);                
    }
    
    return true;
}

b32 LoadMeshInfos(arena* Arena, mesh_info* MeshInfos)
{
    for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = MeshInfos + MeshIndex;
        if(!LoadMeshInfo(Arena, MeshInfo))
        {
            //TODO(JJ): Output some diagnostics
            return false;
        }                
    }    
    return true;
}

b32 LoadAssetInfos(assets_2* Assets)
{
    if(!LoadMeshInfos(&Assets->AssetArena, Assets->MeshInfos))
    {
        //TODO(JJ): Output some diagnostics
        return false;
    }
    
    for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Assets->GraphicsMeshes[MeshIndex] = INVALID_GRAPHICS_MESH_ID;    
    
    return true;
}

mesh* LoadMesh(assets_2* Assets, mesh_asset_id ID)
{
    platform_file_handle* File = GetAssetFile();
    
    mesh_info* MeshInfo = Assets->MeshInfos + ID;            
    
    u32 MeshSize = GetMeshDataSize(MeshInfo);
    u32 AllocationSize = sizeof(mesh) + MeshSize;
    mesh* Mesh = (mesh*)Global_Platform->AllocateMemory(AllocationSize);
    
    Mesh->Vertices = (void*)&Mesh[1];
    Mesh->Indices = (void*)((u8*)Mesh->Vertices + GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount);
    
    Global_Platform->ReadFile(File, Mesh->Vertices, MeshSize, MeshInfo->Header.OffsetToData);
    
    return Mesh;
}