#ifndef ASSET_BUILDER_H
#define ASSET_BUILDER_H
#include <ak_common.h>
#include "../asset_types.h"

struct mesh_pair
{
    mesh_info* MeshInfo;
    mesh* Mesh;
};

struct texture_pair
{
    texture_info* TextureInfo;
    texture* Texture;
};

struct asset_builder
{
    arena AssetArena;
    
    hash_map<char*, mesh_pair> MeshTable;
    hash_map<char*, texture_pair> TextureTable;
    
    list<mesh_info> MeshInfos;
    list<mesh> Meshes;             
    
    list<texture_info> TextureInfos;
    list<texture> Textures;
};

inline void 
ConsoleNewLine()
{
    fprintf(stdout, "\n");
}

inline void 
ConsoleLog(char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vfprintf(stdout, Format, Args);
    ConsoleNewLine();        
    va_end(Args);    
}

inline void
ConsoleError(char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    fprintf(stdout, "Error: ");
    vfprintf(stdout, Format, Args);
    ConsoleNewLine();        
    va_end(Args);
}

inline b32 
ConsoleTrueFalse(char* Message)
{        
    b32 Result = false;
    for(;;)
    {
        ConsoleLog("%s (0 no, 1 yes): ", Message);
        int Option = getchar();        
        if(Option == '0') { Result = false; break; }
        else if(Option == '1') { Result = true; break; }
        else ConsoleError("Please choose a valid option!");                                        
        getchar();
    }
    getchar();
    return Result;
}

#endif