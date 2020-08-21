#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "world.cpp"
#include "collision_detection.cpp"
//#include "player.cpp"
#include "graphics.cpp"

PUZZLE_COMPLETE_CALLBACK(DespawnWallCompleteCallback)
{
    world_entity_id* IDs = (world_entity_id*)UserData;
    
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
        Game->EntityStorage[WorldIndex] = CreatePool<world_entity>(Game->GameStorage, 512);        
        Game->CollisionVolumeStorage[WorldIndex] = CreatePool<collision_volume>(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity*4);        
        Game->PrevTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);        
        Game->CurrentTransforms[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sqt, Clear, 0);
        Game->SimStates[WorldIndex] = PushArray(Game->GameStorage, Game->EntityStorage[WorldIndex].Capacity, sim_state, Clear, 0);
        
        world_entity_id ID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, V3(0.0f, 0.0f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(PI*0.0f, 0.0f*PI, 0.0f*PI), MESH_ASSET_ID_PLAYER, Global_PlayerMaterial);
        
        v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
        capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);        
        AddCollisionVolume(&Game->CollisionVolumeStorage[WorldIndex], GetSimState(Game, ID), &PlayerCapsule);
        
        game_camera* Camera = Game->Cameras + WorldIndex;        
        Camera->Target = GetEntityPosition(Game, ID);        
        Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));        
        Camera->FieldOfView = TO_RAD(65.0f);                        
        Camera->ZNear = CAMERA_ZNEAR;
        Camera->ZFar = CAMERA_ZFAR;        
    }
        
    CreateStaticEntity(Game, 0, V3(0.0f, 0.0f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_FLOOR, Global_Material0);                           
    CreateStaticEntity(Game, 0, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateStaticEntity(Game, 0, V3(-1.6f, -5.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateStaticEntity(Game, 0, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f),  MESH_ASSET_ID_BOX,   Global_Material1);            
}

extern "C"
EXPORT GAME_INITIALIZE(Initialize)
{
    SET_DEVELOPER_CONTEXT(DevContext);
    Global_Platform = Platform;
    
    InitMemory(Global_Platform->TempArena, AllocateMemory, FreeMemory);
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

#define GRAVITY 20.0f

inline f32 GetLinearDamp(f32 dt)
{
    //CONFIRM(JJ): Should the damping constant be configurable per entity?
    f32 Result = 1.0f / (1.0f + dt*MOVE_DAMPING);
    return Result;
}

void PlayerCollisionEvent(game* Game, world_entity* Player, collision_event* CollisionEvent)
{
    if(Player->State == WORLD_ENTITY_STATE_JUMPING)
        Player->State = WORLD_ENTITY_STATE_NOTHING;
}

extern "C"
EXPORT GAME_FIXED_TICK(FixedTick)
{
    
    f32 dt = Game->dtFixed;
    
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
            {               
                sim_state* SimState = GetSimState(Game, Entity->ID);
                if(Entity->State == WORLD_ENTITY_STATE_JUMPING)  
                {
                    SimState->Velocity.z -= (GRAVITY*dt);                
                }
                else
                {
                    v2f MoveAcceleration = SimState->MoveDirection*MOVE_ACCELERATION;
                    SimState->Velocity.xy += MoveAcceleration*dt;
                    SimState->Velocity.xy *= GetLinearDamp(dt);
                    
                    SimState->Velocity.z -= (GRAVITY*dt);                                        
                }
                
            } break;
        }
    }
    
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
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
            case WORLD_ENTITY_TYPE_PLAYER:
            {
                sim_state* SimState = GetSimState(Game, Entity->ID);                
                v3f Position = GetEntityPosition(Game, Entity->ID);                
                
                SimState->MoveDirection = {};
                
                if(Entity->State != WORLD_ENTITY_STATE_JUMPING)
                {
#if 0 
                    for(u32 JumpingQuadIndex = 0; JumpingQuadIndex < ARRAYCOUNT(World->JumpingQuads); JumpingQuadIndex++)
                    {
                        jumping_quad* JumpingQuad = World->JumpingQuads + JumpingQuadIndex;
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
                                    
                                    f32 InitialVelocity = Sqrt(Displacement*GRAVITY);
                                    
                                    Entity->Velocity.xy = (InitialVelocity*SQRT2_2*XDirection);
                                    Entity->Velocity.z = InitialVelocity*SQRT2_2;
                                    
                                    Entity->State = WORLD_ENTITY_STATE_JUMPING;        
                                    
                                    break;                    
                                    
                                }                
                                QuadColor = Yellow3();
                            }
                        }       
                        
                        DEBUG_DRAW_QUAD(JumpingQuad->CenterP, Global_WorldZAxis, JumpingQuad->Dimensions, QuadColor);    
                    }
#endif
                    
                    SimState->MoveDirection = {};
                    if(IsDown(Input->MoveForward))
                        SimState->MoveDirection.y = 1.0f;
                    
                    if(IsDown(Input->MoveBackward))
                        SimState->MoveDirection.y = -1.0f;
                    
                    if(IsDown(Input->MoveRight))
                        SimState->MoveDirection.x = 1.0f;
                    
                    if(IsDown(Input->MoveLeft))
                        SimState->MoveDirection.x = -1.0f;
                    
                    if(SimState->MoveDirection != 0)
                        SimState->MoveDirection = Normalize(SimState->MoveDirection);                    
                }   
                
                game_camera* Camera = Game->Cameras + Game->CurrentWorldIndex;    
                Camera->Target = Position;        
            } break;
        }
    }
    
}

extern "C"
EXPORT GAME_RENDER(Render)
{
    UpdateRenderBuffer(&Game->RenderBuffer, Graphics, Graphics->RenderDim);    
    
    view_settings ViewSettings = GetViewSettings(&Game->Cameras[Game->CurrentWorldIndex]);        
    PushWorldShadingCommands(Game, Graphics, Game->CurrentWorldIndex, Game->RenderBuffer, &ViewSettings, Game->Assets);
    PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Game->RenderBuffer->Resolution);
}

#if 0 
extern "C"
EXPORT GAME_TICK(Tick)
{   
    SET_DEVELOPER_CONTEXT(DevContext);
    
    Global_Platform = Platform;        
    
    platform_time Start = Global_Platform->Clock();
    
    InitMemory(Global_Platform->TempArena, AllocateMemory, FreeMemory);       
    SetGlobalErrorStream(Global_Platform->ErrorStream);
    
    if(!Game->Initialized)
    {       
        b32 AssetResult = InitAssets(&Game->Assets);
        ASSERT(AssetResult);
        
        Game->GameStorage = CreateArena(MEGABYTE(16));                                                                        
        
        Game->RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim);
        
#if 0 
        Game->Assets->TestSkeletonMesh = LoadGraphicsMesh(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestSkeleton = LoadSkeleton(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestAnimation = LoadAnimation(Game->Assets, "TestAnimation.fbx");
#endif
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->WorldIndex = WorldIndex;
            World->EntityPool = CreatePool<world_entity>(&Game->GameStorage, 512);            
            
            v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
            capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);
            World->PlayerEntity = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, V3(0.0f, 0.0f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(PI*0.0f, 0.0f*PI, 0.0f*PI), MESH_ASSET_ID_PLAYER, &Global_PlayerMaterial);
            AddCollisionVolume(Game, World->PlayerEntity, &PlayerCapsule);
            
            game_camera* Camera = &World->Camera;
            
            Camera->Target = World->PlayerEntity->Position;
            
            Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));
            
            Camera->FieldOfView = TO_RAD(65.0f);                        
            Camera->ZNear = CAMERA_ZNEAR;
            Camera->ZFar = CAMERA_ZFAR;
            
            World->JumpingQuads[0].CenterP = V3(-1.0f, 0.0f, 0.0f);
            World->JumpingQuads[0].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[1].CenterP = V3(-4.0f, 0.0f, 0.0f);
            World->JumpingQuads[1].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[0].OtherQuad = &World->JumpingQuads[1];
            World->JumpingQuads[1].OtherQuad = &World->JumpingQuads[0];
        }
        
        CreateStaticEntity(Game, 0, V3(0.0f, 0.0f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f), MESH_ASSET_ID_FLOOR, &Global_Material0);                           
        CreateStaticEntity(Game, 0, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f), MESH_ASSET_ID_BOX, &Global_Material0);
        CreateStaticEntity(Game, 0, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX, &Global_Material0);
        CreateStaticEntity(Game, 0, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX, &Global_Material0);
        CreateStaticEntity(Game, 0, V3(-1.6f, -5.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f), MESH_ASSET_ID_BOX, &Global_Material1);
        CreateStaticEntity(Game, 0, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f), MESH_ASSET_ID_BOX, &Global_Material1);
        CreateStaticEntity(Game, 0, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f), MESH_ASSET_ID_BOX, &Global_Material1);        
        
        Game->Initialized = true;
    }        
    
    if(IsPressed(Game->Input->SwitchWorld)) 
    {
        u32 PrevIndex = Game->CurrentWorldIndex;
        Game->CurrentWorldIndex = !PrevIndex;
        OnWorldSwitch(Game, PrevIndex, Game->CurrentWorldIndex);          
    }    
    
    if(!IN_EDIT_MODE())
    {   
        UpdateWorld(Game);    
    }            
    
#if 0 
    block_puzzle* Puzzle = &Game->TestPuzzle;    
    if(!Puzzle->IsComplete)
    {        
        Puzzle->IsComplete = true;
        for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
        {
            goal_rect* GoalRect = Puzzle->GoalRects + GoalIndex;        
            
            b32 GoalIsMet = false;
            for(u32 BlockEntityIndex = 0; BlockEntityIndex < Puzzle->BlockEntityCount; BlockEntityIndex++)
            {
                world_entity_id EntityID = Puzzle->BlockEntities[BlockEntityIndex];
                if(EntityID.WorldIndex == GoalRect->WorldIndex)
                {
                    world_entity* Entity = GetEntity(Game, EntityID);
                    
                    ASSERT(Entity->Collider.Type == COLLIDER_TYPE_ALIGNED_BOX);
                    
                    aligned_box AlignedBox = GetWorldSpaceAlignedBox(Entity);
                    rect3D Rect = CreateRect3DCenterDim(AlignedBox.CenterP, AlignedBox.Dim);
                    
                    if(IsRectFullyContainedInRect3D(Rect.Min, Rect.Max, GoalRect->Rect.Min, GoalRect->Rect.Max))
                    {
                        GoalIsMet = true;                    
                        break;
                    }
                }
            }
            
            GoalRect->GoalIsMet = GoalIsMet;        
            
            if(!GoalRect->GoalIsMet)
                Puzzle->IsComplete = false;
        }   
        
        if(Puzzle->IsComplete)
        {
            Puzzle->CompleteCallback(Game, Puzzle->CompleteData);        
        }
    }
#endif
    
    
    FOR_EACH(PlayingAudio, &Game->AudioOutput->PlayingAudioPool)
    {
        if(PlayingAudio->IsFinishedPlaying)        
            FreeFromPool(&Game->AudioOutput->PlayingAudioPool, PlayingAudio);        
    }
    
    if(NOT_IN_DEVELOPMENT_MODE())
    {   
        world* World = GetCurrentWorld(Game);        
        view_settings ViewSettings = GetViewSettings(&World->Camera);        
        
        PushWorldShadingCommands(Graphics, Game->RenderBuffer, World, &ViewSettings, &Game->Assets);        
        PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Graphics->RenderDim);
    }    
    
    platform_time End = Global_Platform->Clock();
    CONSOLE_LOG("Elapsed Game Time %f\n", Global_Platform->ElapsedTime(End, Start)*1000.0);
}
#endif