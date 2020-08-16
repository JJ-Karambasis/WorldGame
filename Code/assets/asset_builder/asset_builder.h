#ifndef ASSET_BUILDER_H
#define ASSET_BUILDER_H
#include <ak_common.h>
#include "../asset_types.h"

struct asset_builder
{
    arena AssetArena;
    
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

#endif