#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "player.cpp"

inline entity* 
CreateEntity(game* Game, b32 Simulate, v3f Position, v3f Scale, v3f Euler, c4 Color, triangle3D_mesh* WalkableMesh=NULL)
{
    entity* Result = PushStruct(&Game->WorldStorage, entity, Clear, 0);        
    Result->Transform = CreateSQT(Position, Scale, Euler);
    Result->Color = Color;    
    Result->Simulate = Simulate;
    Result->WalkableMesh = WalkableMesh;
    
    Result->AABB = CreateAABB3D(V3(-0.5f, -0.5f, 0.0f), V3( 0.5f, 0.5f, 1.0f));
    
    AddToList(&Game->Entities, Result);
    
    return Result;
}

extern "C"
EXPORT GAME_TICK(Tick)
{   
    DEVELOPER_GRAPHICS(Graphics);
    DEVELOPER_GAME(Game);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    player* Player = &Game->Player;
    
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->WorldStorage = CreateArena(KILOBYTE(32));
        
        Player->Color = RGBA(0.0f, 0.0f, 1.0f, 1.0f);
        Player->Position = V3(0.0f, 0.0f, 1.0f);
        Player->FacingDirection = V2(0.0f, 1.0f);
        
        CreateEntity(Game, false, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), &Game->Assets->BoxTriangleMesh);  
        CreateEntity(Game, true, V3(2.0f, 0.0f, 1.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.25f, 0.0f, 0.0f, 1.0f));
        CreateEntity(Game, true, V3(0.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.25f, 0.0f, 0.0f, 1.0f));                
    }        
    
    f32 dt = Game->Input->dt;
    for(entity* Entity = Game->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->Simulate)
        {            
            Entity->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));    
            Entity->Velocity.z -= dt*10.0f;            
            Entity->Position += Entity->Velocity*dt;
        }
    }
    
    for(entity* FirstEntity = Game->Entities.First; FirstEntity; FirstEntity = FirstEntity->Next)
    {
        if(FirstEntity->Simulate)
        {
            aabb3D FirstAABB = TransformAABB3D(FirstEntity->AABB, FirstEntity->Transform);
            for(entity* SecondEntity = Game->Entities.First; SecondEntity; SecondEntity = SecondEntity->Next)
            {
                if(FirstEntity != SecondEntity)
                {                    
                    aabb3D SecondAABB = TransformAABB3D(SecondEntity->AABB, SecondEntity->Transform);
                    penetration_result Penetration = PenetrationTestAABB3D(FirstAABB, SecondAABB);
                    if(Penetration.Hit)
                    {
                        f32 VelocityMagntiude = Dot(Penetration.Normal, FirstEntity->Velocity);
                        
                        FirstEntity->Velocity -= (VelocityMagntiude*Penetration.Normal);                        
                        FirstEntity->Position += (Penetration.Normal*Penetration.Distance);
                        
                        if(SecondEntity->Simulate)
                        {
                            SecondEntity->Velocity += VelocityMagntiude*Penetration.Normal;
                            SecondEntity->Position -= (Penetration.Normal*Penetration.Distance);
                        }
                    }
                }
            }
        }
    }
    
    UpdatePlayer(Game, Player);
    
#if DEVELOPER_BUILD
    development_game* DevGame = (development_game*)Game;
    if(DevGame->InDevelopmentMode)
    {
        development_input* DevInput = (development_input*)Game->Input;
        
        camera* DevCamera = &DevGame->DevCamera;
        
        const f32 ANGULAR_DAMPING = 10.0f;
        const f32 ANGULAR_ACCELERATION = 5.0f;
        const f32 LINEAR_DAMPING = 10.0f;     
        const f32 LINEAR_ACCELERATION = 7.5f;
        const f32 SCROLL_ACCELERATION = 300.0f*5;
        const f32 SCROLL_DAMPING = 7.5f;  
        const f32 MIN_DISTANCE = 0.1f;        
        
        if(IsDown(DevInput->Alt))
        {                                    
            if(IsDown(DevInput->LMB))
            {
                DevCamera->AngularVelocity.x += (DevInput->MouseDelta.y*DevInput->dt*ANGULAR_ACCELERATION);
                DevCamera->AngularVelocity.y += (DevInput->MouseDelta.x*DevInput->dt*ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(DevInput->MMB))
            {
                DevCamera->Velocity.x += (DevInput->MouseDelta.x*DevInput->dt*LINEAR_ACCELERATION);
                DevCamera->Velocity.y += (DevInput->MouseDelta.y*DevInput->dt*LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(DevInput->Scroll) > 0.0f)            
                DevCamera->Velocity.z -= DevInput->Scroll*DevInput->dt*SCROLL_ACCELERATION;                                            
        }                
        
        DevCamera->AngularVelocity *= (1.0f / (1.0f+DevInput->dt*ANGULAR_DAMPING));            
        v3f Eulers = (DevCamera->AngularVelocity*DevInput->dt);            
        
        quaternion Orientation = Normalize(RotQuat(DevCamera->Orientation.XAxis, Eulers.pitch)*RotQuat(DevCamera->Orientation.YAxis, Eulers.yaw));
        DevCamera->Orientation *= ToMatrix3(Orientation);
        
        DevCamera->Velocity.xy *= (1.0f /  (1.0f+DevInput->dt*LINEAR_DAMPING));            
        v2f Vel = DevCamera->Velocity.xy*DevInput->dt;
        v3f Delta = Vel.x*DevCamera->Orientation.XAxis - Vel.y*DevCamera->Orientation.YAxis;
        
        DevCamera->FocalPoint += Delta;
        DevCamera->Position += Delta;
        
        DevCamera->Velocity.z *= (1.0f/ (1.0f+DevInput->dt*SCROLL_DAMPING));            
        DevCamera->Distance += DevCamera->Velocity.z*DevInput->dt;            
        
        if(DevCamera->Distance < MIN_DISTANCE)
            DevCamera->Distance = MIN_DISTANCE;
        
        DevCamera->Position = DevCamera->FocalPoint + (DevCamera->Orientation.ZAxis*DevCamera->Distance);
    }
    else    
    #endif
    {
        camera* Camera = &Game->Camera;
        Camera->Position = Player->Position;
        Camera->FocalPoint = Player->Position;
        Camera->Position.z += 6.0f;
        Camera->Orientation = IdentityM3();
    }        
}