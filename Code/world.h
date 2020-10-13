#ifndef WORLD_H
#define WORLD_H

#define COLLISION_EVENT(name) void name(struct game* Game, struct entity* Entity, entity* CollidedEntity, ak_v3f Normal) 
typedef COLLISION_EVENT(collision_event_function);

enum entity_type
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_STATIC,    
    ENTITY_TYPE_RIGID_BODY,
    ENTITY_TYPE_PUSHABLE,
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

enum player_state
{
    PLAYER_STATE_NONE,
    PLAYER_STATE_JUMPING,
    PLAYER_STATE_PUSHING
};

struct pushing_object
{
    ak_u64 ID;
    world_id PlayerID;
    ak_v2f Direction;
};

struct player
{    
    player_state State;        
};

struct jumping_quad
{
    ak_v3f CenterP;
    ak_v2f Dimensions; 
    ak_color3f Color;
    world_id OtherQuad;    
};

typedef ak_pool<entity> entity_storage;
typedef ak_pool<pushing_object> pushing_object_storage;
typedef ak_pool<jumping_quad> jumping_quad_storage;

struct world
{
    pushing_object_storage PushingObjectStorage;  
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

global ak_f32 Global_PlayerAcceleration = 20.0f;
global ak_f32 Global_PlayerDamping = 5.0f;
global ak_f32 Global_Gravity = 10.0f;

#endif