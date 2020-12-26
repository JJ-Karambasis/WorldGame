#include "game.h"

#include <assets.cpp>
#include <src/graphics_state.cpp>

#include "game_common_source.cpp"

#define VERY_CLOSE_DISTANCE 0.001f

extern "C"
AK_EXPORT GAME_STARTUP(Game_Startup)
{
    game* Game = (game*)AK_Allocate(sizeof(game));
    if(!Game)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Game->Scratch = AK_CreateArena(AK_Megabyte(4));
    Game->Graphics = Graphics;
    Game->Assets = Assets;
    Game->dtFixed = TARGET_DT;
    
    Game->Update = Game_Update;
    Game->Shutdown = Game_Shutdown;
    Game->WorldShutdownCommon = Game_WorldShutdownCommon;
    
    Game->Cameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    Game->Cameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    Game->LoopAccum = InitGameLoopAccum(TARGET_DT);
    
    return Game;
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta)
{
    return AK_Max(Delta-VERY_CLOSE_DISTANCE, 0.0f);
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta, ak_f32 t)
{
    return AK_Max(Delta*t-VERY_CLOSE_DISTANCE, 0.0f);
}

void Game_PlayerMovementUpdate(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_f32 dt)
{
    world* World = Game->World;
    physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(ID));
    PhysicsObject->MoveDelta = PhysicsObject->Velocity*dt;
    
    broad_phase BroadPhase = BroadPhase_Begin(World, WorldIndex);
    collision_detection CollisionDetection = CollisionDetection_Begin(BroadPhase, Game->Scratch);
    
    if(AK_Magnitude(PhysicsObject->MoveDelta) > VERY_CLOSE_DISTANCE)
    {
        for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
        {
            ccd_contact ContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID);
            if(!ContactCCD.Intersected)
            {
                PhysicsObject->Position += PhysicsObject->MoveDelta;
                break;
            }
            
            contact* Contact = &ContactCCD.Contact;
            
            ak_v3f Normal = -Contact->Normal;
            ak_f32 tHit = ContactCCD.t;
            
            PhysicsObject->Velocity -= AK_Dot(Normal, PhysicsObject->Velocity)*Normal;
            
            ak_f32 MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
            if(AK_EqualZeroEps(MoveDeltaDistance))
                break;
            
            ak_v3f MoveDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;                                                                                                        
            ak_f32 HitDeltaDistance = GetDeltaClamped(MoveDeltaDistance, tHit);
            
            ak_v3f Destination = PhysicsObject->Transform.Translation + PhysicsObject->MoveDelta;
            ak_v3f NewBasePoint = PhysicsObject->Transform.Translation + MoveDirection*HitDeltaDistance;                        
            
            ak_planef SlidingPlane = AK_Plane(NewBasePoint, Normal);
            ak_v3f NewDestination = Destination - AK_SignDistance(SlidingPlane, Destination)*SlidingPlane.Normal;  
            
            PhysicsObject->MoveDelta = NewDestination - NewBasePoint;        
            
            PhysicsObject->Transform.Translation = NewBasePoint;
            if(AK_Magnitude(PhysicsObject->MoveDelta) < VERY_CLOSE_DISTANCE)                                                    
                break;                                    
        }
    }
    
    PhysicsObject->MoveDelta = {};
}

void Game_GravityMovementUpdate(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_v3f GravityVelocity, ak_f32 dt)
{
    world* World = Game->World;
    physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(ID));
    
    broad_phase BroadPhase = BroadPhase_Begin(World, WorldIndex);
    collision_detection CollisionDetection = CollisionDetection_Begin(BroadPhase, Game->Scratch);
    
    PhysicsObject->MoveDelta = GravityVelocity*dt;
    
    if(AK_Magnitude(PhysicsObject->MoveDelta) > VERY_CLOSE_DISTANCE)
    {
        for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
        {
            ccd_contact ContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID);
            if(!ContactCCD.Intersected)
            {
                PhysicsObject->Position += PhysicsObject->MoveDelta;
                break;
            }
            
            ak_f32 MoveDistance = AK_Magnitude(PhysicsObject->MoveDelta);
            ak_v3f Direction = PhysicsObject->MoveDelta/MoveDistance;
            
            ak_f32 NearestDistance = MoveDistance*ContactCCD.t;                        
            ak_f32 ShortenDistance = AK_Max(NearestDistance-VERY_CLOSE_DISTANCE, 0.0f);
            
            contact* Contact = &ContactCCD.Contact;
            
            if(AK_Dot(-Contact->Normal, AK_ZAxis()) < 0.7f)
            {                            
                ak_v3f Destination = PhysicsObject->Transform.Translation + PhysicsObject->MoveDelta;
                ak_v3f NewBasePoint = PhysicsObject->Transform.Translation + Direction*ShortenDistance;                        
                
                ak_planef SlidingPlane = AK_Plane(NewBasePoint, -Contact->Normal);
                ak_v3f NewDestination = Destination - AK_SignDistance(SlidingPlane, Destination)*SlidingPlane.Normal;       
                
                PhysicsObject->MoveDelta = NewDestination - NewBasePoint;
                
                PhysicsObject->Transform.Translation = NewBasePoint;
                if(AK_Magnitude(PhysicsObject->MoveDelta) < VERY_CLOSE_DISTANCE)                                                    
                    break;                        
            }
            else
            {                                                        
                PhysicsObject->Transform.Translation += Direction*ShortenDistance;                         
                if(ShortenDistance == 0.0f)
                    break;
            }
        }
    }
    
    PhysicsObject->MoveDelta = {};
}

extern "C"
AK_EXPORT GAME_UPDATE(Game_Update)
{
    world* World = Game->World;
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
        ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
        ak_array<ak_sqtf>* OldTransforms = &World->OldTransforms[WorldIndex];
        
        for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
        {
            ak_u64 ID = EntityStorage->IDs[EntityIndex];
            if(AK_PoolIsAllocatedID(ID))
            {
                ak_u32 Index = AK_PoolIndex(ID);
                OldTransforms->Set(Index, PhysicsObjects->Get(Index)->Transform);
            }
        }
    }
    
    World->Update(Game);
    
    ak_f32 dt = Game->dtFixed;
    input* Input = &Game->Input;
    
    if(IsPressed(Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;
    
    ak_v2f InputDirection = {};
    if(IsDown(Input->MoveForward))
        InputDirection.y = 1.0f;
    
    if(IsDown(Input->MoveBackward))
        InputDirection.y = -1.0f;
    
    if(IsDown(Input->MoveRight))
        InputDirection.x = 1.0f;
    
    if(IsDown(Input->MoveLeft))
        InputDirection.x = -1.0f;
    
    if(InputDirection != 0.0f)
        InputDirection = AK_Normalize(InputDirection);    
    
    const ak_f32 PlayerAcceleration = 23.0f;
    const ak_f32 Gravity = 20.0f;
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
        ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
        
        AK_ForEach(Entity, EntityStorage)
        {
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    ak_u32 Index = AK_PoolIndex(Entity->ID);
                    physics_object* PhysicsObject = PhysicsObjects->Get(Index);
                    
                    if(WorldIndex == Game->CurrentWorldIndex)
                        PhysicsObject->Velocity += AK_V3(InputDirection*PlayerAcceleration)*dt;
                    
                    PhysicsObject->Velocity *= (1.0f/(1.0f + 6.0f*dt));
                    Game_PlayerMovementUpdate(Game, WorldIndex, Entity->ID, dt);
                } break;
            }
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<entity>* EntityStorage =  &World->EntityStorage[WorldIndex];
        ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
        
        AK_ForEach(Entity, EntityStorage)
        {
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    player* Player = &World->Players[WorldIndex];
                    Player->GravityVelocity.z -= Gravity*dt;
                    Player->GravityVelocity *= (1.0f/(1.0f+3.0f*dt));
                    
                    Game_GravityMovementUpdate(Game, WorldIndex, Entity->ID, Player->GravityVelocity, dt);
                } break;
            }
        }
    }
}

WORLD_SHUTDOWN(Game_WorldShutdownCommon)
{
    AK_DeletePool(&Game->World->EntityStorage[0]);
    AK_DeletePool(&Game->World->EntityStorage[1]);
    AK_DeletePool(&Game->World->PointLightStorage[0]);
    AK_DeletePool(&Game->World->PointLightStorage[1]);
    AK_DeleteArray(&Game->World->OldTransforms[0]);
    AK_DeleteArray(&Game->World->OldTransforms[1]);
    AK_DeleteArray(&Game->World->PhysicsObjects[0]);
    AK_DeleteArray(&Game->World->PhysicsObjects[1]);
    AK_DeleteArray(&Game->World->GraphicsObjects[0]);
    AK_DeleteArray(&Game->World->GraphicsObjects[1]);
    AK_DeletePool(&Game->World->CollisionVolumeStorage);
}

extern "C"
AK_EXPORT GAME_SHUTDOWN(Game_Shutdown)
{
    Game->World->Shutdown(Game);
    AK_DeleteArena(Game->Scratch);
    AK_Free(Game);
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>