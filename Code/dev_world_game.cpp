/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#define DEBUG_FILLED_BOX_INDICES_COUNT 36
void DrawBox(dev_context* DevContext, v3f P, v3f Dim, c4 Color)
{
    m4 Model = TransformM4(P, Dim);
    PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleBoxMesh.MeshID, Model, Color, DevContext->TriangleBoxMesh.IndexCount, 0, 0);
}

void DrawPoint(dev_context* DevContext, v3f P, c4 Color)
{
    DrawBox(DevContext, P, V3(0.05f), Color);
}

void DrawLineBox(dev_context* DevContext, v3f P, v3f Dim, c4 Color)
{    
    m4 Model = TransformM4(P, Dim);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineBoxMesh.MeshID, Model, Color, DevContext->LineBoxMesh.IndexCount, 0, 0);        
}

void DrawLineBoxMinMax(dev_context* DevContext, v3f Min, v3f Max, c4 Color)
{   
    v3f Dim = Max-Min;
    v3f P = V3(Min.xy + Dim.xy*0.5f, Min.z);
    DrawLineBox(DevContext, P, Dim, Color);    
}

void DrawLineEllipsoid(dev_context* DevContext, v3f CenterP, v3f Radius, c4 Color)
{
    m4 Model = TransformM4(CenterP, Radius);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineSphereMesh.MeshID, Model, Color, DevContext->LineSphereMesh.IndexCount, 0, 0); 
}

void DrawLineEllipsoid(dev_context* DevContext, ellipsoid3D Ellipsoid, c4 Color)
{
    DrawLineEllipsoid(DevContext, Ellipsoid.CenterP, Ellipsoid.Radius, Color);
}

i64 AllocateMesh(graphics* Graphics, mesh_generation_result* Mesh)
{
    i64 Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                        Mesh->Indices, Mesh->IndexCount*sizeof(u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

void CreateDevLineSphereMesh(dev_context* DevContext, u16 CircleSampleCount)
{
    mesh_generation_result MeshGenerationResult = GenerateLineSphere(GetDefaultArena(), 1.0f, CircleSampleCount);        
    DevContext->LineSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineSphereMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void CreateDevLineBoxMesh(dev_context* DevContext)
{
    mesh_generation_result MeshGenerationResult = GenerateLineBox(GetDefaultArena(), V3(1.0f, 1.0f, 1.0f));    
    DevContext->LineBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
}

void CreateDevTriangleBoxMesh(dev_context* DevContext)
{
    mesh_generation_result MeshGenerationResult = GenerateTriangleBox(GetDefaultArena(), V3(1.0f, 1.0f, 1.0f));       
    DevContext->TriangleBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
}

i64 AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &ImGui::GetIO();
    void* ImGuiFontData;
    v2i ImGuiFontDimensions;
    
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &ImGuiFontDimensions.width, &ImGuiFontDimensions.height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    i64 FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, ImGuiFontDimensions, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void DevelopmentImGui(dev_context* DevContext, game* Game, graphics* Graphics)
{    
    ImGui::NewFrame();
    
    //local bool demo_window;
    //ImGui::ShowDemoWindow(&demo_window);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((f32)Graphics->RenderDim.x/4.0f, (f32)Graphics->RenderDim.y));    
    
    local bool open = true;
    ImGui::Begin("Developer Tools", &open, ImGuiWindowFlags_NoCollapse);    
    
    ImGui::Text("FPS: %f", 1.0f/Game->dt);        
    
    if(ImGui::Checkbox("Debug Camera", (bool*)&DevContext->UseDevCamera))
    {
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {                    
            camera* Camera = &Game->Worlds[WorldIndex].Camera;
            camera* DevCamera = &DevContext->Cameras[WorldIndex];
            
            DevCamera->Position = Camera->Position;
            DevCamera->AngularVelocity = {};
            DevCamera->Orientation = IdentityM3();
            DevCamera->FocalPoint = Camera->FocalPoint;
            DevCamera->Distance = Magnitude(DevCamera->FocalPoint-DevCamera->Position);
        }
    }    
    
    frame_recording* FrameRecording = &DevContext->FrameRecording;
    {
        char* RecordingText = (FrameRecording->IsRecording) ? "Stop Recording" : "Start Recording";
        
        b32 PrevIsRecording = FrameRecording->IsRecording;
        if(ImGui::Button(RecordingText)) FrameRecording->IsRecording = !FrameRecording->IsRecording;
        
        if(!FrameRecording->IsRecording)
        {
            if(ImGui::Button("Load Recording"))
            {
                string RecordingPath = Platform_OpenFileDialog("arc_recording");
                if(!IsInvalidString(RecordingPath))
                {
                    if(!IsInvalidBuffer(FrameRecording->RecordingBuffer))
                        Global_Platform->FreeFileMemory(&FrameRecording->RecordingBuffer);            
                    
                    FrameRecording->RecordingBuffer = Global_Platform->ReadEntireFile(RecordingPath.Data);
                    CopyToStorage(&FrameRecording->RecordingPath, RecordingPath);
                }
            }
            
            if(!IsInvalidBuffer(FrameRecording->RecordingBuffer) && !IsInvalidString(FrameRecording->RecordingPath.String))
            {
                ImGui::SameLine();
                ImGui::Text("Recording File: %s\n", FrameRecording->RecordingPath.String.Data);
            }
            
            char* PlayRecordingText = (FrameRecording->IsPlaying) ? "Stop Playing" : "Start Playing";
            if(ImGui::Button(PlayRecordingText)) 
            {
                if(!FrameRecording->IsPlaying && !IsInvalidBuffer(FrameRecording->RecordingBuffer))
                {            
                    FrameRecording->IsPlaying = true;                
                }
                else
                {
                    FrameRecording->IsPlaying = false;
                }
            }
        }
        else
        {
            ImGui::SameLine();
            ImGui::Text("Recording File: %s\n", FrameRecording->RecordingPath.String.Data);
        }
        
        if(PrevIsRecording != FrameRecording->IsRecording)
        {
            if(FrameRecording->IsRecording)
            {
                string RecordingPath = Platform_FindNewFrameRecordingPath();
                if(!IsInvalidString(RecordingPath))
                {
                    CopyToStorage(&FrameRecording->RecordingPath, RecordingPath);
                }
                //NOTE(EVERYONE): Just started recording
            }
            else
            {
                //NOTE(EVERYONE): Just stopped recording
            }
        }
    }
    
    ImGui::Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);        
    
    if(ImGui::CollapsingHeader("Game Information"))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        
        world_entity* PlayerEntity0 = GetPlayerEntity(&Game->Worlds[0]);
        world_entity* PlayerEntity1 = GetPlayerEntity(&Game->Worlds[1]);
        
        v3f PlayerPosition0 = PlayerEntity0->Position;
        v3f PlayerPosition1 = PlayerEntity1->Position;
        
        v3f PlayerVelocity0 = PlayerEntity0->Velocity;
        v3f PlayerVelocity1 = PlayerEntity1->Velocity;
        
        v3f CollisionNormal = PlayerEntity0->CollidedNormal;
        
        ImGui::Text("Player 0 Position: (%.2f, %.2f, %.2f)", PlayerPosition0.x, PlayerPosition0.y, PlayerPosition0.z);
        ImGui::Text("Player 0 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity0.x, PlayerVelocity0.y, PlayerVelocity0.z);
        ImGui::Text("Collided Normal: (%.2f, %.2f, %.2f)", CollisionNormal.x, CollisionNormal.y, CollisionNormal.z);
        
        ImGui::Text("Player 1 Position: (%.2f, %.2f, %.2f)", PlayerPosition1.x, PlayerPosition1.y, PlayerPosition1.z);
        ImGui::Text("Player 1 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity1.x, PlayerVelocity1.y, PlayerVelocity1.z);
        
        ImGui::Text("Movement Time Max Iterations: %I64u", GameInformation->MaxTimeIterations);
        ImGui::Text("GJK Max Iterations: %I64u", GameInformation->MaxGJKIterations);
    }
    
    ImGui::End();    
    
    ImGui::Render();        
}

void DevelopmentRenderWorld(dev_context* DevContext, game* Game, graphics* Graphics, world* World, camera* Camera)
{   
    PushDepth(Graphics, true);    
    
    PushWorldCommands(Graphics, World, Camera, Game->Assets);                
    world_entity* PlayerEntity = GetPlayerEntity(World);
    player* Player = (player*)PlayerEntity->UserData;
    
    c4 PlayerColor = Blue();    
    if(World == &Game->Worlds[1])
        PlayerColor = Red();
    
    PushDepth(Graphics, false);
    
    ellipsoid3D Ellipsoid = GetPlayerEllipsoid(Game, Player);
    DrawLineEllipsoid(DevContext, Ellipsoid, PlayerColor);
    
    for(u32 PointIndex = 0; PointIndex < DevContext->DebugPointCount; PointIndex++)
    {
        debug_point* Point = DevContext->DebugPoints + PointIndex;
        DrawPoint(DevContext, Point->P, Point->Color);
    }
    DevContext->DebugPointCount = 0;    
}

void DevelopmentRender(dev_context* DevContext, game* Game, graphics* Graphics)
{    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGui(DevContext, Game, Graphics);        
    
    dev_input* Input = &DevContext->Input;
    
    world* World = GetCurrentWorld(Game);
    camera* Camera = &World->Camera;
    if(DevContext->UseDevCamera)
    {
        Camera = &DevContext->Cameras[Game->CurrentWorldIndex];
        
        if(IsDown(Input->Alt))
        {                                    
            if(IsDown(Input->LMB))
            {
                Camera->AngularVelocity.x += (Input->MouseDelta.y*Game->dt*CAMERA_ANGULAR_ACCELERATION);
                Camera->AngularVelocity.y += (Input->MouseDelta.x*Game->dt*CAMERA_ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(Input->MMB))
            {
                Camera->Velocity.x += (Input->MouseDelta.x*Game->dt*CAMERA_LINEAR_ACCELERATION);
                Camera->Velocity.y += (Input->MouseDelta.y*Game->dt*CAMERA_LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(Input->Scroll) > 0.0f)            
                Camera->Velocity.z -= Input->Scroll*Game->dt*CAMERA_SCROLL_ACCELERATION;                                            
        }                
        
        Camera->AngularVelocity *= (1.0f / (1.0f+Game->dt*CAMERA_ANGULAR_DAMPING));            
        v3f Eulers = (Camera->AngularVelocity*Game->dt);            
        
        quaternion Orientation = Normalize(RotQuat(Camera->Orientation.XAxis, Eulers.pitch)*RotQuat(Camera->Orientation.YAxis, Eulers.yaw));
        Camera->Orientation *= ToMatrix3(Orientation);
        
        Camera->Velocity.xy *= (1.0f /  (1.0f+Game->dt*CAMERA_LINEAR_DAMPING));            
        v2f Vel = Camera->Velocity.xy*Game->dt;
        v3f Delta = Vel.x*Camera->Orientation.XAxis - Vel.y*Camera->Orientation.YAxis;
        
        Camera->FocalPoint += Delta;
        Camera->Position += Delta;
        
        Camera->Velocity.z *= (1.0f/ (1.0f+Game->dt*CAMERA_SCROLL_DAMPING));            
        Camera->Distance += Camera->Velocity.z*Game->dt;            
        
        if(Camera->Distance < CAMERA_MIN_DISTANCE)
            Camera->Distance = CAMERA_MIN_DISTANCE;
        
        Camera->Position = Camera->FocalPoint + (Camera->Orientation.ZAxis*Camera->Distance);
    }
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    PushClearColorAndDepth(Graphics, Black(), 1.0f);    
    ///////////////////
    
    DevelopmentRenderWorld(DevContext, Game, Graphics, World, Camera);
    
    i32 OtherWidth = Graphics->RenderDim.width/4;
    i32 OtherHeight = Graphics->RenderDim.height/4;
    
    block_puzzle* Puzzle = &Game->TestPuzzle;
    
    for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
    {
        goal_rect* GoalRect = Puzzle->GoalRects + GoalIndex;
        
        c4 Color = Blue();
        if(GoalRect->GoalIsMet)
            Color = Red();
        
        DrawLineBoxMinMax(DevContext, GoalRect->Rect.Min, GoalRect->Rect.Max, Color);
    }   
    
    if(DevContext->DrawOtherWorld)
    {        
        PushViewportAndScissor(Graphics, Graphics->RenderDim.width-OtherWidth, Graphics->RenderDim.height-OtherHeight, 
                               OtherWidth, OtherHeight);
        
        world* OtherWorld = GetNotCurrentWorld(Game);
        camera* OtherCamera = &OtherWorld->Camera;
        if(DevContext->UseDevCamera)
            OtherCamera = &DevContext->Cameras[!Game->CurrentWorldIndex];
        DevelopmentRenderWorld(DevContext, Game, Graphics, OtherWorld, OtherCamera);
    }
    
    PushCull(Graphics, false);
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    m4 Orthographic = OrthographicM4(0, (f32)Graphics->RenderDim.width, 0, (f32)Graphics->RenderDim.height, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ptr IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = ImGui::GetDrawData();        
    
    u32 LastImGuiMeshCount = DevContext->ImGuiMeshCount;
    DevContext->ImGuiMeshCount = DrawData->CmdListsCount;
    ASSERT(DevContext->ImGuiMeshCount <= MAX_IMGUI_MESHES);        
    
    for(i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(DevContext->ImGuiMeshes[CmdListIndex] == -1)
            DevContext->ImGuiMeshes[CmdListIndex] = Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT);
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(Graphics, DevContext->ImGuiMeshes[CmdListIndex], 
                                 CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);                
        
        for(i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            ASSERT(!Cmd->UserCallback);
            
            ImVec4 ClipRect = Cmd->ClipRect;
            if((ClipRect.x < Graphics->RenderDim.width) && (ClipRect.y < Graphics->RenderDim.height) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                i32 X = (i32)ClipRect.x;
                i32 Y = (i32)(Graphics->RenderDim.height-ClipRect.w);
                i32 Width = (i32)(ClipRect.z - ClipRect.x);
                i32 Height = (i32)(ClipRect.w - ClipRect.y);
                
                PushScissor(Graphics, X, Y, Width, Height);
                
                i64 TextureID = (i64)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, DevContext->ImGuiMeshes[CmdListIndex], TextureID, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
            }
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, true);
    PushDepth(Graphics, true);            
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics)
{
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = CreateArena(KILOBYTE(32));                
        DevContext->FrameRecording.RecordingPath = AllocateStringStorage(&DevContext->DevStorage, 8092);
        
        Platform_InitImGui(DevContext->PlatformData);                
        AllocateImGuiFont(Graphics);                
        
        CreateDevLineBoxMesh(DevContext);
        CreateDevLineSphereMesh(DevContext, 60);
        CreateDevTriangleBoxMesh(DevContext);
        
        SetMemoryI64(DevContext->ImGuiMeshes, -1, sizeof(DevContext->ImGuiMeshes));
        
        ImGuiIO* IO = &ImGui::GetIO();
        IO->BackendRendererName = "OpenGL";
        IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData, Graphics->RenderDim, Game->dt);        
        DevelopmentRender(DevContext, Game, Graphics);        
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}

void DevelopmentRecordFrame(dev_context* DevContext, game* Game)
{
    frame_recording* Recording = &DevContext->FrameRecording;
    if(Recording->IsRecording)
    {
        
        
    }
}