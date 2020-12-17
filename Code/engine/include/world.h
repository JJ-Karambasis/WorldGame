#ifndef WORLD_H
#define WORLD_H

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

struct physics_objects
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

struct jumping_quad
{
    ak_u64 ID;
    ak_v3f CenterP;
    ak_v2f Dimensions; 
    ak_color3f Color;
    world_id OtherQuad;    
};

struct button_state
{
    ak_bool IsToggle;    
    ak_bool Collided;    
    ak_bool IsDown;    
};

struct world
{
    entity_storage EntityStorage[2];
    ak_array<ak_sqtf> OldTransforms[2];
    ak_array<physics_objects> PhysicObjects[2];
    camera OldCamera[2];
    camera CurrentCamera[2];
};

#define UserDataToIndex(data) ((ak_u32)(AK_SafeAddrToU32((ak_uaddr)(data))))
#define IndexToUserData(index) ((void*)(ak_uaddr)(index))

#endif
