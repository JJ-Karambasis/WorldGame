#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

texture PNG_LoadTexture(char* File, arena* Storage)
{   
    texture Result = {};
    
    i32 Channels;
    u8* Texels = stbi_load(File, &Result.Dimensions.width, &Result.Dimensions.height, &Channels, 4);
    
    BOOL_CHECK_AND_HANDLE(Texels, "Failed load %s\n", File);
    BOOL_CHECK_AND_HANDLE(Channels == 4, "Texture must have 4 channels. Found %d channels", Channels);        
    
    ptr TextureSizeInBytes = Result.Dimensions.width*Result.Dimensions.height*4;
    Result.Texels = PushSize(Storage, TextureSizeInBytes, Clear, 0);    
    CopyMemory(Result.Texels, Texels, TextureSizeInBytes);
    
    handle_error:    
    if(Texels)
        stbi_image_free(Texels);
    
    return Result;
}