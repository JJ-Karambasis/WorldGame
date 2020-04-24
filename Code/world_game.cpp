#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "world.cpp"
#include "assets.cpp"
#include "graphics_2.cpp"

#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 1.0f

extern "C"
EXPORT GAME_TICK(Tick)
{   
    DEVELOPER_GRAPHICS(Graphics);
    DEVELOPER_GAME(Game);        
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    u64 Start = Global_Platform->Clock();
    u64 StartCycles = GetCycles();
    
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->GameStorage = CreateArena(KILOBYTE(32));
        
        Game->PlayerRadius = 0.35f;
        Game->PlayerHeight = 1.0f;        
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->EntityPool.NextKey = 1;
            World->EntityPool.FreeHead = -1;            
            
            CreatePlayer(Game, WorldIndex, V3(0.0f, 0.0f, 1.0f), PLAYER_RADIUS, PLAYER_HEIGHT, Blue());            
        }
        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f));                
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 10.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f));                
        
        CreateBlockersInBothWorlds(Game, V3(-5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3(-5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f, -5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f, -5.0f, 1.0f), 1.0f);                
        
        CreateBoxEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_STATIC, V3(0.0f, -2.5f, 1.0f), V3(5.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));
        CreateSingleLinkedBoxEntities(Game, WORLD_ENTITY_TYPE_PUSHABLE, 1, V3(0.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));        
    }            
    
    if(IsPressed(Game->Input->SwitchWorld))
    {
        u32 PrevIndex = Game->CurrentWorldIndex;
        Game->CurrentWorldIndex = !PrevIndex;
        OnWorldSwitch(Game, PrevIndex, Game->CurrentWorldIndex);          
    }
    
    UpdateWorld(Game);            
    
    camera* Camera = &Game->Camera;        
    world* World = GetCurrentWorld(Game);
    
    world_entity* PlayerEntity = GetEntity(World, World->Player.EntityID);
    
    Camera->Position = PlayerEntity->Position;
    Camera->FocalPoint = PlayerEntity->Position;
    Camera->Position.z += 6.0f;
    Camera->Orientation = IdentityM3();
    
    m4 Perspective = PerspectiveM4(PI*0.30f, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), 0.01f, 100.0f);
    m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);
    
    if(Graphics->Initialized)
    {        
        PushClear(Graphics, Black());        
        
        PushProjection(Graphics, Perspective); 
        PushCameraView(Graphics, CameraView);
        
        for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
        {
            if(!Game->Assets->BoxGraphicsMesh)        
                Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.obj");
            
            ASSERT(Game->Assets->BoxGraphicsMesh);
            PushDrawShadedColoredMesh(Graphics, Game->Assets->BoxGraphicsMesh, Entity->Transform, Entity->Color); 
        }
    }
    
#if 0 
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
        
        world_entity* PlayerEntity = GetEntity(World, World->Player.EntityID);
        
        Camera->Position = PlayerEntity->Position;
        Camera->FocalPoint = PlayerEntity->Position;
        Camera->Position.z += 6.0f;
        Camera->Orientation = IdentityM3();
    }   
    DevGame->LastTickFrameTime = (f32)Global_Platform->ElapsedTime(Global_Platform->Clock(), Start);
    DevGame->LastFrameCycles = GetCycles()-StartCycles;
#endif
}