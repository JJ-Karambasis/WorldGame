/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

void DrawVerticalCapsule(graphics* Graphics, dev_capsule_mesh* CapsuleMesh, vertical_capsule Capsule, c4 Color)
{
    v3f BottomPosition = V3(Capsule.P.xy, Capsule.P.z + Capsule.Radius);
    m4 Model = IdentityM4();
    
    Model.XAxis.xyz *= Capsule.Radius;
    Model.YAxis.xyz *= Capsule.Radius;
    Model.ZAxis.xyz *= Capsule.Radius;
    
    Model.Translation.xyz = BottomPosition;
    Model.ZAxis.xyz = -Model.ZAxis.xyz;
    
    PushDrawLineMesh(Graphics, CapsuleMesh->MeshID, Model, Color, CapsuleMesh->CapIndexCount, 0, 0);
    
    Model.ZAxis.xyz = -Model.ZAxis.xyz;
    Model.Translation.xyz = V3(BottomPosition.xy, BottomPosition.z + Capsule.Height);
    PushDrawLineMesh(Graphics, CapsuleMesh->MeshID, Model, Color, CapsuleMesh->CapIndexCount, 0, 0);
    
    Model.ZAxis.xyz = V3(0.0f, 0.0f, Capsule.Height);
    Model.Translation.xyz = V3(BottomPosition.xy, BottomPosition.z + (Capsule.Height*0.5f));
    PushDrawLineMesh(Graphics, CapsuleMesh->MeshID, Model, Color, CapsuleMesh->BodyIndexCount, CapsuleMesh->CapIndexCount, CapsuleMesh->BodyVertexOffset);
}

void PopulateCircleIndices(u16** Indices, u16 StartSampleIndex, u16 CircleSampleCount)
{
    u16* IndicesAt = *Indices;    
    u16 TotalSampleCount = StartSampleIndex+CircleSampleCount;
    for(u16 SampleIndex = StartSampleIndex; SampleIndex < TotalSampleCount; SampleIndex++)
    {
        if(SampleIndex == (TotalSampleCount-1))
        {
            *IndicesAt++ = SampleIndex;
            *IndicesAt++ = SampleIndex - (CircleSampleCount-1);
        }
        else
        {
            *IndicesAt++ = SampleIndex;
            *IndicesAt++ = SampleIndex+1;
        }
    }            
    *Indices = IndicesAt;
} 

dev_capsule_mesh CreateDevCapsuleMesh(graphics* Graphics, u16 CircleSampleCount)
{
    dev_capsule_mesh Result = {};    
    
    u16 HalfCircleSampleCountPlusOne = (CircleSampleCount/2)+1;
    f32 CircleSampleIncrement = (2.0f*PI)/(f32)CircleSampleCount;            
    
    u32 CapVertexCount = CircleSampleCount+(HalfCircleSampleCountPlusOne*2);
    Result.BodyVertexOffset = CapVertexCount;    
    Result.CapIndexCount = CapVertexCount*2;    
    Result.BodyIndexCount = 8;    
    
    u32 VertexCount = CapVertexCount+8;
    u32 IndexCount = Result.CapIndexCount+Result.BodyIndexCount;
    
    vertex_p3* VertexData = PushArray(VertexCount, vertex_p3, Clear, 0);
    u16* IndexData = PushArray(IndexCount, u16, Clear, 0); 
    
    vertex_p3* VertexAt = (vertex_p3*)VertexData;
    
    f32 Radians;
    Radians = 0.0f;        
    for(u32 SampleIndex = 0; SampleIndex < CircleSampleCount; SampleIndex++, Radians += CircleSampleIncrement)
        *VertexAt++ = {V3(Cos(Radians), Sin(Radians), 0.0f)};
    
    Radians = 0.0;
    for(u32 SampleIndex = 0; SampleIndex < HalfCircleSampleCountPlusOne; SampleIndex++, Radians += CircleSampleIncrement)                
        *VertexAt++ = {V3(0.0f, Cos(Radians), Sin(Radians))};
    
    Radians = 0.0f;
    for(u32 SampleIndex = 0; SampleIndex < HalfCircleSampleCountPlusOne; SampleIndex++, Radians += CircleSampleIncrement)
        *VertexAt++ = {V3(Cos(Radians), 0.0f, Sin(Radians))};        
    
    u16* IndicesAt = (u16*)IndexData;
    PopulateCircleIndices(&IndicesAt, 0, CircleSampleCount);
    PopulateCircleIndices(&IndicesAt, CircleSampleCount, HalfCircleSampleCountPlusOne);
    PopulateCircleIndices(&IndicesAt, CircleSampleCount+HalfCircleSampleCountPlusOne, HalfCircleSampleCountPlusOne);
    
    *VertexAt++ = {V3( 1.0f,  0.0f, -0.5f)};
    *VertexAt++ = {V3( 1.0f,  0.0f,  0.5f)};
    
    *VertexAt++ = {V3( 0.0f,  1.0f, -0.5f)};
    *VertexAt++ = {V3( 0.0f,  1.0f,  0.5f)};
    
    *VertexAt++ = {V3(-1.0f,  0.0f, -0.5f)};
    *VertexAt++ = {V3(-1.0f,  0.0f,  0.5f)};
    
    *VertexAt++ = {V3( 0.0f, -1.0f, -0.5f)};
    *VertexAt++ = {V3( 0.0f, -1.0f,  0.5f)};
    
    *IndicesAt++ = 0;
    *IndicesAt++ = 1;
    *IndicesAt++ = 2;
    *IndicesAt++ = 3;
    *IndicesAt++ = 4;
    *IndicesAt++ = 5;
    *IndicesAt++ = 6;
    *IndicesAt++ = 7;
    
    Result.MeshID = Graphics->AllocateMesh(Graphics, VertexData, VertexCount*sizeof(vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                           IndexData, IndexCount*sizeof(u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    
    return Result;
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
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(Graphics->RenderDim/5));    
    
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
    
    ImGui::Checkbox("Draw Colliders", (bool*)&DevContext->DrawColliders);
    ImGui::Checkbox("Draw Blockers", (bool*)&DevContext->DrawBlockers);
    
    ImGui::End();    
    
    ImGui::Render();        
}

void DevelopmentRenderWorld(dev_context* DevContext, game* Game, graphics* Graphics, world* World, camera* Camera)
{    
    PushWorldCommands(Graphics, World, Camera);            
    world_entity* PlayerEntity = GetPlayerEntity(World);
    
    c4 PlayerColor = Blue();    
    if(World == &Game->Worlds[1])
        PlayerColor = Red();
    
    vertical_capsule VerticalCapsule = GetWorldSpaceVerticalCapsule(PlayerEntity);    
    DrawVerticalCapsule(Graphics, &DevContext->CapsuleMesh, VerticalCapsule, PlayerColor);        
    
    PushCull(Graphics, false);
    
    if(DevContext->DrawColliders)
    {
        pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);
        for(world_entity* Entity = GetFirst(&Iter); Entity; Entity = GetNext(&Iter))
        {            
            
            switch(Entity->Collider.Type)
            {
                case COLLIDER_TYPE_ALIGNED_BOX:
                {
                    aligned_box Box = GetWorldSpaceAlignedBox(Entity);
                    
                    v3f Vertices[8];
                    GetBoxVerticesFromDimAndCenterP(Vertices, Box.CenterP, Box.Dim);
                    
#define PUSH_QUAD(i0, i1, i2, i3) PushDrawQuad(Graphics, Vertices[i0], Vertices[i1], Vertices[i2], Vertices[i3], Red())
                    
                    PUSH_QUAD(5, 0, 3, 6);
                    PUSH_QUAD(4, 5, 0, 1);
                    PUSH_QUAD(1, 4, 7, 2);
                    PUSH_QUAD(3, 2, 7, 6);
                    
#undef PUSH_QUAD
                    
                } break;
            }        
        }
    }
    
    if(DevContext->DrawBlockers)
    {
        for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
        {
            v3f P[4] = 
            {
                Blocker->P0,
                Blocker->P1,
                V3(Blocker->P1.xy, Blocker->P1.z+Blocker->Height1),
                V3(Blocker->P0.xy, Blocker->P0.z+Blocker->Height0)                
            };
            
            PushDrawQuad(Graphics, P, Red());            
        }            
    }        
    
    PushCull(Graphics, true);
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
    PushDepth(Graphics, true);
    
    ///////////////////
    
    DevelopmentRenderWorld(DevContext, Game, Graphics, World, Camera);
    
    i32 OtherWidth = Graphics->RenderDim.width/4;
    i32 OtherHeight = Graphics->RenderDim.height/4;
    
    PushViewportAndScissor(Graphics, Graphics->RenderDim.width-OtherWidth, Graphics->RenderDim.height-OtherHeight, 
                           OtherWidth, OtherHeight);
        
    world* OtherWorld = GetNotCurrentWorld(Game);
    camera* OtherCamera = &OtherWorld->Camera;
    if(DevContext->UseDevCamera)
        OtherCamera = &DevContext->Cameras[!Game->CurrentWorldIndex];
    DevelopmentRenderWorld(DevContext, Game, Graphics, OtherWorld, OtherCamera);
    
    /////////////
    
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
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = CreateArena(KILOBYTE(32));
        
        Platform_InitImGui(DevContext->PlatformData);                
        AllocateImGuiFont(Graphics);                
        DevContext->CapsuleMesh = CreateDevCapsuleMesh(Graphics, 60);
        
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
