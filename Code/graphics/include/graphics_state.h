#ifndef GRAPHICS_STATE_H
#define GRAPHICS_STATE_H

#define CAMERA_FIELD_OF_VIEW (AK_PI*0.3f)
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 50.0f

struct graphics_entity
{
    material      Material;
    mesh_asset_id MeshID;
    ak_m4f        Transform;
};

struct graphics_entity_array
{
    ak_u32 Count;
    graphics_entity* Ptr;
};

struct graphics_point_light_array
{
    ak_u32 Count;
    graphics_point_light* Ptr;
};

struct graphics_camera
{
    ak_v3f Target;
    ak_v3f SphericalCoordinates;    
};

struct graphics_state
{
    graphics_entity_array EntityArray;
    graphics_point_light_array PointLightArray;
    graphics_camera Camera;
};

#endif