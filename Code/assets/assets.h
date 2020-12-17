#ifndef ASSETS_H
#define ASSETS_H
#include "asset_header.h"
#include "asset_types.h"

struct material_diffuse
{
    ak_bool IsTexture;
    union
    {
        texture_asset_id DiffuseID;
        ak_v3f Diffuse;
    };
};

struct material_normal
{
    ak_bool InUse;
    texture_asset_id NormalID;    
};

struct material_specular
{
    ak_bool InUse;
    ak_bool IsTexture;
    union
    {
        texture_asset_id SpecularID;
        ak_f32 Specular;
    };
    ak_i32 Shininess;
};

struct samples
{
    ak_u64 Count;
    ak_i16* Data;
};

struct audio
{
    ak_u32 ChannelCount;
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
    ak_arena* AssetArena;
    ak_file_handle* AssetFile;
    
    ak_hash_map<char*, mesh_asset_id> MeshNameMap;        
    ak_hash_map<char*, texture_asset_id> TextureNameMap;
    
    mesh_info* MeshInfos;
    mesh** Meshes;
    graphics_mesh_id* GraphicsMeshes;
    
    texture_info* TextureInfos;
    texture** Textures;
    graphics_texture_id* GraphicsTextures;
};

inline material_diffuse
CreateDiffuse(ak_v3f Diffuse)
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
CreateSpecular(ak_f32 Specular, ak_i32 Shininess)
{
    material_specular Result = {};
    Result.InUse = true;
    Result.Specular = Specular;
    Result.Shininess = Shininess;
    return Result;    
}

inline material_specular
CreateSpecular(texture_asset_id SpecularID, ak_i32 Shininess)
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
    CreateDiffuse(AK_Blue3()),
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

inline ak_bool 
MaterialDiffuseSlotsEqual(material_diffuse A, material_diffuse B)
{
    if(A.IsTexture == -1 || B.IsTexture == -1) return false;    
    if(A.IsTexture != B.IsTexture) return false;
    
    if(A.IsTexture) return A.DiffuseID == B.DiffuseID;            
    else return A.Diffuse == B.Diffuse;    
}

inline ak_bool
MaterialNormalSlotsEqual(material_normal A, material_normal B)
{
    if(A.InUse != B.InUse) return false;    
    if(A.InUse) return A.NormalID == B.NormalID;
    else return true;    
}

inline ak_bool
MaterialSpecularSlotsEqual(material_specular A, material_specular B)
{
    if(A.InUse != B.InUse) return false;
    
    if(A.InUse)
    {
        if(A.IsTexture != B.IsTexture) return false;
        if(A.Shininess != B.Shininess) return false;
        
        if(A.IsTexture) return A.SpecularID == B.SpecularID;
        else return A.Specular == B.Specular;
    }
    else return true;
}

inline ak_bool 
AreMaterialsEqual(material A, material B)
{
    ak_bool Result = (MaterialDiffuseSlotsEqual(A.Diffuse, B.Diffuse) && 
                      MaterialNormalSlotsEqual(A.Normal, B.Normal) && 
                      MaterialSpecularSlotsEqual(A.Specular, B.Specular));
    return Result;
}

#endif