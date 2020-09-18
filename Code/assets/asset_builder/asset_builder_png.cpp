ak_bool AreTexturesIdentical(texture* NewTexture, texture_info* NewTextureInfo, texture* OldTexture, texture_info* OldTextureInfo)
{
    if(NewTextureInfo->Header.Width != OldTextureInfo->Header.Width)   return false;
    if(NewTextureInfo->Header.Height != OldTextureInfo->Header.Height) return false;
    if(NewTextureInfo->Header.ComponentCount != OldTextureInfo->Header.ComponentCount) return false;
    if(NewTextureInfo->Header.IsSRGB != OldTextureInfo->Header.IsSRGB) return false;
    
    ak_u8* NewTexels = (ak_u8*)NewTexture->Texels;
    ak_u8* OldTexels = (ak_u8*)OldTexture->Texels;
    
    ak_u32 TotalBytes = NewTextureInfo->Header.Width*NewTextureInfo->Header.Height*NewTextureInfo->Header.ComponentCount;
    for(ak_u32 ByteIndex = 0; ByteIndex < TotalBytes; ByteIndex++)
    {
        if(NewTexels[ByteIndex] != OldTexels[ByteIndex]) 
            return false;    
    }
    
    return true;
}

void ParsePNG(asset_builder* AssetBuilder, ak_string Path)
{   
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    ak_png PNG = AK_LoadPNG(GlobalArena, Path);    
    if(PNG.IsValid())
    {
        ak_u32 TextureSize = PNG.Width*PNG.Height*PNG.ComponentCount;
        
        ak_link_entry<texture>* TextureLink = (ak_link_entry<texture>*)AssetBuilder->AssetArena->Push(TextureSize+sizeof(ak_link_entry<texture>));
        ak_link_entry<texture_info>* TextureInfoLink = AssetBuilder->AssetArena->Push<ak_link_entry<texture_info>>();
        
        ak_bool IsSRGB = false;        
        if((PNG.ComponentCount == 3) || (PNG.ComponentCount == 4))        
            IsSRGB = ConsoleTrueFalse("Is file SRGB");                                
        
        ak_string Filename = AK_GetFilenameWithoutExtension(Path);    
        
        texture_info* TextureInfo = &TextureInfoLink->Entry;
        TextureInfo->Header.Width          = PNG.Width;
        TextureInfo->Header.Height         = PNG.Height;
        TextureInfo->Header.ComponentCount = PNG.ComponentCount;
        TextureInfo->Header.IsSRGB         = IsSRGB;
        TextureInfo->Header.NameLength     = (ak_u32)Filename.Length;
        TextureInfo->Name                  = AssetBuilder->AssetArena->PushArray<char>(Filename.Length+1);
        
        
        AK_MemoryCopy(TextureInfo->Name, Filename.Data, Filename.Length);
        TextureInfo->Name[Filename.Length] = 0;
        
        texture* Texture = &TextureLink->Entry;
        Texture->Texels = (void*)(TextureLink+1);
        
        AK_MemoryCopy(Texture->Texels, PNG.Texels, TextureSize);
        
        texture_pair* Pair = AssetBuilder->TextureTable.Find(TextureInfo->Name);
        if(Pair)
        {
            if(!AreTexturesIdentical(Texture, TextureInfo, Pair->Texture, Pair->TextureInfo))
            {
                ak_bool OverrideTexture = ConsoleTrueFalse("Duplicate textures found with the same name are not identical. Override the texture");
                if(OverrideTexture)
                {
                    ak_link_entry<texture_info>* OldTextureInfo = (ak_link_entry<texture_info>*)Pair->TextureInfo;
                    ak_link_entry<texture>* OldTexture = (ak_link_entry<texture>*)Pair->Texture;
                    
                    AssetBuilder->TextureInfos.Remove(OldTextureInfo);
                    AssetBuilder->Textures.Remove(OldTexture);
                    
                    AssetBuilder->TextureInfos.Push(TextureInfoLink);
                    AssetBuilder->Textures.Push(TextureLink);                    
                }
            }
        }
        else
        {            
            texture_pair TexturePair = {TextureInfo, Texture};
            AssetBuilder->TextureTable.Insert(TextureInfo->Name, TexturePair);
            
            AssetBuilder->TextureInfos.Push(TextureInfoLink);
            AssetBuilder->Textures.Push(TextureLink);            
        }
        
    }
    else
    {
        ConsoleError("PNG file could not be loaded. ErrorMessage: %s. Skipping PNG", AK_PNG_GetErrorMessage().Data);
    }    
    
    GlobalArena->EndTemp(&TempArena);
}