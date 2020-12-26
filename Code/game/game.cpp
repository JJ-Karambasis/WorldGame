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

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_FilterStaticEntitiesFunc)
{
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    ak_bool AreStatic = (EntityStorage->Get(Pair.EntityA)->Type == ENTITY_TYPE_STATIC ||
                         EntityStorage->Get(Pair.EntityB)->Type == ENTITY_TYPE_STATIC);
    return !AreStatic;
}

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_FilterOnlyStaticEntitiesFunc)
{
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    ak_bool AreStatic = (EntityStorage->Get(Pair.EntityA)->Type == ENTITY_TYPE_STATIC ||
                         EntityStorage->Get(Pair.EntityB)->Type == ENTITY_TYPE_STATIC);
    return AreStatic;
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
            ccd_contact ContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID, 
                                                            BroadPhase_FilterOnlyStaticEntitiesFunc);
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
            ccd_contact ContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID, BroadPhase_FilterOnlyStaticEntitiesFunc);
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
            if(ContactCCD.t == 0.0f && Contact->Penetration > 0)
                ShortenDistance -= Contact->Penetration+VERY_CLOSE_DISTANCE;
            
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
    
    if(IsPressed(&Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;
    
    ak_v2f InputDirection = {};
    if(IsDown(&Input->MoveForward))
        InputDirection.y = 1.0f;
    
    if(IsDown(&Input->MoveBackward))
        InputDirection.y = -1.0f;
    
    if(IsDown(&Input->MoveRight))
        InputDirection.x = 1.0f;
    
    if(IsDown(&Input->MoveLeft))
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
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
        ak_array<button_state>* ButtonStates = &World->ButtonStates[WorldIndex];
        ak_pool<collision_volume>* CollisionVolumeStorage = &World->CollisionVolumeStorage;
        
        AK_Assert(PhysicsObjects->Size == ButtonStates->Size, "Invalid game array size");
        ak_u32 Size = PhysicsObjects->Size;
        
        for(ak_u32 Index = 0; Index < Size; Index++) 
        {
            button_state* ButtonState = ButtonStates->Get(Index);
            physics_object* PhysicsObject = PhysicsObjects->Get(Index);
            
            if(IsPressed(ButtonState))
            {
                PhysicsObject->Transform.Scale.z *= 0.01f;
                collision_volume* CollisionVolume = CollisionVolumeStorage->Get(PhysicsObject->CollisionVolumeID);
                CollisionVolume->ConvexHull.Header.Transform.Scale.z *= 100.0f;
            }
            
            if(IsReleased(ButtonState))
            {
                PhysicsObject->Transform.Scale.z *= 100.0f;
                collision_volume* CollisionVolume = CollisionVolumeStorage->Get(PhysicsObject->CollisionVolumeID);
                CollisionVolume->ConvexHull.Header.Transform.Scale.z *= 0.01f;
            }
            
            ButtonState->WasDown = ButtonState->IsDown;
            if(ButtonState->IsToggled)
                ButtonState->IsDown = false;
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
        ak_array<button_state>* ButtonStates = &World->ButtonStates[WorldIndex];
        
        broad_phase BroadPhase = BroadPhase_Begin(World, WorldIndex);
        collision_detection CollisionDetection = CollisionDetection_Begin(BroadPhase, Game->Scratch);
        
        broad_phase_pair_list Pairs = BroadPhase.GetAllPairs(Game->Scratch, BroadPhase_FilterStaticEntitiesFunc);
        AK_ForEach(Pair, &Pairs)
        {
            entity* EntityA = EntityStorage->Get(Pair->EntityA);
            entity* EntityB = EntityStorage->Get(Pair->EntityB);
            
            if((EntityA->Type == ENTITY_TYPE_BUTTON) || (EntityB->Type == ENTITY_TYPE_BUTTON))
            {
                if(CollisionDetection_Intersect(&CollisionDetection, Pair))
                {
                    entity* Button = (EntityA->Type == ENTITY_TYPE_BUTTON) ? EntityA : EntityB;
                    button_state* ButtonState = ButtonStates->Get(AK_PoolIndex(Button->ID));
                    ButtonState->IsDown = true;
                }
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