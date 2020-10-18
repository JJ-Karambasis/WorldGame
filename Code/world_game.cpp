#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "simulation/simulation.cpp"
#include "graphics_state.cpp"
#include "world.cpp"
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
    entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(UserDataToIndex(SimEntity->UserData));
    switch(Entity->Type)
    {
        case ENTITY_TYPE_PLAYER:
        {            
            player* Player = (player*)Entity->UserData;
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
            pushing_object* PushingObject = GetPushingObject(&Game->World, Entity);
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
            broad_phase_pair_list Pairs = Simulation->GetAllPairs(RigidBody);
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
            broad_phase_pair_list Pairs = Simulation->GetAllPairs(RigidBody);            
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
    
    world* World = &Game->World;
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {        
        simulation* Simulation = &World->Simulations[WorldIndex];
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {
            ak_u32 EntityIndex = UserDataToIndex(RigidBody->UserData);
            entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(EntityIndex);            
            RigidBody->Transform = World->NewTransforms[WorldIndex][EntityIndex];
            
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    Entity->OnCollision = OnPlayerCollision;                
                    Simulation->AddConstraint(RigidBody, NULL, LockConstraint);
                    
                    player* Player = (player*)Entity->UserData;
                    
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
                    
                    pushing_object* PushingObject = GetPushingObject(&Game->World, Entity);
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
        simulation* Simulation = &World->Simulations[WorldIndex];
        broad_phase_pair_list PairList = Simulation->GetAllPairs();
        for(ak_u32 PairIndex = 0; PairIndex < PairList.Count; PairIndex++)
        {
            broad_phase_pair* Pair = PairList.Ptr + PairIndex;
            
            ak_u32 EntityIndexA = UserDataToIndex(Pair->SimEntityA->UserData);
            ak_u32 EntityIndexB = UserDataToIndex(Pair->SimEntityB->UserData);
            
            entity_type TypeA = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexA)->Type;
            entity_type TypeB = World->EntityStorage[WorldIndex].GetByIndex(EntityIndexB)->Type;
            
            if((TypeA == ENTITY_TYPE_RIGID_BODY) ||
               (TypeB == ENTITY_TYPE_RIGID_BODY))
            {
                contact_list ContactList = Simulation->ComputeContacts(Pair);
                if(ContactList.Count > 0)
                {                                
                    if(TypeA == ENTITY_TYPE_RIGID_BODY)
                    {
                        AddRigidBodyContacts(Game, WorldIndex, Pair->SimEntityA->ToRigidBody(), Pair->SimEntityB, ContactList);                    
                    }
                    else if(TypeB == ENTITY_TYPE_RIGID_BODY)
                    {                    
                        ContactList.FlipNormals();
                        AddRigidBodyContacts(Game, WorldIndex, Pair->SimEntityB->ToRigidBody(), Pair->SimEntityA, ContactList);
                    }                
                }
            }
        }        
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        Simulation->Integrate(dt);    
        Simulation->SolveConstraints(30, dt);
    }    
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {
            ak_u32 EntityIndex = UserDataToIndex(RigidBody->UserData);
            
            entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(EntityIndex);
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    player* Player = (player*)Entity->UserData;
                    
                    HandleSlidingCollisions(Simulation, RigidBody);
                    
                    World->NewCameras[WorldIndex].Target = RigidBody->Transform.Translation;                   
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {           
                    ak_v3f StartPosition = RigidBody->Transform.Translation;                                                            
                    HandleSlidingCollisions(Simulation, RigidBody);                                        
                    
                    pushing_object* PushingObject = GetPushingObject(&Game->World, Entity);
                    if(PushingObject->PlayerID.IsValid())
                    {
                        entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                        sim_entity* PlayerRigidBody = Simulation->GetSimEntity(PlayerEntity->SimEntityID);
                        
                        PlayerRigidBody->Transform.Translation.xy += (RigidBody->Transform.Translation.xy-StartPosition.xy);                                                                    
                        World->NewCameras[WorldIndex].Target = PlayerRigidBody->Transform.Translation;
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
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)        
            World->NewTransforms[WorldIndex][UserDataToIndex(RigidBody->UserData)] = RigidBody->Transform;                        
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
    
    
    world* World = &Game->World;
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = &World->Simulations[WorldIndex];
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
        simulation* Simulation = &World->Simulations[WorldIndex];
        AK_ForEach(RigidBody, &Simulation->RigidBodyStorage)
        {               
            entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(UserDataToIndex(RigidBody->UserData));
            if(WorldIndex == Game->CurrentWorldIndex)
            {
                switch(Entity->Type)
                {
                    case ENTITY_TYPE_PLAYER:
                    {
                        player* Player = (player*)Entity->UserData;
                        
                        switch(Player->State)
                        {
                            case PLAYER_STATE_NONE:
                            {                                                                   
                                AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[WorldIndex])        
                                    JumpingQuad->Color = AK_Yellow3();        
                                
                                
                                ak_v3f Position = World->NewTransforms[Entity->ID.WorldIndex][AK_PoolIndex(Entity->ID.ID)].Translation;
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
                                                
                                                Player->State = PLAYER_STATE_JUMPING;                                    
                                                                                                
                                                break;                                                
                                            }                                                   
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
                        pushing_object* PushingObject = GetPushingObject(&Game->World, Entity);
                        if(PushingObject->PlayerID.IsValid())
                        {
                            entity* PlayerEntity = GetEntity(Game, PushingObject->PlayerID);
                            player* Player = (player*)PlayerEntity->UserData;
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
    InterpolateState(&Game->World, WorldIndex, tInterpolate);                     
    graphics_state* GraphicsState = &Game->World.GraphicsStates[WorldIndex];
    
    AK_SetGlobalArena(Game->TempStorage);
    UpdateRenderBuffer(Graphics, &GraphicsState->RenderBuffer, Game->Resolution);
    
    camera* Camera = &GraphicsState->Camera;
    view_settings ViewSettings = GetViewSettings(Camera);
    
    graphics_light_buffer LightBuffer = GetLightBuffer(GraphicsState);
    ShadowPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);
    
    StandardEntityCommands(Graphics, GraphicsState, &ViewSettings);
    EntityLitPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);            
    JumpingQuadPass(Graphics, &Game->World.JumpingQuadStorage[WorldIndex], &Game->QuadMesh);
    
#if !DEVELOPER_BUILD
    PushCopyToOutput(Graphics, Game->RenderBuffer, AK_V2(0, 0), Game->RenderBuffer->Resolution);
#endif    
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>