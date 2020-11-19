#ifndef ASSET_HEADER_H
#define ASSET_HEADER_H
//NOTE(EVERYONE): This header file was generated by the AssetBuilder

#define INVALID_MESH_ID    ((mesh_asset_id)-1)
#define INVALID_TEXTURE_ID ((texture_asset_id)-1)

enum mesh_asset_id
{
	MESH_ASSET_ID_BOX,
	MESH_ASSET_ID_SPHERE,
	MESH_ASSET_ID_BUTTON,
	MESH_ASSET_ID_FLOOR,
	MESH_ASSET_ID_WALL,
	MESH_ASSET_ID_RAMP,
	MESH_ASSET_ID_PLAYER,
	MESH_ASSET_COUNT
};

enum texture_asset_id
{
	TEXTURE_ASSET_ID_TESTMATERIAL0_NORMAL,
	TEXTURE_ASSET_ID_TESTMATERIAL0_SPECULAR,
	TEXTURE_ASSET_ID_TESTMATERIAL1_DIFFUSE,
	TEXTURE_ASSET_ID_TESTMATERIAL1_NORMAL,
	TEXTURE_ASSET_ID_TESTMATERIAL1_SPECULAR,
	TEXTURE_ASSET_ID_TESTMATERIAL0_DIFFUSE,
	TEXTURE_ASSET_COUNT
};

#endif
