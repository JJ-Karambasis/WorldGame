#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "simulation/simulation.cpp"
#include "graphics_state.cpp"
#include "entity.cpp"
//#include "player.cpp"

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

void LoadTestLevel(game* Game)
{    
    //TODO(JJ): Load entity data at runtime    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {           
        Game->EntityStorage[WorldIndex] = AK_CreatePool<entity>(512);                
        Game->OldTransforms[WorldIndex] = AK_CreateArray<ak_sqtf>(Game->EntityStorage[WorldIndex].Capacity);
        Game->NewTransforms[WorldIndex] = AK_CreateArray<ak_sqtf>(Game->EntityStorage[WorldIndex].Capacity);
        
        ak_v3f P0 = AK_V3<ak_f32>() + AK_ZAxis()*PLAYER_RADIUS;
        capsule PlayerCapsule = CreateCapsule(P0, P0+AK_ZAxis()*PLAYER_HEIGHT, PLAYER_RADIUS);        
        world_id PlayerID = CreatePlayerEntity(Game, WorldIndex, AK_V3(0.0f, 0.0f, 0.1f), AK_IdentityQuat<ak_f32>(), 65, Global_PlayerMaterial, &PlayerCapsule);
        
        camera* Camera = Game->CurrentCameras + WorldIndex;        
        Camera->Target = GetEntityPositionNew(Game, PlayerID);        
        Camera->SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(-90.0f), AK_ToRadians(35.0f));        
        Camera->FieldOfView = AK_ToRadians(65.0f);                        
        Camera->ZNear = CAMERA_ZNEAR;
        Camera->ZFar = CAMERA_ZFAR;        
        
        Game->PrevCameras[WorldIndex] = *Camera;
    }
    
    CreateDualStaticEntity(Game, AK_V3(0.0f, 0.0f, -0.01f), AK_V3(10.0f, 10.0f, 0.01f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.0f)),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, AK_V3(-6.2f, -4.5f, 0.0f), AK_V3(1.0f, 1.0f, 1.0f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.1f)),  MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, AK_V3(-3.0f, -4.5f, 0.0f), AK_V3(1.0f, 1.0f, 1.0f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.25f)), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, AK_V3(-4.6f, -4.5f, 0.0f), AK_V3(1.0f, 1.0f, 1.0f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.33f)), MESH_ASSET_ID_BOX,   Global_Material0);
    CreateDualStaticEntity(Game, AK_V3(-1.6f, -3.5f, -2.0f), AK_V3(1.0f, 1.0f, 10.0f), AK_EulerToQuat(AK_V3(AK_PI*0.3f, 0.0f, AK_PI*0.0f)),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateDualStaticEntity(Game, AK_V3(-1.0f, 5.5f, 0.0f),  AK_V3(1.0f, 1.0f, 1.0f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.2f)),  MESH_ASSET_ID_BOX,   Global_Material1);
    CreateDualStaticEntity(Game, AK_V3(1.0f, 4.5f, 0.0f),   AK_V3(1.0f, 1.0f, 1.0f), AK_EulerToQuat(AK_V3(0.0f, 0.0f, AK_PI*0.6f)),  MESH_ASSET_ID_BOX,   Global_Material1);                    
    CreateSphereRigidBody(Game, 0, AK_V3( 1.0f, 1.0f, 5.0f), 0.5f, 30.0f, 0.2f, Global_Material0);
    CreateDualPushableBox(Game, AK_V3(-2.0f, 0.0f, 0.001f), 1.0f, 35.0f, Global_Material0);
    
    Game->JumpingQuads[0].CenterP = AK_V3(-1.0f, 0.0f, 0.0f);
    Game->JumpingQuads[0].Dimensions = AK_V2(1.0f, 2.0f);
    
    Game->JumpingQuads[1].CenterP = AK_V3(-4.0f, 0.0f, 0.0f);
    Game->JumpingQuads[1].Dimensions = AK_V2(1.0f, 2.0f);
    
    Game->JumpingQuads[0].OtherQuad = &Game->JumpingQuads[1];
    Game->JumpingQuads[1].OtherQuad = &Game->JumpingQuads[0];
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
    
    Game->Input       = Input;
    Game->AudioOutput = AudioOutput;
    Game->Assets      = Assets;        
    
    return Game;
}

void AddRigidBodyContacts(game* Game, simulation* Simulation, rigid_body* RigidBody, sim_entity* SimEntity, entity_type TypeB, contact_list ContactList)
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
            pushing_object* PushingObject = GetPushingObject(Game, GetUserData(SimEntity, entity));
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
    ak_v3f MoveDelta = RigidBody->MoveDelta;
    RigidBody->MoveDelta = AK_V3(MoveDelta.xy);
    
    for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
    {
        ak_f32 DeltaLength = AK_Magnitude(RigidBody->MoveDelta);
        if(DeltaLength > 0)
        {
            ak_v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;
            broad_phase_pair_list Pairs = Simulation->FilterPairs(Simulation->GetAllPairs(RigidBody), 
                                                                  [](broad_phase_pair* Pair) -> ak_bool
                                                                  {
                                                                      if(GetUserData(Pair->SimEntityB, entity)->Type == ENTITY_TYPE_RIGID_BODY)
                                                                          return false;
                                                                      return true;
                                                                  });        
            continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                        
            if(ContactTOI.HitEntity)
            {
                ak_v3f Normal = -ContactTOI.Contact.Normal;
                ak_f32 tMin = ContactTOI.t;
                
                RigidBody->Transform.Translation += Normal*(ContactTOI.Contact.Penetration+0.001f);
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
    
    RigidBody->MoveDelta = AK_V3(0.0f, 0.0f, MoveDelta.z);    
    for(ak_u32 Iterations = 0; Iterations < 4; Iterations++)
    {
        ak_f32 DeltaLength = AK_Magnitude(RigidBody->MoveDelta);
        if(DeltaLength > 0)
        {
            ak_v3f TargetPosition = RigidBody->Transform.Translation + RigidBody->MoveDelta;
            broad_phase_pair_list Pairs = Simulation->FilterPairs(Simulation->GetAllPairs(RigidBody), 
                                                                  [](broad_phase_pair* Pair) -> ak_bool
                                                                  {
                                                                      if(GetUserData(Pair->SimEntityB, entity)->Type == ENTITY_TYPE_RIGID_BODY)
                                                                          return false;
                                                                      return true;
                                                                  });        
            continuous_contact ContactTOI = Simulation->ComputeTOI(RigidBody, Pairs);                        
            if(ContactTOI.HitEntity)
            {
                ak_v3f Normal = -ContactTOI.Contact.Normal;
                ak_f32 tMin = ContactTOI.t;
                
                RigidBody->Transform.Translation += Normal*(ContactTOI.Contact.Penetration+0.001f);
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
    
}

extern "C"
AK_EXPORT GAME_FIXED_TICK(FixedTick)
{   
    Dev_SetDeveloperContext(DevContext);
    
    ak_high_res_clock Start = AK_WallClock();    
    AK_SetGlobalArena(Game->TempStorage);
    
    ak_f32 dt = Game->dtFixed;
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, Game->CurrentWorldIndex);    
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {
            entity* Entity = GetUserData(RigidBody, entity);
            RigidBody->Transform = *GetEntityTransformNew(Game, Entity->ID);
            
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
                            RigidBody->LinearDamping = AK_V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
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
                    
                    pushing_object* PushingObject = GetPushingObject(Game, Entity);
                    if(PushingObject->PlayerID.IsValid())
                    {                                                                           
                        RigidBody->LinearDamping = AK_V3(Global_PlayerDamping, Global_PlayerDamping, 0.0f);
                        if(Entity->LinkID.IsValid())
                        {
                            rigid_body* OtherRigidBody = GetRigidBody(Game, Entity->LinkID);
                            OtherRigidBody->LinearDamping = RigidBody->LinearDamping;
                        }                        
                    }
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    RigidBody->LinearDamping = AK_V3(4.0f, 4.0f, 0.0f);
                    RigidBody->AngularDamping = AK_V3(1.0f, 1.0f, 1.0f);
                } break;            
            }
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        broad_phase_pair_list PairList = Simulation->GetAllPairs();
        for(ak_u32 PairIndex = 0; PairIndex < PairList.Count; PairIndex++)
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
                        AddRigidBodyContacts(Game, Simulation, Pair->SimEntityA->ToRigidBody(), Pair->SimEntityB, TypeB, ContactList);                    
                    }
                    else if(TypeB == ENTITY_TYPE_RIGID_BODY)
                    {                    
                        ContactList.FlipNormals();
                        AddRigidBodyContacts(Game, Simulation, Pair->SimEntityB->ToRigidBody(), Pair->SimEntityA, TypeA, ContactList);
                    }                
                }
            }
        }        
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        Simulation->Integrate(dt);    
        Simulation->SolveConstraints(30, dt);
    }    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
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
                    ak_v3f StartPosition = RigidBody->Transform.Translation;                                                            
                    HandleSlidingCollisions(Simulation, RigidBody);                                        
                    
                    pushing_object* PushingObject = GetPushingObject(Game, Entity);
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
        simulation* Simulation = GetSimulation(Game, WorldIndex);            
        for(ak_u32 CollisionEventIndex = 0; CollisionEventIndex < Simulation->CollisionEvents.Count; CollisionEventIndex++)
        {
            collision_event* Event = &Simulation->CollisionEvents.Ptr[CollisionEventIndex];
            
            entity* EntityA = GetUserData(Event->SimEntityA, entity);
            entity* EntityB = GetUserData(Event->SimEntityB, entity);
            
            if(EntityA->OnCollision) EntityA->OnCollision(Game, EntityA, EntityB,  Event->Normal);
            if(EntityB->OnCollision) EntityB->OnCollision(Game, EntityB, EntityA, -Event->Normal);
        }
        Simulation->CollisionEvents.Count = 0;
    }
    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);            
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {        
            ak_sqtf* SQT = GetEntityTransformNew(Game, GetUserData(RigidBody, entity)->ID);        
            *SQT = RigidBody->Transform;        
        }                
    }
}

extern "C"
AK_EXPORT GAME_TICK(Tick)
{   
    Dev_SetDeveloperContext(DevContext);
    
    AK_SetGlobalArena(Game->TempStorage);
    ak_f32 dt = Game->dt;
    
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
    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {   
            RigidBody->MoveDelta = {};
            RigidBody->Acceleration = {};
            RigidBody->AngularAcceleration = {};
            RigidBody->ClearForce();
            RigidBody->ClearTorque();
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);    
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
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
                                ak_v3f Position = GetEntityPositionNew(Game, Entity->ID);                               
                                for(ak_u32 JumpingQuadIndex = 0; JumpingQuadIndex < AK_Count(Game->JumpingQuads); JumpingQuadIndex++)
                                {
                                    jumping_quad* JumpingQuad = Game->JumpingQuads + JumpingQuadIndex;
                                    ak_v2f HalfDim = JumpingQuad->Dimensions*0.5f;
                                    
                                    ak_v2f Min = JumpingQuad->CenterP.xy - HalfDim;
                                    ak_v2f Max = JumpingQuad->CenterP.xy + HalfDim;
                                    
                                    ak_color3f QuadColor = AK_Red3();
                                    if(Position.xy >= Min && Position.xy <= Max)
                                    {
                                        if(AK_Abs(Position.z - JumpingQuad->CenterP.z) < 1e-2f)
                                        {
                                            if(IsPressed(Input->Action))
                                            {
                                                jumping_quad* TargetQuad = JumpingQuad->OtherQuad;
                                                
                                                ak_v2f XDirection = TargetQuad->CenterP.xy-JumpingQuad->CenterP.xy;
                                                ak_f32 Displacement = AK_Magnitude(XDirection);
                                                XDirection /= Displacement;                    
                                                
                                                ak_f32 InitialVelocity = AK_Sqrt(Displacement*Global_Gravity);
                                                
                                                RigidBody->Velocity.xy = (InitialVelocity*AK_SQRT2_2*XDirection);
                                                RigidBody->Velocity.z = InitialVelocity*AK_SQRT2_2;
                                                
                                                Player->State = PLAYER_STATE_JUMPING;                                    
                                                
                                                break;                                                
                                            }                
                                            QuadColor = AK_Yellow3();
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
                        pushing_object* PushingObject = GetPushingObject(Game, Entity);
                        if(PushingObject->PlayerID.IsValid())
                        {
                            entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                            player* Player = GetUserData(PlayerEntity, player);
                            AK_Assert(Player->State == PLAYER_STATE_PUSHING, "Player state is not pushing but has a pushing entity associated with. This is a programming error");
                            
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
                                PushingObject->PlayerID = InvalidWorldID();
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
AK_EXPORT GAME_RENDER(Render)
{
#if 0 
    AK_SetGlobalArena(Game->TempStorage);
    UpdateRenderBuffer(&Game->RenderBuffer, Graphics, Graphics->RenderDim);        
    view_settings ViewSettings = GetViewSettings(&GraphicsState->Camera);
    PushWorldShadingCommands(Graphics, Game->RenderBuffer, &ViewSettings, Game->Assets, GraphicsState->GraphicsObjects);
#if !DEVELOPER_BUILD
    PushCopyToOutput(Graphics, Game->RenderBuffer, AK_V2(0, 0), Game->RenderBuffer->Resolution);
#endif
#endif
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>