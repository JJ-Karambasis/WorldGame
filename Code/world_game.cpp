#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "world.cpp"
#include "fbx.cpp"
#include "assets.cpp"
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

extern "C"
EXPORT GAME_TICK(Tick)
{   
    SET_DEVELOPER_CONTEXT(DevContext);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    SetGlobalErrorStream(Global_Platform->ErrorStream);
    
    float min = FLT_MIN;
    float true_min = FLT_TRUE_MIN;
    
    world_entity_id ID = {};
    if(!Game->Initialized)
    {        
        Game->Initialized = true;
        Game->GameStorage = CreateArena(MEGABYTE(16));
        
        Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.fbx");
        Game->Assets->BoxWalkableMesh = LoadWalkableMesh(Game->Assets, "Box.fbx");
        Game->Assets->QuadGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Quad.fbx");
        Game->Assets->QuadWalkableMesh = LoadWalkableMesh(Game->Assets, "Quad.fbx");
        Game->Assets->PlayerMesh = LoadGraphicsMesh(Game->Assets, "TestPlayerMesh.fbx");
        
#if 0 
        Game->Assets->TestSkeletonMesh = LoadGraphicsMesh(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestSkeleton = LoadSkeleton(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestAnimation = LoadAnimation(Game->Assets, "TestAnimation.fbx");
#endif
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->EntityPool = CreatePool<world_entity>(&Game->GameStorage, 512);            
            CreatePlayer(Game, WorldIndex, V3(-1.0f, 0.0f, 0.0f), V3(0.35f, 0.35f, 1.0f), WorldIndex == 0 ? Blue() : Red());            
            
            world_entity* PlayerEntity = GetPlayerEntity(World);
            camera* Camera = &World->Camera;
            
            Camera->Position = PlayerEntity->Position;
            Camera->FocalPoint = PlayerEntity->Position;
            Camera->Position.z += 6.0f;
            Camera->Orientation = IdentityM3();                
        }
        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3( 50.0f, 0.0f, 0.0f), V3(1.0f, 100.0f, 100.0f), V3(0, -PI*0.5f, 0), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->QuadGraphicsMesh, &Game->Assets->QuadWalkableMesh);        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3( 2.0f, -5.0f, 0.0f), V3(2.0f, 80.0f, 1.0f), V3(0.2f*PI, 0, PI*0.0f), RGBA(0.5f, 0.0f, 0.0f, 1.0f), &Game->Assets->BoxGraphicsMesh, &Game->Assets->BoxWalkableMesh);
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(-2.0f,  0.0f, 0.0f), V3(1.0f, 1.0f, 0.75f), V3(0.0f*PI, 0, PI*0.0f), RGBA(0.5f, 0.0f, 0.0f, 1.0f), &Game->Assets->BoxGraphicsMesh, &Game->Assets->BoxWalkableMesh);        
    }        
    
    
    if(IsPressed(Game->Input->SwitchWorld))
    {
        u32 PrevIndex = Game->CurrentWorldIndex;
        Game->CurrentWorldIndex = !PrevIndex;
        OnWorldSwitch(Game, PrevIndex, Game->CurrentWorldIndex);          
    }
    
    UpdateWorld(Game);            
    
#if 0 
    block_puzzle* Puzzle = &Game->TestPuzzle;    
    if(!Puzzle->IsComplete)
    {        
        Puzzle->IsComplete = true;
        for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
        {math
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
    
    if(NOT_IN_DEVELOPMENT_MODE())
    {           
        PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);        
        
        PushClearColorAndDepth(Graphics, Black(), 1.0f);
        PushDepth(Graphics, true);
        
        PushWorldCommands(Graphics, GetCurrentWorld(Game), Game->Assets);        
    }    
}