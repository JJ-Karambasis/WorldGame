#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "world.cpp"
#include "player.cpp"
#include "wav.cpp"
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
    
    world_entity_id ID = {};
    if(!Game->Initialized)
    {                
        Game->GameStorage = CreateArena(MEGABYTE(16));        
        
        Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.fbx");
        Game->Assets->BoxWalkableMesh = LoadWalkableMesh(Game->Assets, "Box.fbx");
        Game->Assets->QuadGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Quad.fbx");
        Game->Assets->QuadWalkableMesh = LoadWalkableMesh(Game->Assets, "Quad.fbx");
        Game->Assets->PlayerMesh = LoadGraphicsMesh(Game->Assets, "TestPlayerMesh.fbx");
        Game->Assets->TestAudio = LoadAudio(Game->Assets, "TestSound.wav");
        Game->Assets->TestAudio2 = LoadAudio(Game->Assets, "TestSound2.wav");
        
        PlayAudio(Game, &Game->Assets->TestAudio, 1.0f);
        
#if 0 
        Game->Assets->TestSkeletonMesh = LoadGraphicsMesh(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestSkeleton = LoadSkeleton(Game->Assets, "TestSkeleton.fbx");
        Game->Assets->TestAnimation = LoadAnimation(Game->Assets, "TestAnimation.fbx");
#endif
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            world* World = GetWorld(Game, WorldIndex);
            World->WorldIndex = WorldIndex;
            World->EntityPool = CreatePool<world_entity>(&Game->GameStorage, 512);            
            CreatePlayer(Game, WorldIndex, V3(-1.0f, 0.0f, 0.0f), V3(PLAYER_RADIUS, PLAYER_RADIUS, PLAYER_HEIGHT), WorldIndex == 0 ? Blue4() : Red4());            
            
            world_entity* PlayerEntity = GetPlayerEntity(World);
            camera* Camera = &World->Camera;
            
            Camera->Position = PlayerEntity->Position;
            Camera->FocalPoint = PlayerEntity->Position;
            Camera->Position.z += 6.0f;
            Camera->Orientation = IdentityM3();                
            
            World->JumpingQuads[0].CenterP = V3(-1.0f, 0.0f, 0.0f);
            World->JumpingQuads[0].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[1].CenterP = V3(-4.0f, 0.0f, 0.0f);
            World->JumpingQuads[1].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[0].OtherQuad = &World->JumpingQuads[1];
            World->JumpingQuads[1].OtherQuad = &World->JumpingQuads[0];
        }
        
        CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(1.5f, 0.0f, 0.0f), V3(1.0f, 30.0f, 30.0f), V3(0.0f, -0.5f*PI, 0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->QuadGraphicsMesh, &Game->Assets->QuadWalkableMesh);                                                        
        //CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(-3.5f, 0.0f, 0.0f), V3(1.0f, 3.0f, 3.0f), V3(0.0f, -0.5f*PI, PI*0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->QuadGraphicsMesh, &Game->Assets->QuadWalkableMesh);                                                        
        
        //CreateEntityInBothWorlds(Game, WORLD_ENTITY_TYPE_WALKABLE, V3(-2.2f, 0.0f, 1.0f), V3(1.0f, 3.0f, 1.0f), V3(0.0f, 0.0f*PI, 0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), RGBA(0.45f, 0.45f, 0.45f, 1.0f), &Game->Assets->BoxGraphicsMesh, &Game->Assets->BoxWalkableMesh);                                                                
        
        Game->Initialized = true;
    }        
    
    
    if(IsPressed(Game->Input->SwitchWorld)) 
    {
        u32 PrevIndex = Game->CurrentWorldIndex;
        Game->CurrentWorldIndex = !PrevIndex;
        OnWorldSwitch(Game, PrevIndex, Game->CurrentWorldIndex);          
    }
    
    if(IsPressed(Game->Input->Action))
        PlayAudio(Game, &Game->Assets->TestAudio2, 0.15f);
    
    UpdateWorld(Game);            
    
#if 0 
    block_puzzle* Puzzle = &Game->TestPuzzle;    
    if(!Puzzle->IsComplete)
    {        
        Puzzle->IsComplete = true;
        for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
        {
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
    
    
    FOR_EACH(PlayingAudio, &Game->AudioOutput->PlayingAudioPool)
    {
        if(PlayingAudio->IsFinishedPlaying)        
            FreeFromPool(&Game->AudioOutput->PlayingAudioPool, PlayingAudio);        
    }
    
    if(NOT_IN_DEVELOPMENT_MODE())
    {   
        PushGameCommands(Graphics, Game);        
    }    
}