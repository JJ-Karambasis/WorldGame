#include "world_game.h"
#include "assets/assets.cpp"
#include "audio.cpp"
#include "animation.cpp"
#include "collision_detection.cpp"
#include "world.cpp"
#include "player.cpp"
#include "wav.cpp"
#include "fbx.cpp"
#include "png.cpp"
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
    
    platform_time Start = Global_Platform->Clock();
    
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    SetGlobalErrorStream(Global_Platform->ErrorStream);
    
    world_entity_id ID = {};
    if(!Game->Initialized)
    {       
        b32 AssetResult = InitAssets(&Game->Assets2);
        ASSERT(AssetResult);
        
        Game->GameStorage = CreateArena(MEGABYTE(16));                                
        
        Game->Assets->BoxWalkableMesh = LoadWalkableMesh(Game->Assets, "Box.fbx");
        
        Game->Assets->QuadWalkableMesh = LoadWalkableMesh(Game->Assets, "Quad.fbx");        
        Game->Assets->BoxConvexHull = LoadConvexHull(Game->Assets, "assets/raw/fbx/BoxConvexHull.fbx");
        
        Game->Assets->FloorWalkableMesh = LoadWalkableMesh(Game->Assets, "assets/raw/fbx/FloorMesh.fbx");
        Game->Assets->FloorConvexHull = LoadConvexHull(Game->Assets, "assets/raw/fbx/FloorConvexHull.fbx");
        
        
        
        
        
        Game->Assets->TestAudio = LoadAudio(Game->Assets, "TestSound.wav");
        Game->Assets->TestAudio2 = LoadAudio(Game->Assets, "TestSound2.wav");
        
        Game->Assets->TestMaterial0_Diffuse = LoadTexture(Game->Assets, "TestMaterial0_diffuse.png", true);   
        Game->Assets->TestMaterial0_Normal = LoadTexture(Game->Assets, "TestMaterial0_normal.png", false);
        Game->Assets->TestMaterial0_Specular = LoadTexture(Game->Assets, "TestMaterial0_specular.png", false);        
        Game->Assets->TestMaterial1_Diffuse = LoadTexture(Game->Assets, "TestMaterial1_diffuse.png", true);
        Game->Assets->TestMaterial1_Normal = LoadTexture(Game->Assets, "TestMaterial1_normal.png", false);
        Game->Assets->TestMaterial1_Specular = LoadTexture(Game->Assets, "TestMaterial1_specular.png", false);                
        
        Game->Assets->Material_DiffuseC = CreateMaterial_DCon(Game->Assets, Blue3());
        Game->Assets->Material_DiffuseT = CreateMaterial_DTex(Game->Assets, &Game->Assets->TestMaterial1_Diffuse);                        
        Game->Assets->Material_DiffuseC_SpecularC = CreateMaterial_DCon_SCon(Game->Assets, Blue3(), 0.5f, 8);
        Game->Assets->Material_DiffuseC_SpecularT = CreateMaterial_DCon_STex(Game->Assets, Red3(), &Game->Assets->TestMaterial0_Specular, 8);
        Game->Assets->Material_DiffuseT_SpecularC = CreateMaterial_DTex_SCon(Game->Assets, &Game->Assets->TestMaterial1_Diffuse, 1.0f, 8);
        Game->Assets->Material_DiffuseT_SpecularT = CreateMaterial_DTex_STex(Game->Assets, &Game->Assets->TestMaterial0_Diffuse, &Game->Assets->TestMaterial0_Specular, 16);
        Game->Assets->Material_DiffuseT_Normal = CreateMaterial_DTex_NTex(Game->Assets, &Game->Assets->TestMaterial1_Diffuse, &Game->Assets->TestMaterial1_Normal);
        Game->Assets->Material_DiffuseC_Normal = CreateMaterial_DCon_NTex(Game->Assets, Yellow3(), &Game->Assets->TestMaterial1_Normal);
        Game->Assets->Material_DiffuseC_SpecularC_Normal = CreateMaterial_DCon_SCon_NTex(Game->Assets, Green3(), 1.0f, 32, &Game->Assets->TestMaterial1_Normal);
        Game->Assets->Material_DiffuseT_SpecularT_Normal = CreateMaterial_DTex_STex_NTex(Game->Assets, &Game->Assets->TestMaterial0_Diffuse, &Game->Assets->TestMaterial0_Specular, 8, &Game->Assets->TestMaterial0_Normal);
        Game->Assets->Material_DiffuseT_SpecularT_Normal_2 = CreateMaterial_DTex_STex_NTex(Game->Assets, &Game->Assets->TestMaterial1_Diffuse, &Game->Assets->TestMaterial1_Specular, 8, &Game->Assets->TestMaterial1_Normal);
        
        PlayAudio(Game, &Game->Assets->TestAudio, 1.0f);
        
        Game->RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim);
        
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
            
            v3f P0 = V3() + Global_WorldZAxis*PLAYER_RADIUS;
            capsule PlayerCapsule = CreateCapsule(P0, P0+Global_WorldZAxis*PLAYER_HEIGHT, PLAYER_RADIUS);
            World->PlayerEntity = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, V3(0.0f, 0.0f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(PI*0.0f, 0.0f*PI, 0.0f*PI), MESH_ASSET_ID_PLAYER, &Game->Assets->Material_DiffuseC_SpecularC);
            AddCollisionVolume(Game, World->PlayerEntity, &PlayerCapsule);
            
            game_camera* Camera = &World->Camera;
            
            Camera->Target = World->PlayerEntity->Position;
            
            Camera->Coordinates = SphericalCoordinates(6, TO_RAD(-90.0f), TO_RAD(35.0f));
            
            Camera->FieldOfView = TO_RAD(65.0f);                        
            Camera->ZNear = CAMERA_ZNEAR;
            Camera->ZFar = CAMERA_ZFAR;
            
            World->JumpingQuads[0].CenterP = V3(-1.0f, 0.0f, 0.0f);
            World->JumpingQuads[0].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[1].CenterP = V3(-4.0f, 0.0f, 0.0f);
            World->JumpingQuads[1].Dimensions = V2(1.0f, 2.0f);
            
            World->JumpingQuads[0].OtherQuad = &World->JumpingQuads[1];
            World->JumpingQuads[1].OtherQuad = &World->JumpingQuads[0];
        }
        
        CreateStaticEntity(Game, 0, V3(0.0f, 0.0f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f), MESH_ASSET_ID_FLOOR, &Game->Assets->Material_DiffuseT);                           
        CreateStaticEntity(Game, 0, V3(-6.2f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.1f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseT_SpecularT_Normal_2);
        CreateStaticEntity(Game, 0, V3(-3.0f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.25f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseT_SpecularT_Normal);
        CreateStaticEntity(Game, 0, V3(-4.6f, -4.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.33f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseC_SpecularC_Normal);
        CreateStaticEntity(Game, 0, V3(-1.6f, -5.5f, 0.0f), V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.0f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseT_Normal);
        CreateStaticEntity(Game, 0, V3(-1.0f, 5.5f, 0.0f),  V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.2f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseT_SpecularT);
        CreateStaticEntity(Game, 0, V3(1.0f, 4.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.6f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseT_SpecularC);
        CreateStaticEntity(Game, 0, V3(1.5f, 2.5f, 0.0f),   V3(1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, PI*0.5f), MESH_ASSET_ID_BOX, &Game->Assets->Material_DiffuseC_Normal);                        
        
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
    
    b32 Simulate = true;
    if(Simulate)
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
        world* World = GetCurrentWorld(Game);        
        view_settings ViewSettings = GetViewSettings(&World->Camera);        
        
        PushWorldShadingCommands(Graphics, Game->RenderBuffer, World, &ViewSettings, Game->Assets, &Game->Assets2);        
        PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Graphics->RenderDim);
    }    
    
    platform_time End = Global_Platform->Clock();
    CONSOLE_LOG("Elapsed Game Time %f\n", Global_Platform->ElapsedTime(End, Start)*1000.0);
}