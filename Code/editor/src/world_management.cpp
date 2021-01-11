ak_string Internal__GetWorldDirectoryPath(ak_arena* Scratch, ak_string WorldName)
{
    ak_string Result = AK_StringConcat(WORLDS_PATH, WorldName, Scratch);
    Result = AK_StringConcat(Result, AK_OS_PATH_DELIMITER, Scratch);
    return Result;
}

ak_u32 Internal__WriteName(ak_binary_builder* Builder, ak_string Name)
{
    ak_u32 Result = 0;
    Result += Builder->Write(Name.Length);
    if(Name.Length != 0)
        Builder->WriteString(Name.Data);
    Result += Name.Length;
    return Result;
}

void Internal__WriteMaterial(ak_binary_builder* Builder, assets* Assets, material* Material)
{        
    Builder->Write(Material->Diffuse.IsTexture);
    if(Material->Diffuse.IsTexture)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Diffuse.DiffuseID);            
        Internal__WriteName(Builder, AK_CreateString(TextureInfo->Name, TextureInfo->Header.NameLength));
    }
    else    
        Builder->Write(Material->Diffuse.Diffuse);
    
    Builder->Write(Material->Specular.InUse);
    if(Material->Specular.InUse)
    {
        Builder->Write(Material->Specular.IsTexture);
        if(Material->Specular.IsTexture)
        {
            texture_info* TextureInfo = GetTextureInfo(Assets, Material->Specular.SpecularID); 
            Internal__WriteName(Builder, AK_CreateString(TextureInfo->Name, TextureInfo->Header.NameLength));           
        }
        else        
            Builder->Write(Material->Specular.Specular);
        
        Builder->Write(Material->Specular.Shininess);
    }
    
    Builder->Write(Material->Normal.InUse);
    if(Material->Normal.InUse)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Normal.NormalID); 
        Internal__WriteName(Builder, AK_CreateString(TextureInfo->Name, (ak_u8)TextureInfo->Header.NameLength));        
    }                        
}

ak_string Internal__ReadAssetName(ak_stream* Stream, ak_arena* Arena)
{    
    ak_string Result;
    Result.Length = Stream->CopyConsume<ak_u32>();
    Result.Data = Arena->PushArray<char>(Result.Length+1);
    Result.Data[Result.Length] = 0;
    AK_MemoryCopy(Result.Data, Stream->PeekConsume<char>(Result.Length), Result.Length);
    return Result;
}

ak_bool Internal__ReadMaterial(ak_stream* Stream, assets* Assets, ak_arena* Scratch, material* Material)
{
    Material->Diffuse.IsTexture = Stream->CopyConsume<ak_bool>();
    if(Material->Diffuse.IsTexture)
    {
        texture_asset_id* ID = Assets->TextureNameMap.Find(Internal__ReadAssetName(Stream, Scratch).Data);
        if(!ID)
            return false;        
        Material->Diffuse.DiffuseID = *ID;
    }
    else
        Material->Diffuse.Diffuse = Stream->CopyConsume<ak_color3f>();
    
    Material->Specular.InUse = Stream->CopyConsume<ak_bool>();
    if(Material->Specular.InUse)
    {
        Material->Specular.IsTexture = Stream->CopyConsume<ak_bool>();
        if(Material->Specular.IsTexture)
        {
            texture_asset_id* ID = Assets->TextureNameMap.Find(Internal__ReadAssetName(Stream, Scratch).Data);
            if(!ID)
                return false;
            Material->Specular.SpecularID = *ID;
        }        
        else
            Material->Specular.Specular = Stream->CopyConsume<ak_f32>();        
        Material->Specular.Shininess = Stream->CopyConsume<ak_i32>();
    }
    
    Material->Normal.InUse = Stream->CopyConsume<ak_bool>();
    if(Material->Normal.InUse)
    {
        texture_asset_id* ID = Assets->TextureNameMap.Find(Internal__ReadAssetName(Stream, Scratch).Data);
        if(!ID)
            return false;
        Material->Normal.NormalID = *ID;
    }
    
    return true;
}

ak_u64 Internal__CreateDevEntity(ak_arena* StringArena, ak_pool<dev_entity>* DevEntities, ak_char* Name, entity_type Type, ak_v3f Position, 
                                 ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = DevEntities->Allocate();
    dev_entity* Entity = DevEntities->Get(ID);
    
    Entity->Name = AK_PushString(Name, StringArena);
    Entity->Type = Type;
    Entity->Transform = AK_SQT(Position, Orientation, Scale);
    Entity->Material = Material;
    Entity->MeshID = MeshID;
    
    return ID;
}

ak_u64 Internal__CreateDevEntity(ak_arena* StringArena, ak_pool<dev_entity>* DevEntities, ak_char* Name, entity_type Type, ak_v3f Position, 
                                 ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = Internal__CreateDevEntity(StringArena, DevEntities, Name, Type, Position,
                                          AK_RotQuat(Axis, Angle), Scale, Material, MeshID);
    return ID;
}

ak_u64 Internal__CreateDevPointLight(ak_arena* StringArena, ak_pool<dev_point_light>* DevPointLights, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    ak_u64 ID = DevPointLights->Allocate();
    dev_point_light* PointLight = DevPointLights->Get(ID);
    
    PointLight->Name = AK_PushString(Name, StringArena);
    PointLight->Light.Color = Color;
    PointLight->Light.Intensity = Intensity;
    PointLight->Light.Position = Position;
    PointLight->Light.Radius = Radius;
    
    return ID;
}

ak_array<ak_string> Internal__GetAllWorldNames(ak_arena* Scratch)
{
    ak_array<ak_string> Result = {};
    
    ak_array<ak_string> Files = AK_GetAllFilesInDirectory(WORLDS_PATH, Scratch);
    AK_ForEach(File, &Files)
    {
        if(AK_DirectoryExists(File->Data))
        {
            ak_string WorldName = AK_GetFilename(*File);
            ak_string WorldPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
            
            ak_string WorldFilePath = AK_StringConcat(WorldPath, WorldName, Scratch);
            WorldFilePath = AK_StringConcat(WorldFilePath, ".world", Scratch);
            
            if(AK_FileExists(WorldFilePath))
                Result.Add(WorldName);
        }
    }
    
    AK_DeleteArray(&Files);
    
    return Result;
}

void Internal__CloseButton(ak_string WorldPath, world_management_state* NewState)
{
    if(AK_StringIsNullOrEmpty(WorldPath))
        UI_PushDisabledItem();
    
    ImGui::SameLine();
    if(ImGui::Button("Close"))
    {
        *NewState = WORLD_MANAGEMENT_STATE_NONE;
        ImGui::CloseCurrentPopup();
    }
    
    if(AK_StringIsNullOrEmpty(WorldPath))
        UI_PopDisabledItem();
}

template <typename type> 
void Internal__WriteStruct(ak_string_builder* Builder, ak_pool<type>* Pool, const ak_char* Name)
{
    Builder->WriteLine("struct %s", Name);
    Builder->WriteLine("{");
    AK_ForEach(Entry, Pool)
        Builder->WriteLine("\tak_u64 %.*s;", Entry->Name.Length, Entry->Name.Data);
    Builder->WriteLine("};");
}

template <typename type>
void Internal__InsertIndices(ak_pool<type>* Pool, ak_hash_map<ak_string, ak_u32>* HashMap)
{
    ak_u32 Index = 0;
    AK_ForEach(Entry, Pool)
        HashMap->Insert(Entry->Name, Index++);
}

void Internal__CreateBuildAllWorldsFile(ak_arena* Scratch)
{
    ak_string BuildAllWorldPath = AK_StringConcat(WORLDS_PATH, "build_all.bat", Scratch);
    
    ak_string_builder StringBuilder = {};
    
    StringBuilder.WriteLine("@echo off");
    
    ak_array<ak_string> WorldNames = Internal__GetAllWorldNames(Scratch);
    
    AK_ForEach(World, &WorldNames)
    {
        StringBuilder.WriteLine("call %.*s/build.bat", World->Length, World->Data);
        StringBuilder.WriteLine("popd");
    }
    
    ak_string String = StringBuilder.PushString(Scratch);
    AK_WriteEntireFile(BuildAllWorldPath, String.Data, String.Length);
    
    AK_DeleteArray(&WorldNames);
    StringBuilder.ReleaseMemory();
}

void world_management::Update(editor* Editor, platform* Platform, dev_platform* DevPlatform, assets* Assets)
{
    ak_arena* Scratch = Editor->Scratch;
    ak_array<ak_string> Worlds = Internal__GetAllWorldNames(Scratch);
    
    if(NewState == WORLD_MANAGEMENT_STATE_NONE)
    {
        if(AK_StringIsNullOrEmpty(CurrentWorldPath))
        {
            if(Worlds.Size == 0)
            {
                NewState = WORLD_MANAGEMENT_STATE_CREATE;
            }
            else
            {
                NewState = WORLD_MANAGEMENT_STATE_LOAD;
            }
        }
    }
    
    local ak_u32 WorldSelectedIndex = (ak_u32)-1;
    switch(NewState)
    {
        case WORLD_MANAGEMENT_STATE_CREATE:
        {
            if(!ImGui::IsPopupOpen("Create World"))
                ImGui::OpenPopup("Create World");
            
            if(ImGui::BeginPopupModal("Create World", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                local ak_char WorldNameData[MAX_WORLD_NAME] = {};
                
                if(OldState != NewState)
                    AK_MemoryClear(WorldNameData, sizeof(WorldNameData));
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("World Name");
                ImGui::InputText("", WorldNameData, MAX_WORLD_NAME, ImGuiInputTextFlags_CharsNoBlank);
                
                
                ak_string WorldName = AK_CreateString(WorldNameData);
                if(AK_StringIsNullOrEmpty(WorldName))
                    UI_PushDisabledItem();
                
                ak_bool ShowDuplicateWorldNameText = false;
                AK_ForEach(World, &Worlds)
                {
                    if(AK_StringEquals(*World, WorldName)) ShowDuplicateWorldNameText = true;
                }
                
                if(ImGui::Button("Create"))
                {
                    if(CreateWorld(WorldName, Editor, DevPlatform, Assets, Platform))
                    {
                        Editor->GizmoState.SelectedObject = {};
                        NewState = WORLD_MANAGEMENT_STATE_NONE;
                        ImGui::CloseCurrentPopup();
                    }
                }
                
                if(AK_StringIsNullOrEmpty(WorldName))
                    UI_PopDisabledItem();
                
                if(Worlds.Size == 0)
                    UI_PushDisabledItem();
                
                ImGui::SameLine();
                if(ImGui::Button("Load"))
                {
                    NewState = WORLD_MANAGEMENT_STATE_LOAD;
                    ImGui::CloseCurrentPopup();
                }
                
                if(Worlds.Size == 0)
                    UI_PopDisabledItem();
                
                Internal__CloseButton(CurrentWorldPath, &NewState);
                
                if(ShowDuplicateWorldNameText)
                {
                    UI_ErrorText("World with name '%.*s' already exists, cannot create world without deleting already existing world or renaming new one", 
                                 WorldName.Length, WorldName.Data); 
                }
                
                ImGui::EndPopup();
            }
            
        } break;
        
        case WORLD_MANAGEMENT_STATE_LOAD:
        {
            if(!ImGui::IsPopupOpen("Load World"))
                ImGui::OpenPopup("Load World");
            
            if(OldState != NewState)
                WorldSelectedIndex = (ak_u32)-1;
            
            if(ImGui::BeginPopupModal("Load World", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("World List");
                ImGui::Separator();
                
                ak_u32 Index = 0;
                AK_ForEach(World, &Worlds)
                {
                    ak_char* Text = AK_FormatString(Scratch, "%d. %.*s", Index+1, World->Length, World->Data).Data;
                    
                    ak_bool Selected = WorldSelectedIndex == Index;
                    if(ImGui::Selectable(Text, Selected))
                    {
                        if(WorldSelectedIndex == Index)
                            WorldSelectedIndex = (ak_u32)-1;
                        else 
                            WorldSelectedIndex = Index;
                    }
                    Index++;
                }
                
                ImGui::Separator();
                if(WorldSelectedIndex == (ak_u32)-1)
                    UI_PushDisabledItem();
                
                if(ImGui::Button("Load"))
                {
                    ak_string WorldName = Worlds[WorldSelectedIndex];
                    if(LoadWorld(WorldName, Editor, Assets, DevPlatform))
                    {
                        Editor->GizmoState.SelectedObject = {};
                        NewState = WORLD_MANAGEMENT_STATE_NONE;
                        ImGui::CloseCurrentPopup();
                    }
                }
                
                if(WorldSelectedIndex == (ak_u32)-1)
                    UI_PopDisabledItem();
                
                ImGui::SameLine();
                if(ImGui::Button("New"))
                {
                    NewState = WORLD_MANAGEMENT_STATE_CREATE;
                    ImGui::CloseCurrentPopup();
                }
                
                Internal__CloseButton(CurrentWorldPath, &NewState);
                
                ImGui::EndPopup();
            }
            
        } break;
        
        case WORLD_MANAGEMENT_STATE_SAVE:
        {
            ak_string ErrorMessage = BuildWorld(Scratch, DevPlatform, Assets);
            if(!AK_StringIsNullOrEmpty(ErrorMessage))
                Editor_DebugLog(ErrorMessage);
            NewState = WORLD_MANAGEMENT_STATE_NONE;
        } break;
        
        case WORLD_MANAGEMENT_STATE_DELETE:
        {
            if(!ImGui::IsPopupOpen("Delete World"))
                ImGui::OpenPopup("Delete World");
            
            if(OldState != NewState)
                WorldSelectedIndex = (ak_u32)-1;
            
            if(ImGui::BeginPopupModal("Delete World", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("World List");
                ImGui::Separator();
                
                ak_u32 Index = 0;
                AK_ForEach(World, &Worlds)
                {
                    ak_char* Text = AK_FormatString(Scratch, "%d. %.*s", Index+1, World->Length, World->Data).Data;
                    
                    ak_bool Selected = WorldSelectedIndex == Index;
                    if(ImGui::Selectable(Text, Selected))
                    {
                        if(WorldSelectedIndex == Index)
                            WorldSelectedIndex = (ak_u32)-1;
                        else 
                            WorldSelectedIndex = Index;
                    }
                    Index++;
                }
                
                ImGui::Separator();
                if(WorldSelectedIndex == (ak_u32)-1)
                    UI_PushDisabledItem();
                
                if(ImGui::Button("Delete"))
                {
                    ak_string WorldName = Worlds[WorldSelectedIndex];
                    DeleteWorld(Scratch, WorldName);
                    
                    NewState = WORLD_MANAGEMENT_STATE_NONE;
                    ImGui::CloseCurrentPopup();
                    
                    if(AK_StringEquals(WorldName, CurrentWorldName))
                    {
                        CurrentWorldName = AK_CreateEmptyString();
                        CurrentWorldPath = AK_CreateEmptyString();
                    }
                    
                    Editor->GizmoState.SelectedObject = {};
                }
                
                if(WorldSelectedIndex == (ak_u32)-1)
                    UI_PopDisabledItem();
                
                Internal__CloseButton(CurrentWorldPath, &NewState);
                
                ImGui::EndPopup();
            }
        } break;
    }
    
    AK_DeleteArray(&Worlds);
    OldState = NewState;
}

void world_management::DeleteIndices()
{
    AK_DeleteHashMap(&EntityIndices[0]);
    AK_DeleteHashMap(&EntityIndices[1]);
    AK_DeleteHashMap(&PointLightIndices[0]);
    AK_DeleteHashMap(&PointLightIndices[1]);
}

void world_management::DeleteAll()
{
    DeleteIndices();
    AK_DeletePool(&DevEntities[0]);
    AK_DeletePool(&DevEntities[1]);
    AK_DeletePool(&DevPointLights[0]);
    AK_DeletePool(&DevPointLights[1]);
    AK_DeleteHashMap(&EntityTables[0]);
    AK_DeleteHashMap(&EntityTables[1]);
    AK_DeleteHashMap(&PointLightTables[0]);
    AK_DeleteHashMap(&PointLightTables[1]);
    
    AK_DeleteArena(StringArena);
    
    CurrentWorldPath = AK_CreateEmptyString();
    CurrentWorldName = AK_CreateEmptyString();
}

dev_entity* world_management::CreateDevEntity(ak_u32 WorldIndex, ak_char* Name, entity_type Type, ak_v3f Position, 
                                              ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = Internal__CreateDevEntity(StringArena, &DevEntities[WorldIndex], Name, Type, Position, Orientation, Scale, Material, MeshID);
    dev_entity* Entity = DevEntities[WorldIndex].Get(ID);
    EntityTables[WorldIndex].Insert(Entity->Name, ID);
    return Entity;
}

dual_dev_entity world_management::CreateDevEntityInBothWorlds(ak_char* Name, entity_type Type, ak_v3f Position, 
                                                              ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    dual_dev_entity Result;
    Result.EntityA = CreateDevEntity(0, Name, Type, Position, Orientation, Scale, Material, MeshID);
    Result.EntityB = CreateDevEntity(1, Name, Type, Position, Orientation, Scale, Material, MeshID);
    return Result;
}

dev_point_light* world_management::CreateDevPointLight(ak_u32 WorldIndex, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    ak_u64 ID = Internal__CreateDevPointLight(StringArena, &DevPointLights[WorldIndex], Name, Position, Radius, Color, Intensity);
    dev_point_light* PointLight = DevPointLights[WorldIndex].Get(ID);
    PointLightTables[WorldIndex].Insert(PointLight->Name, ID);
    return PointLight;
}

dual_dev_point_light world_management::CreateDevPointLightInBothWorlds(ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    dual_dev_point_light Result;
    Result.PointLightA = CreateDevPointLight(0, Name, Position, Radius, Color, Intensity);
    Result.PointLightB = CreateDevPointLight(1, Name, Position, Radius, Color, Intensity);
    return Result;
}

void world_management::SetState(world_management_state State)
{
    NewState = State;
}

dev_entity* world_management::CopyDevEntity(dev_entity* CopyDevEntity, ak_u32 WorldIndex)
{
    ak_u64 IDDevEntity0 = DevEntities[WorldIndex].Allocate();
    dev_entity* DevEntity0 = DevEntities[WorldIndex].Get(IDDevEntity0);
    *DevEntity0 = CopyDevEntity[0];
    EntityTables[WorldIndex].Insert(DevEntity0->Name, IDDevEntity0);
    
    if(!AK_StringIsNullOrEmpty(DevEntity0->LinkName))
    {
        ak_u64 IDDevEntity1 = DevEntities[!WorldIndex].Allocate();
        dev_entity* DevEntity1 = DevEntities[!WorldIndex].Get(IDDevEntity1);
        *DevEntity1 = CopyDevEntity[1];
        EntityTables[!WorldIndex].Insert(DevEntity1->Name, IDDevEntity1);
        
        DevEntity0->LinkName = DevEntity1->Name;
        DevEntity1->LinkName = DevEntity0->Name;
    }
    
    return DevEntity0;
}

dev_point_light* world_management::CopyDevPointLight(dev_point_light* CopyPointLight, ak_u32 WorldIndex)
{
    ak_u64 IDDevPointLight = DevPointLights[WorldIndex].Allocate();
    dev_point_light* DevPointLight = DevPointLights[WorldIndex].Get(IDDevPointLight);
    *DevPointLight = *CopyPointLight;
    return DevPointLight;
}

template <typename type>
type* Internal__CopyObject(world_management* WorldManagement, ak_arena* Scratch, 
                           ak_pool<type>* Objects, ak_hash_map<ak_string, ak_u64>* Tables,
                           ak_u32 WorldIndex, type* Object)
{
    ak_u64 ID = Objects[WorldIndex].Allocate();
    type* DuplicateObject = Objects[WorldIndex].Get(ID);
    *DuplicateObject = *Object;
    
    ak_u32 Index = 1;
    ak_string Name = AK_CreateEmptyString();
    for(;;)
    {
        ak_string CopyString = AK_StringFind(DuplicateObject->Name, "_copy_");
        
        ak_string ObjectName = AK_CreateEmptyString();
        if(!AK_StringIsNullOrEmpty(CopyString))
            ObjectName = AK_CreateString(DuplicateObject->Name.Data, DuplicateObject->Name.Length-CopyString.Length);
        else
            ObjectName = DuplicateObject->Name;
        
        Name = AK_FormatString(Scratch, "%.*s_copy_%d", ObjectName.Length, ObjectName.Data, Index++);
        if(!Tables[WorldIndex].Find(Name))
            break;
    }
    
    DuplicateObject->Name = AK_PushString(Name, WorldManagement->StringArena);
    Tables[WorldIndex].Insert(DuplicateObject->Name, ID);
    return DuplicateObject;
}


dev_entity* world_management::DuplicateEntity(ak_arena* Scratch, dev_entity* Entity, ak_u32 WorldIndex)
{
    dev_entity* Result = Internal__CopyObject(this, Scratch, DevEntities, EntityTables, 
                                              WorldIndex, Entity);
    
    if(!AK_StringIsNullOrEmpty(Entity->LinkName))
    {
        ak_u64* LinkID = EntityTables[!WorldIndex].Find(Entity->LinkName);
        AK_Assert(LinkID, "Cannot have an entity with a linked object that is deleted");
        dev_entity* LinkEntity = DevEntities[!WorldIndex].Get(*LinkID);
        dev_entity* LinkResult = Internal__CopyObject(this, Scratch, DevEntities, EntityTables, !WorldIndex, LinkEntity);
        
        LinkResult->LinkName = Result->Name;
        Result->LinkName = LinkResult->Name;
    }
    
    return Result;
}

dev_point_light* world_management::DuplicatePointLight(ak_arena* Scratch, dev_point_light* PointLight, ak_u32 WorldIndex)
{
    dev_point_light* Result = Internal__CopyObject(this, Scratch, DevPointLights, PointLightTables, WorldIndex, PointLight);
    return Result;
}

void world_management::DeleteDevEntity(ak_u32 WorldIndex, ak_string Name, ak_bool ProcessLink)
{
    ak_u64* ID = EntityTables[WorldIndex].Find(Name);
    
    if(ID)
    {
        dev_entity* DevEntity = DevEntities[WorldIndex].Get(*ID);
        if(ProcessLink)
        {
            if(!AK_StringIsNullOrEmpty(DevEntity->LinkName))
            {
                DeleteDevEntity(!WorldIndex, DevEntity->LinkName, false);
            }
        }
        DevEntities[WorldIndex].Free(*ID);
        EntityTables[WorldIndex].Remove(Name);
    }
}

void world_management::DeleteDevPointLight(ak_u32 WorldIndex, ak_string Name)
{
    ak_u64* ID = PointLightTables[WorldIndex].Find(Name);
    if(ID)
    {
        DevPointLights[WorldIndex].Free(*ID);
        PointLightTables[WorldIndex].Remove(Name);
    }
}

ak_string world_management::BuildWorld(ak_arena* Scratch, dev_platform* DevPlatform, assets* Assets)
{
    ak_string_builder HeaderFile = {};
    
    HeaderFile.WriteLine("#ifndef GENERATED_H");
    HeaderFile.WriteLine("#define GENERATED_H");
    
    Internal__WriteStruct(&HeaderFile, &DevEntities[0], "entities_a");
    Internal__WriteStruct(&HeaderFile, &DevEntities[1], "entities_b");
    Internal__WriteStruct(&HeaderFile, &DevPointLights[0], "point_lights_a");
    Internal__WriteStruct(&HeaderFile, &DevPointLights[1], "point_lights_b");
    
    HeaderFile.WriteLine("#endif");
    
    DeleteIndices();
    
    Internal__InsertIndices(&DevEntities[0], &EntityIndices[0]);
    Internal__InsertIndices(&DevEntities[1], &EntityIndices[1]);
    Internal__InsertIndices(&DevPointLights[0], &PointLightIndices[0]);
    Internal__InsertIndices(&DevPointLights[1], &PointLightIndices[1]);
    
    ak_string HeaderFileString = HeaderFile.PushString(Scratch);
    ak_string HeaderPath = AK_StringConcat(CurrentWorldPath, "generated.h", Scratch);
    ak_string RenameHeaderPath = AK_StringConcat(CurrentWorldPath, "generated_rename.h", Scratch);
    
    ak_string BuildWorldLogFilePath = 
        AK_StringConcat(CurrentWorldPath, BUILD_WORLD_LOG_FILE, Scratch);
    
    if(AK_FileExists(HeaderPath))
        AK_FileRename(HeaderPath, RenameHeaderPath);
    
    AK_WriteEntireFile(HeaderPath, HeaderFileString.Data, HeaderFileString.Length);
    
    if(!DevPlatform->BuildWorld(CurrentWorldPath))
    {
        ak_string Message = AK_CreateString(AK_ReadEntireFile(BuildWorldLogFilePath, Scratch));
        AK_FileRemove(BuildWorldLogFilePath);
        
        AK_FileRemove(HeaderPath);
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRename(RenameHeaderPath, HeaderPath);
        return Message;
    }
    else
    {
        Editor_DebugLog("Successfully built game!");
        AK_FileRemove(BuildWorldLogFilePath);
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRemove(RenameHeaderPath);
    }
    
    //TODO(JJ): Task to actual build the world file (make it async if it takes awhile)
    ak_string WorldFilePath = AK_FormatString(Scratch, "%s%s.world", CurrentWorldPath.Data, CurrentWorldName.Data);
    
    ak_binary_builder BinaryBuilder = {};
    
    world_file_header WorldFileHeader = Editor_GetWorldFileHeader(AK_SafeU16(DevEntities[0].Size), AK_SafeU16(DevEntities[1].Size), 
                                                                  AK_SafeU16(DevPointLights[0].Size), AK_SafeU16(DevPointLights[1].Size));
    
    
    BinaryBuilder.Write(WorldFileHeader);
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_entity>* LocalDevEntities = &DevEntities[WorldIndex];
        AK_ForEach(DevEntity, LocalDevEntities)
        {
            Internal__WriteName(&BinaryBuilder, DevEntity->Name);
            Internal__WriteName(&BinaryBuilder, DevEntity->LinkName);
            
            BinaryBuilder.Write(DevEntity->Type);
            BinaryBuilder.Write(DevEntity->Transform.Translation);
            BinaryBuilder.Write(DevEntity->Transform.Orientation);
            BinaryBuilder.Write(DevEntity->Transform.Scale);
            Internal__WriteMaterial(&BinaryBuilder, Assets, &DevEntity->Material);
            
            mesh_info* MeshInfo = GetMeshInfo(Assets, DevEntity->MeshID);
            Internal__WriteName(&BinaryBuilder, AK_CreateString(MeshInfo->Name, MeshInfo->Header.NameLength));
            
            if(DevEntity->Type == ENTITY_TYPE_BUTTON)
                BinaryBuilder.Write(DevEntity->IsToggled);
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_point_light>* LocalDevPointLights = &DevPointLights[WorldIndex];
        AK_ForEach(DevPointLight, LocalDevPointLights)
        {
            Internal__WriteName(&BinaryBuilder, DevPointLight->Name);
            BinaryBuilder.Write(DevPointLight->Light.Color);
            BinaryBuilder.Write(DevPointLight->Light.Intensity);
            BinaryBuilder.Write(DevPointLight->Light.Position);
            BinaryBuilder.Write(DevPointLight->Light.Radius);
        }
    }
    
    BinaryBuilder.WriteString(WORLD_FILE_CHECKSUM);
    
    ak_buffer BinaryBuffer = BinaryBuilder.PushBuffer(Scratch);
    AK_WriteEntireFile(WorldFilePath, BinaryBuffer.Data, AK_SafeU32(BinaryBuffer.Size));
    
    BinaryBuilder.ReleaseMemory();
    
    return AK_CreateEmptyString();
}

ak_bool world_management::LoadWorld(ak_string WorldName, editor* Editor, assets* Assets, dev_platform* DevPlatform)
{
    ak_arena* Scratch = Editor->Scratch;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
    
    Editor->UI.EntitySpawner.Init = false;
    Editor->UI.LightSpawner.Init = false;
    
    ak_string WorldDirectoryPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
    ak_string WorldAssetFilename = AK_StringConcat(WorldName, ".world", Scratch);
    ak_string WorldFilePath = AK_StringConcat(WorldDirectoryPath, WorldAssetFilename, Scratch);
    
    ak_buffer WorldFileBuffer = AK_ReadEntireFile(WorldFilePath, Scratch);
    if(!WorldFileBuffer.IsValid())
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to load world. World '%.*s' does not exist", WorldName.Length, WorldName.Data);
        AK_MessageBoxOk("Failed to load world", Message);
        return false;
    }
    
    ak_stream WorldFileStream = AK_BeginStream(WorldFileBuffer);
    
    world_file_header* Header = WorldFileStream.PeekConsume<world_file_header>();
    
    if(!AK_StringEquals(Header->Signature, WORLD_FILE_SIGNATURE))
    {
        AK_MessageBoxOk("Failed to load world", "Failed to load world. World file is corrupted. Signatures do not match");
        return false;
    }
    
    if(Header->MajorVersion != WORLD_FILE_MAJOR_VERSION ||
       Header->MinorVersion != WORLD_FILE_MINOR_VERSION)
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to load world. Editor does not support version %d.%d, it only supports %d.%d", Header->MajorVersion, Header->MinorVersion, WORLD_FILE_MAJOR_VERSION, 
                                            WORLD_FILE_MINOR_VERSION);
        AK_MessageBoxOk("Failed to load world", Message);
        return false;
    }
    
#define DELETE_TEMP_DATA() \
do \
{ \
AK_DeleteArena(LocalStringArena); \
AK_DeletePool(&DevEntitiesArray[0]); \
AK_DeletePool(&DevEntitiesArray[1]); \
AK_DeletePool(&DevPointLightsArray[0]); \
AK_DeletePool(&DevPointLightsArray[1]); \
AK_DeleteHashMap(&LocalEntityTables[0]); \
AK_DeleteHashMap(&LocalEntityTables[1]); \
AK_DeleteHashMap(&LocalPointLightTables[0]); \
AK_DeleteHashMap(&LocalPointLightTables[1]); \
AK_DeleteHashMap(&LinkHashMaps[0]); \
AK_DeleteHashMap(&LinkHashMaps[1]); \
} while(0)
    
    ak_arena* LocalStringArena = AK_CreateArena();
    
    ak_pool<dev_entity> DevEntitiesArray[2];
    DevEntitiesArray[0] = AK_CreatePool<dev_entity>(Header->EntityCount[0]);
    DevEntitiesArray[1] = AK_CreatePool<dev_entity>(Header->EntityCount[1]);
    
    ak_pool<dev_point_light> DevPointLightsArray[2];
    DevPointLightsArray[0] = AK_CreatePool<dev_point_light>(Header->PointLightCount[0]);
    DevPointLightsArray[1] = AK_CreatePool<dev_point_light>(Header->PointLightCount[1]);
    
    ak_hash_map<ak_string, ak_u64> LocalEntityTables[2] = {};
    ak_hash_map<ak_string, ak_u64> LocalPointLightTables[2] = {};
    
    ak_hash_map<ak_string, ak_string> LinkHashMaps[2] = {};
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_entity>* LocalDevEntities = &DevEntitiesArray[WorldIndex];
        ak_hash_map<ak_string, ak_string>* LinkHashMap = &LinkHashMaps[WorldIndex];
        ak_hash_map<ak_string, ak_u64>* EntityTable = &LocalEntityTables[WorldIndex];
        ak_hash_map<ak_string, ak_u64>* PointLightTable = &LocalPointLightTables[WorldIndex];
        
        for(ak_u32 EntityIndex = 0; EntityIndex < Header->EntityCount[WorldIndex]; EntityIndex++)
        {
            ak_string Name = Internal__ReadAssetName(&WorldFileStream, Scratch);
            ak_string LinkName = Internal__ReadAssetName(&WorldFileStream, Scratch);
            
            entity_type Type = WorldFileStream.CopyConsume<entity_type>();
            ak_v3f Position = WorldFileStream.CopyConsume<ak_v3f>();
            ak_quatf Orientation = WorldFileStream.CopyConsume<ak_quatf>();
            ak_v3f Scale = WorldFileStream.CopyConsume<ak_v3f>();
            
            material Material;
            if(!Internal__ReadMaterial(&WorldFileStream, Assets, Scratch, &Material))
            {
                DELETE_TEMP_DATA();
                AK_MessageBoxOk("Failed to load world", "Failed to load world. Could not read entity material. Textures were not found in asset file");
                return false;
            }
            
            mesh_asset_id* MeshID = Assets->MeshNameMap.Find(Internal__ReadAssetName(&WorldFileStream, Scratch).Data);
            if(!MeshID)
            {
                
                DELETE_TEMP_DATA();
                AK_MessageBoxOk("Failed to load world", "Failed to load world. Could not read mesh asset. Mesh was not found in asset file");
                return false;
            }
            
            ak_u64 ID =  Internal__CreateDevEntity(LocalStringArena, LocalDevEntities, Name.Data, Type, Position, Orientation, Scale, Material, *MeshID);
            dev_entity* Entity = LocalDevEntities->Get(ID);
            EntityTable->Insert(Entity->Name, ID);
            if(!AK_StringIsNullOrEmpty(LinkName))
            {
                LinkHashMap->Insert(LinkName, Entity->Name);
            }
            
            if(Type == ENTITY_TYPE_BUTTON)
                Entity->IsToggled = WorldFileStream.CopyConsume<ak_bool>();
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_hash_map<ak_string, ak_string>* LinkHashMap = &LinkHashMaps[!WorldIndex];
        ak_pool<dev_entity>* LocalDevEntities = &DevEntitiesArray[WorldIndex];
        AK_ForEach(DevEntity, LocalDevEntities)
        {
            ak_string* LinkName = LinkHashMap->Find(DevEntity->Name);
            if(LinkName) DevEntity->LinkName = AK_PushString(*LinkName, LocalStringArena);
        }
    }
    
    AK_DeleteHashMap(&LinkHashMaps[0]);
    AK_DeleteHashMap(&LinkHashMaps[1]);
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_point_light>* LocalDevPointLights = &DevPointLightsArray[WorldIndex];
        ak_hash_map<ak_string, ak_u64>* PointLightTable = &LocalPointLightTables[WorldIndex];
        for(ak_u32 PointLightIndex = 0; PointLightIndex < Header->PointLightCount[WorldIndex]; 
            PointLightIndex++)
        {
            ak_string Name = Internal__ReadAssetName(&WorldFileStream, Scratch);
            ak_color3f Color = WorldFileStream.CopyConsume<ak_color3f>();
            ak_f32 Intensity = WorldFileStream.CopyConsume<ak_f32>();
            ak_v3f Position = WorldFileStream.CopyConsume<ak_v3f>();
            ak_f32 Radius = WorldFileStream.CopyConsume<ak_f32>();
            
            ak_u64 ID = Internal__CreateDevPointLight(LocalStringArena, LocalDevPointLights, Name.Data, Position, Radius, Color, Intensity);
            dev_point_light* DevPointLight = LocalDevPointLights->Get(ID);
            PointLightTable->Insert(DevPointLight->Name, ID);
        }
    }
    
    ak_u32 ChecksumCount = AK_Count(WORLD_FILE_CHECKSUM)-1;
    ak_char Checksum[AK_Count(WORLD_FILE_CHECKSUM)];
    AK_MemoryCopy(Checksum, WorldFileStream.PeekConsume(ChecksumCount), ChecksumCount);
    Checksum[ChecksumCount] = 0;
    if(!AK_StringEquals(Checksum, ChecksumCount, WORLD_FILE_CHECKSUM, ChecksumCount))
    {
        DELETE_TEMP_DATA();
        AK_MessageBoxOk("Failed to load world", "Failed to load world. File is corrupted. Checksums do not match");
        return false;
    }
    
    EditRecordings->Clear();
    
    DeleteAll();
    
    StringArena = LocalStringArena;
    
    DevEntities[0] = DevEntitiesArray[0];
    DevEntities[1] = DevEntitiesArray[1];
    DevPointLights[0] = DevPointLightsArray[0];
    DevPointLights[1] = DevPointLightsArray[1];
    
    EntityTables[0] = LocalEntityTables[0];
    EntityTables[1] = LocalEntityTables[1];
    PointLightTables[0] = LocalPointLightTables[0];
    PointLightTables[1] = LocalPointLightTables[1];
    
    CurrentWorldPath = AK_PushString(WorldDirectoryPath, StringArena);
    CurrentWorldName = AK_PushString(WorldName, StringArena);
    
    ak_string ErrorMessage = BuildWorld(Scratch, DevPlatform, Assets);
    if(!AK_StringIsNullOrEmpty(ErrorMessage))
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to load world. Building of the world failed.\n Compiler errors: \n%.*s", ErrorMessage.Length, ErrorMessage.Data);
        DeleteAll();
        Editor_DebugLog(ErrorMessage);
        AK_MessageBoxOk("Failed to load world", Message);
        return false;
    }
    
#undef DELETE_TEMP_DATA
    
    return true;
}

ak_bool world_management::CreateWorld(ak_string WorldName, editor* Editor, dev_platform* DevPlatform, assets* Assets, platform* Platform)
{
    AK_Assert(!AK_StringIsNullOrEmpty(WorldName), "WorldName cannot be null or empty");
    
    ak_arena* Scratch = Editor->Scratch;
    
    Editor->EditRecordings.Clear();
    
    DeleteAll();
    
    StringArena = AK_CreateArena();
    
    material PlayerMaterial = {CreateDiffuse(AK_Blue3()), InvalidNormal(), CreateSpecular(0.5f, 8)};
    dual_dev_entity DualPlayerEntity = CreateDevEntityInBothWorlds("Player", ENTITY_TYPE_PLAYER, AK_V3(0.0f, 0.0f, 0.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 1.0f, 1.0f), PlayerMaterial, MESH_ASSET_ID_PLAYER);
    
    material FloorMaterial = { CreateDiffuse(AK_White3()) };
    dual_dev_entity DualStaticEntity = CreateDevEntityInBothWorlds("Default_Floor", ENTITY_TYPE_STATIC, AK_V3(0.0f, 0.0f, -1.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 10.0f, 1.0f), FloorMaterial, MESH_ASSET_ID_BOX);
    
    dual_dev_point_light DualPointLights = CreateDevPointLightInBothWorlds("Default_Light", AK_V3(0.0f, 0.0f, 10.0f), 20.0f, AK_White3(), 1.0f);
    
    ak_string NewWorldDirectoryPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
    
    ak_string GameCodePath = AK_FormatString(Scratch, "%.*s..%cCode%c", 
                                             Platform->ProgramPath.Length, Platform->ProgramPath.Data, AK_OS_PATH_DELIMITER, AK_OS_PATH_DELIMITER);
    
    ak_string WorldHeaderString = GetWorldHeaderFile(Scratch, WorldName);
    ak_string WorldSourceString = GetWorldSourceFile(Scratch, WorldName);
    ak_string WorldBuildString = GetWorldBuildFile(Scratch, GameCodePath, WorldName);
    
    ak_string WorldHeaderPath = AK_FormatString(Scratch, "%.*s%.*s.h", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    ak_string WorldSourcePath = AK_FormatString(Scratch, "%.*s%.*s.cpp", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    ak_string WorldBuildPath = AK_StringConcat(NewWorldDirectoryPath, "build.bat", Scratch);
    
    
    if(!AK_DirectoryExists(NewWorldDirectoryPath))
        AK_CreateDirectory(NewWorldDirectoryPath);
    
    AK_WriteEntireFile(WorldHeaderPath, WorldHeaderString.Data, WorldHeaderString.Length);
    AK_WriteEntireFile(WorldSourcePath, WorldSourceString.Data, WorldSourceString.Length);
    AK_WriteEntireFile(WorldBuildPath, WorldBuildString.Data, WorldBuildString.Length);
    
    CurrentWorldName = AK_PushString(WorldName, StringArena);
    CurrentWorldPath = AK_PushString(NewWorldDirectoryPath, StringArena);
    
    ak_string ErrorMessage = BuildWorld(Scratch, DevPlatform, Assets);
    if(!AK_StringIsNullOrEmpty(ErrorMessage))
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to create world. Building of the world failed.\n Compiler errors: \n%.*s", ErrorMessage.Length, ErrorMessage.Data);
        AK_MessageBoxOk("Failed to create world", Message);
        Editor_DebugLog(ErrorMessage);
        DeleteAll();
        
        AK_DirectoryRemoveRecursively(NewWorldDirectoryPath);
    }
    else
    {
        Internal__CreateBuildAllWorldsFile(Scratch);
    }
    
    return AK_StringIsNullOrEmpty(ErrorMessage);
}

void world_management::DeleteWorld(ak_arena* Scratch, ak_string WorldName)
{
    ak_string WorldDirectoryPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
    AK_DirectoryRemoveRecursively(WorldDirectoryPath);
    Internal__CreateBuildAllWorldsFile(Scratch);
}