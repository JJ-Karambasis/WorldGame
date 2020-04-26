/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

graphics_texture* AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &ImGui::GetIO();
    void* ImGuiFontData;
    v2i ImGuiFontDimensions;
    
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &ImGuiFontDimensions.width, &ImGuiFontDimensions.height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture* FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, ImGuiFontDimensions, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void DevelopmentImGui(dev_context* DevContext, game* Game, graphics* Graphics)
{    
    ImGui::NewFrame();
    
    local bool Open; 
    ImGui::ShowDemoWindow(&Open);
    
    ImGui::Render();        
}

void DevelopmentRender(dev_context* DevContext, game* Game, graphics* Graphics)
{    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGui(DevContext, Game, Graphics);
    
    camera* Camera = &DevContext->Camera;
        
    PushClearColorAndDepth(Graphics, Black(), 1.0f);            
    
    PushDepth(Graphics, true);
    
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);        
    
    PushProjection(Graphics, Perspective); 
    PushCameraView(Graphics, CameraView);        
    
    world* World = GetCurrentWorld(Game);
    for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
    {
        if(!Game->Assets->BoxGraphicsMesh)        
            Game->Assets->BoxGraphicsMesh = LoadGraphicsMesh(Game->Assets, "Box.obj");
        
        ASSERT(Game->Assets->BoxGraphicsMesh);
        PushDrawShadedColoredMesh(Graphics, Game->Assets->BoxGraphicsMesh, Entity->Transform, Entity->Color); 
    }   
        
    ptr IndexSize = sizeof(ImDrawIdx);
    
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);    
    PushCull(Graphics, false);
    PushDepth(Graphics, false);        
    
    m4 Orthographic = OrthographicM4(0, (f32)Graphics->RenderDim.width, 0, (f32)Graphics->RenderDim.height, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ImDrawData* DrawData = ImGui::GetDrawData();
    for(i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(!DevContext->ImGuiMesh)
            DevContext->ImGuiMesh = Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT);
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(DevContext->ImGuiMesh, CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(graphics_vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);
        
        for(i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            ASSERT(!Cmd->UserCallback);
            
            graphics_texture* Texture = (graphics_texture*)Cmd->TextureId;
            PushDrawImGuiUI(Graphics, DevContext->ImGuiMesh, Texture, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, true);
    PushDepth(Graphics, true);            
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics)
{
    if(!DevContext->Initialized)
    {
        Platform_InitImGui(DevContext->PlatformData);        
        
        AllocateImGuiFont(Graphics);
        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsPressed(Input->ToggleDevState))
    {
        DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
        if(IsInDevelopmentMode(DevContext))
        {
            DevContext->Camera.Position = Game->Camera.Position;
            DevContext->Camera.AngularVelocity = {};
            DevContext->Camera.Orientation = IdentityM3();
            DevContext->Camera.FocalPoint = Game->Camera.FocalPoint;
            DevContext->Camera.Distance = Magnitude(DevContext->Camera.FocalPoint-DevContext->Camera.Position);
        }
    }
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData, Graphics->RenderDim, Game->dt);
        
        camera* Camera = &DevContext->Camera;
        
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
        
        DevelopmentRender(DevContext, Game, Graphics);        
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}
