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
        entity_id PlayerID = CreatePlayerEntity(Game, WorldIndex, V3(0.0f, 0.0f, 0.1f), V3(), 65, Global_PlayerMaterial, &PlayerCapsule);
        
        game_camera* Camera = Game->CurrentCameras + WorldIndex;        
        Camera->Target = GetEntityPosition(Game, PlayerID);        
        Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));        
        Camera->FieldOfView = TO_RAD(65.0f);                        
        Camera->ZNear = CAMERA_ZNEAR;
        Camera->ZFar = CAMERA_ZFAR;        
        
        Game->PrevCameras[WorldIndex] = *Camera;
    }
        
    CreateDualStaticEntity(Game, V3(0, 0, -0.01f), V3(10.0f, 10.0f, 0.01f), V3(0.0f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, V3(-1.6f, -3.5f, -2.0f), V3(1.0f, 1.0f, 10.0f), V3(PI*0.3f, 0.0f, PI*0.0f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateDualStaticEntity(Game, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateDualStaticEntity(Game, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f),  MESH_ASSET_ID_BOX,   Global_Material1);                    
    CreateSphereRigidBody(Game, 0, V3( 1.0f, 1.0f, 5.0f), 0.5f, 30.0f, 0.2f, Global_Material0);
    CreateDualPushableBox(Game, V3(-2.0f, 0.0f, 0.001f), 1.0f, 35.0f, Global_Material0);
    
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

void AddRigidBodyContacts(simulation* Simulation, rigid_body* RigidBody, sim_entity* SimEntity, entity_type TypeB, contact_list ContactList)
{
    switch(TypeB)
    {
        case ENTITY_TYPE_PLAYER:
        {
            player* Player = GetUserData(GetUserData(SimEntity, entity), player);
            if(Player->State == PLAYER_STATE_PUSHING)
            {
                Simulation->AddContactConstraints(RigidBody, NULL, ContactList);            
            }
            else
            {
                Simulation->AddContactConstraints(RigidBody, SimEntity->ToRigidBody(), ContactList);            
            }
        } break;
        
        case ENTITY_TYPE_PUSHABLE:
        {
            pushing_object* PushingObject = GetUserData(GetUserData(SimEntity, entity), pushing_object);
            if(PushingObject->PlayerID.IsValid())
            {
                Simulation->AddContactConstraints(RigidBody, SimEntity->ToRigidBody(), ContactList);
            }
            else
            {
                Simulation->AddContactConstraints(RigidBody, NULL, ContactList);
            }
        } break;
                        
        case ENTITY_TYPE_RIGID_BODY:
        case ENTITY_TYPE_STATIC:
        {
            Simulation->AddContactConstraints(RigidBody, SimEntity->ToRigidBody(), ContactList);            
        } break;
    }
}

void HandleSlidingCollisions(simulation* Simulation, rigid_body* RigidBody)
{            
    v3f MoveDelta = RigidBody->MoveDelta;
    RigidBody->MoveDelta = V3(MoveDelta.xy);
    
    for(u32 Iterations = 0; Iterations < 4; Iterations++)
    {
        f32 DeltaLength = Magnitude(RigidBody->MoveDelta);
        if(DeltaLength > 0)
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
            if(ContactTOI.HitEntity)
            {
                v3f Normal = -ContactTOI.Contact.Normal;
                f32 tMin = ContactTOI.t;
                
                RigidBody->Transform.Translation += Normal*(ContactTOI.Contact.Penetration+0.001f);
                RigidBody->Transform.Translation += tMin*RigidBody->MoveDelta;                                
                
                RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;                
                RigidBody->MoveDelta -= Dot(RigidBody->MoveDelta, Normal)*Normal;
                RigidBody->Velocity -= Dot(RigidBody->Velocity, Normal)*Normal;
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
    
    RigidBody->MoveDelta = V3(0.0f, 0.0f, MoveDelta.z);    
    for(u32 Iterations = 0; Iterations < 4; Iterations++)
    {
        f32 DeltaLength = Magnitude(RigidBody->MoveDelta);
        if(DeltaLength > 0)
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
            if(ContactTOI.HitEntity)
            {
                v3f Normal = -ContactTOI.Contact.Normal;
                f32 tMin = ContactTOI.t;
                
                RigidBody->Transform.Translation += Normal*(ContactTOI.Contact.Penetration+0.001f);
                RigidBody->Transform.Translation += tMin*RigidBody->MoveDelta;                                
                
                RigidBody->MoveDelta = TargetPosition - RigidBody->Transform.Translation;                
                RigidBody->MoveDelta -= Dot(RigidBody->MoveDelta, Normal)*Normal;
                RigidBody->Velocity -= Dot(RigidBody->Velocity, Normal)*Normal;
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
    
}

extern "C"
EXPORT GAME_FIXED_TICK(FixedTick)
{   
    platform_time Start = WallClock();    
    SetDefaultArena(Game->TempStorage);
    
    f32 dt = Game->dtFixed;
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);    
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
                        RigidBody->LinearDamping = V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                        if(Entity->LinkID.IsValid())
                        {
                            rigid_body* OtherRigidBody = GetRigidBody(Game, Entity->LinkID);
                            OtherRigidBody->LinearDamping = RigidBody->LinearDamping;
                        }                        
                    }
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    RigidBody->LinearDamping = V3(4.0f, 4.0f, 0.0f);
                    RigidBody->AngularDamping = V3(1.0f, 1.0f, 1.0f);
                } break;            
            }
        }
    }
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        broad_phase_pair_list PairList = Simulation->GetAllPairs();
        for(u32 PairIndex = 0; PairIndex < PairList.Count; PairIndex++)
        {
            broad_phase_pair* Pair = PairList.Ptr + PairIndex;
            
            entity_type TypeA = GetUserData(Pair->SimEntityA, entity)->Type;
            entity_type TypeB = GetUserData(Pair->SimEntityB, entity)->Type;
            
            if((TypeA == ENTITY_TYPE_RIGID_BODY) ||
               (TypeB == ENTITY_TYPE_RIGID_BODY))
            {
                contact_list ContactList = Simulation->ComputeContacts(Pair);
                if(ContactList.Count > 0)
                {                                
                    if(TypeA == ENTITY_TYPE_RIGID_BODY)
                    {
                        AddRigidBodyContacts(Simulation, Pair->SimEntityA->ToRigidBody(), Pair->SimEntityB, TypeB, ContactList);                    
                    }
                    else if(TypeB == ENTITY_TYPE_RIGID_BODY)
                    {                    
                        ContactList.FlipNormals();
                        AddRigidBodyContacts(Simulation, Pair->SimEntityB->ToRigidBody(), Pair->SimEntityA, TypeA, ContactList);
                    }                
                }
            }
        }        
    }
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        Simulation->Integrate(dt);    
        Simulation->SolveConstraints(30, dt);
    }    
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
        {
            entity* Entity = GetUserData(RigidBody, entity);
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    player* Player = GetUserData(Entity, player); 
                    
                    HandleSlidingCollisions(Simulation, RigidBody);
                    
                    Game->CurrentCameras[WorldIndex].Target = RigidBody->Transform.Translation;                   
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {           
                    v3f StartPosition = RigidBody->Transform.Translation;                                                            
                    HandleSlidingCollisions(Simulation, RigidBody);                                        
                    
                    pushing_object* PushingObject = GetUserData(Entity, pushing_object);
                    if(PushingObject->PlayerID.IsValid())
                    {
                        entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                        sim_entity* PlayerRigidBody = Simulation->GetSimEntity(PlayerEntity->SimEntityID);
                        
                        PlayerRigidBody->Transform.Translation.xy += (RigidBody->Transform.Translation.xy-StartPosition.xy);                                                                    
                        Game->CurrentCameras[WorldIndex].Target = PlayerRigidBody->Transform.Translation;
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
    }
    
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);            
        for(u32 CollisionEventIndex = 0; CollisionEventIndex < Simulation->CollisionEvents.Count; CollisionEventIndex++)
        {
            collision_event* Event = &Simulation->CollisionEvents.Ptr[CollisionEventIndex];
            
            entity* EntityA = GetUserData(Event->SimEntityA, entity);
            entity* EntityB = GetUserData(Event->SimEntityB, entity);
            
            if(EntityA->OnCollision) EntityA->OnCollision(Game, EntityA, EntityB,  Event->Normal);
            if(EntityB->OnCollision) EntityB->OnCollision(Game, EntityB, EntityA, -Event->Normal);
        }
        Simulation->CollisionEvents.Count = 0;
    }
    
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);            
        FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
        {        
            sqt* SQT = GetEntityTransform(Game, GetUserData(RigidBody, entity)->ID);        
            *SQT = RigidBody->Transform;        
        }                
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
    
    if(IsPressed(Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;
    
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
        {   
            RigidBody->MoveDelta = {};
            RigidBody->Acceleration = {};
            RigidBody->AngularAcceleration = {};
            RigidBody->ClearForce();
            RigidBody->ClearTorque();
        }
    }
    
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        FOR_EACH(RigidBody, &Simulation->RigidBodyStorage)
        {               
            entity* Entity = GetUserData(RigidBody, entity);            
            if(WorldIndex == Game->CurrentWorldIndex)
            {
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
                            
                            case PLAYER_STATE_JUMPING:
                            case PLAYER_STATE_PUSHING:
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
                                if(Entity->LinkID.IsValid())
                                {
                                    rigid_body* OtherRigidBody = GetRigidBody(Game, Entity->LinkID);
                                    OtherRigidBody->ApplyConstantAcceleration(MoveDirection, Global_PlayerAcceleration);
                                }
                            }                    
                            else
                            {
                                Player->State = PLAYER_STATE_NONE;
                                PushingObject->PlayerID = InvalidEntityID();
                                Simulation->GetSimEntity(PlayerEntity->SimEntityID)->ToRigidBody()->Velocity = RigidBody->Velocity;
                                RigidBody->Velocity = {};
                                if(Entity->LinkID.IsValid())
                                {
                                    rigid_body* OtherRigidBody = GetRigidBody(Game, Entity->LinkID);
                                    OtherRigidBody->Velocity = {};
                                }
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
            else
            {
                switch(Entity->Type)
                {
                    case ENTITY_TYPE_PLAYER:
                    case ENTITY_TYPE_RIGID_BODY:
                    case ENTITY_TYPE_PUSHABLE:
                    {
                        RigidBody->ApplyGravity(Global_Gravity);
                    } break;
                }
            }
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