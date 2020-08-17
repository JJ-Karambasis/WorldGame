#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

using namespace ImGui;

i64 AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &GetIO();
    void* ImGuiFontData;
    
    i32 Width, Height;        
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &Width, &Height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    i64 FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, (u32)Width, (u32)Height, GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void DevelopmentImGuiInit(dev_context* DevContext)
{    
    Platform_InitImGui(DevContext->PlatformData[0]);                
    AllocateImGuiFont(DevContext->Graphics);                
    
    SetMemoryI64(DevContext->ImGuiMeshes, -1, sizeof(DevContext->ImGuiMeshes));
    
    ImGuiIO* IO = &ImGui::GetIO();
    IO->BackendRendererName = "OpenGL";
    IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

void DevelopmentImGuiUpdate(dev_context* DevContext)
{
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    NewFrame();
    
    //IMPORTANT(EVERYONE): If you need help figuring out how to use ImGui you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start
#if 0
    local bool demo_window;
    ShowDemoWindow(&demo_window);
#endif
    
    SetNextWindowPos(ImVec2(0, 0));
    SetNextWindowSize(ImVec2((f32)Graphics->RenderDim.x/3.0f, (f32)Graphics->RenderDim.y));    
    
    local bool open = true;
    Begin("Developer Tools", &open, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize);    
    
    Text("FPS: %f", 1.0f/Game->dt);        
    
    if(Checkbox("Debug Camera", (bool*)&DevContext->UseDevCamera))
    {
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {           
 
            game_camera* Camera = &(Game->Worlds[WorldIndex].Camera);
            camera* DevCamera = &DevContext->Cameras[WorldIndex];

            rigid_transform_matrix CameraTransform = GetCameraTransform(Camera);  
            
            DevCamera->Position = CameraTransform.Translation;
            DevCamera->AngularVelocity = {};
            DevCamera->Orientation = CameraTransform.Orientation;
            DevCamera->FocalPoint = Camera->Target;
            DevCamera->Distance = Camera->Coordinates.Radius;
        }
    }    
    
    //const char* ShadingTypes[] = {"Normal Shading", "Wireframe Shading", "Wireframe on Normal Shading"};
    
    const char* ViewModeTypes[] = {"Lit", "Unlit", "Wireframe", "Wireframe on Lit"};
    AlignTextToFramePadding();
    Text("View Modes");
    SameLine();
    Combo("", (int*)&DevContext->ViewModeType, ViewModeTypes, ARRAYCOUNT(ViewModeTypes));
   
    DevelopmentFrameRecording(DevContext);    
    
    ImGui::Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    ImGui::Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);    
    ImGui::Checkbox("Draw colliders", (bool*)&DevContext->DrawColliders);    
    ImGui::Checkbox("Draw Grid", (bool*)&DevContext->DrawGrid);  
    ImGui::Checkbox("Edit Mode", (bool*)&DevContext->EditMode);  
    
    local b32 Open = true;
    if(CollapsingHeader("Game Information", ImGuiTreeNodeFlags_DefaultOpen))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        
        world_entity* PlayerEntity0 = Game->Worlds[0].PlayerEntity;
        world_entity* PlayerEntity1 = Game->Worlds[1].PlayerEntity;
        
        v3f PlayerPosition0 = PlayerEntity0->Position;
        v3f PlayerPosition1 = PlayerEntity1->Position;
        
        v3f PlayerVelocity0 = PlayerEntity0->Velocity;
        v3f PlayerVelocity1 = PlayerEntity1->Velocity;
        
        Text("Player 0 Position: (%.2f, %.2f, %.6f)", PlayerPosition0.x, PlayerPosition0.y, PlayerPosition0.z);
        Text("Player 0 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity0.x, PlayerVelocity0.y, PlayerVelocity0.z);        
        
        Text("Player 1 Position: (%.2f, %.2f, %.2f)", PlayerPosition1.x, PlayerPosition1.y, PlayerPosition1.z);
        Text("Player 1 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity1.x, PlayerVelocity1.y, PlayerVelocity1.z);
        
        Text("Movement Time Max Iterations: %I64u", GameInformation->MaxTimeIterations);
        Text("GJK Max Iterations: %I64u", GameInformation->MaxGJKIterations);
        
        if(!DevContext->UseDevCamera)
        {            
            for(u32 WorldIndex = 0; WorldIndex < ARRAYCOUNT(Game->Worlds); WorldIndex++)
            {                                
                game_camera* Camera = &Game->Worlds[WorldIndex].Camera;
                
                NewLine();
                
                Text("Camera %d Settings:", WorldIndex);                                
                
                PushID(WorldIndex*6+0);
                DragFloat("Radius", &Camera->Coordinates.Radius, 0.01f, 0.001f, 50.0f, "%.3f");
                PopID();                                
                
                PushID(WorldIndex*6+1);
                f32 AzimuthDegree = TO_DEGREE(Camera->Coordinates.Azimuth);
                DragFloat("Azimuth", &AzimuthDegree, 1.0f, -180.0f, 180.0f, "%.1f");
                Camera->Coordinates.Azimuth = TO_RAD(AzimuthDegree);
                PopID();
                
                PushID(WorldIndex*6+2);
                f32 InclinationDegree = TO_DEGREE(Camera->Coordinates.Inclination);
                DragFloat("Inclination", &InclinationDegree, 1.0f, -180.0f, 180.0f, "%.1f");
                Camera->Coordinates.Inclination = TO_RAD(InclinationDegree);
                PopID();
                
                PushID(WorldIndex*6 + 3);
                SliderAngle("Field Of View", &Camera->FieldOfView, 0.0f, 90.0f);        
                PopID();
                
                PushID(WorldIndex*6 + 4);
                DragFloat("Near Plane", &Camera->ZNear, 0.001f, 0.001f, 1.0f, "%.3f");
                PopID();
                
                PushID(WorldIndex*6 + 5);
                DragFloat("Far Plane", &Camera->ZFar, 0.01f, Camera->ZNear, 10000.0f, "%.3f");                                                  
                PopID();                
            }
        }
    }

    if(ImGui::CollapsingHeader("SelectedObject"))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        if(DevContext->SelectedObject != nullptr)
        {
            v3f ObjectVelocity = DevContext->SelectedObject->Velocity;

            ImGui::InputFloat3("Position", &DevContext->SelectedObject->Position.x, 3);
            SliderFloat("X Scale", &DevContext->SelectedObject->Scale.x, 0.1f, 100.0f);
            SliderFloat("Y Scale", &DevContext->SelectedObject->Scale.y, 0.1f, 100.0f);
            SliderFloat("Z Scale", &DevContext->SelectedObject->Scale.z, 0.1f, 100.0f);
            ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", ObjectVelocity.x, ObjectVelocity.y, ObjectVelocity.z);
            ImGui::Text("Type: (%d)", DevContext->SelectedObject->Type);
            ImGui::Text("ID: (%d)", DevContext->SelectedObject->ID.ID);
            ImGui::Text("WorldIndex: (%d)", DevContext->SelectedObject->ID.WorldIndex);
            if(!IsInvalidEntityID(DevContext->SelectedObject->LinkID))
            {
                ImGui::Text("Link ID: (%d)", DevContext->SelectedObject->LinkID.ID);
                ImGui::Text("Link WorldIndex: (%d)", DevContext->SelectedObject->LinkID.WorldIndex);
            }
            else
            {
                ImGui::Text("Link ID: (%s)", "No Linked Entity");
                ImGui::Text("Link WorldIndex: (%s)", "No Linked Entity");
            }
            ImGui::Text("RayCast Direction: (%.2f, %.2f, %.2f)", DevContext->InspectRay.x, DevContext->InspectRay.y, DevContext->InspectRay.z);
            
        }
        else
        {
            ImGui::Text("No object selected");
            ImGui::Text("RayCast Direction: (%.2f, %.2f, %.2f)", DevContext->InspectRay.x, DevContext->InspectRay.y, DevContext->InspectRay.z);
        }
    }
    
    if(CollapsingHeader("Debug Logs"))
    {
        if(Button("Clear"))
        {
            DevContext->Logs.Size = 0;
            ResetArena(&DevContext->LogStorage);
        }        
        
        SameLine();
        
        if(Button("Copy")) LogToClipboard();       
        
        for(u32 LogIndex = 0; LogIndex < DevContext->Logs.Size; LogIndex++)
        {
            string Log = DevContext->Logs[LogIndex];
            TextUnformatted(Log.Data, Log.Data+Log.Length);
        }
    }
    
    End();        
    Render();        
}

void DevelopmentImGuiRender(dev_context* DevContext)
{
    graphics* Graphics = DevContext->Graphics;
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    m4 Orthographic = OrthographicM4(0, (f32)Graphics->RenderDim.width, 0, (f32)Graphics->RenderDim.height, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ptr IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = GetDrawData();        
    
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
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushDepth(Graphics, true);            
}
