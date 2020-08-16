void ParsePNG(asset_builder* AssetBuilder, string Path)
{           
    png_result PNG = PNG_Load(Path);
    if(PNG.IsValid())
    {
        ptr TextureSize = PNG.Width*PNG.Height*PNG.ComponentCount;
        
        list_entry<texture>* TextureLink = (list_entry<texture>*)PushSize(&AssetBuilder->AssetArena, TextureSize+sizeof(list_entry<texture>), Clear, 0);
        list_entry<texture_info>* TextureInfoLink = (list_entry<texture_info>*)PushStruct(&AssetBuilder->AssetArena, list_entry<texture_info>, Clear, 0);                
        
        b32 IsSRGB = false;        
        if((PNG.ComponentCount == 3) || (PNG.ComponentCount == 4))
        {
            b32 InvalidOption = true;
            
            while(InvalidOption)
            {
                ConsoleLog("Is file sRGB (0 no, 1 yes): ");
                int Option = getchar();
                
                if(Option == '0')
                {                        
                    InvalidOption = false;
                    IsSRGB = false;
                }
                else if(Option == '1')
                {
                    InvalidOption = false;
                    IsSRGB = true;
                }
                else
                {
                    ConsoleError("Please choose a valid option!");                        
                }
                
                getchar();
            }
        }
        
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
        Texture->Texels = (void*)(Texture+1);
        
        CopyMemory(Texture->Texels, PNG.Texels, TextureSize);
        
        PNG_Free(&PNG);
        
        AddToList(&AssetBuilder->TextureInfos, TextureInfoLink);
        AddToList(&AssetBuilder->Textures, TextureLink);        
    }
    else
    {
        ConsoleError("PNG file could not be loaded. ErrorMessage: %s. Skipping PNG", PNG_GetErrorMessage());
    }    
}