/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

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

dev_capsule_mesh CreateDevCapsuleMesh(arena* Storage, graphics* Graphics, u16 CircleSampleCount)
{
    dev_capsule_mesh Result = {};
    
    graphics_vertex_buffer VertexBuffer = {};
    graphics_index_buffer IndexBuffer = {};
    
    VertexBuffer.Format = GRAPHICS_VERTEX_FORMAT_P3;
    IndexBuffer.Format = GRAPHICS_INDEX_FORMAT_16_BIT;
    
    
    u16 HalfCircleSampleCountPlusOne = (CircleSampleCount/2)+1;
    f32 CircleSampleIncrement = (2.0f*PI)/(f32)CircleSampleCount;            
    
    u32 CapVertexCount = CircleSampleCount+(HalfCircleSampleCountPlusOne*2);
    Result.BodyVertexOffset = CapVertexCount;    
    Result.CapIndexCount = CapVertexCount*2;    
    Result.BodyIndexCount = 8;    
    
    VertexBuffer.VertexCount = CapVertexCount+8;
    IndexBuffer.IndexCount = Result.CapIndexCount+Result.BodyIndexCount;
    
    VertexBuffer.Data = PushArray(Storage, VertexBuffer.VertexCount, graphics_vertex_p3, Clear, 0);
    IndexBuffer.Data = PushArray(Storage, IndexBuffer.IndexCount, u16, Clear, 0); 
    
    graphics_vertex_p3* VertexAt = (graphics_vertex_p3*)VertexBuffer.Data;
    
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
    
    u16* IndicesAt = (u16*)IndexBuffer.Data;
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
    
    Result.Mesh = Graphics->AllocateMesh(Graphics, VertexBuffer, IndexBuffer);
    
    return Result;
}

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
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(Graphics->RenderDim/5));    
    
    local bool open = true;
    ImGui::Begin("Developer Tools", &open, ImGuiWindowFlags_NoCollapse);    
    
    ImGui::Text("FPS: %f", 1.0f/Game->dt);        
    
    if(ImGui::Checkbox("Debug Camera", (bool*)&DevContext->UseDevCamera))
    {
        DevContext->Camera.Position = Game->Camera.Position;
        DevContext->Camera.AngularVelocity = {};
        DevContext->Camera.Orientation = IdentityM3();
        DevContext->Camera.FocalPoint = Game->Camera.FocalPoint;
        DevContext->Camera.Distance = Magnitude(DevContext->Camera.FocalPoint-DevContext->Camera.Position);
    }    
    
    ImGui::Checkbox("Draw Colliders", (bool*)&DevContext->DrawColliders);
    ImGui::Checkbox("Draw Blockers", (bool*)&DevContext->DrawBlockers);
    
    ImGui::End();    
    
    ImGui::Render();        
}

void DevelopmentRender(dev_context* DevContext, game* Game, graphics* Graphics, camera* Camera)
{    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGui(DevContext, Game, Graphics);
    
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);        
    
    RecordGameCommands(Game, Graphics, Perspective, CameraView);
    
    PushCull(Graphics, false);
    
    world* World = GetCurrentWorld(Game);
    world_entity* PlayerEntity = GetPlayerEntity(World);
    
    v3f PlayerZ = V3(0.0f, 0.0f, 1.0f);
    v3f PlayerY = V3(0.0f, 1.0f, 0.0f);
    v3f PlayerX = Cross(PlayerY, PlayerZ);
    
    v3f BodyZ = PlayerZ*Game->PlayerHeight;
    
    v3f XAxis = PlayerX*Game->PlayerRadius;
    v3f YAxis = PlayerY*Game->PlayerRadius;
    v3f ZAxis = PlayerZ*Game->PlayerRadius;    
    
    v3f BottomPosition = PlayerEntity->Position+ZAxis;
    m4 Model = TransformM4(BottomPosition, M3(XAxis, YAxis, -ZAxis));
    
    dev_capsule_mesh* CapsuleMesh = &DevContext->CapsuleMesh;
    PushDrawLineMesh(Graphics, CapsuleMesh->Mesh, Model, Blue(), CapsuleMesh->CapIndexCount, 0, 0);
    
    Model = TransformM4(BottomPosition + BodyZ, M3(XAxis, YAxis, ZAxis));
    PushDrawLineMesh(Graphics, CapsuleMesh->Mesh, Model, Blue(), CapsuleMesh->CapIndexCount, 0, 0);
    
    Model = TransformM4(BottomPosition + (BodyZ*0.5f), M3(XAxis, YAxis, BodyZ));
    PushDrawLineMesh(Graphics, CapsuleMesh->Mesh, Model, Blue(), CapsuleMesh->BodyIndexCount, CapsuleMesh->CapIndexCount, CapsuleMesh->BodyVertexOffset);
    
    if(DevContext->DrawColliders)
    {
        
        for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
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
    
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    m4 Orthographic = OrthographicM4(0, (f32)Graphics->RenderDim.width, 0, (f32)Graphics->RenderDim.height, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ptr IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = ImGui::GetDrawData();        
    
    u32 LastImGuiMeshCount = DevContext->ImGuiMeshCount;
    DevContext->ImGuiMeshCount = DrawData->CmdListsCount;
    ASSERT(DevContext->ImGuiMeshCount <= MAX_IMGUI_MESHES);        
    
    for(i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(!DevContext->ImGuiMeshes[CmdListIndex])
            DevContext->ImGuiMeshes[CmdListIndex] = Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT);
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(DevContext->ImGuiMeshes[CmdListIndex], CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(graphics_vertex_p2_uv_c), 
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
                
                graphics_texture* Texture = (graphics_texture*)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, DevContext->ImGuiMeshes[CmdListIndex], Texture, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
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
        DevContext->CapsuleMesh = CreateDevCapsuleMesh(&DevContext->DevStorage, Graphics, 60);
        
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
        
        camera* Camera = &Game->Camera;
        if(DevContext->UseDevCamera)
        {
            Camera = &DevContext->Camera;
            
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
        
        DevelopmentRender(DevContext, Game, Graphics, Camera);        
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}
