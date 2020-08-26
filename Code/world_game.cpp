#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "entity.cpp"

#include "simulation/simulation.cpp"
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
        Game->PrevTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);        
        Game->CurrentTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);        
        
        simulation* Simulation = Game->Simulations + WorldIndex;
        Simulation->CollisionVolumeStorage = CreatePool<collision_volume>(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity*4);
        Simulation->ContactStorage = CreatePool<list_entry<contact_constraint>>(Game->GameStorage, 128);
        Simulation->ManifoldStorage = CreatePool<manifold>(Game->GameStorage, 32);
        Simulation->RigidBodyStorage = CreatePool<rigid_body>(Game->GameStorage, 64);
        Simulation->SimEntityStorage = CreatePool<sim_entity>(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity);
        
        v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
        capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);        
        entity_id PlayerID = CreatePlayerEntity(Game, WorldIndex, V3(), V3(), 35, Global_PlayerMaterial, &PlayerCapsule);
        
        game_camera* Camera = Game->CurrentCameras + WorldIndex;        
        Camera->Target = GetEntityPosition(Game, PlayerID);        
        Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));        
        Camera->FieldOfView = TO_RAD(65.0f);                        
        Camera->ZNear = CAMERA_ZNEAR;
        Camera->ZFar = CAMERA_ZFAR;        
        
        Game->PrevCameras[WorldIndex] = *Camera;
    }
        
    CreateStaticEntity(Game, 0, V3(0, 0, -0.01f), V3(10.0f, 10.0f, 0.01f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-1.6f, -5.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f),  MESH_ASSET_ID_BOX,   Global_Material1);                    
    CreateSphereRigidBody(Game, 0, V3( 1.0f, 0.5f, 5.0f), 0.5f, 30.0f, 0.2f, Global_Material0);
    
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

inline f32 GetDamp(f32 dt, f32 Damping)
{
    //CONFIRM(JJ): Should the damping constant be configurable per entity?
    f32 Result = 1.0f / (1.0f + dt * Damping);
    return Result;
}

extern "C"
EXPORT GAME_FIXED_TICK(FixedTick)
{    
    f32 dt = Game->dtFixed;    
    platform_time Start = WallClock();
    
    entity_storage* WorldStorage = Game->EntityStorage + Game->CurrentWorldIndex;
    simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);
    
    Simulation->ContactStorage.FreeAll();
    Simulation->ManifoldStorage.FreeAll();    
    
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage)
    {
        entity* Entity = (entity*)SimEntity->UserData;
        sqt* SQT = GetEntityTransform(Game, Entity->ID);
        SimEntity->Transform = *SQT;
        
        rigid_body* RigidBody = SimEntity->RigidBody;
        if(RigidBody)            
        {
            RigidBody->WorldInvInertiaTensor = GetWorldInvInertiaTensor(RigidBody->LocalInvInertiaTensor, SimEntity->Transform.Orientation);                
            RigidBody->WorldCenterOfMass = TransformV3(RigidBody->LocalCenterOfMass, SimEntity->Transform);
        }
    }
    
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage)
    {
        entity* Entity = (entity*)SimEntity->UserData;
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {                
                rigid_body* RigidBody = SimEntity->RigidBody;
                RigidBody->AngularVelocity = {};
                
                SimEntity->Velocity += SimEntity->Acceleration*dt;                                
                if(!IsEntityState(Entity, ENTITY_STATE_JUMPING))
                    SimEntity->Velocity.xy *= GetDamp(dt, Global_PlayerDamping);                                
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {
                SimEntity->Velocity += SimEntity->Acceleration*dt;
                SimEntity->Velocity.xy *= GetDamp(dt, 1.0f);
                
                rigid_body* RigidBody = SimEntity->RigidBody;
                
                RigidBody->AngularVelocity += RigidBody->AngularAcceleration*dt;
                RigidBody->AngularAcceleration *= GetDamp(dt, 5.0f);
            } break;
        }                
    }
    
    //NOTE(EVERYONE): This checks for all possible collision pairs between non-static entities. There are no
    //duplicate entries. 
    //TODO(SOMEONE): Right now this is will give O(n^2) pairs so if n starts getting pretty large we should probably use
    //some sort of spatial partioning to reduce the amount of pairs even further. Recommand a bounding-volume hierarchy
    //but other types of spatial partioning data structures may work well too.
    collision_pair_list PotentialPairs = {};
    PotentialPairs.Capacity = 1024;
    PotentialPairs.Ptr = PushArray(PotentialPairs.Capacity, collision_pair, Clear, 0);    
    
#define PAIR_CHECK_COUNT 8092
    collision_pair_check PairCheck[PAIR_CHECK_COUNT] = {};
    
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage)
    {
        if(!IsEntityType((entity*)SimEntity->UserData, ENTITY_TYPE_STATIC))
        {
            FOR_EACH(TestSimEntity, &Simulation->SimEntityStorage)
            {
                if(!IsEntityType((entity*)TestSimEntity->UserData, ENTITY_TYPE_STATIC) && (TestSimEntity != SimEntity))
                {
                    u32 AIndex = Simulation->SimEntityStorage.GetIndex(SimEntity);
                    u32 BIndex = Simulation->SimEntityStorage.GetIndex(TestSimEntity);
                    
                    if(BIndex < AIndex)
                        SWAP(AIndex, BIndex);
                    
                    u64 ID = BijectiveMap(AIndex, BIndex);
                    u64 Index = ID % PAIR_CHECK_COUNT;
                    
                    collision_pair_check* Check = PairCheck + Index++;
                    
                    b32 AlreadyCollided = false;
                    while(Check->Collided)
                    {
                        if(Check->ID == ID)
                        {
                            AlreadyCollided = true;
                            break;
                        }                        
                        Check = PairCheck + Index++;
                        ASSERT(Index < PAIR_CHECK_COUNT);
                    }
                    
                    if(!AlreadyCollided)
                    {
                        Check->Collided = true;
                        Check->ID = ID;
                        
                        FOR_EACH(EntityVolume, SimEntity->CollisionVolumes)
                        {
                            FOR_EACH(TestEntityVolume, TestSimEntity->CollisionVolumes)
                            {
                                collision_pair* Pair = PotentialPairs.Ptr + PotentialPairs.Count++;
                                Pair->A = {SimEntity,     EntityVolume};
                                Pair->B = {TestSimEntity, TestEntityVolume};
                            }
                        }                        
                    }
                }
            }
        }
    }
    
    collision_pair_list RigidBodyPairs = {};
    RigidBodyPairs.Capacity = 64;
    RigidBodyPairs.Ptr = PushArray(RigidBodyPairs.Capacity, collision_pair, Clear, 0);
    
    for(u32 PotentialPairIndex = 0; PotentialPairIndex < PotentialPairs.Count; PotentialPairIndex++)
    {
        collision_pair* CollisionPair = PotentialPairs.Ptr + PotentialPairIndex;
        
        sim_entity_volume_pair* PairA = &CollisionPair->A;
        sim_entity_volume_pair* PairB = &CollisionPair->B;                
        
        sim_entity* SimEntityA = PairA->SimEntity;
        sim_entity* SimEntityB = PairB->SimEntity;        
        
        //NOTE(EVERYONE): If any are rigid bodies add to list to generate later
        if(SimEntityA->RigidBody || SimEntityB->RigidBody)
        {
            collision_pair* RigidBodyPair = RigidBodyPairs.Ptr + RigidBodyPairs.Count++;
            if(SimEntityB->RigidBody && !SimEntityA->RigidBody)
            {
                RigidBodyPair->A = *PairB;
                RigidBodyPair->B = *PairA;
            }
            else
            {
                RigidBodyPair->A = *PairA;
                RigidBodyPair->B = *PairB;
            }           
        }                
        
        //NOTE(EVERYONE): This is where we generate other collision events here
    }
    
    //NOTE(EVERYONE): Generate collision events with non-static entities and static entities now (not player since we need to interate that)        
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage)
    {
        entity* Entity = (entity*)SimEntity->UserData;
        switch(Entity->Type)
        {            
            case ENTITY_TYPE_RIGID_BODY:
            {
                FOR_EACH(TestSimEntity, &Simulation->SimEntityStorage)
                {
                    entity* TestEntity = (entity*)TestSimEntity->UserData;
                    if(TestEntity->Type == ENTITY_TYPE_STATIC)
                    {                        
                        FOR_EACH(EntityVolume, SimEntity->CollisionVolumes)
                        {
                            FOR_EACH(TestEntityVolume, TestSimEntity->CollisionVolumes)
                            {
                                collision_pair* Pair = RigidBodyPairs.Ptr + RigidBodyPairs.Count++;
                                Pair->A = {SimEntity,     EntityVolume};
                                Pair->B = {TestSimEntity, TestEntityVolume};
                            }
                        }                        
                    }
                }
            } break;
        }
    }
    
    Simulation->GenerateContacts(&RigidBodyPairs);
    Simulation->SolveConstraints(30, Game->dtFixed);
    
    //NOTE(EVERYONE): After physics, for some objects, we need to perform continuous collision detection on their linear velocities. 
    //This is primarily for entities that are small and fast and are critical for gameplay that cannot tunnel through the colliders,
    //and for player movement to get a gliding mechanic (which is extremely expensive)
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage) SimEntity->MoveDelta = SimEntity->Velocity*Game->dtFixed;
    
    b32 HasCollided = false;
    
    FOR_EACH(SimEntity, &Simulation->SimEntityStorage)
    {
        entity* Entity = (entity*)SimEntity->UserData;                        
        
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                //CONFIRM(JJ): I think we can limit the iterations to 3
                for(u32 Iterations = 0; Iterations < 4; Iterations++)
                {
                    f32 DeltaLength = Magnitude(SimEntity->MoveDelta);
                    
                    if(DeltaLength > 1e-4f)
                    {            
                        v3f TargetPosition = SimEntity->Transform.Translation + SimEntity->MoveDelta;                           
                        continuous_collision CollisionResult = Simulation->DetectStaticContinuousCollisions(SimEntity);
                        
                        if(!CollisionResult.HitEntity)
                        {
                            SimEntity->Transform.Translation = TargetPosition;                            
                            break;
                        }
                        else
                        {   
                            HasCollided = true;
                            penetration* Penetration = &CollisionResult.Penetration;
                            
                            SimEntity->Transform.Translation += SimEntity->MoveDelta*CollisionResult.t;
                            SimEntity->Transform.Translation += Penetration->Normal*1e-5f;
                            
                            SimEntity->MoveDelta = TargetPosition - SimEntity->Transform.Translation;
                            
                            SimEntity->MoveDelta -= Dot(SimEntity->MoveDelta, Penetration->Normal)*Penetration->Normal;
                            SimEntity->Velocity -= Dot(SimEntity->Velocity, Penetration->Normal)*Penetration->Normal;                                                        
                        }
                    }
                    else
                    {
                        break;
                    }                    
                }      
                
                Game->CurrentCameras[Game->CurrentWorldIndex].Target = SimEntity->Transform.Translation;                
                
                if(HasCollided)
                {
                    entity* PlayerEntity = (entity*)SimEntity->UserData;
                    if(IsEntityState(PlayerEntity, ENTITY_STATE_JUMPING))
                        SetEntityState(PlayerEntity, ENTITY_STATE_NONE);
                }
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {
                //NOTE(EVERYONE): Constraint solver can apply an instant velocity, so we need to recompute the move delta. This is
                //fine since the solver is in the last step.                 
                SimEntity->Transform.Translation += SimEntity->MoveDelta;
                rigid_body* RigidBody = SimEntity->RigidBody;
                
                quaternion Delta = RotQuat(RigidBody->AngularVelocity*Game->dtFixed, 0.0f)*SimEntity->Transform.Orientation;
                SimEntity->Transform.Orientation = Normalize(SimEntity->Transform.Orientation + (Delta*0.5f));
                
            } break;
        }
    }
    FOR_EACH(Entity, WorldStorage)
    {
        sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
        sqt* SQT = GetEntityTransform(Game, Entity->ID);        
        *SQT = SimEntity->Transform;        
    }            
}

extern "C"
EXPORT GAME_TICK(Tick)
{    
    f32 dt = Game->dt;
    
    input* Input = Game->Input;
    simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        sim_entity* SimState = Simulation->GetSimEntity(Entity->SimEntityID);                
        
        SimState->Acceleration = {};
        
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {                
                v3f Position = GetEntityPosition(Game, Entity->ID);                
                
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
            
            case ENTITY_TYPE_RIGID_BODY:
            {
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