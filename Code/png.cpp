#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

texture PNG_LoadTexture(char* File, arena* Storage)
{   
    texture Result = {};
    
    i32 Channels;
    u8* Texels = stbi_load(File, &Result.Dimensions.width, &Result.Dimensions.height, &Channels, 0);
    
    BOOL_CHECK_AND_HANDLE(Texels, "Failed load %s\n", File);
    
    switch(Channels)
    {
        case 4:
        {
            Result.Format = GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8;
        } break;
        
        case 3:
        {
            Result.Format = GRAPHICS_TEXTURE_FORMAT_R8G8B8;
        } break;
        
        case 1:
        {
            Result.Format = GRAPHICS_TEXTURE_FORMAT_R8;
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    ptr TextureSizeInBytes = Result.Dimensions.width*Result.Dimensions.height*Channels;
    Result.Texels = PushSize(Storage, TextureSizeInBytes, Clear, 0);    
    CopyMemory(Result.Texels, Texels, TextureSizeInBytes);
    
    handle_error:    
    if(Texels)
        stbi_image_free(Texels);
    
    return Result;
}