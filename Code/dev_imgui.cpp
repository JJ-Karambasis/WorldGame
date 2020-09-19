#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#define TRANSFORM_ITEM_WIDTH 80

using namespace ImGui;

void DragInt(ak_u32 ID, const ak_char* Label, ak_i32* Value, ak_f32 Speed, ak_i32 Min, ak_i32 Max)
{
    PushID(ID);    
    DragInt(Label, Value, Speed, Min, Max);    
    PopID();
}

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

ak_bool Combo(ak_u32 ID, const ak_char* Label, ak_i32* Data, const ak_char** List, ak_i32 ListCount)
{
    PushID(ID);
    ak_bool Result = Combo(Label, Data, List, ListCount);
    PopID();
    return Result;
}


ak_bool ColorEdit3(ak_u32 ID, const ak_char* Label, ak_f32* Data, ImGuiColorEditFlags Flags)
{
    PushID(ID);
    ak_bool Result = ColorEdit3(Label, Data, Flags);
    PopID();
    return Result;
}

ak_bool Checkbox(ak_u32 ID, const ak_char* Label, ak_bool* Flag)
{
    PushID(ID);
    ak_bool Result = Checkbox(Label, (bool*)Flag);
    PopID();
    return Result;
}

ak_bool Button(ak_u32 ID, const ak_char* ButtonText)
{
    PushID(ID);
    ak_bool Result = Button(ButtonText);
    PopID();
    return Result;
}

const ak_char* EntityTypeUI(entity_type Type)
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

void ClearSpawner(entity_spawner* Spawner)
{
    Spawner->EntityType = ENTITY_TYPE_STATIC;                
    Spawner->Translation = {};
    Spawner->Scale = AK_V3(1.0f, 1.0f, 1.0f);
    Spawner->Radius = 1.0f;
    Spawner->Restitution = 0.0f;
    Spawner->Axis = {};
    Spawner->Angle = 0;
    Spawner->WorldIndex = 0;
    Spawner->MeshID = (mesh_asset_id)0;
    Spawner->Material = {};
    Spawner->Mass = 1.0f;
}

ak_fixed_array<const ak_char*> GetAllEntityTypesNotPlayerUI()
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_u32 Size = ENTITY_TYPE_COUNT-1;
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(Size), Size);    
    ak_u32 Counter = 0;
    
    for(ak_u32 TypeIndex = 0; TypeIndex < ENTITY_TYPE_COUNT; TypeIndex++)
    {
        if((entity_type)TypeIndex != ENTITY_TYPE_PLAYER)
            Result[Counter++] = EntityTypeUI((entity_type)TypeIndex);
    }
        
    return Result;
}

ak_fixed_array<const ak_char*> GetAllMeshInfoNamesUI(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(MESH_ASSET_COUNT), MESH_ASSET_COUNT);
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Result[MeshIndex] = Assets->MeshInfos[MeshIndex].Name;          
    return Result;
}

ak_fixed_array<const ak_char*> GetAllTextureInfoNamesUI(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(TEXTURE_ASSET_COUNT), TEXTURE_ASSET_COUNT);
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)    
        Result[TextureIndex] = Assets->TextureInfos[TextureIndex].Name;          
    return Result;
}

void WorldIndexImGui(ak_u32* WorldIndex, const ak_char** WorldIndexList, ak_u32 WorldIndexCount)
{
    AlignTextToFramePadding();
    Text("World Index");
    SameLine();
    Combo(AK_HashFunction("World Index"), "", (int*)WorldIndex, WorldIndexList, WorldIndexCount);    
}

void MeshImGui(assets* Assets, mesh_asset_id* MeshID)
{
    ak_fixed_array<const ak_char*> MeshNames = GetAllMeshInfoNamesUI(Assets);
    AlignTextToFramePadding();
    Text("Mesh");
    SameLine();
    Combo(AK_HashFunction("Mesh"), "", (int*)MeshID, MeshNames.Data, MeshNames.Size);
}

void MassImGui(ak_f32* Mass)
{
    AlignTextToFramePadding();
    Text("Mass");                    
    SameLine();
    DragFloat(AK_HashFunction("Mass"), "", Mass, 0.01f, 1.0f, 10000.0f);
}

void MaterialImGui(assets* Assets, material* Material)
{    
    ak_fixed_array<const ak_char*> TextureNames = GetAllTextureInfoNamesUI(Assets);
    
    Text("Material");    
    {
        material_diffuse* Diffuse = &Material->Diffuse;
        
        AlignTextToFramePadding();
        Text("Diffuse: "); SameLine(); 
        AlignTextToFramePadding();
        Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Diffuse Is Texture"), "", &Diffuse->IsTexture); SameLine();
        if(Diffuse->IsTexture)
        {
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Diffuse Texture"), "", (int*)&Diffuse->DiffuseID, TextureNames.Data, TextureNames.Size);
        }
        else
        {
            AlignTextToFramePadding();
            Text("Color");
            SameLine();
            ColorEdit3(AK_HashFunction("Diffuse Color"), "", (ak_f32*)&Diffuse->Diffuse, ImGuiColorEditFlags_RGB);
        }
    }
    
    {
        material_specular* Specular = &Material->Specular;
        
        AlignTextToFramePadding();
        Text("Specular: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Specular In Use"), "", &Specular->InUse); 
        
        if(Specular->InUse)
        {
            SameLine();
            AlignTextToFramePadding();
            Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Specular Is Texture"), "", &Specular->IsTexture); SameLine();
            if(Specular->IsTexture)
            {
                AlignTextToFramePadding();
                Text("Texture");
                SameLine();
                Combo(AK_HashFunction("Specular Texture"), "", (int*)&Specular->SpecularID, TextureNames.Data, TextureNames.Size);
            }
            else
            {
                AlignTextToFramePadding();
                Text("Value");
                SameLine();
                PushItemWidth(60);
                DragFloat(AK_HashFunction("Specular Color"), "", &Specular->Specular, 0.01f, 0.0f, 1.0f); SameLine();                                
                PopItemWidth();
                
                ak_v3f SpecularDisplay = AK_V3(Specular->Specular, Specular->Specular, Specular->Specular);
                ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
            }
            
            SameLine();
            Text("Shininess");
            PushItemWidth(40);
            SameLine();
            DragInt(AK_HashFunction("Specular Shininess"), "", &Specular->Shininess, 0.1f, 1, 512);
            PopItemWidth();
        }                                                
    }                    
    
    {
        material_normal* Normal = &Material->Normal;
        
        AlignTextToFramePadding();
        Text("Normal: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Normal In Use"), "", &Normal->InUse);
        
        if(Normal->InUse)
        {
            SameLine();
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Normal Texture"), "", (int*)&Normal->NormalID, TextureNames.Data, TextureNames.Size);
        }
    }    
}

void TranslationImGui(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    Text("Translation");
    PushItemWidth(ItemWidth);
        
    DragFloat(Hash+0, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);                                
    
    PopItemWidth();
}

void ScaleImGui(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    Text("Scale");
    PushItemWidth(ItemWidth);
        
    DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Scale->z, 0.1f, 0.0f, 100.0f);                 
    
    PopItemWidth();
}

void RadiusImGui(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Radius)
{
    Text("Radius");
    SameLine();
    PushItemWidth(ItemWidth);
    DragFloat(Hash, "", Radius, 0.01f, 0.0f, 100.0f);
    PopItemWidth();
}

void RestitutionImGui(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Restitution)
{
    Text("Restitution");
    SameLine();
    PushItemWidth(ItemWidth);
    DragFloat(Hash, "", Restitution, 0.001f, 0.0f, 1.0f);
    PopItemWidth();
}

ak_bool ValidateMaterial(material* Material)
{
    if(Material->Diffuse.IsTexture && (Material->Diffuse.DiffuseID >= MESH_ASSET_COUNT))
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid diffuse texture. Please select a valid one");
        return false;
    }
    else if(Material->Specular.InUse && Material->Specular.IsTexture && (Material->Specular.SpecularID >= TEXTURE_ASSET_COUNT))
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid specular texture. Please select a valid one");
        return false;
    }
    else if(Material->Normal.InUse && Material->Normal.NormalID >= TEXTURE_ASSET_COUNT)
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid normal texture. Please select a valid one");
        return false;
    }
    
    return true;
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

void DevelopmentEntitySpawner(dev_context* DevContext)
{
    if(TreeNode("Entity Spawner"))
    {                        
        entity_spawner* Spawner = &DevContext->EntitySpawner;
        if(!Spawner->Init)
        {
            Spawner->Init = true;
            ClearSpawner(Spawner);                
        }
        
        ak_fixed_array<const ak_char*> EntityTypes = GetAllEntityTypesNotPlayerUI();
        AlignTextToFramePadding();
        Text("Entity Type");            
        SameLine();            
        
        entity_type PrevType = Spawner->EntityType;
        
        ak_i32 SpawnType = Spawner->EntityType-1;            
        Combo(AK_HashFunction("Entity Type"), "", (int*)&SpawnType, EntityTypes.Data, EntityTypes.Size);            
        entity_type Type = (entity_type)(SpawnType+1);
        
        if(PrevType != Type)
        {
            ClearSpawner(Spawner);
            Spawner->EntityType = Type;
        }
        
        Separator();                        
        TranslationImGui(AK_HashFunction("Translation Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Translation);                            
        
        switch(Spawner->EntityType)
        {
            case ENTITY_TYPE_STATIC:
            {                    
                Separator();                    
                ScaleImGui(AK_HashFunction("Scale Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Scale);                    
                Separator();
                
                {
                    ak_u32 Hash = AK_HashFunction("Rotation Spawner");
                    Text("Rotation");
                    
                    PushItemWidth(TRANSFORM_ITEM_WIDTH);
                    
                    DragFloat(Hash+0, "Axis X", &Spawner->Axis.x, 0.01f, -1, 1); SameLine();
                    DragFloat(Hash+1, "Axis Y", &Spawner->Axis.y, 0.01f, -1, 1); SameLine();
                    DragFloat(Hash+2, "Axis Z", &Spawner->Axis.z, 0.01f, -1, 1); SameLine();
                    DragAngle(Hash+3, "Angle", &Spawner->Angle,   0.1f, -180.0f, 180.0f);
                    
                    Spawner->Axis = AK_Normalize(Spawner->Axis);                                                                        
                    
                    PopItemWidth();
                }
                
                Separator();                    
                const ak_char* WorldIndexList[] = {"World A", "World B", "Both"}; 
                WorldIndexImGui(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
                Separator();                    
                MeshImGui(DevContext->Game->Assets, &Spawner->MeshID);                    
                Separator();                    
                MaterialImGui(DevContext->Game->Assets, &Spawner->Material);
                Separator();
                
                if(Button(AK_HashFunction("Create Entity Button"), "Create"))
                {
                    if(ValidateMaterial(&Spawner->Material))                        
                    {
                        if(Spawner->WorldIndex == 2)
                        {
                            dual_entity_id IDs = CreateDualStaticEntity(DevContext->Game, Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle),
                                                                        Spawner->MeshID, Spawner->Material);
                            
                            if(DevContext->Game->CurrentWorldIndex == IDs.EntityA.WorldIndex)
                                DevContext->SelectedObjectID = IDs.EntityA;
                            else
                                DevContext->SelectedObjectID = IDs.EntityB;
                        }
                        else
                        {                            
                            entity_id ID = CreateStaticEntity(DevContext->Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle), 
                                                              Spawner->MeshID, Spawner->Material);
                            if(ID.WorldIndex == DevContext->Game->CurrentWorldIndex)
                                DevContext->SelectedObjectID = ID;
                        }
                    }
                }
            } break;
            
            case ENTITY_TYPE_RIGID_BODY:
            {
                Separator();          
                RadiusImGui(AK_HashFunction("Radius Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
                Separator();
                RestitutionImGui(AK_HashFunction("Restitution Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Restitution);
                Separator();
                const ak_char* WorldIndexList[] = {"World A", "World B"};
                WorldIndexImGui(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
                Separator();
                MassImGui(&Spawner->Mass);
                Separator();
                MaterialImGui(DevContext->Game->Assets, &Spawner->Material);
                Separator();                    
                
                
                if(Button(AK_HashFunction("Create Entity Button"), "Create"))
                {
                    if(ValidateMaterial(&Spawner->Material))                        
                    {
                        entity_id ID = CreateSphereRigidBody(DevContext->Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius, 
                                                             Spawner->Mass, Spawner->Restitution, Spawner->Material);
                        if(ID.WorldIndex == DevContext->Game->CurrentWorldIndex)
                            DevContext->SelectedObjectID = ID;
                    }
                }
            } break;
            
            case ENTITY_TYPE_PUSHABLE:
            {
                Separator();
                RadiusImGui(AK_HashFunction("Radius Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
                Separator();
                const ak_char* WorldIndexList[] = {"World A", "World B", "Both", "Linked"};
                WorldIndexImGui(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
                Separator();
                MassImGui(&Spawner->Mass);
                Separator();
                MaterialImGui(DevContext->Game->Assets, &Spawner->Material);
                Separator();
                
                if(Button(AK_HashFunction("Create Entity Button"), "Create"))
                {
                    if(ValidateMaterial(&Spawner->Material))
                    {
                        if(Spawner->WorldIndex == 2)
                        {
                            entity_id A = CreatePushableBox(DevContext->Game, 0, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);
                            entity_id B = CreatePushableBox(DevContext->Game, 1, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);
                            
                            if(DevContext->Game->CurrentWorldIndex == 0)
                                DevContext->SelectedObjectID = A;
                            else
                                DevContext->SelectedObjectID = B;
                        }
                        else if(Spawner->WorldIndex == 3)
                        {
                            dual_entity_id IDs = CreateDualPushableBox(DevContext->Game, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);
                            if(IDs.EntityA.WorldIndex == DevContext->Game->CurrentWorldIndex)
                                DevContext->SelectedObjectID = IDs.EntityA;
                            else
                                DevContext->SelectedObjectID = IDs.EntityB;
                        }
                        else
                        {
                            entity_id ID = CreatePushableBox(DevContext->Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);
                            if(ID.WorldIndex == DevContext->Game->CurrentWorldIndex)
                                DevContext->SelectedObjectID = ID;
                        }
                    }
                }
            } break;
            
            AK_INVALID_DEFAULT_CASE;
        }
        
        TreePop();
    }                
    
    Separator();       
    Separator();    
}

void DevelopmentImGuiUpdate(dev_context* DevContext)
{
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    NewFrame();
    
    //IMPORTANT(EVERYONE): If you need help figuring out how to use ImGui you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start
#if SHOW_IMGUI_DEMO_WINDOW
    local bool demo_window;
    ShowDemoWindow(&demo_window);
#endif
    
    SetNextWindowPos(ImVec2(0, 0));
    //SetNextWindowSize(ImVec2((ak_f32)Graphics->RenderDim.x/3.0f, (ak_f32)Graphics->RenderDim.y));    
    
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
    Combo(AK_HashFunction("ViewModes"), "", (int*)&DevContext->ViewModeType, ViewModeTypes, AK_Count(ViewModeTypes));
    
    const char* TransformTypes[] = {"Translate", "Scale", "Rotate"};
    AlignTextToFramePadding();
    Text("Object Transform Mode");
    SameLine();
    Combo("TransformMode", (int*)&DevContext->TransformationMode, TransformTypes, AK_Count(TransformTypes));
   
    Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);    
    Checkbox("Draw colliders", (bool*)&DevContext->DrawColliders);    
    Checkbox("Draw Grid", (bool*)&DevContext->DrawGrid);
    if(DevContext->DrawGrid)
    {
        DragFloat("GridSize", &DevContext->GridDistance, 0.1f, 0.1f, 10);
    }  
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
        DevelopmentEntitySpawner(DevContext);
        
        Text("Entity Information");
        if(DevContext->SelectedObjectID.IsValid())
        {   
            entity* Entity = GetEntity(Game, DevContext->SelectedObjectID);            
            ak_sqtf* Transform = GetEntityTransform(Game, DevContext->SelectedObjectID);
            
            ak_array<ak_v3f>* EntityRotations = &DevContext->EntityRotations[DevContext->SelectedObjectID.WorldIndex];            
            ak_u32 Index = AK_PoolIndex(DevContext->SelectedObjectID.ID);
            if((Index+1) > EntityRotations->Size)
                EntityRotations->Resize(Index+1);                        
            
            ak_v3f Translation = Transform->Translation;            
            ak_v3f Scale = Transform->Scale;
            ak_v3f Rotation = EntityRotations->Entries[Index];            
            
            {                
                TranslationImGui(AK_HashFunction("Edit Translation"), TRANSFORM_ITEM_WIDTH, &Translation);                                
            }
            
            {
                ScaleImGui(AK_HashFunction("Edit Scale"), TRANSFORM_ITEM_WIDTH, &Scale);                
            }
            
            {
                ak_u32 Hash = AK_HashFunction("Edit Rotation");
                Text("Rotation");
                PushItemWidth(TRANSFORM_ITEM_WIDTH);
                
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
            
            material Material = Entity->Material;
            MaterialImGui(DevContext->Game->Assets, &Material);
            
            if(DevContext->EditMode)
            {                                
                Transform->Translation = Translation;
                Transform->Scale = Scale;
                DevelopmentUpdateSelectedObjectRotation(Transform, EntityRotations->Get(Index), Rotation);
                SimEntity->Transform = *Transform;
                
                if(ValidateMaterial(&Material))
                    Entity->Material = Material;
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
