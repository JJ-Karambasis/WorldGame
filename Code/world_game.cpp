#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "simulation/simulation.cpp"
#include "entity.cpp"
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
        
        Game->Simulations[WorldIndex] = CreateSimulation(Game->GameStorage);
        
        v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
        capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);        
        entity_id PlayerID = CreatePlayerEntity(Game, WorldIndex, V3(), V3(), 65, Global_PlayerMaterial, &PlayerCapsule);
        
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
    CreateSphereRigidBody(Game, 0, V3( 1.0f, 1.0f, 5.0f), 0.5f, 30.0f, 0.2f, Global_Material0);
    CreatePushableBox(Game, 0, V3(-2.0f, 0.0f, 0.001f), 1.0f, 35.0f, Global_Material0);
    
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
    SET_DEVELOPER_CONTEXT();
            
    arena GameStorage = CreateArena(MEGABYTE(32));            
    assets* Assets = InitAssets(&GameStorage, AssetPath);
    if(!Assets)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }        
    
    arena TempStorage = CreateArena(MEGABYTE(32));
    
    game* Game = PushStruct(&GameStorage, game, Clear, 0);
    Game->_Internal_GameStorage_ = GameStorage;
    Game->GameStorage = &Game->_Internal_GameStorage_;
    
    Game->_Internal_TempStorage_ = TempStorage;
    Game->TempStorage = &Game->_Internal_TempStorage_;
    
    SetDefaultArena(Game->TempStorage);
    
    Game->Input       = Input;
    Game->AudioOutput = AudioOutput;
    Game->Assets      = Assets;    
    
    //TODO(JJ): Load world/entity data at runtime    
    LoadTestLevel(Game);
    
    return Game;
}

extern "C"
EXPORT GAME_FIXED_TICK(FixedTick)
{   
    platform_time Start = WallClock();    
    SetDefaultArena(Game->TempStorage);
    
    entity_storage* WorldStorage = Game->EntityStorage + Game->CurrentWorldIndex;
    simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);
    
    f32 dt = Game->dtFixed;
    
    FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
    {
        entity* Entity = GetUserData(RigidBody, entity);
        RigidBody->Transform = *GetEntityTransform(Game, Entity->ID);
        
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                Entity->OnCollision = OnPlayerCollision;                
                Simulation->AddConstraint(RigidBody, NULL, LockConstraint);
                
                player* Player = GetUserData(Entity, player);
                
                switch(Player->State)
                {
                    case PLAYER_STATE_NONE:
                    {
                        RigidBody->LinearDamping = V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                    } break;
                    
                    case PLAYER_STATE_JUMPING:
                    {
                        RigidBody->LinearDamping = {};
                    } break;
                }                                
            } break;
            
            case ENTITY_TYPE_PUSHABLE:
            {
                Simulation->AddConstraint(RigidBody, NULL, LockConstraint);
                
                pushing_object* PushingObject = GetUserData(Entity, pushing_object);
                if(PushingObject->PlayerID.IsValid())
                {                    
                    ASSERT(GetUserData(GetEntity(Game, PushingObject->PlayerID), player)->State == PLAYER_STATE_PUSHING);                    
                    RigidBody->LinearDamping = V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                }
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {
                RigidBody->LinearDamping = V3(4.0f, 4.0f, 0.0f);
                RigidBody->AngularDamping = V3(1.0f, 1.0f, 1.0f);
            } break;            
        }
    }
    
    broad_phase_pair_list PairList = Simulation->GetAllPairs();
    for(u32 PairIndex = 0; PairIndex < PairList.Count; PairIndex++)
    {
        broad_phase_pair* Pair = PairList.Ptr + PairIndex;
        
        if((GetUserData(Pair->SimEntityA, entity)->Type == ENTITY_TYPE_RIGID_BODY) ||
           (GetUserData(Pair->SimEntityB, entity)->Type == ENTITY_TYPE_RIGID_BODY))
        {
            Simulation->ComputeContacts(Pair);            
        }
    }
    
    Simulation->Integrate(dt);    
    Simulation->SolveConstraints(30, dt);
    
    FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
    {
        entity* Entity = GetUserData(RigidBody, entity);
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                player* Player = GetUserData(Entity, player);
                if(Player->State == PLAYER_STATE_PUSHING)
                    continue;
                
                for(u32 Iterations = 0; Iterations < 4; Iterations++)
                {
                    f32 DeltaLength = Magnitude(RigidBody->MoveDelta);
                    
                    if(DeltaLength > 1e-4f)
                    {            
                        v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;           
                        
                        broad_phase_pair_list Pairs = Simulation->FilterPairs(Simulation->GetAllPairs(RigidBody), 
                                                                              [](broad_phase_pair* Pair) -> b32
                                                                              {
                                                                                  if(GetUserData(Pair->SimEntityB, entity)->Type == ENTITY_TYPE_RIGID_BODY)
                                                                                      return false;
                                                                                  return true;
                                                                              });
                                                
                        continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                        
                        
                        sim_entity* HitEntity = ContactTOI.HitEntity;
                        if(!HitEntity)
                        {
                            RigidBody->Transform.Translation = TargetPosition;                            
                            break;
                        }
                        else
                        {
                            contact* Contact = &ContactTOI.Contact;
                            DEBUG_DRAW_CONTACT(Contact);                 
                            
                            v3f Normal = -Contact->Normal;
                            
                            RigidBody->Transform.Translation += RigidBody->MoveDelta*ContactTOI.t;
                            RigidBody->Transform.Translation += Normal*1e-4f;
                            
                            RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;
                            
                            RigidBody->MoveDelta -= Dot(RigidBody->MoveDelta, Normal)*Normal;
                            RigidBody->Velocity -= Dot(RigidBody->Velocity, Normal)*Normal;                                                        
                        }                        
                    }
                    else
                    {
                        break;
                    }                                                       
                }     
                
                Game->CurrentCameras[Game->CurrentWorldIndex].Target = RigidBody->Transform.Translation;                   
            } break;
            
            case ENTITY_TYPE_PUSHABLE:
            {           
                v3f StartPosition = RigidBody->Transform.Translation;
                for(u32 Iterations = 0; Iterations < 4; Iterations++)
                {
                    f32 DeltaLength = Magnitude(RigidBody->MoveDelta);
                    
                    if(DeltaLength > 1e-4f)
                    {            
                        v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;           
                        
                        broad_phase_pair_list Pairs = Simulation->FilterPairs(Simulation->GetAllPairs(RigidBody), 
                                                                              [](broad_phase_pair* Pair) -> b32
                                                                              {
                                                                                  if(GetUserData(Pair->SimEntityB, entity)->Type == ENTITY_TYPE_RIGID_BODY)
                                                                                      return false;
                                                                                  return true;
                                                                              });
                        
                        continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                        
                        
                        sim_entity* HitEntity = ContactTOI.HitEntity;
                        if(!HitEntity)
                        {
                            RigidBody->Transform.Translation = TargetPosition;                            
                            break;
                        }
                        else
                        {
                            contact* Contact = &ContactTOI.Contact;
                            DEBUG_DRAW_CONTACT(Contact);                 
                            
                            v3f Normal = -Contact->Normal;
                            
                            RigidBody->Transform.Translation += RigidBody->MoveDelta*ContactTOI.t;
                            RigidBody->Transform.Translation += Normal*1e-4f;
                            
                            RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;
                            
                            RigidBody->MoveDelta -= Dot(RigidBody->MoveDelta, Normal)*Normal;
                            RigidBody->Velocity -= Dot(RigidBody->Velocity, Normal)*Normal;                                                        
                        }                        
                    }
                    else
                    {
                        break;
                    }                                                       
                }     
                
                pushing_object* PushingObject = GetUserData(Entity, pushing_object);
                if(PushingObject->PlayerID.IsValid())
                {
                    entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                    sim_entity* PlayerRigidBody = Simulation->GetSimEntity(PlayerEntity->SimEntityID);
                    
                    PlayerRigidBody->Transform.Translation += (RigidBody->Transform.Translation-StartPosition);
                    Game->CurrentCameras[Game->CurrentWorldIndex].Target = PlayerRigidBody->Transform.Translation;                    
                }
                
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {
                
                broad_phase_pair_list StaticOnlyPairs = Simulation->GetSimEntityOnlyPairs(RigidBody);
                continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, StaticOnlyPairs);
                
                f32 t = 1.0f;
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
    
    for(u32 CollisionEventIndex = 0; CollisionEventIndex < Simulation->CollisionEvents.Count; CollisionEventIndex++)
    {
        collision_event* Event = &Simulation->CollisionEvents.Ptr[CollisionEventIndex];
        
        entity* EntityA = GetUserData(Event->SimEntityA, entity);
        entity* EntityB = GetUserData(Event->SimEntityB, entity);
        
        if(EntityA->OnCollision) EntityA->OnCollision(Game, EntityA, EntityB,  Event->Normal);
        if(EntityB->OnCollision) EntityB->OnCollision(Game, EntityB, EntityA, -Event->Normal);
    }
    Simulation->CollisionEvents.Count = 0;
    
    FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
    {        
        sqt* SQT = GetEntityTransform(Game, GetUserData(RigidBody, entity)->ID);        
        *SQT = RigidBody->Transform;        
    }                
}

extern "C"
EXPORT GAME_TICK(Tick)
{   
    SET_DEVELOPER_CONTEXT();
    
    SetDefaultArena(Game->TempStorage);
    f32 dt = Game->dt;
        
    input* Input = Game->Input;
    
    v2f MoveDirection = {};    
    if(IsDown(Input->MoveForward))
        MoveDirection.y = 1.0f;
    
    if(IsDown(Input->MoveBackward))
        MoveDirection.y = -1.0f;
    
    if(IsDown(Input->MoveRight))
        MoveDirection.x = 1.0f;
    
    if(IsDown(Input->MoveLeft))
        MoveDirection.x = -1.0f;
    
    if(MoveDirection != 0)
        MoveDirection = Normalize(MoveDirection);
    
    simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);
    FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
    {   
        RigidBody->MoveDelta = {};
        RigidBody->Acceleration = {};
        RigidBody->AngularAcceleration = {};
        RigidBody->ClearForce();
        RigidBody->ClearTorque();
        
        entity* Entity = GetUserData(RigidBody, entity);
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                player* Player = GetUserData(Entity, player);
                
                switch(Player->State)
                {
                    case PLAYER_STATE_NONE:
                    {            
                        v3f Position = GetEntityPosition(Game, Entity->ID);                               
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
                                        
                                        RigidBody->Velocity.xy = (InitialVelocity*SQRT2_2*XDirection);
                                        RigidBody->Velocity.z = InitialVelocity*SQRT2_2;
                                        
                                        Player->State = PLAYER_STATE_JUMPING;                                    
                                        
                                        break;                                                
                                    }                
                                    QuadColor = Yellow3();
                                }
                            }       
                        }            
                        
                        RigidBody->ApplyConstantAcceleration(MoveDirection, Global_PlayerAcceleration);
                        RigidBody->ApplyGravity(Global_Gravity);                        
                    } break;
                    
                    case PLAYER_STATE_PUSHING:
                    {
                        RigidBody->Velocity = {};
                        RigidBody->AngularVelocity = {};
                    } break;
                    
                    case PLAYER_STATE_JUMPING:
                    {
                        RigidBody->ApplyGravity(Global_Gravity);                        
                    } break;
                }
                
            } break;
            
            case ENTITY_TYPE_PUSHABLE:
            {
                pushing_object* PushingObject = GetUserData(Entity, pushing_object);
                if(PushingObject->PlayerID.IsValid())
                {
                    entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                    player* Player = GetUserData(PlayerEntity, player);
                    ASSERT(Player->State == PLAYER_STATE_PUSHING);
                    
                    if(CanBePushed(MoveDirection, PushingObject->Direction))
                    {
                        RigidBody->ApplyConstantAcceleration(MoveDirection, Global_PlayerAcceleration);
                    }                    
                    else
                    {
                        Player->State = PLAYER_STATE_NONE;
                        PushingObject->PlayerID = InvalidEntityID();
                        Simulation->GetSimEntity(PlayerEntity->SimEntityID)->ToRigidBody()->Velocity = RigidBody->Velocity;
                        RigidBody->Velocity = {};
                    }
                }
                
                RigidBody->ApplyGravity(Global_Gravity);
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {                
                RigidBody->ApplyGravity(Global_Gravity); 
            } break;
        }
    }                    
}

extern "C"
EXPORT GAME_RENDER(Render)
{
    SetDefaultArena(Game->TempStorage);
    UpdateRenderBuffer(&Game->RenderBuffer, Graphics, Graphics->RenderDim);        
    view_settings ViewSettings = GetViewSettings(&GraphicsState->Camera);
    PushWorldShadingCommands(Graphics, Game->RenderBuffer, &ViewSettings, Game->Assets, GraphicsState->GraphicsObjects);
    PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Game->RenderBuffer->Resolution);
}