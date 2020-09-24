#ifndef GRAPHICS_STATE_H
#define GRAPHICS_STATE_H

#define CAMERA_FIELD_OF_VIEW (AK_PI*0.3f)
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 500.0f

struct graphics_entity
{
    ak_u64        ID;
    material      Material;
    mesh_asset_id MeshID;
    ak_m4f        Transform;
    void*         UserData;
};

struct point_light
{    
    ak_u64 ID;
    ak_v3f Position; 
    ak_f32 Radius;
    ak_color3f Color;
    ak_f32 Intensity;
    ak_bool On;
};

struct camera_transform
{
    ak_v3f Translation;
    ak_m3f Orientation;
};

struct camera
{
    ak_v3f Target;
    ak_v3f SphericalCoordinates;
    
    ak_f32 FieldOfView;
    ak_f32 ZNear;
    ak_f32 ZFar;
};

typedef ak_pool<graphics_entity> graphics_entity_storage;
typedef ak_pool<point_light> point_light_storage;

struct graphics_state
{    
    graphics_render_buffer* RenderBuffer;                
    camera Camera;
    graphics_entity_storage GraphicsEntityStorage;
    point_light_storage     PointLightStorage;    
};

#endif