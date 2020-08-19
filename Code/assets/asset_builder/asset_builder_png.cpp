b32 AreTexturesIdentical(texture* NewTexture, texture_info* NewTextureInfo, texture* OldTexture, texture_info* OldTextureInfo)
{
    if(NewTextureInfo->Header.Width != OldTextureInfo->Header.Width)   return false;
    if(NewTextureInfo->Header.Height != OldTextureInfo->Header.Height) return false;
    if(NewTextureInfo->Header.ComponentCount != OldTextureInfo->Header.ComponentCount) return false;
    if(NewTextureInfo->Header.IsSRGB != OldTextureInfo->Header.IsSRGB) return false;
    
    u8* NewTexels = (u8*)NewTexture->Texels;
    u8* OldTexels = (u8*)OldTexture->Texels;
    
    u32 TotalBytes = NewTextureInfo->Header.Width*NewTextureInfo->Header.Height*NewTextureInfo->Header.ComponentCount;
    for(u32 ByteIndex = 0; ByteIndex < TotalBytes; ByteIndex++)
    {
        if(NewTexels[ByteIndex] != OldTexels[ByteIndex]) 
            return false;    
    }
    
    return true;
}

void ParsePNG(asset_builder* AssetBuilder, string Path)
{           
    png_result PNG = PNG_Load(Path);
    if(PNG.IsValid())
    {
        ptr TextureSize = PNG.Width*PNG.Height*PNG.ComponentCount;
        
        list_entry<texture>* TextureLink = (list_entry<texture>*)PushSize(&AssetBuilder->AssetArena, TextureSize+sizeof(list_entry<texture>), Clear, 0);
        list_entry<texture_info>* TextureInfoLink = PushStruct(&AssetBuilder->AssetArena, list_entry<texture_info>, Clear, 0);                
        
        b32 IsSRGB = false;        
        if((PNG.ComponentCount == 3) || (PNG.ComponentCount == 4))        
            IsSRGB = ConsoleTrueFalse("Is file SRGB");                                
        
        string Filename = GetFilenameWithoutExtension(Path);    
        
        texture_info* TextureInfo = &TextureInfoLink->Entry;
        TextureInfo->Header.Width          = PNG.Width;
        TextureInfo->Header.Height         = PNG.Height;
        TextureInfo->Header.ComponentCount = PNG.ComponentCount;
        TextureInfo->Header.IsSRGB         = IsSRGB;
        TextureInfo->Header.NameLength     = (u32)Filename.Length;
        TextureInfo->Name                  = PushArray(&AssetBuilder->AssetArena, Filename.Length+1, char, Clear, 0);    
        
        
        CopyMemory(TextureInfo->Name, Filename.Data, Filename.Length);
        TextureInfo->Name[Filename.Length] = 0;
        
        texture* Texture = &TextureLink->Entry;
        Texture->Texels = (void*)(TextureLink+1);
        
        CopyMemory(Texture->Texels, PNG.Texels, TextureSize);
        
        PNG_Free(&PNG);
        
        texture_pair Pair;
        if(AssetBuilder->TextureTable.Find(TextureInfo->Name, &Pair))
        {
            if(!AreTexturesIdentical(Texture, TextureInfo, Pair.Texture, Pair.TextureInfo))
            {
                b32 OverrideTexture = ConsoleTrueFalse("Duplicate textures found with the same name are not identical. Override the texture");
                if(OverrideTexture)
                {
                    list_entry<texture_info>* OldTextureInfo = (list_entry<texture_info>*)Pair.TextureInfo;
                    list_entry<texture>* OldTexture = (list_entry<texture>*)Pair.Texture;
                    
                    RemoveFromList(&AssetBuilder->TextureInfos, OldTextureInfo);
                    RemoveFromList(&AssetBuilder->Textures, OldTexture);
                    
                    AddToList(&AssetBuilder->TextureInfos, TextureInfoLink);
                    AddToList(&AssetBuilder->Textures, TextureLink);
                }
            }
        }
        else
        {            
            Pair = {TextureInfo, Texture};
            AssetBuilder->TextureTable.Insert(TextureInfo->Name, Pair);
                        
            AddToList(&AssetBuilder->TextureInfos, TextureInfoLink);
            AddToList(&AssetBuilder->Textures, TextureLink);        
        }
    
    }
    else
    {
        ConsoleError("PNG file could not be loaded. ErrorMessage: %s. Skipping PNG", PNG_GetErrorMessage());
    }    
}