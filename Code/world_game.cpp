#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "entity.cpp"
#include "collision_detection.cpp"
//#include "player.cpp"
#include "graphics.cpp"

PUZZLE_COMPLETE_CALLBACK(DespawnWallCompleteCallback)
{
    entity_id* IDs = (entity_id*)UserData;
    
    FreeEntity(Game, IDs[0]);
    FreeEntity(Game, IDs[1]);
}

inline goal_rect 
CreateGoalRect(v3f RectMin, v3f RectMax, u32 WorldIndex)
{
    goal_rect Result = {WorldIndex, CreateRect3D(RectMin, RectMax)};
    return Result;
}

inline goal_rect 
CreateGoalRect(v3f RectMin, v3f RectMax, u32 WorldIndex, f32 Padding)
{
    goal_rect Result = CreateGoalRect(RectMin-Padding, RectMax+Padding, WorldIndex);
    return Result;
}

void LoadTestLevel(game* Game)
{    
    //TODO(JJ): Load entity data at runtime    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {           
        Game->EntityStorage[WorldIndex] = CreatePool<entity>(Game->GameStorage, 512);        
        Game->CollisionVolumeStorage[WorldIndex] = CreatePool<collision_volume>(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity*4);        
        Game->PrevTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);        
        Game->CurrentTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);
        Game->SimStates[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sim_state, Clear, 0);
        
        v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
        capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);        
        entity_id PlayerID = CreatePlayerEntity(Game, WorldIndex, V3(), V3(), Global_PlayerMaterial, &PlayerCapsule);
        
        game_camera* Camera = Game->CurrentCameras + WorldIndex;        
        Camera->Target = GetEntityPosition(Game, PlayerID);        
        Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));        
        Camera->FieldOfView = TO_RAD(65.0f);                        
        Camera->ZNear = CAMERA_ZNEAR;
        Camera->ZFar = CAMERA_ZFAR;        
        
        Game->PrevCameras[WorldIndex] = *Camera;
    }
    
    CreateStaticEntity(Game, 0, V3(0.0f, 0.0f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_FLOOR, Global_Material0);                               
    CreateStaticEntity(Game, 0, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-1.6f, -5.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f),  MESH_ASSET_ID_BOX,   Global_Material1);                
    
    
    Game->JumpingQuads[0].CenterP = V3(-1.0f, 0.0f, 0.0f);
    Game->JumpingQuads[0].Dimensions = V2(1.0f, 2.0f);
    
    Game->JumpingQuads[1].CenterP = V3(-4.0f, 0.0f, 0.0f);
    Game->JumpingQuads[1].Dimensions = V2(1.0f, 2.0f);
    
    Game->JumpingQuads[0].OtherQuad = &Game->JumpingQuads[1];
    Game->JumpingQuads[1].OtherQuad = &Game->JumpingQuads[0];
}

extern "C"
EXPORT GAME_INITIALIZE(Initialize)
{
    SET_DEVELOPER_CONTEXT(DevContext);
    Global_Platform = Platform;
        
    SetDefaultArena(Global_Platform->TempArena);
    SetGlobalErrorStream(Global_Platform->ErrorStream);
    
    arena GameStorage = CreateArena(MEGABYTE(32));            
    assets* Assets = InitAssets(&GameStorage);
    if(!Assets)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }        
    
    game* Game = PushStruct(&GameStorage, game, Clear, 0);
    Game->_Internal_GameStorage_ = GameStorage;
    Game->GameStorage = &Game->_Internal_GameStorage_;
    
    Game->Input       = Input;
    Game->AudioOutput = AudioOutput;
    Game->Assets      = Assets;
    
    //TODO(JJ): Load world/entity data at runtime    
    LoadTestLevel(Game);
    
    return Game;
}

inline f32 GetLinearDamp(f32 dt, f32 Damping)
{
    //CONFIRM(JJ): Should the damping constant be configurable per entity?
    f32 Result = 1.0f / (1.0f + dt * Damping);
    return Result;
}

void PlayerCollisionEvent(game* Game, entity* Player, collision_event* CollisionEvent)
{
    if(IsEntityState(Player, ENTITY_STATE_JUMPING))
        SetEntityState(Player, ENTITY_STATE_NONE);    
}

extern "C"
EXPORT GAME_FIXED_TICK(FixedTick)
{    
    f32 dt = Game->dtFixed;
    
    platform_time Start = WallClock();
    
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {               
                sim_state* SimState = GetSimState(Game, Entity->ID);                
                SimState->Velocity += SimState->Acceleration*dt;
                
                if(!IsEntityState(Entity, ENTITY_STATE_JUMPING))
                    SimState->Velocity.xy *= GetLinearDamp(dt, Global_PlayerDamping);                                                    
            } break;
        }
    }
    
#define VERY_CLOSE_DISTANCE 1e-4f
    
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                collision_event CollisionEvent = {};
                
                sim_state* SimState = GetSimState(Game, Entity->ID);        
                sqt* EntityTransform = GetEntityTransform(Game, Entity->ID);
                
                SimState->MoveDelta = V3(SimState->Velocity.xy*dt);                                
                
                //CONFIRM(JJ): I think we can limit the iterations to 3
                for(u32 Iterations = 0; Iterations < 4; Iterations++)
                {
                    f32 DeltaLength = Magnitude(SimState->MoveDelta);
                    
                    if(DeltaLength > VERY_CLOSE_DISTANCE)
                    {            
                        v3f TargetPosition = EntityTransform->Translation + SimState->MoveDelta;                           
                        continuous_collision_result CollisionResult = DetectStaticContinuousCollisions(Game, Entity->ID);
                        
                        if(IsInvalidEntityID(CollisionResult.HitEntityID))
                        {
                            EntityTransform->Translation = TargetPosition;                            
                            break;
                        }
                        else
                        {   
                            penetration* Penetration = &CollisionResult.Penetration;
                            
                            EntityTransform->Translation += SimState->MoveDelta*CollisionResult.t;
                            EntityTransform->Translation += Penetration->Normal*1e-4f;
                            
                            SimState->MoveDelta = TargetPosition - EntityTransform->Translation;
                            
                            SimState->MoveDelta -= Dot(SimState->MoveDelta, Penetration->Normal)*Penetration->Normal;
                            SimState->Velocity -= Dot(SimState->Velocity, Penetration->Normal)*Penetration->Normal;                
                            
                            CollisionEvent = CreateCollisionEvent(CollisionResult.HitEntityID, *Penetration);                            
                        }
                    }
                    else
                    {
                        break;
                    }                    
                }
                
                SimState->MoveDelta = V3(0, 0, SimState->Velocity.z*dt);
                
                //CONFIRM(JJ): Same as above, can we limit the iterations to 3?
                for(u32 Iterations = 0; Iterations < 4; Iterations++)
                {            
                    f32 DeltaLength = Magnitude(SimState->MoveDelta);
                    
                    if(DeltaLength > VERY_CLOSE_DISTANCE)
                    {            
                        v3f TargetPosition = EntityTransform->Translation + SimState->MoveDelta;                
                        continuous_collision_result CollisionResult = DetectStaticContinuousCollisions(Game, Entity->ID);
                        
                        if(IsInvalidEntityID(CollisionResult.HitEntityID))
                        {
                            EntityTransform->Translation = TargetPosition;                                
                            break;
                        }
                        else
                        {   
                            penetration* Penetration = &CollisionResult.Penetration;
                            
                            EntityTransform->Translation += SimState->MoveDelta*CollisionResult.t;                
                            
                            SimState->MoveDelta = TargetPosition - EntityTransform->Translation;
                            
                            SimState->MoveDelta -= Dot(SimState->MoveDelta, Penetration->Normal)*Penetration->Normal;
                            SimState->Velocity -= Dot(SimState->Velocity, Penetration->Normal)*Penetration->Normal;                
                            
                            CollisionEvent = CreateCollisionEvent(CollisionResult.HitEntityID, *Penetration);                            
                        }
                    }
                    else
                    {
                        break;
                    }
                }                   
                
                if(!IsInvalidEntityID(CollisionEvent.HitEntityID))
                    PlayerCollisionEvent(Game, Entity, &CollisionEvent);  
                
                Game->CurrentCameras[Entity->ID.WorldIndex].Target = EntityTransform->Translation;
            } break;
        }
    }    
}

extern "C"
EXPORT GAME_TICK(Tick)
{    
    f32 dt = Game->dt;
    
    input* Input = Game->Input;
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                sim_state* SimState = GetSimState(Game, Entity->ID);                
                v3f Position = GetEntityPosition(Game, Entity->ID);                
                
                SimState->Acceleration = {};
                
                if(!IsEntityState(Entity, ENTITY_STATE_JUMPING))                
                {
                    for(u32 JumpingQuadIndex = 0; JumpingQuadIndex < ARRAYCOUNT(Game->JumpingQuads); JumpingQuadIndex++)
                    {
                        jumping_quad* JumpingQuad = Game->JumpingQuads + JumpingQuadIndex;
                        v2f HalfDim = JumpingQuad->Dimensions*0.5f;
                        
                        v2f Min = JumpingQuad->CenterP.xy - HalfDim;
                        v2f Max = JumpingQuad->CenterP.xy + HalfDim;
                        
                        c3 QuadColor = Red3();
                        if(Position.xy >= Min && Position.xy <= Max)
                        {
                            if(Abs(Position.z - JumpingQuad->CenterP.z) < 1e-2f)
                            {
                                if(IsPressed(Input->Action))
                                {
                                    jumping_quad* TargetQuad = JumpingQuad->OtherQuad;
                                    
                                    v2f XDirection = TargetQuad->CenterP.xy-JumpingQuad->CenterP.xy;
                                    f32 Displacement = Magnitude(XDirection);
                                    XDirection /= Displacement;                    
                                    
                                    f32 InitialVelocity = Sqrt(Displacement*Global_Gravity);
                                    
                                    SimState->Velocity.xy = (InitialVelocity*SQRT2_2*XDirection);
                                    SimState->Velocity.z = InitialVelocity*SQRT2_2;
                                    
                                    SetEntityState(Entity, ENTITY_STATE_JUMPING);
                                    
                                    break;                    
                                    
                                }                
                                QuadColor = Yellow3();
                            }
                        }       
                        
                        DEBUG_DRAW_QUAD(JumpingQuad->CenterP, Global_WorldZAxis, JumpingQuad->Dimensions, QuadColor);    
                    }                    
                    
                    if(IsDown(Input->MoveForward))
                        SimState->Acceleration.y = 1.0f;
                    
                    if(IsDown(Input->MoveBackward))
                        SimState->Acceleration.y = -1.0f;
                    
                    if(IsDown(Input->MoveRight))
                        SimState->Acceleration.x = 1.0f;
                    
                    if(IsDown(Input->MoveLeft))
                        SimState->Acceleration.x = -1.0f;
                    
                    if(SimState->Acceleration.xy != 0)
                        SimState->Acceleration.xy = Normalize(SimState->Acceleration.xy) * Global_PlayerAcceleration;                    
                }                                   
                
                SimState->Acceleration.z -= Global_Gravity;
            } break;
        }
    }
    
}

extern "C"
EXPORT GAME_RENDER(Render)
{
    UpdateRenderBuffer(&Game->RenderBuffer, Graphics, Graphics->RenderDim);        
    view_settings ViewSettings = GetViewSettings(&GraphicsState->Camera);
    PushWorldShadingCommands(Graphics, Game->RenderBuffer, &ViewSettings, Game->Assets, GraphicsState->GraphicsObjects);
    PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Game->RenderBuffer->Resolution);
}