#ifndef WORLD_FILE_H
#define WORLD_FILE_H

global const ak_char WORLD_FILE_SIGNATURE[] = "WGWORLD";
global const ak_u16 WORLD_FILE_MAJOR_VERSION = 1;
global const ak_u16 WORLD_FILE_MINOR_VERSION = 0;

#pragma pack(push, 1)

struct world_file_header
{
    ak_char Signature[AK_Count(WORLD_FILE_SIGNATURE)];
    ak_u16 MajorVersion;
    ak_u16 MinorVersion;
    ak_u32 EntityCountWorldA;    
    ak_u32 EntityCountWorldB;
    ak_u32 PointLightCountWorldA;
    ak_u32 PointLightCountWorldB;
};

#pragma pack(pop)

struct file_entity
{
    entity_type   Type;
    ak_u32        LinkIndex;
    ak_sqtf       Transform;
    material      Material;
    mesh_asset_id MeshID;    
    ak_f32 Mass;
    ak_f32 Restitution;
};

#endif