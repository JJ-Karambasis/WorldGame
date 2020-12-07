#ifndef WORLD_H
#define WORLD_H

#define COLLISION_EVENT(name) void name(struct game* Game, struct entity* Entity, entity* CollidedEntity, ak_v3f Normal) 
typedef COLLISION_EVENT(collision_event_function);

enum entity_type
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_STATIC,    
    ENTITY_TYPE_RIGID_BODY,        
    ENTITY_TYPE_BUTTON,    
    ENTITY_TYPE_MOVABLE, 
    ENTITY_TYPE_COUNT
};

struct world_id
{
    ak_u64 ID;
    ak_u32 WorldIndex;    
    
    
    inline ak_bool IsValid()
    {
        ak_bool Result = (ID != 0) && ((WorldIndex == 0) || (WorldIndex == 1));
        return Result;
    }
};

struct dual_world_id
{
    world_id A;
    world_id B;    
};

struct entity
{    
    entity_type Type;    
    world_id ID;
    world_id LinkID;         
    sim_entity_id SimEntityID;                 
    ak_u64 GraphicsEntityID;    
    void* UserData;    
    
    collision_event_function* OnCollision;
};

struct player
{            
    ak_v3f GravityVelocity;
};

struct movable
{
    world_id ChildID;
    world_id ParentID;
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

typedef ak_pool<entity> entity_storage;
typedef ak_pool<jumping_quad> jumping_quad_storage;
typedef ak_pool<button_state> button_state_storage;
typedef ak_pool<movable> movable_storage;

struct world
{    
    movable_storage MovableStorage;
    button_state_storage ButtonStateStorage;    
    jumping_quad_storage JumpingQuadStorage[2];
    entity_storage EntityStorage[2];    
    ak_array<ak_sqtf> OldTransforms[2];
    ak_array<ak_sqtf> NewTransforms[2];    
    camera OldCameras[2];    
    camera NewCameras[2];        
    simulation Simulations[2];
    graphics_state GraphicsStates[2];        
};

inline world_id MakeWorldID(ak_u64 ID, ak_u32 WorldIndex) { world_id Result = {ID, WorldIndex}; return Result; }
inline world_id InvalidWorldID() { return MakeWorldID(0, (ak_u32)-1); }
inline ak_bool AreEqualIDs(world_id A, world_id B) { return (A.ID == B.ID) && (A.WorldIndex == B.WorldIndex); }

#define UserDataToIndex(data) ((ak_u32)(AK_SafeAddrToU32((ak_uaddr)(data))))
#define IndexToUserData(index) ((void*)(ak_uaddr)(index))

COLLISION_EVENT(OnPlayerCollision);
COLLISION_EVENT(OnCollisionStub) { }

global ak_f32 Global_PlayerAcceleration = 23.0f;
global ak_f32 Global_PlayerDamping = 5.0f;
global ak_f32 Global_Gravity = 20.0f;

#endif