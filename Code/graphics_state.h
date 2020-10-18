#ifndef GRAPHICS_STATE_H
#define GRAPHICS_STATE_H

#define CAMERA_FIELD_OF_VIEW (AK_PI*0.3f)
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 50.0f

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

struct camera
{
    ak_v3f Target;
    ak_v3f SphericalCoordinates;    
};

struct jumping_quad_graphics_mesh 
{
    ak_u32 VertexCount;
    ak_u32 IndexCount;
    ak_vertex_p3* Vertices;
    ak_u16* Indices;    
    graphics_mesh_id MeshID;
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