#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

using namespace ImGui;

graphics_texture_id AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &GetIO();
    void* ImGuiFontData;
    
    ak_i32 Width, Height;        
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &Width, &Height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture_id FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, (ak_u32)Width, (ak_u32)Height, GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void DevelopmentImGuiInit(dev_context* DevContext)
{    
    Platform_InitImGui(DevContext->PlatformData[0]);                
    AllocateImGuiFont(DevContext->Graphics);                
    
    ImGuiIO* IO = &ImGui::GetIO();
    IO->BackendRendererName = "OpenGL";
    IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

//Rotates the current SelectedObject, by difference between OldRotation and NewRotation, then updates the OldRotation
void DevelopmentUpdateSelectedObjectRotation(ak_sqtf* Transform, ak_v3f* OldRotation, ak_v3f NewRotation)
{
    *OldRotation = NewRotation;
    Transform->Orientation = AK_EulerToQuat(NewRotation.r, NewRotation.p, NewRotation.y);
}

void DevelopmentFramePlayback(dev_context* DevContext, frame_playback* FramePlayback)
{
    ak_char* RecordButtonText = NULL;
    
    frame_recording* Recording = &FramePlayback->Recording;
    
    frame_playback_state State = FramePlayback->PlaybackState;
    
    if(State == FRAME_PLAYBACK_STATE_NONE)
        RecordButtonText = "Start Recording";
    
    if(State == FRAME_PLAYBACK_STATE_RECORDING)
        RecordButtonText = "Stop Recording";
    
    if(RecordButtonText)
    {
        if(ImGui::Button(RecordButtonText))
        {
            if(State == FRAME_PLAYBACK_STATE_NONE)
            {
                ak_string RecordingPath = Platform_FindNewFrameRecordingPath();
                if(!AK_StringIsNullOrEmpty(RecordingPath))
                {
                    AK_Assert(RecordingPath.Length < FramePlayback->MaxRecordingPathLength, "File path to large for recording path");
                    
                    CopyMemory(FramePlayback->RecordingPath, RecordingPath.Data, RecordingPath.Length);
                    FramePlayback->RecordingPath[RecordingPath.Length] = 0;
                    
                    Recording->StartRecording();
                    
                    State = FRAME_PLAYBACK_STATE_RECORDING;
                }
            }
            else if(State == FRAME_PLAYBACK_STATE_RECORDING)
            {
                Recording->EndRecording(FramePlayback->RecordingPath);
                State = FRAME_PLAYBACK_STATE_NONE;
            }
            AK_INVALID_ELSE;
        }        
    }
    
    if(State == FRAME_PLAYBACK_STATE_NONE)
    {
        if(ImGui::Button("Load Recording"))
        {
            ak_string RecordingPath = AK_OpenFileDialog("recording", AK_GetGlobalArena());
            if(!AK_StringIsNullOrEmpty(RecordingPath))
            {
                AK_MemoryCopy(FramePlayback->RecordingPath, RecordingPath.Data, RecordingPath.Length);
                FramePlayback->RecordingPath[RecordingPath.Length] = 0;                
            }
        }        
    }         
    
    if((FramePlayback->RecordingPath[0] != 0) && (State != FRAME_PLAYBACK_STATE_RECORDING))
    {
        ak_char* PlayButtonText = "Start Playing";
        
        ak_bool StopPlaying = (State == FRAME_PLAYBACK_STATE_PLAYING) || (State == FRAME_PLAYBACK_STATE_INSPECT_FRAMES);
        if(StopPlaying)
            PlayButtonText = "Stop Playing";
        
        if(ImGui::Button(PlayButtonText))
        {
            if(StopPlaying)
            {                
                FramePlayback->CurrentFrameIndex = 0;
                Recording->PlayFrame(DevContext->Game, 0);
                Recording->EndPlaying();
                State = FRAME_PLAYBACK_STATE_NONE;                
            }
            else
            {
                Recording->StartPlaying(FramePlayback->RecordingPath);
                State = FRAME_PLAYBACK_STATE_PLAYING;
            }
        }               
    }
    
    if(FramePlayback->RecordingPath[0] != 0)
    {
        ImGui::SameLine();
        ImGui::Text("Recording File: %s\n", FramePlayback->RecordingPath);
    }
    
    if((State == FRAME_PLAYBACK_STATE_PLAYING) || (State == FRAME_PLAYBACK_STATE_INSPECT_FRAMES))
    {
        ak_char* InspectText = "Start Inspecting";        
        ak_bool Inspecting = (State == FRAME_PLAYBACK_STATE_INSPECT_FRAMES);
        if(Inspecting)
            InspectText = "Stop Inspecting";
        
        if(ImGui::Button(InspectText))
        {
            if(Inspecting)            
                State = FRAME_PLAYBACK_STATE_PLAYING;            
            else            
                State = FRAME_PLAYBACK_STATE_INSPECT_FRAMES;            
        }
        
        ImGui::SameLine();
        ImGui::Text("Frame %d/%d", FramePlayback->CurrentFrameIndex, Recording->FrameCount-1);                 
        
        if(Inspecting)
        {            
            ImGuiIO* IO = &ImGui::GetIO();
            
            if(ImGui::IsKeyPressedMap(ImGuiKey_LeftArrow))       
            {
                if(FramePlayback->CurrentFrameIndex == 0)
                    FramePlayback->CurrentFrameIndex = Recording->FrameCount-1;
                else
                    FramePlayback->CurrentFrameIndex--;
            }
            
            if(ImGui::IsKeyPressedMap(ImGuiKey_RightArrow))            
            {
                if(FramePlayback->CurrentFrameIndex == (Recording->FrameCount-1))
                    FramePlayback->CurrentFrameIndex = 0;
                else
                    FramePlayback->CurrentFrameIndex++;                                                
            }                        
        }       
    }
    
    FramePlayback->PlaybackState = State;
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
    SetNextWindowSize(ImVec2((ak_f32)Graphics->RenderDim.x/3.0f, (ak_f32)Graphics->RenderDim.y));    
    
    local bool open = true;
    Begin("Developer Tools", &open, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize);    
    
    Text("FPS: %f", 1.0f/Game->dt);        
    
    if(Checkbox("Debug Camera", (bool*)&DevContext->UseDevCamera))
    {
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {           
            
            game_camera* Camera = &Game->CurrentCameras[WorldIndex];
            camera* DevCamera = &DevContext->Cameras[WorldIndex];
            
            camera_transform CameraTransform = GetCameraTransform(Camera);  
            
            DevCamera->Position = CameraTransform.Translation;
            DevCamera->AngularVelocity = {};
            DevCamera->Orientation = CameraTransform.Orientation;
            DevCamera->FocalPoint = Camera->Target;
            DevCamera->Distance = Camera->SphericalCoordinates.x;
        }
    }    
    
    if(!DevContext->EditMode)
    {        
        DevelopmentFramePlayback(DevContext, &DevContext->FramePlayback);    
    }
    
    //const char* ShadingTypes[] = {"Normal Shading", "Wireframe Shading", "Wireframe on Normal Shading"};
    
    const char* ViewModeTypes[] = {"Lit", "Unlit", "Wireframe", "Wireframe on Lit"};
    AlignTextToFramePadding();
    Text("View Modes");
    SameLine();
    Combo("", (int*)&DevContext->ViewModeType, ViewModeTypes, AK_Count(ViewModeTypes));

    const char* TransformTypes[] = {"Translate", "Scale", "Rotate"};
    AlignTextToFramePadding();
    Text("Object Transform Mode");
    SameLine();
    Combo("TransformMode", (int*)&DevContext->TransformationMode, TransformTypes, AK_Count(TransformTypes));
   
    ImGui::Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    ImGui::Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);    
    ImGui::Checkbox("Draw colliders", (bool*)&DevContext->DrawColliders);    
    ImGui::Checkbox("Draw Grid", (bool*)&DevContext->DrawGrid);  
    ImGui::Checkbox("Edit Mode", (bool*)&DevContext->EditMode);  
    
    local ak_bool Open = true;
    if(CollapsingHeader("Game Information", ImGuiTreeNodeFlags_DefaultOpen))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        
        Text("Movement Time Max Iterations: %I64u", GameInformation->MaxTimeIterations);
        Text("GJK Max Iterations: %I64u", GameInformation->MaxGJKIterations);
        
        if(!DevContext->UseDevCamera)
        {            
            for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
            {                                
                game_camera* Camera = &Game->CurrentCameras[WorldIndex];
                
                NewLine();
                
                Text("Camera %d Settings:", WorldIndex);                                
                
                PushID(WorldIndex*6+0);
                DragFloat("Radius", &Camera->SphericalCoordinates.radius, 0.01f, 0.001f, 50.0f, "%.3f");
                PopID();                                
                
                PushID(WorldIndex*6+1);
                ak_f32 AzimuthDegree = AK_ToDegree(Camera->SphericalCoordinates.azimuth);
                DragFloat("Azimuth", &AzimuthDegree, 1.0f, -180.0f, 180.0f, "%.1f");
                Camera->SphericalCoordinates.azimuth = AK_ToRadians(AzimuthDegree);
                PopID();
                
                PushID(WorldIndex*6+2);
                ak_f32 InclinationDegree = AK_ToDegree(Camera->SphericalCoordinates.inclination);
                DragFloat("Inclination", &InclinationDegree, 1.0f, -180.0f, 180.0f, "%.1f");
                Camera->SphericalCoordinates.inclination = AK_ToRadians(InclinationDegree);
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
            entity_id EntityID = DevContext->SelectedObject->ID;
            entity* Entity = GetEntity(Game, EntityID);
            
            simulation* Simulation = GetSimulation(Game, EntityID);
            
            sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
            
            ak_sqtf* Transform = GetEntityTransform(Game, EntityID);
            
            ak_array<ak_v3f>* EntityRotations = &DevContext->EntityRotations[EntityID.WorldIndex];            
            ak_u32 Index = AK_PoolIndex(EntityID.ID);
            if(Index > EntityRotations->Size)
                EntityRotations->Resize(Index);            
            
            ak_v3f* Rotation = EntityRotations->Get(Index);            
            if(DevContext->EditMode)
            {
                DragFloat("X Translation", &Transform->Translation.x, 0.1f, -1000.0f, 1000.0f);
                DragFloat("Y Translation", &Transform->Translation.y, 0.1f, -1000.0f, 1000.0f);
                DragFloat("Z Translation", &Transform->Translation.z, 0.1f, -1000.0f, 1000.0f);
                
                DragFloat("X Scale", &Transform->Scale.x, 0.1f, 0.0f, 100.0f);
                DragFloat("Y Scale", &Transform->Scale.y, 0.1f, 0.0f, 100.0f);
                DragFloat("Z Scale", &Transform->Scale.z, 0.1f, 0.0f, 100.0f);
                
                ak_f32 ObjectRoll = AK_ToDegree(Rotation->r);
                ak_f32 ObjectPitch = AK_ToDegree(Rotation->p);
                ak_f32 ObjectYaw = AK_ToDegree(Rotation->y);
                DragFloat("Roll", &ObjectRoll, 0.1f, -180.0f, 180.0f, "%.3f");
                DragFloat("Pitch", &ObjectPitch, 0.1f, -180.0f, 180.0f, "%.3f");
                DragFloat("Yaw", &ObjectYaw, 0.1f, -180.0f, 180.0f, "%.3f");
                
                
                DevelopmentUpdateSelectedObjectRotation(Transform, Rotation, AK_V3(AK_ToRadians(ObjectRoll), AK_ToRadians(ObjectPitch), AK_ToRadians(ObjectYaw)));
            }
            else
            {
                ak_v3f Translation = Transform->Translation;
                DragFloat("X Translation", &Translation.x, 0.0f, Translation.x, Translation.x);
                DragFloat("Y Translation", &Translation.y, 0.0f, Translation.y, Translation.y);
                DragFloat("Z Translation", &Translation.z, 0.0f, Translation.z, Translation.z);
                
                ak_v3f Scale = Transform->Scale;
                DragFloat("X Scale", &Scale.x, 0.0f, Scale.x, Scale.x);
                DragFloat("Y Scale", &Scale.y, 0.0f, Scale.y, Scale.y);
                DragFloat("Z Scale", &Scale.z, 0.0f, Scale.z, Scale.z);
                                
                ak_f32 ObjectRoll = AK_ToDegree(Rotation->r);
                ak_f32 ObjectPitch = AK_ToDegree(Rotation->p);
                ak_f32 ObjectYaw = AK_ToDegree(Rotation->y);
                DragFloat("Roll", &ObjectRoll, 0.0f, ObjectRoll, ObjectRoll, "%.3f");
                DragFloat("Pitch", &ObjectPitch, 0.0f, ObjectPitch, ObjectPitch, "%.3f");
                DragFloat("Yaw", &ObjectYaw, 0.0f, ObjectYaw, ObjectYaw, "%.3f");
            }
            
            
            ak_v3f ObjectVelocity = {};
            rigid_body* RigidBody = SimEntity->ToRigidBody();
            if(RigidBody)
                ObjectVelocity = RigidBody->Velocity;            
            
            ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", ObjectVelocity.x, ObjectVelocity.y, ObjectVelocity.z);
            ImGui::Text("Type: (%d)", DevContext->SelectedObject->Type);
            ImGui::Text("ID: (%d)", DevContext->SelectedObject->ID.ID);
            ImGui::Text("WorldIndex: (%d)", DevContext->SelectedObject->ID.WorldIndex);
            if(DevContext->SelectedObject->LinkID.IsValid())
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
            
            SimEntity->Transform = *Transform;
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
            DevContext->LogStorage->Clear();
        }        
        
        SameLine();
        
        if(Button("Copy")) LogToClipboard();       
        
        for(ak_u32 LogIndex = 0; LogIndex < DevContext->Logs.Size; LogIndex++)
        {
            ak_string Log = DevContext->Logs[LogIndex];
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
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.w, Graphics->RenderDim.h);
    
    ak_m4f Orthographic = AK_Orthographic(0.0f, (ak_f32)Graphics->RenderDim.w, 0.0f, (ak_f32)Graphics->RenderDim.h, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ak_u32 IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = GetDrawData();        
    
    ak_u32 LastImGuiMeshCount = DevContext->ImGuiMeshCount;
    DevContext->ImGuiMeshCount = DrawData->CmdListsCount;
    AK_Assert(DevContext->ImGuiMeshCount <= MAX_IMGUI_MESHES, "ImGUI Mesh overflow");        
    
    for(ak_i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(!DevContext->ImGuiMeshes[CmdListIndex])
            DevContext->ImGuiMeshes[CmdListIndex] = Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT);
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(Graphics, DevContext->ImGuiMeshes[CmdListIndex], 
                                 CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(ak_vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);                
        
        for(ak_i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            AK_Assert(!Cmd->UserCallback, "ImGui User callback is not supported");
            
            ImVec4 ClipRect = Cmd->ClipRect;
            if((ClipRect.x < Graphics->RenderDim.w) && (ClipRect.y < Graphics->RenderDim.h) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                ak_i32 X = (ak_i32)ClipRect.x;
                ak_i32 Y = (ak_i32)(Graphics->RenderDim.h-ClipRect.w);
                ak_i32 Width = (ak_i32)(ClipRect.z - ClipRect.x);
                ak_i32 Height = (ak_i32)(ClipRect.w - ClipRect.y);
                
                PushScissor(Graphics, X, Y, Width, Height);
                
                graphics_texture_id TextureID = (graphics_texture_id)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, DevContext->ImGuiMeshes[CmdListIndex], TextureID, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
            }
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushDepth(Graphics, true);            
}
