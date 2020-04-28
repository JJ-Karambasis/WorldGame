#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "world.cpp"
#include "assets.cpp"
#include "graphics.cpp"

#define PLAYER_RADIUS 0.35f
#define PLAYER_HEIGHT 1.0f

extern "C"
EXPORT GAME_TICK(Tick)
{       
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->GameStorage = CreateArena(KILOBYTE(32));
        
        Game->PlayerRadius = 0.35f;
        Game->PlayerHeight = 1.0f;        
        
        Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.obj");
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->EntityPool.NextKey = 1;
            World->EntityPool.FreeHead = -1;            
            
            CreatePlayer(Game, WorldIndex, V3(0.0f, 0.0f, 1.0f), PLAYER_RADIUS, PLAYER_HEIGHT, Blue());            
        }
        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), Game->Assets->BoxGraphicsMesh);                
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 10.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), Game->Assets->BoxGraphicsMesh);                
        
        CreateBlockersInBothWorlds(Game, V3(-5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3(-5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f, -5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f, -5.0f, 1.0f), 1.0f);                
        
        CreateBoxEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_STATIC, V3(0.0f, -2.5f, 1.0f), V3(5.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));
        
#if 1
        CreateDualLinkedBoxEntities(Game, WORLD_ENTITY_TYPE_PUSHABLE, V3(0.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));                
#else
        CreateSingleLinkedBoxEntities(Game, WORLD_ENTITY_TYPE_PUSHABLE, 1, V3(0.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));                
#endif
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
    
    if(NOT_IN_DEVELOPMENT_MODE())
    {        
        m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
        m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);        
        
        RecordGameCommands(Game, Graphics, Perspective, CameraView);        
    }    
}