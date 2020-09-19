#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

using namespace ImGui;

void DragFloat(ak_u32 ID, const ak_char* Label, ak_f32* Value, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    PushID(ID);
    DragFloat(Label, Value, Speed, Min, Max);
    PopID();
}

void DragAngle(ak_u32 ID, const ak_char* Label, ak_f32* Radians, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ak_f32 Degree = AK_ToDegree(*Radians);
    PushID(ID);
    DragFloat(Label, &Degree, Speed, Min, Max, Format);
    PopID();
    *Radians = AK_ToRadians(Degree);
}

ak_char* EntityTypeUI(entity_type Type)
{
#define ENUM_TYPE(type) case type: return #type
    switch(Type)
    {
        ENUM_TYPE(ENTITY_TYPE_STATIC);
        ENUM_TYPE(ENTITY_TYPE_PLAYER);
        ENUM_TYPE(ENTITY_TYPE_RIGID_BODY);
        ENUM_TYPE(ENTITY_TYPE_PUSHABLE);
        
        AK_INVALID_DEFAULT_CASE;
    }   
#undef ENUM_TYPE
    
    return NULL;
}

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
    
    ImGuiIO* IO = &GetIO();
    IO->BackendRendererName = "OpenGL";
    IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

//Rotates the current SelectedObject, by difference between OldRotation and NewRotation, then updates the OldRotation
void DevelopmentUpdateSelectedObjectRotation(ak_sqtf* Transform, ak_v3f* OldRotation, ak_v3f NewRotation)
{
    *OldRotation = NewRotation;
    Transform->Orientation = AK_EulerToQuat(NewRotation.roll, NewRotation.pitch, NewRotation.yaw);
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
        if(Button(RecordButtonText))
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
        if(Button("Load Recording"))
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
        
        if(Button(PlayButtonText))
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
        SameLine();
        Text("Recording File: %s\n", FramePlayback->RecordingPath);
    }
    
    if((State == FRAME_PLAYBACK_STATE_PLAYING) || (State == FRAME_PLAYBACK_STATE_INSPECT_FRAMES))
    {
        ak_char* InspectText = "Start Inspecting";        
        ak_bool Inspecting = (State == FRAME_PLAYBACK_STATE_INSPECT_FRAMES);
        if(Inspecting)
            InspectText = "Stop Inspecting";
        
        if(Button(InspectText))
        {
            if(Inspecting)            
                State = FRAME_PLAYBACK_STATE_PLAYING;            
            else            
                State = FRAME_PLAYBACK_STATE_INSPECT_FRAMES;            
        }
        
        SameLine();
        Text("Frame %d/%d", FramePlayback->CurrentFrameIndex, Recording->FrameCount-1);                 
        
        if(Inspecting)
        {            
            ImGuiIO* IO = &GetIO();
            
            if(IsKeyPressedMap(ImGuiKey_LeftArrow))       
            {
                if(FramePlayback->CurrentFrameIndex == 0)
                    FramePlayback->CurrentFrameIndex = Recording->FrameCount-1;
                else
                    FramePlayback->CurrentFrameIndex--;
            }
            
            if(IsKeyPressedMap(ImGuiKey_RightArrow))            
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
#if 1 
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
   
    Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);    
    Checkbox("Draw colliders", (bool*)&DevContext->DrawColliders);    
    Checkbox("Draw Grid", (bool*)&DevContext->DrawGrid);  
    Checkbox("Edit Mode", (bool*)&DevContext->EditMode);  
    
    local ak_bool Open = true;
    if(CollapsingHeader("Game Information"))
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
    
    if(CollapsingHeader("Entity Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {        
        if(TreeNode("Entity Spawner"))
        {
            
            TreePop();
        }                
                
        Text("Entity Information");
        if(DevContext->SelectedObjectID.IsValid())
        {   
            entity* Entity = GetEntity(Game, DevContext->SelectedObjectID);            
            ak_sqtf* Transform = GetEntityTransform(Game, DevContext->SelectedObjectID);
            
            ak_array<ak_v3f>* EntityRotations = &DevContext->EntityRotations[DevContext->SelectedObjectID.WorldIndex];            
            ak_u32 Index = AK_PoolIndex(DevContext->SelectedObjectID.ID);
            if(Index > EntityRotations->Size)
                EntityRotations->Resize(Index);                        
            
            ak_v3f Translation = Transform->Translation;            
            ak_v3f Scale = Transform->Scale;
            ak_v3f Rotation = EntityRotations->Entries[Index];            
            
            ak_f32 TransformItemWidth = 80;
            
            {
                ak_u32 Hash = AK_HashFunction("Translation");
                Text("Translation");                
                PushItemWidth(TransformItemWidth);
                
                DragFloat(Hash+0, "X", &Translation.x, 0.1f, -1000.0f, 1000.0f); SameLine();                                
                DragFloat(Hash+1, "Y", &Translation.y, 0.1f, -1000.0f, 1000.0f); SameLine();                                
                DragFloat(Hash+2, "Z", &Translation.z, 0.1f, -1000.0f, 1000.0f);                            
                
                PopItemWidth();
            }
            
            {
                ak_u32 Hash = AK_HashFunction("Scale");
                Text("Scale");
                PushItemWidth(TransformItemWidth);
                
                DragFloat(Hash+0, "X", &Scale.x, 0.1f, 0.0f, 100.0f); SameLine();                                
                DragFloat(Hash+1, "Y", &Scale.y, 0.1f, 0.0f, 100.0f); SameLine();                                
                DragFloat(Hash+2, "Z", &Scale.z, 0.1f, 0.0f, 100.0f);                 
                
                PopItemWidth();
            }
            
            {
                ak_u32 Hash = AK_HashFunction("Rotation");
                Text("Rotation");
                PushItemWidth(TransformItemWidth);
                
                DragAngle(Hash+0, "X", &Rotation.x, 0.1f, -180.0f, 180.0f); SameLine();
                DragAngle(Hash+1, "Y", &Rotation.y, 0.1f, -180.0f, 180.0f); SameLine();
                DragAngle(Hash+2, "Z", &Rotation.z, 0.1f, -180.0f, 180.0f); 
                
                PopItemWidth();
            }
            
            simulation* Simulation = GetSimulation(Game, Entity->ID);                
            sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
            
            ak_v3f ObjectVelocity = {};
            rigid_body* RigidBody = SimEntity->ToRigidBody();
            if(RigidBody)
                ObjectVelocity = RigidBody->Velocity;            
            
            Text("Velocity: (%.2f, %.2f, %.2f)", ObjectVelocity.x, ObjectVelocity.y, ObjectVelocity.z);
            Text("Type: %s", EntityTypeUI(Entity->Type));                        
            
            if(DevContext->EditMode)
            {                                
                Transform->Translation = Translation;
                Transform->Scale = Scale;
                DevelopmentUpdateSelectedObjectRotation(Transform, EntityRotations->Get(Index), Rotation);
                SimEntity->Transform = *Transform;
            }
            
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
