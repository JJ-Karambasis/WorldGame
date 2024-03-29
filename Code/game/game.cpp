#include "game.h"

#include <assets.cpp>
#include <src/graphics_state.cpp>

#include "game_common_source.cpp"

#define VERY_CLOSE_DISTANCE 0.001f

extern "C"
AK_EXPORT GAME_STARTUP(Game_Startup)
{
    ak_arena* Storage = AK_CreateArena(AK_Megabyte(4));
    game* Game = Storage->Push<game>();
    Game->Storage = Storage;
    
    Debug_Log("Game Started!");
    
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

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_PlayerFilter)
{
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    entity* EntityB = EntityStorage->Get(Pair.EntityB);
    return !(EntityB->Type == ENTITY_TYPE_BUTTON);
}

struct movable_filter_user_data
{
    ak_u64 IgnoreID;
    id_list ChildIDs;
    id_list ParentIDs;
};

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_MovableFilter)
{
    movable_filter_user_data* Filter = (movable_filter_user_data*)UserData;
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    if(Filter->IgnoreID == Pair.EntityB)
        return false;
    
    for(ak_u32 Index = 0; Index < Filter->ChildIDs.Size; Index++)
    {
        if(Filter->ChildIDs.IDs[Index] == Pair.EntityB)
            return false;
    }
    
    for(ak_u32 Index = 0; Index < Filter->ParentIDs.Size; Index++)
    {
        if(Filter->ParentIDs.IDs[Index] == Pair.EntityB)
            return false;
    }
    
    entity* EntityB = EntityStorage->Get(Pair.EntityB);
    return !(EntityB->Type == ENTITY_TYPE_BUTTON);
}

struct gravity_filter_user_data
{
    broad_phase_pair_list* Movables;
};

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhase_GravityFilter)
{
    gravity_filter_user_data* Filter = (gravity_filter_user_data*)UserData;
    ak_pool<entity>* EntityStorage = &BroadPhase->World->EntityStorage[BroadPhase->WorldIndex];
    entity* EntityB = EntityStorage->Get(Pair.EntityB);
    
    if(EntityB->Type == ENTITY_TYPE_MOVABLE)
    {
        BroadPhase_AddPair(Filter->Movables, Pair);
        return false;
    }
    
    return !(EntityB->Type == ENTITY_TYPE_BUTTON);
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta)
{
    return AK_Max(Delta-VERY_CLOSE_DISTANCE, 0.0f);
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta, ak_f32 t)
{
    return AK_Max(Delta*t-VERY_CLOSE_DISTANCE, 0.0f);
}

void AddToList(ak_arena* Arena, id_list* List, ak_u64 ID)
{
    if(List->Size == List->Capacity)
    {
        ak_u32 OldCapcity = List->Capacity;
        List->Capacity += 8;
        ak_u64* IDs = Arena->PushArray<ak_u64>(List->Capacity);
        AK_CopyArray(IDs, List->IDs, OldCapcity);
        List->IDs = IDs;
    }
    
    ak_u32 Index = AK_PoolIndex(ID);
    if(!List->IndexAllocated[Index])
    {
        List->IDs[List->Size++] = ID;
        List->IndexAllocated[Index] = true;
    }
}

void Game_MovementUpdateChildren(game* Game, collision_detection* CollisionDetection, id_list* Children, ak_v3f ParentMoveDelta, ak_u64 IgnoreID)
{
    if(AK_Magnitude(ParentMoveDelta) < VERY_CLOSE_DISTANCE)
        return;
    
    world* World = CollisionDetection->BroadPhase.World;
    ak_u32 WorldIndex = CollisionDetection->BroadPhase.WorldIndex;
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    ak_array<movable>* Movables = &World->Movables[WorldIndex];
    
    for(ak_u32 Index = 0; Index < Children->Size; Index++)
    {
        ak_u64 ChildID = Children->IDs[Index];
        ak_u32 ChildIndex = AK_PoolIndex(ChildID);
        physics_object* PhysicsObject = PhysicsObjects->Get(ChildIndex);
        entity* Entity = EntityStorage->GetByIndex(ChildIndex);
        
        PhysicsObject->MoveDelta = ParentMoveDelta;
        
        movable* Movable = Movables->Get(ChildIndex);
        movable_filter_user_data* MovableFilter = Game->Scratch->Push<movable_filter_user_data>();
        MovableFilter->IgnoreID = IgnoreID;
        MovableFilter->ChildIDs = Movable->ChildIDs;
        MovableFilter->ParentIDs = Movable->ParentIDs;
        
        ccd_contact ContactTOI = CCD_GetEarliestContact(CollisionDetection, ChildID, BroadPhase_MovableFilter, MovableFilter);
        if(!ContactTOI.Intersected)
        {
            Game_MovementUpdateChildren(Game, CollisionDetection, &MovableFilter->ChildIDs, PhysicsObject->MoveDelta, IgnoreID);
            PhysicsObject->Position += PhysicsObject->MoveDelta;
            PhysicsObject->MoveDelta = {};
        }
        else
        {
            ak_f32 MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
            ak_v3f MoveDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;
            MoveDeltaDistance = GetDeltaClamped(MoveDeltaDistance, ContactTOI.t);
            Game_MovementUpdateChildren(Game, CollisionDetection, &MovableFilter->ChildIDs, MoveDirection*MoveDeltaDistance, IgnoreID);
            PhysicsObject->Position += MoveDirection*MoveDeltaDistance;
            PhysicsObject->MoveDelta = {};
        }
    }
}

#define MOVABLE_MIN_BOUNDS 0.999f
#define MOVABLE_MAX_BOUNDS 1.001f

ccd_contact Game_MovableMovementUpdate(game* Game, collision_detection* CollisionDetection, 
                                       ccd_contact AContactTOI, ak_u64 IgnoreID)
{
    world* World = CollisionDetection->BroadPhase.World;
    ak_u32 WorldIndex = CollisionDetection->BroadPhase.WorldIndex;
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    ak_array<movable>* Movables = &World->Movables[WorldIndex];
    entity* EntityB = EntityStorage->Get(AContactTOI.EntityB);
    
    if(EntityB->Type == ENTITY_TYPE_MOVABLE)
    {
        ak_u32 BIndex = AK_PoolIndex(EntityB->ID);
        movable* MovableB = Movables->Get(BIndex);
        
        for(;;)
        {
            ak_bool FoundBetter = false;
            for(ak_u32 ParentIndex = 0; ParentIndex < MovableB->ParentIDs.Size; ParentIndex++)
            {
                ccd_contact ParentContactTOI = CCD_ComputeContact(CollisionDetection, AContactTOI.EntityA, 
                                                                  MovableB->ParentIDs.IDs[ParentIndex]);
                if(ParentContactTOI.Intersected && (ParentContactTOI.t <= AContactTOI.t))
                {
                    AContactTOI = ParentContactTOI;
                    FoundBetter = true;
                }
            }
            
            if(!FoundBetter)
                break;
            
            BIndex = AK_PoolIndex(AContactTOI.EntityB);
            MovableB = Movables->Get(BIndex);
            
            if(MovableB->ParentIDs.Size == 0)
                break;
        }
        
        EntityB = EntityStorage->GetByIndex(BIndex);
        physics_object* PhysicsObjectB = PhysicsObjects->Get(BIndex);
        
        ak_v3f LocalX = AK_QuatToXAxis(PhysicsObjectB->Orientation);
        ak_v3f LocalY = AK_QuatToYAxis(PhysicsObjectB->Orientation);
        ak_f32 XTest = AK_Abs(AK_Dot(LocalX, AContactTOI.Contact.Normal)); 
        ak_f32 YTest = AK_Abs(AK_Dot(LocalY, AContactTOI.Contact.Normal));
        if((XTest > MOVABLE_MIN_BOUNDS && XTest < MOVABLE_MAX_BOUNDS) || (YTest > MOVABLE_MIN_BOUNDS && YTest < MOVABLE_MAX_BOUNDS))
        {
            ak_u32 AIndex = AK_PoolIndex(AContactTOI.EntityA);
            physics_object* PhysicsObjectA = PhysicsObjects->Get(AIndex);
            movable* MovableA = Movables->Get(AIndex);
            
            ak_f32 MoveDeltaDistanceA = AK_Magnitude(PhysicsObjectA->MoveDelta);
            ak_v3f MoveDirectionA = PhysicsObjectA->MoveDelta/MoveDeltaDistanceA;
            ak_v3f MoveDirectionB = AK_Normalize(AK_V3(AContactTOI.Contact.Normal.xy));
            
            ak_f32 RemainderDeltaDistanceA = MoveDeltaDistanceA-(MoveDeltaDistanceA*AContactTOI.t);
            
            PhysicsObjectB->MoveDelta = AK_Dot(MoveDirectionB, MoveDirectionA*RemainderDeltaDistanceA)*MoveDirectionB;
            
            if(AK_Magnitude(PhysicsObjectB->MoveDelta) < VERY_CLOSE_DISTANCE)
            {
                PhysicsObjectB->MoveDelta = {};
                if(AContactTOI.Contact.Penetration >= 0.0f)
                {
                    //TODO(JJ): We probably need to update Movable A children by this amount as well
                    PhysicsObjectA->Position += -AContactTOI.Contact.Normal*(AContactTOI.Contact.Penetration+VERY_CLOSE_DISTANCE);
                }
                
                AContactTOI.Contact.Normal = {};
                
                return AContactTOI;
            }
            
            movable_filter_user_data* MovableFilterA = Game->Scratch->Push<movable_filter_user_data>();
            MovableFilterA->IgnoreID = AContactTOI.EntityA;
            MovableFilterA->ChildIDs = MovableB->ChildIDs;
            MovableFilterA->ParentIDs = MovableB->ParentIDs;
            
            ccd_contact BContactTOI = CCD_GetEarliestContact(CollisionDetection, EntityB->ID, 
                                                             BroadPhase_MovableFilter, 
                                                             MovableFilterA);
            if(!BContactTOI.Intersected)
            {
                Game_MovementUpdateChildren(Game, CollisionDetection, &MovableFilterA->ChildIDs, PhysicsObjectB->MoveDelta, AContactTOI.EntityA);
                
                PhysicsObjectB->Position += PhysicsObjectB->MoveDelta;
                PhysicsObjectB->MoveDelta = {};
                
                PhysicsObjectA->MoveDelta = MoveDirectionA*MoveDeltaDistanceA;
                
                movable_filter_user_data* MovableFilterB = Game->Scratch->Push<movable_filter_user_data>();
                MovableFilterB->IgnoreID = IgnoreID;
                MovableFilterB->ChildIDs = MovableA->ChildIDs;
                MovableFilterB->ParentIDs = MovableA->ParentIDs;
                
                ccd_contact CContactTOI = CCD_GetEarliestContact(CollisionDetection, AContactTOI.EntityA, 
                                                                 BroadPhase_MovableFilter, 
                                                                 MovableFilterB);
                
                if(CContactTOI.Intersected)
                {
                    return Game_MovableMovementUpdate(Game, CollisionDetection, 
                                                      CContactTOI, IgnoreID);
                }
            }
            else
            {
                ccd_contact CContactTOI = Game_MovableMovementUpdate(Game, CollisionDetection, 
                                                                     BContactTOI, AContactTOI.EntityA);
                if(CContactTOI.Intersected)
                {
                    ak_f32 MoveDeltaDistanceB = AK_Magnitude(PhysicsObjectB->MoveDelta);
                    MoveDeltaDistanceB = GetDeltaClamped(MoveDeltaDistanceB, BContactTOI.t);
                    
                    Game_MovementUpdateChildren(Game, CollisionDetection, &MovableFilterA->ChildIDs, MoveDirectionB*MoveDeltaDistanceB, AContactTOI.EntityA);
                    
                    PhysicsObjectB->Position += MoveDirectionB*MoveDeltaDistanceB;
                    PhysicsObjectB->MoveDelta = {};
                    
                    PhysicsObjectA->MoveDelta = MoveDirectionA*MoveDeltaDistanceA;
                    
                    movable_filter_user_data* MovableFilterB = Game->Scratch->Push<movable_filter_user_data>();
                    MovableFilterB->IgnoreID = IgnoreID;
                    MovableFilterB->ChildIDs = MovableA->ChildIDs;
                    MovableFilterB->ParentIDs = MovableA->ParentIDs;
                    
                    ccd_contact DContactTOI = 
                        CCD_GetEarliestContact(CollisionDetection, AContactTOI.EntityA, 
                                               BroadPhase_MovableFilter, 
                                               MovableFilterB);
                    if(DContactTOI.Intersected)
                    {
                        return DContactTOI;
                    }
                }
                else
                {
                    ak_f32 MoveDeltaDistanceB = AK_Magnitude(PhysicsObjectB->MoveDelta);
                    Game_MovementUpdateChildren(Game, CollisionDetection, &MovableFilterA->ChildIDs, MoveDirectionB*MoveDeltaDistanceB, AContactTOI.EntityA);
                    
                    PhysicsObjectB->Position += MoveDirectionB*MoveDeltaDistanceB;
                    
                    PhysicsObjectB->MoveDelta = {};
                    
                    PhysicsObjectA->MoveDelta = MoveDirectionA*MoveDeltaDistanceA;
                    
                    movable_filter_user_data* MovableFilterB = Game->Scratch->Push<movable_filter_user_data>();
                    MovableFilterB->IgnoreID = IgnoreID;
                    MovableFilterB->ChildIDs = MovableA->ChildIDs;
                    MovableFilterB->ParentIDs = MovableA->ParentIDs;
                    
                    ccd_contact DContactTOI = CCD_GetEarliestContact(CollisionDetection, AContactTOI.EntityA, 
                                                                     BroadPhase_MovableFilter, 
                                                                     MovableFilterB);
                    if(DContactTOI.Intersected)
                        return Game_MovableMovementUpdate(Game, CollisionDetection, DContactTOI, IgnoreID);
                }
            }
        }
        else
        {
            return AContactTOI;
        }
    }
    else
    {
        return AContactTOI;
    }
    
    return {};
}

void Game_PlayerMovementUpdate(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_f32 dt)
{
    BeginTimedBlock(Game_PlayerMovementUpdate);
    world* World = Game->World;
    physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(ID));
    PhysicsObject->MoveDelta = PhysicsObject->Velocity*dt;
    
    broad_phase BroadPhase = BroadPhase_Begin(World, WorldIndex);
    collision_detection CollisionDetection = CollisionDetection_Begin(BroadPhase, Game->Scratch);
    
    if(AK_Magnitude(PhysicsObject->MoveDelta) > VERY_CLOSE_DISTANCE)
    {
        for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
        {
            ak_f32 MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
            if(MoveDeltaDistance < VERY_CLOSE_DISTANCE) break;
            ak_v3f MoveDeltaDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;
            
            ccd_contact ContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID, 
                                                            BroadPhase_PlayerFilter);
            if(!ContactCCD.Intersected)
            {
                PhysicsObject->Position += MoveDeltaDirection*MoveDeltaDistance;
                break;
            }
            
            ccd_contact MovableCCD = Game_MovableMovementUpdate(Game, &CollisionDetection, 
                                                                ContactCCD, 0);
            if(MovableCCD.Intersected)
            {
                ak_v3f Normal = -MovableCCD.Contact.Normal;
                ak_f32 tHit = MovableCCD.t;
                MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
                MoveDeltaDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;
                
                ak_v3f TargetPosition = PhysicsObject->Position + MoveDeltaDirection*MoveDeltaDistance;
                
                if(tHit == 0 && MovableCCD.Contact.Penetration >= 0.0f)
                {
                    PhysicsObject->Position += Normal*(MovableCCD.Contact.Penetration+VERY_CLOSE_DISTANCE);
                }
                
                ak_f32 HitDeltaDistance = GetDeltaClamped(MoveDeltaDistance, tHit);
                
                PhysicsObject->Position += MoveDeltaDirection*HitDeltaDistance;
                
                PhysicsObject->MoveDelta = TargetPosition-PhysicsObject->Position;
                PhysicsObject->MoveDelta -= AK_Dot(Normal, PhysicsObject->MoveDelta)*Normal;
                PhysicsObject->Velocity -= AK_Dot(Normal, PhysicsObject->Velocity)*Normal;
            }
            else
            {
                MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
                MoveDeltaDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;
                PhysicsObject->Position += MoveDeltaDirection*MoveDeltaDistance;
                break;
            }
        }
    }
    
    PhysicsObject->MoveDelta = {};
}

void Game_GravityMovementUpdate(game* Game, ak_u32 WorldIndex, ak_u64 ID, ak_v3f GravityVelocity, ak_f32 dt)
{
    BeginTimedBlock(Game_GravityMovementUpdate);
    world* World = Game->World;
    physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(ID));
    ak_array<movable>* Movables = &World->Movables[WorldIndex];
    
    broad_phase BroadPhase = BroadPhase_Begin(World, WorldIndex);
    collision_detection CollisionDetection = CollisionDetection_Begin(BroadPhase, Game->Scratch);
    
    PhysicsObject->MoveDelta = GravityVelocity*dt;
    
    if(AK_Magnitude(PhysicsObject->MoveDelta) > VERY_CLOSE_DISTANCE)
    {
        for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
        {
            BeginTimedBlock(GravityUpdate_Iterations);
            ak_v3f TargetPosition = PhysicsObject->Position + PhysicsObject->MoveDelta;
            ak_f32 MoveDeltaDistance = AK_Magnitude(PhysicsObject->MoveDelta);
            if(MoveDeltaDistance < VERY_CLOSE_DISTANCE) 
            {
                EndTimedBlock(GravityUpdate_Iterations);
                break;
            }
            
            broad_phase_pair_list MovablePairs = BroadPhase_AllocateList(Game->Scratch);
            gravity_filter_user_data* Filter = Game->Scratch->Push<gravity_filter_user_data>();
            
            Filter->Movables = &MovablePairs;
            broad_phase_pair_list NonMovablePairs = BroadPhase.GetPairs(Game->Scratch, ID, BroadPhase_GravityFilter, Filter);
            
            BeginTimedBlock(CCD);
            ccd_contact MovableContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID, 
                                                                   &MovablePairs);
            ccd_contact NonMovableContactCCD = CCD_GetEarliestContact(&CollisionDetection, ID, 
                                                                      &NonMovablePairs);
            EndTimedBlock(CCD);
            
            if(!MovableContactCCD.Intersected && !NonMovableContactCCD.Intersected)
            {
                PhysicsObject->Transform.Translation += PhysicsObject->MoveDelta;
                EndTimedBlock(GravityUpdate_Iterations);
                break;
            }
            
            ccd_contact BestContactCCD = NonMovableContactCCD;
            
            if(!BestContactCCD.Intersected)
                BestContactCCD = MovableContactCCD;
            else
            {
                if(MovableContactCCD.Intersected && (BestContactCCD.t >= MovableContactCCD.t))
                    BestContactCCD = MovableContactCCD;
            }
            
            if(Iterations == 0)
            {
                entity* EntityB = World->EntityStorage[WorldIndex].Get(BestContactCCD.EntityB); 
                if(EntityB->Type == ENTITY_TYPE_MOVABLE)
                {
                    ak_f32 ZTest = AK_Abs(AK_Dot(AK_ZAxis(), BestContactCCD.Contact.Normal));
                    if(ZTest > MOVABLE_MIN_BOUNDS && ZTest < MOVABLE_MAX_BOUNDS)
                    {
                        ak_u32 IndexA = AK_PoolIndex(BestContactCCD.EntityA);
                        ak_u32 IndexB = AK_PoolIndex(BestContactCCD.EntityB);
                        movable* MovableB = Movables->Get(IndexB);
                        
                        entity* EntityA = World->EntityStorage[WorldIndex].Get(BestContactCCD.EntityA);
                        if(BestContactCCD.Contact.Normal.z > 0)
                        {
                            AddToList(Game->Storage, &MovableB->ParentIDs, EntityA->ID);
                            if(EntityA->Type == ENTITY_TYPE_MOVABLE)
                            {
                                movable* MovableA = Movables->Get(IndexA);
                                AddToList(Game->Storage, &MovableA->ChildIDs, EntityB->ID);
                            }
                        }
                        else
                        {
                            AddToList(Game->Storage, &MovableB->ChildIDs, EntityA->ID);
                            if(EntityA->Type == ENTITY_TYPE_MOVABLE)
                            {
                                movable* MovableA = Movables->Get(IndexA);
                                AddToList(Game->Storage, &MovableA->ParentIDs, EntityB->ID);
                            }
                        }
                    }
                }
            }
            
            ak_v3f Normal = -BestContactCCD.Contact.Normal;
            ak_f32 tHit = BestContactCCD.t;
            if(tHit == 0 && BestContactCCD.Contact.Penetration > 0.0f)
            {
                PhysicsObject->Position += Normal*(BestContactCCD.Contact.Penetration+VERY_CLOSE_DISTANCE);
            }
            
            ak_f32 HitDeltaDistance = GetDeltaClamped(MoveDeltaDistance, tHit);
            ak_v3f MoveDirection = PhysicsObject->MoveDelta/MoveDeltaDistance;
            PhysicsObject->Position += MoveDirection*HitDeltaDistance;
            
            if(AK_Dot(Normal, AK_ZAxis()) < 0.7f)
            {                            
                PhysicsObject->MoveDelta = TargetPosition - PhysicsObject->Position;
                PhysicsObject->MoveDelta -= AK_Dot(Normal, PhysicsObject->MoveDelta)*Normal;
            }
            else
            {                                                        
                if(HitDeltaDistance == 0.0f)
                {
                    EndTimedBlock(GravityUpdate_Iterations);
                    break;
                }
            }
            EndTimedBlock(GravityUpdate_Iterations);
        }
    }
    
    PhysicsObject->MoveDelta = {};
    EndTimedBlock(Game_GravityMovementUpdate);
}

void Game_ClearAllMovableChildren(ak_arena* Scratch, world* World)
{
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {        
        ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];         
        AK_ForEach(Movable, &World->Movables[WorldIndex])
        {
            Movable->ChildIDs.Size = 0;
            Movable->ParentIDs.Size = 0;
            Movable->ChildIDs.IndexAllocated = Scratch->PushArray<ak_bool>(EntityStorage->MaxUsed);
            Movable->ParentIDs.IndexAllocated = Scratch->PushArray<ak_bool>(EntityStorage->MaxUsed);
        }
    }
}

extern "C"
AK_EXPORT GAME_UPDATE(Game_Update)
{
    BeginTimedBlock(Game_Update);
    
    Game->WorldShutdownCommon = Game_WorldShutdownCommon;
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
    
    Game_ClearAllMovableChildren(Game->Scratch, World);
    
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
                
                case ENTITY_TYPE_MOVABLE:
                {
                    movable* Movable = World->Movables[WorldIndex].Get(AK_PoolIndex(Entity->ID));
                    Movable->GravityVelocity.z -= Gravity*dt;
                    Movable->GravityVelocity *= (1.0f/(1.0f+3.0f*dt));
                    
                    Game_GravityMovementUpdate(Game, WorldIndex, Entity->ID, 
                                               Movable->GravityVelocity, dt);
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
    
    EndTimedBlock(Game_Update);
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
    AK_DeleteArena(Game->Storage);
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>