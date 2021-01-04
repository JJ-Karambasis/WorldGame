#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "simulation/simulation.cpp"
#include "graphics_state.cpp"
#include "world.cpp"
//#include "player.cpp"

#define VERY_CLOSE_DISTANCE 0.001f

PUZZLE_COMPLETE_CALLBACK(DespawnWallCompleteCallback)
{
    world_id* IDs = (world_id*)UserData;
    
    FreeEntity(Game, IDs[0]);
    FreeEntity(Game, IDs[1]);
}

inline goal_rect 
CreateGoalRect(ak_v3f RectMin, ak_v3f RectMax, ak_u32 WorldIndex)
{
    goal_rect Result = {WorldIndex, AK_Rect(RectMin, RectMax)};
    return Result;
}

inline goal_rect 
CreateGoalRect(ak_v3f RectMin, ak_v3f RectMax, ak_u32 WorldIndex, ak_f32 Padding)
{
    goal_rect Result = CreateGoalRect(RectMin-Padding, RectMax+Padding, WorldIndex);
    return Result;
}

extern "C"
AK_EXPORT GAME_INITIALIZE(Initialize)
{        
    ak_arena* GameStorage = AK_CreateArena(AK_Megabyte(32));            
    assets* Assets = InitAssets(GameStorage, AssetPath);
    if(!Assets)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }        
    
    game* Game = GameStorage->Push<game>();    
    Game->GameStorage = GameStorage;        
    Game->TempStorage = TempStorage;
    
    AK_SetGlobalArena(Game->TempStorage);
    
    ak_mesh_result<ak_vertex_p3> QuadMeshResult = AK_GenerateTriangleQuad(Game->GameStorage);
    
    Game->QuadMesh.VertexCount = QuadMeshResult.VertexCount;
    Game->QuadMesh.IndexCount = QuadMeshResult.IndexCount;
    Game->QuadMesh.Vertices = QuadMeshResult.Vertices;
    Game->QuadMesh.Indices = QuadMeshResult.Indices;
    
    Game->Input       = Input;
    Game->AudioOutput = AudioOutput;
    Game->Assets      = Assets;        
    
    Game->World.NewCameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));    
    Game->World.NewCameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    Game->World.OldCameras[0] = Game->World.NewCameras[0];
    Game->World.OldCameras[1] = Game->World.NewCameras[1];
    
    
    return Game;
}

void AddRigidBodyContacts(game* Game, ak_u32 WorldIndex, rigid_body* RigidBody, sim_entity* SimEntity, contact_list ContactList)
{
    world* World = &Game->World;
    simulation* Simulation = &World->Simulations[WorldIndex];            
}

struct filter_common
{
    world* World;
    ak_u32 WorldIndex;
};

struct filter_ignore_entity : public filter_common
{
    world_id Entity0;
    world_id Entity1;
    world_id Entity2;
    world_id Entity3;
};

filter_ignore_entity* GetFilterIgnoreEntity(world* World, ak_u32 WorldIndex, world_id EntityA = InvalidWorldID(), world_id EntityB = InvalidWorldID(), world_id EntityC = InvalidWorldID(), world_id EntityD = InvalidWorldID())
{
    filter_ignore_entity* FilterData = AK_GetGlobalArena()->Push<filter_ignore_entity>();
    FilterData->World = World;
    FilterData->WorldIndex = WorldIndex;
    FilterData->Entity0 = EntityA;
    FilterData->Entity1 = EntityB;
    FilterData->Entity2 = EntityC;
    FilterData->Entity3 = EntityD;
    return FilterData;
}

inline ak_bool 
BroadPhaseFilterCommon(entity* EntityB)
{        
    if((EntityB->Type == ENTITY_TYPE_RIGID_BODY) || (EntityB->Type == ENTITY_TYPE_BUTTON))
        return false;       
    return true;
}

inline ak_bool 
BroadPhaseFilterCommon(broad_phase_pair* Pair, world* World, ak_u32 WorldIndex)
{    
    entity* EntityB = World->EntityStorage[WorldIndex].GetByIndex(UserDataToIndex(Pair->SimEntityB->UserData));
    return BroadPhaseFilterCommon(EntityB);    
}

BROAD_PHASE_PAIR_FILTER_FUNC(FilterCommon)
{
    filter_common* Filter = (filter_common*)UserData;
    return BroadPhaseFilterCommon(Pair, Filter->World, Filter->WorldIndex);    
}

BROAD_PHASE_PAIR_FILTER_FUNC(FilterEntity)
{
    filter_ignore_entity* Filter = (filter_ignore_entity*)UserData;
    entity* EntityB = GetEntity(&Filter->World->EntityStorage[Filter->WorldIndex], Pair->SimEntityB);
    
    if(AreEqualIDs(EntityB->ID, Filter->Entity0) || AreEqualIDs(EntityB->ID, Filter->Entity1) || 
       AreEqualIDs(EntityB->ID, Filter->Entity2) || AreEqualIDs(EntityB->ID, Filter->Entity3))
    {
        return false;
    }
    
    return BroadPhaseFilterCommon(EntityB);    
}

#define PAIR_IS_TYPE(typeA, typeB, type) ((typeA == type) || (typeB == type))

ak_bool CanPushMovable(rigid_body* Pushable, ak_v3f Normal)
{
    ak_m3f Orientation = AK_QuatToMatrix(Pushable->Transform.Orientation);    
    ak_f32 XTest = AK_Abs(AK_Dot(Orientation.XAxis, Normal));
    ak_f32 YTest = AK_Abs(AK_Dot(Orientation.YAxis, Normal));    
    return (XTest > 0.999f && XTest < 1.001f) || (YTest > 0.999f && YTest < 1.001f);
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta)
{
    return AK_Max(Delta-VERY_CLOSE_DISTANCE, 0.0f);
}

inline ak_f32 GetDeltaClamped(ak_f32 Delta, ak_f32 t)
{
    return AK_Max(Delta*t-VERY_CLOSE_DISTANCE, 0.0f);
}

void MovementPushableUpdateChild(world* World, ak_u32 WorldIndex, entity* Entity, ak_v3f MoveDelta, world_id IgnoreEntityID_0, world_id IgnoreEntityID_1)
{
    if(Entity->Type == ENTITY_TYPE_MOVABLE)
    {
        
        simulation* Simulation = &World->Simulations[WorldIndex];
        movable* Movable = GetMovable(World, Entity);
        if(Movable->ChildID.IsValid())
        {
            rigid_body* RigidBody = GetRigidBody(World, Movable->ChildID);
            RigidBody->MoveDelta = MoveDelta;
            
            entity* TestEntity = GetEntity(&World->EntityStorage[WorldIndex], RigidBody);
            movable* TestMovable = GetMovable(World, TestEntity);
            
            filter_ignore_entity* FilterData = GetFilterIgnoreEntity(World, WorldIndex, IgnoreEntityID_0, IgnoreEntityID_1, TestMovable->ChildID, TestMovable->ParentID); 
            
            broad_phase_pair_list Pairs = Simulation->GetPairs(RigidBody, FilterEntity, FilterData);
            continuous_contact ChildContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);
            
            if(ChildContactTOI.HitEntity)
            {
                ak_f32 MoveDeltaLength = AK_Magnitude(MoveDelta);
                ak_v3f MoveDirection = MoveDelta/MoveDeltaLength;
                ak_v3f NewMoveDelta = MoveDirection*GetDeltaClamped(MoveDeltaLength, ChildContactTOI.t);
                MovementPushableUpdateChild(World, WorldIndex, TestEntity, NewMoveDelta, IgnoreEntityID_0, IgnoreEntityID_1);
                RigidBody->Transform.Translation += NewMoveDelta;
            }
            else
            {
                MovementPushableUpdateChild(World, WorldIndex, TestEntity, RigidBody->MoveDelta, IgnoreEntityID_0, IgnoreEntityID_1);
                RigidBody->Transform.Translation += RigidBody->MoveDelta;
            }
            
            RigidBody->MoveDelta = {};
        }
    }
}

toi_normal MovementPushableUpdateRecursive(world* World, ak_u32 WorldIndex, rigid_body* RigidBody, continuous_contact ContactTOI, world_id IgnoreEntityID)
{   
    toi_normal Result = {};
    
    ak_arena* GlobalArena = AK_GetGlobalArena();    
    simulation* Simulation = World->Simulations + WorldIndex;    
    entity_storage* EntityStorage = World->EntityStorage + WorldIndex;    
    
    rigid_body* RigidBodyA = RigidBody;
    entity* EntityA = GetEntity(EntityStorage, RigidBodyA);            
    entity* EntityB = GetEntity(EntityStorage, ContactTOI.HitEntity);    
    
    movable* MovableA = (EntityA->Type == ENTITY_TYPE_MOVABLE) ? GetMovable(World, EntityA) : NULL;
    
    if(EntityB->Type == ENTITY_TYPE_MOVABLE)
    {
        rigid_body* RigidBodyB = ContactTOI.HitEntity->ToRigidBody();                        
        movable* MovableB = GetMovable(World, EntityB);
        
        if(MovableB->ParentID.IsValid())
        {
            rigid_body* ParentRigidBody = GetRigidBody(World, MovableB->ParentID);
            continuous_contact ParentContactTOI = Simulation->ComputeTOI(RigidBodyA, ParentRigidBody);
            if(ParentContactTOI.HitEntity && ParentContactTOI.t <= ContactTOI.t)
            {
                EntityB = GetEntity(World, MovableB->ParentID);
                RigidBodyB = ParentRigidBody;
                MovableB = GetMovable(World, EntityB);
                ContactTOI = ParentContactTOI;
            }
        }
        
        
        if(CanPushMovable(RigidBodyB, ContactTOI.Contact.Normal))
        {   
            filter_ignore_entity* FilterDataRigidBodyB = GetFilterIgnoreEntity(World, WorldIndex, EntityA->ID, IgnoreEntityID, MovableB->ChildID, MovableB->ParentID);
            
            ak_v3f MovableMoveDirection = AK_Normalize(AK_V3(ContactTOI.Contact.Normal.xy));
            ak_f32 MoveDeltaDistance = RigidBodyA->GetMoveDistance();
            ak_v3f MoveDirection = RigidBodyA->MoveDelta/MoveDeltaDistance;
            
            ak_f32 RemainderDistance = GetDeltaClamped(MoveDeltaDistance-(MoveDeltaDistance*ContactTOI.t));
            
            ak_v3f RemainderDelta = MoveDirection*RemainderDistance;                                    
            
            RigidBodyB->MoveDelta = AK_Dot(MovableMoveDirection, RemainderDelta)*MovableMoveDirection;
            
            broad_phase_pair_list MovablePairs = Simulation->GetPairs(RigidBodyB, FilterEntity, FilterDataRigidBodyB);
            continuous_contact MovableContactTOI = Simulation->ComputeTOI(RigidBodyB, MovablePairs);
            
            filter_ignore_entity* FilterDataRigidBodyA = GetFilterIgnoreEntity(World, WorldIndex, IgnoreEntityID, GetChildID(MovableA), GetParentID(MovableA));
            
            if(!MovableContactTOI.HitEntity)
            {
                MovementPushableUpdateChild(World, WorldIndex, EntityB, RigidBodyB->MoveDelta,
                                            EntityA->ID, IgnoreEntityID);
                
                ak_v3f MoveDeltaA = MoveDirection*GetDeltaClamped(MoveDeltaDistance, ContactTOI.t);
                MovementPushableUpdateChild(World, WorldIndex, EntityA, MoveDeltaA, 
                                            IgnoreEntityID, InvalidWorldID());
                
                RigidBodyB->Transform.Translation += RigidBodyB->MoveDelta;
                
                RigidBodyA->Transform.Translation += MoveDeltaA;
                RigidBodyA->MoveDelta = MoveDirection*RemainderDistance;   
                
                broad_phase_pair_list RemainderPairs = Simulation->GetPairs(RigidBodyA, FilterEntity, FilterDataRigidBodyA);                                    
                continuous_contact RemainderContactTOI = Simulation->ComputeTOI(RigidBodyA, RemainderPairs);                                                    
                if(RemainderContactTOI.HitEntity)
                {
                    Result = MakeTOINormal(RemainderContactTOI.t, RemainderContactTOI.Contact.Normal);
                }
            }
            else
            {
                ak_v3f PrevP = RigidBodyB->Transform.Translation;
                toi_normal TOINormal = MovementPushableUpdateRecursive(World, WorldIndex, RigidBodyB, MovableContactTOI, EntityA->ID);                
                if(TOINormal.Intersected)
                {                    
                    ak_f32 MovableDeltaDistance = AK_Magnitude(RigidBodyB->MoveDelta);                                        
                    ak_f32 MovableHitDeltaDistance = GetDeltaClamped(MovableDeltaDistance, TOINormal.tHit);
                    
                    MovementPushableUpdateChild(World, WorldIndex, EntityB, MovableMoveDirection*MovableHitDeltaDistance, EntityA->ID, IgnoreEntityID);
                    
                    ak_v3f MoveDeltaA = MoveDirection*GetDeltaClamped(MoveDeltaDistance, ContactTOI.t);
                    MovementPushableUpdateChild(World, WorldIndex, EntityA, MoveDeltaA, 
                                                IgnoreEntityID, InvalidWorldID());
                    
                    RigidBodyB->Transform.Translation += MovableMoveDirection*MovableHitDeltaDistance;      
                    RigidBodyA->Transform.Translation += MoveDeltaA;
                    RigidBodyA->MoveDelta = MoveDirection*RemainderDistance;
                    
                    broad_phase_pair_list RemainderPairs = Simulation->GetPairs(RigidBodyA, FilterEntity, FilterDataRigidBodyA);                                    
                    continuous_contact RemainderContactTOI = Simulation->ComputeTOI(RigidBodyA, RemainderPairs);                                                    
                    if(RemainderContactTOI.HitEntity)
                    {
                        Result = MakeTOINormal(RemainderContactTOI.t, RemainderContactTOI.Contact.Normal);
                    }
                    else
                    {
                        Result = MakeTOINormal(TOINormal.tHit, ContactTOI.Contact.Normal);
                    }
                }
                else
                {
                    
                    MovementPushableUpdateChild(World, WorldIndex, EntityB, RigidBodyB->MoveDelta, EntityA->ID, IgnoreEntityID);
                    
                    ak_v3f MoveDeltaA = MoveDirection*GetDeltaClamped(MoveDeltaDistance, ContactTOI.t);
                    MovementPushableUpdateChild(World, WorldIndex, EntityA, MoveDeltaA, 
                                                IgnoreEntityID, InvalidWorldID());
                    
                    RigidBodyB->Transform.Translation += RigidBodyB->MoveDelta;                    
                    ak_f32 Dist = AK_Magnitude(PrevP-RigidBodyB->Transform.Translation);
                    
                    RigidBodyA->Transform.Translation += MoveDeltaA;
                    RigidBodyA->MoveDelta = MoveDirection*Dist;                                
                    
                    broad_phase_pair_list RemainderPairs = Simulation->GetPairs(RigidBodyA, FilterEntity, FilterDataRigidBodyA);                                    
                    continuous_contact RemainderContactTOI = Simulation->ComputeTOI(RigidBodyA, RemainderPairs);                                                    
                    if(RemainderContactTOI.HitEntity)
                    {
                        Result = MakeTOINormal(RemainderContactTOI.t, RemainderContactTOI.Contact.Normal);
                    }                    
                }
            }
            
            RigidBodyB->MoveDelta = {};
        }
        else
        {
            Result = MakeTOINormal(ContactTOI.t, ContactTOI.Contact.Normal);
        }
    }
    else
    {        
        Result = MakeTOINormal(ContactTOI.t, ContactTOI.Contact.Normal);
    }
    
    return Result;    
}

void MovementPushableUpdate(world* World, ak_u32 WorldIndex, rigid_body* RigidBody, ak_f32 dt)
{   
    ak_high_res_clock StartClock = AK_WallClock();
    
    RigidBody->MoveDelta = RigidBody->Velocity*dt;
    if(AK_Magnitude(RigidBody->MoveDelta) > VERY_CLOSE_DISTANCE)
    {        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        simulation* Simulation = World->Simulations + WorldIndex;    
        entity_storage* EntityStorage = World->EntityStorage + WorldIndex;
        
        filter_common* FilterData = GlobalArena->Push<filter_common>();
        FilterData->World = World;
        FilterData->WorldIndex = WorldIndex;                        
        
        for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
        {
            broad_phase_pair_list Pairs = Simulation->GetPairs(RigidBody, FilterCommon, FilterData);                                    
            continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                                                    
            if(!ContactTOI.HitEntity)
            {
                RigidBody->Transform.Translation += RigidBody->MoveDelta;
                break;
            }
            
            toi_normal TOINormal = MovementPushableUpdateRecursive(World, WorldIndex, RigidBody, ContactTOI, InvalidWorldID());            
            if(TOINormal.Intersected)
            {
                ak_v3f Normal = -TOINormal.Normal;
                ak_f32 tHit = TOINormal.tHit;
                
                RigidBody->Velocity -= AK_Dot(Normal, RigidBody->Velocity)*Normal;
                
                ak_f32 MoveDeltaDistance = AK_Magnitude(RigidBody->MoveDelta);
                if(AK_EqualZeroEps(MoveDeltaDistance))
                    break;
                
                ak_v3f MoveDirection = RigidBody->MoveDelta/MoveDeltaDistance;                                                                                                        
                ak_f32 HitDeltaDistance = GetDeltaClamped(MoveDeltaDistance, tHit);
                
                ak_v3f Destination = RigidBody->Transform.Translation + RigidBody->MoveDelta;
                ak_v3f NewBasePoint = RigidBody->Transform.Translation + MoveDirection*HitDeltaDistance;                        
                
                ak_planef SlidingPlane = AK_Plane(NewBasePoint, Normal);
                ak_v3f NewDestination = Destination - AK_SignDistance(SlidingPlane, Destination)*SlidingPlane.Normal;  
                
                RigidBody->MoveDelta = NewDestination - NewBasePoint;        
                
                RigidBody->Transform.Translation = NewBasePoint;
                if(AK_Magnitude(RigidBody->MoveDelta) < VERY_CLOSE_DISTANCE)                                                    
                    break;                                    
            }
            else
            {
                RigidBody->Transform.Translation += RigidBody->MoveDelta;
                break;
            }
        }
    }    
    
    RigidBody->MoveDelta = {};
    
    ak_high_res_clock EndClock = AK_WallClock();
    ak_f64 Elapsed = AK_GetElapsedTime(EndClock, StartClock);
}

struct broad_phase_filter_movable_data : public filter_common
{    
    broad_phase_pair_list* NonMovables;
};

BROAD_PHASE_PAIR_FILTER_FUNC(BroadPhaseFilterPushableCallback)
{
    broad_phase_filter_movable_data* Filter = (broad_phase_filter_movable_data*)UserData;
    
    entity* EntityB = GetEntity(&Filter->World->EntityStorage[Filter->WorldIndex], Pair->SimEntityB);
    if(EntityB->Type == ENTITY_TYPE_MOVABLE)
        return true;
    else
    {
        Filter->NonMovables->AddPair(Pair->SimEntityA, Pair->SimEntityB, Pair->AVolumeID, Pair->BVolumeID);
        return false;
    }
}

void MovementGravityUpdate(world* World, ak_u32 WorldIndex, rigid_body* RigidBody, ak_v3f GravityVelocity, ak_f32 dt)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    simulation* Simulation = World->Simulations + WorldIndex;    
    entity_storage* EntityStorage = World->EntityStorage + WorldIndex;    
    
    RigidBody->MoveDelta = GravityVelocity*dt;
    for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
    {
        broad_phase_filter_movable_data* Filter = GlobalArena->Push<broad_phase_filter_movable_data>();
        Filter->World = World;
        Filter->WorldIndex = WorldIndex;                                        
        
        broad_phase_pair_list Pairs = Simulation->GetPairs(RigidBody, FilterCommon, Filter);                                    
        broad_phase_pair_list NonMovablePairs = Simulation->AllocatePairList(Pairs.Count);
        
        Filter->NonMovables = &NonMovablePairs;
        broad_phase_pair_list MovablePairs = Simulation->FilterPairs(Pairs, BroadPhaseFilterPushableCallback, Filter);
        
        continuous_contact MovableContactTOI = Simulation->ComputeTOI(RigidBody, MovablePairs);
        continuous_contact NonMovableContactTOI = Simulation->ComputeTOI(RigidBody, NonMovablePairs);
        
        if(!MovableContactTOI.HitEntity && !NonMovableContactTOI.HitEntity)
        {
            RigidBody->Transform.Translation += RigidBody->MoveDelta;
            break;
        }     
        
        continuous_contact BestContact = NonMovableContactTOI;
        if(BestContact.t >= MovableContactTOI.t)
            BestContact = MovableContactTOI;
        
        entity* HitEntity = GetEntity(EntityStorage, BestContact.HitEntity);            
        if(HitEntity->Type == ENTITY_TYPE_MOVABLE)
        {
            ak_f32 ZTest = AK_Abs(AK_Dot(AK_ZAxis(), BestContact.Contact.Normal));
            if(ZTest > 0.999f && ZTest < 1.001f)
            {
                movable* HitMovable = GetMovable(World, HitEntity);
                entity* TestEntity = GetEntity(EntityStorage, RigidBody);                    
                
                if(BestContact.Contact.Normal.z < 0)
                {
                    HitMovable->ChildID = TestEntity->ID;                       
                    if(TestEntity->Type == ENTITY_TYPE_MOVABLE)
                    {
                        movable* TestMovable = GetMovable(World, TestEntity);
                        TestMovable->ParentID = HitEntity->ID;
                        AK_Assert(ValidateMovable(TestMovable), "Corrupted movable");
                    }
                }
                else
                {
                    HitMovable->ParentID = TestEntity->ID;                       
                    if(TestEntity->Type == ENTITY_TYPE_MOVABLE)
                    {
                        movable* TestMovable = GetMovable(World, TestEntity);
                        TestMovable->ChildID = HitEntity->ID;
                        AK_Assert(ValidateMovable(TestMovable), "Corrupted movable");
                    }
                }
                
                AK_Assert(ValidateMovable(HitMovable), "Corrupted movable");
            }
        }
        
        
        ak_f32 MoveDistance = AK_Magnitude(RigidBody->MoveDelta);
        ak_v3f Direction = RigidBody->MoveDelta/MoveDistance;
        
        ak_f32 NearestDistance = MoveDistance*BestContact.t;                        
        ak_f32 ShortenDistance = AK_Max(NearestDistance-VERY_CLOSE_DISTANCE, 0.0f);
        
        if(AK_Dot(-BestContact.Contact.Normal, AK_ZAxis()) < 0.7f)
        {                            
            ak_v3f Destination = RigidBody->Transform.Translation + RigidBody->MoveDelta;
            ak_v3f NewBasePoint = RigidBody->Transform.Translation + Direction*ShortenDistance;                        
            
            ak_planef SlidingPlane = AK_Plane(NewBasePoint, -BestContact.Contact.Normal);
            ak_v3f NewDestination = Destination - AK_SignDistance(SlidingPlane, Destination)*SlidingPlane.Normal;       
            
            RigidBody->MoveDelta = NewDestination - NewBasePoint;
            
            RigidBody->Transform.Translation = NewBasePoint;
            if(AK_Magnitude(RigidBody->MoveDelta) < VERY_CLOSE_DISTANCE)                                                    
                break;                        
        }
        else
        {                                                        
            RigidBody->Transform.Translation += Direction*ShortenDistance;                         
            if(ShortenDistance == 0.0f)
                break;
        }
    }    
    
    RigidBody->MoveDelta = {};
}

void ClearAllMovableChildren(world* World)
{    
    for(ak_u32 WorldIndex = 0; WorldIndex < 1; WorldIndex++)
    {        
        entity_storage* EntityStorage = &World->EntityStorage[WorldIndex];         
        AK_ForEach(Entity, EntityStorage)
        {
            if(Entity->Type == ENTITY_TYPE_MOVABLE)
            {
                movable* Movable = GetMovable(World, Entity);
                Movable->ChildID = InvalidWorldID();
                Movable->ParentID = InvalidWorldID();
            }
        }
    }    
}

#if 1
extern "C"
AK_EXPORT GAME_FIXED_TICK(FixedTick)
{
    Dev_SetDeveloperContext(DevContext);
    AK_SetGlobalArena(Game->TempStorage);
    
    ak_arena* GlobalArena = Game->TempStorage;
    
    ak_f32 dt = Game->dtFixed;
    
    world* World = &Game->World;
    input* Input = Game->Input;
    
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
    
    if(IsPressed(Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;     
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];                
        AK_ForEach(SimEntity, &Simulation->SimEntityStorage)
            SimEntity->Transform = World->NewTransforms[WorldIndex][UserDataToIndex(SimEntity->UserData)];        
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)        
            RigidBody->Transform = World->NewTransforms[WorldIndex][UserDataToIndex(RigidBody->UserData)];                                
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 1; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        entity_storage* EntityStorage = &World->EntityStorage[WorldIndex]; 
        
        AK_ForEach(Entity, EntityStorage)
        {
            switch(Entity->Type)
            { 
                case ENTITY_TYPE_PLAYER:
                {
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    player* Player = (player*)Entity->UserData;
                    
                    RigidBody->Velocity += AK_V3(InputDirection*Global_PlayerAcceleration)*dt;
                    RigidBody->Velocity *= (1.0f/(1.0f + 6.0f*dt));                                                            
                    MovementPushableUpdate(World, WorldIndex, RigidBody, dt);                                                            
                } break;
            }
        }
    }
    
    ClearAllMovableChildren(World);
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 1; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        entity_storage* EntityStorage = &World->EntityStorage[WorldIndex]; 
        
        AK_ForEach(Entity, EntityStorage)
        {
            switch(Entity->Type)
            { 
                case ENTITY_TYPE_PLAYER:
                {
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    player* Player = (player*)Entity->UserData;
                    
                    Player->GravityVelocity.z -= Global_Gravity*dt;
                    Player->GravityVelocity *= (1.0f/(1.0f + 3.0f*dt));
                    
                    MovementGravityUpdate(World, WorldIndex, RigidBody, Player->GravityVelocity, dt);
                    
                    World->NewCameras[WorldIndex].Target = RigidBody->Transform.Translation;
                } break;
                
                case ENTITY_TYPE_MOVABLE:
                {
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    movable* Movable = GetMovable(World, Entity);
                    
                    Movable->GravityVelocity.z -= Global_Gravity*dt;
                    Movable->GravityVelocity *= (1.0f/(1.0f + 3.0f*dt));
                    
                    MovementGravityUpdate(World, WorldIndex, RigidBody, Movable->GravityVelocity, dt);
                    
                } break;
            }
        }
    }
    
    AK_Assert(Game_ValidateAllMovables(World), "Failed to validate movables");
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];                
        Simulation->CollisionEvents.Count = 0;
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {        
        simulation* Simulation = &World->Simulations[WorldIndex];                
        AK_ForEach(SimEntity, &Simulation->SimEntityStorage)
            World->NewTransforms[WorldIndex][UserDataToIndex(SimEntity->UserData)] = SimEntity->Transform;        
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)        
            World->NewTransforms[WorldIndex][UserDataToIndex(RigidBody->UserData)] = RigidBody->Transform;                                
    }    
}
#endif

#if 0
extern "C"
AK_EXPORT GAME_FIXED_TICK(FixedTick)
{   
    Dev_SetDeveloperContext(DevContext);
    
    ak_high_res_clock Start = AK_WallClock();    
    AK_SetGlobalArena(Game->TempStorage);
    
    ak_f32 dt = Game->dtFixed;
    
    world* World = &Game->World;    
    input* Input = Game->Input;
    
    ak_v2f MoveDirection = {};    
    if(IsDown(Input->MoveForward))
        MoveDirection.y = 1.0f;
    
    if(IsDown(Input->MoveBackward))
        MoveDirection.y = -1.0f;
    
    if(IsDown(Input->MoveRight))
        MoveDirection.x = 1.0f;
    
    if(IsDown(Input->MoveLeft))
        MoveDirection.x = -1.0f;
    
    if(MoveDirection != 0.0f)
        MoveDirection = AK_Normalize(MoveDirection);
    
    if(IsPressed(Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;     
    
    local ak_bool ApplyGravity;        
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        graphics_state* GraphicsState = &World->GraphicsStates[WorldIndex];
        entity_storage* EntityStorage = &World->EntityStorage[WorldIndex]; 
        
        AK_ForEach(Entity, EntityStorage)
        {
            sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);            
            SimEntity->Transform = World->NewTransforms[WorldIndex][AK_PoolIndex(Entity->ID.ID)];
            
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    Entity->OnCollision = OnPlayerCollision;                
                    rigid_body* RigidBody = SimEntity->ToRigidBody();
                    
                    Simulation->AddConstraint(RigidBody, NULL, LockConstraint);
                    
                    player* Player = (player*)Entity->UserData;
                    
                    switch(Player->PlayerState)
                    {
                        case PLAYER_STATE_NONE:
                        {
                            RigidBody->LinearDamping = AK_V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                            
                            if(ApplyGravity)
                                RigidBody->Acceleration.z = -Global_Gravity;
                        } break;
                        
                        case PLAYER_STATE_MOVING_OBJECT:
                        {
                            RigidBody->LinearDamping = AK_V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                            rigid_body* MovingSimEntity = Simulation->GetSimEntity(EntityStorage->Get(Player->MovingEntityID.ID)->SimEntityID)->ToRigidBody();
                            MovingSimEntity->LinearDamping = AK_V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                            
                            if(ApplyGravity)
                                RigidBody->Acceleration.z = -Global_Gravity;
                            MovingSimEntity->Acceleration.z = -Global_Gravity;                            
                        } break;
                        
                        case PLAYER_STATE_JUMPING:
                        {
                            RigidBody->LinearDamping = {};
                            RigidBody->Acceleration.z = -Global_Gravity;                            
                        } break;
                    }     
                    
                    if(Game->CurrentWorldIndex == WorldIndex)
                    {                        
                        switch(Player->PlayerState)
                        {
                            case PLAYER_STATE_NONE:
                            {                                
                                AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[WorldIndex])        
                                    JumpingQuad->Color = AK_Yellow3();                                        
                                
                                ak_v3f Position = RigidBody->Transform.Translation;
                                AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[Entity->ID.WorldIndex])
                                {                                                                        
                                    ak_v2f HalfDim = JumpingQuad->Dimensions*0.5f;
                                    
                                    ak_v2f Min = JumpingQuad->CenterP.xy - HalfDim;
                                    ak_v2f Max = JumpingQuad->CenterP.xy + HalfDim;
                                    
                                    if(Position.xy >= Min && Position.xy <= Max)
                                    {
                                        if(AK_Abs(Position.z - JumpingQuad->CenterP.z) < 1e-2f)
                                        {
                                            jumping_quad* TargetQuad = World->JumpingQuadStorage[JumpingQuad->OtherQuad.WorldIndex].Get(JumpingQuad->OtherQuad.ID);                                                                                        
                                            JumpingQuad->Color = AK_Green3();                                            
                                            TargetQuad->Color = AK_Red3();
                                            if(IsPressed(Input->Action))
                                            {
                                                
                                                ak_v2f XDirection = TargetQuad->CenterP.xy-JumpingQuad->CenterP.xy;
                                                ak_f32 Displacement = AK_Magnitude(XDirection);
                                                XDirection /= Displacement;                    
                                                
                                                ak_f32 InitialVelocity = AK_Sqrt(Displacement*Global_Gravity);
                                                
                                                RigidBody->Velocity.xy = (InitialVelocity*AK_SQRT2_2*XDirection);
                                                RigidBody->Velocity.z = InitialVelocity*AK_SQRT2_2;
                                                
                                                Player->PlayerState = PLAYER_STATE_JUMPING;                                    
                                                
                                                break;                                                
                                            }                                                   
                                        }
                                    }       
                                }            
                                
                                RigidBody->Acceleration.xy = MoveDirection*Global_PlayerAcceleration;                                
                            } break;
                            
                            case PLAYER_STATE_MOVING_OBJECT:
                            {
                                RigidBody->Acceleration.xy = MoveDirection*Global_PlayerAcceleration;                                
                                rigid_body* MovingSimEntity = Simulation->GetSimEntity(EntityStorage->Get(Player->MovingEntityID.ID)->SimEntityID)->ToRigidBody();                                                
                                if(IsDown(Input->Action))            
                                {
                                    MovingSimEntity->Acceleration.xy = MoveDirection*Global_PlayerAcceleration;                                                                    
                                }
                                else                  
                                {
                                    Player->PlayerState = PLAYER_STATE_NONE;                            
                                    MovingSimEntity->Velocity = {};
                                    MovingSimEntity->Acceleration = {};
                                }
                            } break;                    
                        }                             
                    }                    
                } break;
                
                case ENTITY_TYPE_BUTTON:
                {
                    
                    button_state* ButtonState = GetButtonState(World, Entity);
                    if(ButtonState->Collided && !ButtonState->IsDown)
                    {
                        ButtonState->IsDown = true;
                        SimEntity->Transform.Scale.z *= 0.01f;
                        
                        collision_volume* CollisionVolume = Simulation->CollisionVolumeStorage.Get(SimEntity->CollisionVolumeID);
                        CollisionVolume->ConvexHull.Header.Transform.Scale.z *= 100.0f;
                    }
                    
                    if(!ButtonState->Collided && ButtonState->IsDown && ButtonState->IsToggle)
                    {
                        ButtonState->IsDown = false;
                        SimEntity->Transform.Scale.z *= 100.0f;
                        
                        collision_volume* CollisionVolume = Simulation->CollisionVolumeStorage.Get(SimEntity->CollisionVolumeID);
                        CollisionVolume->ConvexHull.Header.Transform.Scale.z *= 0.01f;
                    }
                    
                    ButtonState->Collided = false;                    
                    Entity->OnCollision = OnButtonCollision;
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    rigid_body* RigidBody = SimEntity->ToRigidBody();
                    RigidBody->LinearDamping = AK_V3(4.0f, 4.0f, 0.0f);
                    RigidBody->AngularDamping = AK_V3(1.0f, 1.0f, 1.0f);
                    RigidBody->ApplyGravity(Global_Gravity);                        
                } break;
            }
        }
    }
    
    ApplyGravity = false;    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        broad_phase_pair_list PairList = Simulation->GetAllPairs();
        for(ak_u32 PairIndex = 0; PairIndex < PairList.Count; PairIndex++)
        {
            broad_phase_pair* Pair = PairList.Ptr + PairIndex;
            
            ak_u32 EntityIndexA = UserDataToIndex(Pair->SimEntityA->UserData);
            ak_u32 EntityIndexB = UserDataToIndex(Pair->SimEntityB->UserData);
            
            entity_type TypeA = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexA)->Type;
            entity_type TypeB = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexB)->Type;
            
            if(PAIR_IS_TYPE(TypeA, TypeB, ENTITY_TYPE_RIGID_BODY))
            {
                contact_list ContactList = Simulation->ComputeContacts(Pair);
                if(!PAIR_IS_TYPE(TypeA, TypeB, ENTITY_TYPE_BUTTON))
                    Simulation->AddContactConstraints(Pair->SimEntityA->ToRigidBody(), Pair->SimEntityB->ToRigidBody(), ContactList);                
            }            
            else if(PAIR_IS_TYPE(TypeA, TypeB, ENTITY_TYPE_BUTTON))
            {
                Simulation->ComputeIntersection(Pair);
            }
            else if(PAIR_IS_TYPE(TypeA, TypeB, ENTITY_TYPE_MOVABLE) &&
                    PAIR_IS_TYPE(TypeA, TypeB, ENTITY_TYPE_PLAYER))
            {
                sim_entity* MovableSimEntity = (TypeA == ENTITY_TYPE_MOVABLE) ? Pair->SimEntityA : Pair->SimEntityB;
                ak_v3f OldScale = MovableSimEntity->Transform.Scale;
                MovableSimEntity->Transform.Scale *= 1.3f;                
                Simulation->ComputeDeepestContact(Pair);                             
                MovableSimEntity->Transform.Scale = OldScale;                
            }
        }        
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        Simulation->Integrate(dt);    
        Simulation->SolveConstraints(30, dt);
    }    
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        entity_storage* EntityStorage = &World->EntityStorage[WorldIndex];
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {
            ak_u32 EntityIndex = UserDataToIndex(RigidBody->UserData);
            
            entity* Entity = EntityStorage->GetByIndex(EntityIndex);
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    player* Player = (player*)Entity->UserData;
                    
                    filter_common* FilterCommonData = GlobalArena->Push<filter_common>();
                    FilterCommonData->World = World;
                    FilterCommonData->WorldIndex = WorldIndex;
                    
                    ak_v3f OriginalDelta = RigidBody->MoveDelta;                                                          
                    RigidBody->MoveDelta = AK_V3(OriginalDelta.xy);             
                    
                    for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
                    {
                        ak_f32 DeltaLength = AK_Magnitude(RigidBody->MoveDelta);                        
                        if(DeltaLength > 0)
                        {
                            ak_v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;
                            broad_phase_pair_list Pairs = Simulation->GetPairs(RigidBody, FilterCommon, FilterCommonData);            
                            
                            continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                        
                            if(ContactTOI.HitEntity)
                            {                                
                                ak_v3f Normal = -ContactTOI.Contact.Normal; 
                                
                                ak_f32 tMin = ContactTOI.t;
                                
                                RigidBody->Transform.Translation += Normal*0.0001f;
                                RigidBody->Transform.Translation += tMin*RigidBody->MoveDelta;                                
                                
                                RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;                
                                RigidBody->MoveDelta -= AK_Dot(RigidBody->MoveDelta, Normal)*Normal;                                  
                                RigidBody->Velocity -= AK_Dot(RigidBody->Velocity, Normal)*Normal;                                
                            }
                            else
                            {
                                RigidBody->Transform.Translation = TargetPosition;
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }  
                    
                    RigidBody->MoveDelta = AK_V3(0.0f, 0.0f, OriginalDelta.z);                                        
                    for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
                    {
                        ak_f32 DeltaLength = AK_Magnitude(RigidBody->MoveDelta);
                        if(DeltaLength > 0)
                        {
                            ak_v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;
                            broad_phase_pair_list Pairs = Simulation->GetPairs(RigidBody, FilterCommon, FilterCommonData);            
                            
                            continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);     
                            
                            if(ContactTOI.HitEntity)
                            {                                
                                ak_v3f Normal = -ContactTOI.Contact.Normal;                                                                       
                                ak_f32 tMin = ContactTOI.t;
                                
                                RigidBody->Transform.Translation += Normal*0.0001f;
                                RigidBody->Transform.Translation += tMin*RigidBody->MoveDelta;                
                                
                                if(ApplyGravity)
                                {                
                                    RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;                
                                    RigidBody->MoveDelta -= AK_Dot(RigidBody->MoveDelta, Normal)*Normal;                                                                                                                                                                                   
                                    //RigidBody->Velocity -= AK_Dot(RigidBody->Velocity, Normal)*Normal;
                                }
                            }
                            else
                            {
                                RigidBody->Transform.Translation = TargetPosition;
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    
                    World->NewCameras[WorldIndex].Target = RigidBody->Transform.Translation;                   
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    
                    broad_phase_pair_list StaticOnlyPairs = Simulation->GetPairs(RigidBody, FilterSimEntityOnlyCollisions);
                    continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, StaticOnlyPairs);
                    
                    ak_f32 t = 1.0f;
                    if(ContactTOI.HitEntity)
                    {                    
                        t = ContactTOI.t;                    
                        if(t != 0)
                            Simulation->AddContactConstraint(RigidBody, NULL, ContactTOI.Contact);                                                                                    
                        else
                            t = 1.0f;
                    }
                    
                    RigidBody->Transform.Translation += RigidBody->MoveDelta*t;
                } break;
            }
        }
    }
    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        for(ak_u32 CollisionEventIndex = 0; CollisionEventIndex < Simulation->CollisionEvents.Count; CollisionEventIndex++)
        {
            collision_event* Event = &Simulation->CollisionEvents.Ptr[CollisionEventIndex];
            
            ak_u32 EntityIndexA = UserDataToIndex(Event->SimEntityA->UserData);
            ak_u32 EntityIndexB = UserDataToIndex(Event->SimEntityB->UserData);
            
            entity* EntityA = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexA);
            entity* EntityB = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexB);
            
            if(EntityA->OnCollision) EntityA->OnCollision(Game, EntityA, EntityB,  Event->Normal);
            if(EntityB->OnCollision) EntityB->OnCollision(Game, EntityB, EntityA, -Event->Normal);
        }
        Simulation->CollisionEvents.Count = 0;
    }    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];        
        
        AK_ForEach(SimEntity, &Simulation->SimEntityStorage)
            World->NewTransforms[WorldIndex][UserDataToIndex(SimEntity->UserData)] = SimEntity->Transform;
        
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)        
            World->NewTransforms[WorldIndex][UserDataToIndex(RigidBody->UserData)] = RigidBody->Transform;                        
    }
}

#endif

extern "C"
AK_EXPORT GAME_TICK(Tick)
{   
    Dev_SetDeveloperContext(DevContext);    
    AK_SetGlobalArena(Game->TempStorage);       
}

extern "C"
AK_EXPORT GAME_RENDER(Render)
{   
    Dev_SetDeveloperContext(DevContext);
    
    InterpolateState(&Game->World, WorldIndex, tInterpolate);                     
    graphics_state* GraphicsState = &Game->World.GraphicsStates[WorldIndex];
    
    AK_SetGlobalArena(Game->TempStorage);
    UpdateRenderBuffer(Graphics, &GraphicsState->RenderBuffer, Game->Resolution);
    
    camera* Camera = Dev_GetCamera(WorldIndex);
    view_settings ViewSettings = GetViewSettings(Camera);
    
    NormalEntityPassPlusJumpingQuad(Graphics, Game, GraphicsState, &Game->World.JumpingQuadStorage[WorldIndex], &ViewSettings);
    
    Dev_DrawGrid(GraphicsState, &ViewSettings);
    
#if !DEVELOPER_BUILD
    PushCopyToOutput(Graphics, Game->RenderBuffer, AK_V2(0, 0), Game->RenderBuffer->Resolution);
#endif    
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>