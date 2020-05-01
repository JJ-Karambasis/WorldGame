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
    SET_DEVELOPER_CONTEXT(DevContext);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->GameStorage = CreateArena(MEGABYTE(16));
        
        Game->PlayerRadius = 0.35f;
        Game->PlayerHeight = 1.0f;        
        
        Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.obj");
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->EntityPool = CreatePool<world_entity>(&Game->GameStorage, 512);            
            CreatePlayer(Game, WorldIndex, V3(0.0f, 0.0f, 1.0f), PLAYER_RADIUS, PLAYER_HEIGHT, Blue());            
            
            world_entity* PlayerEntity = GetPlayerEntity(World);
            camera* Camera = &World->Camera;
            
            Camera->Position = PlayerEntity->Position;
            Camera->FocalPoint = PlayerEntity->Position;
            Camera->Position.z += 6.0f;
            Camera->Orientation = IdentityM3();    
        }
        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->BoxGraphicsMesh);                
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(0.0f, 0.0f, 10.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->BoxGraphicsMesh);                
        
        CreateBlockersInBothWorlds(Game, V3(-5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3(-5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f,  5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f,  5.0f, 1.0f), 1.0f, V3( 5.0f, -5.0f, 1.0f), 1.0f);
        CreateBlockersInBothWorlds(Game, V3( 5.0f, -5.0f, 1.0f), 1.0f, V3(-5.0f, -5.0f, 1.0f), 1.0f);                
        
        CreateBoxEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_STATIC, V3(0.0f, -2.5f, 1.0f), V3(5.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.65f, 0.0f, 0.65f, 1.0f));
                
        CreateDualLinkedBoxEntities(Game, WORLD_ENTITY_TYPE_PUSHABLE, V3(2.0f, 2.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f), RGBA(0.35f, 0.0f, 0.35f, 1.0f));
        CreateSingleLinkedBoxEntities(Game, WORLD_ENTITY_TYPE_PUSHABLE, 1, V3(-2.0f, 3.5f, 1.0f), V3(1.0f, 1.0f, 1.0f), RGBA(0.35f, 0.7f, 0.35f, 1.0f), RGBA(0.65f, 0.9f, 0.65f, 1.0f));                
    }            
    
    if(IsPressed(Game->Input->SwitchWorld))
    {
        u32 PrevIndex = Game->CurrentWorldIndex;
        Game->CurrentWorldIndex = !PrevIndex;
        OnWorldSwitch(Game, PrevIndex, Game->CurrentWorldIndex);          
    }
    
    UpdateWorld(Game);            
    
    block_puzzle* Puzzle = &Game->TestPuzzle;
    
    b32 GoalMet = Puzzle->GoalRectCount > 0;    
    for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
    {
        rect3D_center_dim GoalRect = Puzzle->GoalRects[GoalIndex];
        
        b32 IsContained = false;
        for(u32 BlockEntityIndex = 0; BlockEntityIndex < Puzzle->BlockEntityCount; BlockEntityIndex++)
        {
            world_entity_id EntityID = Puzzle->BlockEntities[BlockEntityIndex];
            world_entity* Entity = GetEntity(Game, EntityID);
            
            ASSERT(Entity->Collider.Type == COLLIDER_TYPE_ALIGNED_BOX);
            
            aligned_box AlignedBox = GetWorldSpaceAlignedBox(Entity);
            
            if(IsRectFullyContainedInRect3D(AlignedBox.CenterP, AlignedBox.Dim, GoalRect.CenterP, GoalRect.Dim))
            {
                IsContained = true;                
                break;
            }
        }
        
        if(!IsContained)
        {
            GoalMet = false;        
            break;
        }
    }    
    
    if(GoalMet)
    {
        ASSERT(false);
    }
    
    
    if(NOT_IN_DEVELOPMENT_MODE())
    {           
        PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);        
        
        PushClearColorAndDepth(Graphics, Black(), 1.0f);
        PushDepth(Graphics, true);
        
        PushWorldCommands(Graphics, GetCurrentWorld(Game));        
    }    
}