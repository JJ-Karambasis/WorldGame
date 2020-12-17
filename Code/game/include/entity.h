#ifndef ENTITY_H
#define ENTITY_H

enum entity_type
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_STATIC,    
    ENTITY_TYPE_BUTTON,    
    ENTITY_TYPE_MOVABLE, 
    ENTITY_TYPE_COUNT
};

struct entity
{    
    entity_type Type;    
    ak_u64 ID;
    ak_u64 LinkID;
};

struct physics_object
{
    union
    {
        ak_sqtf Transform;
        struct
        {
            ak_v3f Position;
            ak_quatf Orientation;
            ak_v3f Scale;
        };
    };
    ak_v3f Velocity;
    ak_v3f MoveDelta;
    ak_u64 CollisionVolumeID;
};

struct graphics_object
{
    mesh_asset_id MeshID;
    material Material;
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

struct player
{            
    ak_v3f GravityVelocity;
};

struct movable
{
    ak_u64 ChildID;
    ak_u64 ParentID;
    ak_v3f GravityVelocity;
};

#endif
