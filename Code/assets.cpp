mesh LoadGraphicsMesh(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));
    
    fbx_context FBX = FBX_LoadFile(File);
    mesh Result = FBX_LoadFirstMesh(&FBX, &Assets->Storage);    
    if(Result.Vertices && Result.Indices)
    {        
        Result.GDIHandle = Assets->Graphics->AllocateMesh(Assets->Graphics, Result.Vertices, GetVertexBufferSize(Result.VertexFormat, Result.VertexCount), Result.VertexFormat,
                                                          Result.Indices, GetIndexBufferSize(Result.IndexFormat, Result.IndexCount), Result.IndexFormat);
    }    
    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

walkable_mesh LoadWalkableMesh(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    walkable_mesh Result = FBX_LoadFirstWalkableMesh(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

skeleton LoadSkeleton(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    skeleton Result = FBX_LoadFirstSkeleton(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

animation_clip LoadAnimation(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "fbx"));    
    fbx_context FBX = FBX_LoadFile(File);
    animation_clip Result = FBX_LoadFirstAnimation(&FBX, &Assets->Storage);    
    ASSERT(IsAssetValid(&Result));
    return Result;
}

audio 
LoadAudio(assets* Assets, char* File)
{
    ASSERT(StringEquals(GetFileExtension(File), "wav"));
    audio Result = WAV_LoadAudio(File, &Assets->Storage);
    ASSERT(IsAssetValid(&Result));
    return Result;    
}

texture
LoadTexture(assets* Assets, char* File, b32 sRGB)
{
    ASSERT(StringEquals(GetFileExtension(File), "png"));
    texture Result = PNG_LoadTexture(File, &Assets->Storage);
    if(Result.Texels)
    {
        graphics_sampler_info SamplerInfo = {};
        SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
        SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
        
        Result.Format = sRGB ? ConvertToSRGBFormat(Result.Format) : Result.Format;        
        Result.GDIHandle = Assets->Graphics->AllocateTexture(Assets->Graphics, Result.Texels, Result.Dimensions, Result.Format, &SamplerInfo);
    }
    ASSERT(IsAssetValid(&Result));
    return Result;
}

graphics_material CreateMaterial_DCon(assets* Assets, c3 Diffuse)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    return Result;
}

graphics_material CreateMaterial_DTex(assets* Assets, texture* Diffuse)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DCon_NTex(assets* Assets, c3 Diffuse, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DTex_NTex(assets* Assets, texture* Diffuse, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DCon_SCon(assets* Assets, c3 Diffuse, f32 Specular, i32 Shininess)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    Result.Specular = CreateSpecularMaterialSlot(Specular, Shininess);
    return Result;
}

graphics_material CreateMaterial_DCon_SCon_NTex(assets* Assets, c3 Diffuse, f32 Specular, i32 Shininess, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    Result.Specular = CreateSpecularMaterialSlot(Specular, Shininess);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DCon_STex(assets* Assets, c3 Diffuse, texture* Specular, i32 Shininess)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    Result.Specular = CreateSpecularMaterialSlot(Specular->GDIHandle, Shininess);
    return Result;
}

graphics_material CreateMaterial_DCon_STex_NTex(assets* Assets, c3 Diffuse, texture* Specular, i32 Shininess, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse);
    Result.Specular = CreateSpecularMaterialSlot(Specular->GDIHandle, Shininess);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DTex_SCon(assets* Assets, texture* Diffuse, f32 Specular, i32 Shininess)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    Result.Specular = CreateSpecularMaterialSlot(Specular, Shininess);
    return Result;
}

graphics_material CreateMaterial_DTex_SCon_NTex(assets* Assets, texture* Diffuse, f32 Specular, i32 Shininess, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    Result.Specular = CreateSpecularMaterialSlot(Specular, Shininess);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}

graphics_material CreateMaterial_DTex_STex(assets* Assets, texture* Diffuse, texture* Specular, i32 Shininess)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    Result.Specular = CreateSpecularMaterialSlot(Specular->GDIHandle, Shininess);
    return Result;
}

graphics_material CreateMaterial_DTex_STex_NTex(assets* Assets, texture* Diffuse, texture* Specular, i32 Shininess, texture* Normal)
{
    graphics_material Result = {};
    Result.Diffuse = CreateDiffuseMaterialSlot(Diffuse->GDIHandle);
    Result.Specular = CreateSpecularMaterialSlot(Specular->GDIHandle, Shininess);
    Result.Normal = CreateNormalMaterialSlot(Normal->GDIHandle);
    return Result;
}