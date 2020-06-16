#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

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

void DevelopmentImGuiInit(dev_context* DevContext)
{    
    Platform_InitImGui(DevContext->PlatformData);                
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
    
    ImGui::NewFrame();
    
    //IMPORTANT(EVERYONE): If you need help figuring out how to use IMGUI you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start
#if 0 
    local bool demo_window;
    ImGui::ShowDemoWindow(&demo_window);
#endif
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((f32)Graphics->RenderDim.x/3.0f, (f32)Graphics->RenderDim.y));    
    
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
    
    //const char* ShadingTypes[] = {"Normal Shading", "Wireframe Shading", "Wireframe on Normal Shading"};
    
    const char* ViewModeTypes[] = {"Lit", "Unlit", "Wireframe", "Wireframe on Lit"};
    ImGui::AlignTextToFramePadding();
    ImGui::Text("View Modes");
    ImGui::SameLine();
    ImGui::Combo("", (int*)&DevContext->ViewModeType, ViewModeTypes, ARRAYCOUNT(ViewModeTypes));
   
    DevelopmentFrameRecording(DevContext);
        
    ImGui::Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    ImGui::Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);        
    ImGui::Checkbox("Draw Frames", (bool*)&DevContext->DrawFrames);
    ImGui::Checkbox("Draw Player Collision Volume", (bool*)&DevContext->DrawPlayerCollisionVolume);
    
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

void DevelopmentImGuiRender(dev_context* DevContext)
{
    graphics* Graphics = DevContext->Graphics;
    
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
