#ifndef ASSETS_2_H
#define ASSETS_2_H
#include "asset_header.h"
#include "asset_types.h"

struct material_diffuse
{
    b32 IsTexture;
    union
    {
        texture_asset_id DiffuseID;
        v3f Diffuse;
    };
};

struct material_normal
{
    b32 InUse;
    texture_asset_id NormalID;    
};

struct material_specular
{
    b32 InUse;
    b32 IsTexture;
    union
    {
        texture_asset_id SpecularID;
        f32 Specular;
    };
    i32 Shininess;
};

struct samples
{
    u64 Count;
    i16* Data;
};

struct audio
{
    u32 ChannelCount;
    samples Samples;    
};

struct material
{
    material_diffuse Diffuse;
    material_normal Normal;
    material_specular Specular;
};

struct assets
{        
    hash_map<char*, mesh_asset_id> MeshNameMap;        
    hash_map<char*, texture_asset_id> TextureNameMap;
    
    mesh_info* MeshInfos;
    mesh** Meshes;
    graphics_mesh_id* GraphicsMeshes;
    
    texture_info* TextureInfos;
    texture** Textures;
    graphics_texture_id* GraphicsTextures;
};

inline material_diffuse
CreateDiffuse(v3f Diffuse)
{
    material_diffuse Result = {};
    Result.Diffuse = Diffuse;
    return Result;
}

inline material_diffuse
CreateDiffuse(texture_asset_id DiffuseID)
{
    material_diffuse Result = {};
    Result.IsTexture = true;
    Result.DiffuseID = DiffuseID;
    return Result;
}

inline material_specular
CreateSpecular(f32 Specular, i32 Shininess)
{
    material_specular Result = {};
    Result.InUse = true;
    Result.Specular = Specular;
    Result.Shininess = Shininess;
    return Result;    
}

inline material_specular
CreateSpecular(texture_asset_id SpecularID, i32 Shininess)
{
    material_specular Result = {};
    Result.InUse = true;
    Result.IsTexture = true;
    Result.SpecularID = SpecularID;
    Result.Shininess = Shininess;
    return Result;
}

inline material_specular
InvalidSpecular()
{
    material_specular Result = {};
    Result.Shininess = -1;
    Result.SpecularID = INVALID_TEXTURE_ID;
    return Result;
}

inline material_normal
CreateNormal(texture_asset_id NormalID)
{
    material_normal Result = {};
    Result.InUse = true;
    Result.NormalID = NormalID;
    return Result;
}

inline material_normal
InvalidNormal()
{
    material_normal Result = {};
    Result.NormalID = INVALID_TEXTURE_ID;
    return Result;
}

global material Global_PlayerMaterial
{
    CreateDiffuse(Blue3()),
    InvalidNormal(),
    CreateSpecular(0.5f, 8)
};

global material Global_Material0
{
    CreateDiffuse(TEXTURE_ASSET_ID_TESTMATERIAL0_DIFFUSE),    
    CreateNormal(TEXTURE_ASSET_ID_TESTMATERIAL0_NORMAL),
    CreateSpecular(TEXTURE_ASSET_ID_TESTMATERIAL0_SPECULAR, 8)
};

global material Global_Material1
{
    CreateDiffuse(TEXTURE_ASSET_ID_TESTMATERIAL1_DIFFUSE),    
    CreateNormal(TEXTURE_ASSET_ID_TESTMATERIAL1_NORMAL),
    CreateSpecular(TEXTURE_ASSET_ID_TESTMATERIAL1_SPECULAR, 8)
};

#endif