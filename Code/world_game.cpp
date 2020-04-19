#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "world.cpp"

extern "C"
EXPORT GAME_TICK(Tick)
{   
    DEVELOPER_GRAPHICS(Graphics);
    DEVELOPER_GAME(Game);        
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    u64 Start = Global_Platform->Clock();
    
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->GameStorage = CreateArena(KILOBYTE(32));
        
        Game->PlayerRadius = 0.35f;
        Game->PlayerHeight = 1.0f;        
        
        Game->Worlds[0].Player.Color = RGBA(0.0f, 0.0f, 1.0f, 1.0f);
        Game->Worlds[0].Player.Position = V3(0.0f, 0.0f, 1.0f);
        Game->Worlds[0].Player.FacingDirection = V2(0.0f, 1.0f);        
        
        Game->Worlds[1].Player.Color = RGBA(1.0f, 0.0f, 0.0f, 1.0f);
        Game->Worlds[1].Player.Position = V3(0.0f, 0.0f, 1.0f);
        Game->Worlds[1].Player.FacingDirection = V2(0.0f, 1.0f);        
        
        CreateEntityInBothWorlds(Game, BOX_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f));                
        CreateEntityInBothWorlds(Game, BOX_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 10.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f));                
        
        CreateBlockersInBothWorlds(Game, V3(-5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3(-5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f, -5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f, -5.0f, 1.0f), 1.0f);                
        
        CreateEntityInBothWorlds(Game, BOX_ENTITY_TYPE_DEFAULT, V3(0.0f, -2.5f, 1.0f), V3(5.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));
        CreateLinkedEntities(Game, BOX_ENTITY_TYPE_PUSHABLE, V3(0.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));
    }            
    
    if(IsPressed(Game->Input->SwitchWorld))
        Game->CurrentWorldIndex = !Game->CurrentWorldIndex;
    
    UpdateWorld2(Game, 0);
    UpdateWorld2(Game, 1);
    
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
                DevCamera->AngularVelocity.x += (DevInput->MouseDelta.y*Game->dt*ANGULAR_ACCELERATION);
                DevCamera->AngularVelocity.y += (DevInput->MouseDelta.x*Game->dt*ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(DevInput->MMB))
            {
                DevCamera->Velocity.x += (DevInput->MouseDelta.x*Game->dt*LINEAR_ACCELERATION);
                DevCamera->Velocity.y += (DevInput->MouseDelta.y*Game->dt*LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(DevInput->Scroll) > 0.0f)            
                DevCamera->Velocity.z -= DevInput->Scroll*Game->dt*SCROLL_ACCELERATION;                                            
        }                
        
        DevCamera->AngularVelocity *= (1.0f / (1.0f+Game->dt*ANGULAR_DAMPING));            
        v3f Eulers = (DevCamera->AngularVelocity*Game->dt);            
        
        quaternion Orientation = Normalize(RotQuat(DevCamera->Orientation.XAxis, Eulers.pitch)*RotQuat(DevCamera->Orientation.YAxis, Eulers.yaw));
        DevCamera->Orientation *= ToMatrix3(Orientation);
        
        DevCamera->Velocity.xy *= (1.0f /  (1.0f+Game->dt*LINEAR_DAMPING));            
        v2f Vel = DevCamera->Velocity.xy*Game->dt;
        v3f Delta = Vel.x*DevCamera->Orientation.XAxis - Vel.y*DevCamera->Orientation.YAxis;
        
        DevCamera->FocalPoint += Delta;
        DevCamera->Position += Delta;
        
        DevCamera->Velocity.z *= (1.0f/ (1.0f+Game->dt*SCROLL_DAMPING));            
        DevCamera->Distance += DevCamera->Velocity.z*Game->dt;            
        
        if(DevCamera->Distance < MIN_DISTANCE)
            DevCamera->Distance = MIN_DISTANCE;
        
        DevCamera->Position = DevCamera->FocalPoint + (DevCamera->Orientation.ZAxis*DevCamera->Distance);
    }
    else    
    #endif
    {
        camera* Camera = &Game->Camera;
        
        world* World = GetCurrentWorld(Game);
        Camera->Position = World->Player.Position;
        Camera->FocalPoint = World->Player.Position;
        Camera->Position.z += 6.0f;
        Camera->Orientation = IdentityM3();
    }   
    
    DevGame->LastTickFrameTime = (f32)Global_Platform->ElapsedTime(Global_Platform->Clock(), Start);
}