
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
            ak_f32 __pad0__;
            ak_quatf Orientation;
            ak_v3f   Scale;
            ak_f32 __pad1__;
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
    ak_m4f Transform;
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

struct button_state : public button
{
    ak_bool IsToggled;    
};

struct id_list
{
    ak_u32  Capacity;
    ak_u32  Size;
    ak_u64* IDs; 
    ak_bool* IndexAllocated;
};

struct movable
{
    id_list ChildIDs;
    id_list ParentIDs;
    ak_v3f GravityVelocity;
};

void DeleteEntity(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_bool ProcessLink=true);
entity* CreateEntity(game* Game, ak_u32 WorldIndex, entity_type Type, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, 
                     mesh_asset_id MeshID, material Material);
entity* CreateStaticEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Scale, ak_quatf Orientation, mesh_asset_id MeshID, material Material);
entity* CreateButtonEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Dimensions, 
                           material Material, ak_bool IsToggled);
entity* CreateMovableEntity(game* Game, ak_u32 WorldIndex, ak_v3f Position, ak_v3f Dimensions, 
                            material Material);

#endif
