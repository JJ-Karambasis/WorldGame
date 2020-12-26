ak_string Internal__GetWorldDirectoryPath(ak_arena* Scratch, ak_string WorldName)
{
    ak_string Result = AK_StringConcat(WORLDS_PATH, WorldName, Scratch);
    Result = AK_StringConcat(Result, AK_OS_PATH_DELIMITER, Scratch);
    return Result;
}

ak_u32 Internal__WriteName(ak_binary_builder* Builder, ak_char* Name, ak_u8 NameLength)
{
    ak_u32 Result = 0;
    Result += Builder->Write(NameLength);
    Builder->WriteString(Name);
    Result += NameLength;
    return Result;
}

void Internal__WriteMaterial(ak_binary_builder* Builder, assets* Assets, material* Material)
{        
    Builder->Write(Material->Diffuse.IsTexture);
    if(Material->Diffuse.IsTexture)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Diffuse.DiffuseID);            
        Internal__WriteName(Builder, TextureInfo->Name, (ak_u8)TextureInfo->Header.NameLength);        
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
            Internal__WriteName(Builder, TextureInfo->Name, (ak_u8)TextureInfo->Header.NameLength);            
        }
        else        
            Builder->Write(Material->Specular.Specular);
        
        Builder->Write(Material->Specular.Shininess);
    }
    
    Builder->Write(Material->Normal.InUse);
    if(Material->Normal.InUse)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Normal.NormalID); 
        Internal__WriteName(Builder, TextureInfo->Name, (ak_u8)TextureInfo->Header.NameLength);        
    }                        
}

ak_string Internal__ReadAssetName(ak_stream* Stream, ak_arena* Arena)
{    
    ak_string Result;
    Result.Length = Stream->CopyConsume<ak_u8>();
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

dev_entity* Internal__CreateDevEntity(ak_pool<dev_entity>* DevEntities, ak_char* Name, entity_type Type, ak_v3f Position, 
                                      ak_quatf Orientation, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = DevEntities->Allocate();
    dev_entity* Entity = DevEntities->Get(ID);
    
    AK_CopyArray(Entity->Name, Name, MAX_OBJECT_NAME_LENGTH);
    Entity->Type = Type;
    Entity->ID = ID;
    Entity->LinkID = 0;
    Entity->Transform = AK_SQT(Position, Orientation, Scale);
    Entity->Material = Material;
    Entity->MeshID = MeshID;
    Entity->Euler = AK_QuatToEuler(Entity->Transform.Orientation);
    
    return Entity;
}

dev_entity* Internal__CreateDevEntity(ak_pool<dev_entity>* DevEntities, ak_char* Name, entity_type Type, ak_v3f Position, 
                                      ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = DevEntities->Allocate();
    dev_entity* Entity = DevEntities->Get(ID);
    
    AK_CopyArray(Entity->Name, Name, MAX_OBJECT_NAME_LENGTH);
    Entity->Type = Type;
    Entity->ID = ID;
    Entity->LinkID = 0;
    Entity->Transform = AK_SQT(Position, AK_RotQuat(Axis, Angle), Scale);
    Entity->Material = Material;
    Entity->MeshID = MeshID;
    Entity->Euler = AK_QuatToEuler(Entity->Transform.Orientation);
    
    return Entity;
}

dev_point_light* Internal__CreateDevPointLight(ak_pool<dev_point_light>* DevPointLights, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    ak_u64 ID = DevPointLights->Allocate();
    dev_point_light* PointLight = DevPointLights->Get(ID);
    
    AK_CopyArray(PointLight->Name, Name, MAX_OBJECT_NAME_LENGTH);
    PointLight->Light.Color = Color;
    PointLight->Light.Intensity = Intensity;
    PointLight->Light.Position = Position;
    PointLight->Light.Radius = Radius;
    PointLight->ID = ID;
    
    return PointLight;
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
        Builder->WriteLine("\tak_u64 %s;", Entry->Name);
    Builder->WriteLine("};");
}

template <typename type>
void Internal__InsertIndices(ak_pool<type>* Pool, ak_hash_map<ak_char*, ak_u32>* HashMap)
{
    ak_u32 Index = 0;
    AK_ForEach(Entry, Pool)
        HashMap->Insert(Entry->Name, Index++);
}

ak_string Internal__BuildWorld(ak_arena* Scratch, world_management* WorldManagement, dev_platform* DevPlatform, assets* Assets)
{
    ak_string_builder HeaderFile = {};
    
    HeaderFile.WriteLine("#ifndef GENERATED_H");
    HeaderFile.WriteLine("#define GENERATED_H");
    
    Internal__WriteStruct(&HeaderFile, &WorldManagement->DevEntities[0], "entities_a");
    Internal__WriteStruct(&HeaderFile, &WorldManagement->DevEntities[1], "entities_b");
    Internal__WriteStruct(&HeaderFile, &WorldManagement->DevPointLights[0], "point_lights_a");
    Internal__WriteStruct(&HeaderFile, &WorldManagement->DevPointLights[1], "point_lights_b");
    
    HeaderFile.WriteLine("#endif");
    
    WorldManagement->DeleteIndices();
    
    Internal__InsertIndices(&WorldManagement->DevEntities[0], &WorldManagement->EntityIndices[0]);
    Internal__InsertIndices(&WorldManagement->DevEntities[1], &WorldManagement->EntityIndices[1]);
    Internal__InsertIndices(&WorldManagement->DevPointLights[0], &WorldManagement->PointLightIndices[0]);
    Internal__InsertIndices(&WorldManagement->DevPointLights[1], &WorldManagement->PointLightIndices[1]);
    
    ak_string HeaderFileString = HeaderFile.PushString(Scratch);
    ak_string HeaderPath = AK_StringConcat(WorldManagement->CurrentWorldPath, "generated.h", Scratch);
    ak_string RenameHeaderPath = AK_StringConcat(WorldManagement->CurrentWorldPath, "generated_rename.h", Scratch);
    
    if(AK_FileExists(HeaderPath))
        AK_FileRename(HeaderPath, RenameHeaderPath);
    
    AK_WriteEntireFile(HeaderPath, HeaderFileString.Data, HeaderFileString.Length);
    
    if(!DevPlatform->BuildWorld(WorldManagement->CurrentWorldName))
    {
        ak_string Message = AK_CreateString(AK_ReadEntireFile("build_world_log.txt", Scratch));
        AK_FileRemove("build_world_log.txt");
        
        AK_FileRemove(HeaderPath);
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRename(RenameHeaderPath, HeaderPath);
        return Message;
    }
    else
    {
        Editor_DebugLog("Successfully built game!");
        AK_FileRemove("build_world_log.txt");
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRemove(RenameHeaderPath);
    }
    
    //TODO(JJ): Task to actual build the world file (make it async if it takes awhile)
    ak_string WorldFilePath = AK_FormatString(Scratch, "%s%s.world", WorldManagement->CurrentWorldPath.Data, WorldManagement->CurrentWorldName.Data);
    
    ak_binary_builder BinaryBuilder = {};
    
    world_file_header WorldFileHeader = Editor_GetWorldFileHeader(AK_SafeU16(WorldManagement->DevEntities[0].Size), AK_SafeU16(WorldManagement->DevEntities[1].Size), 
                                                                  AK_SafeU16(WorldManagement->DevPointLights[0].Size), AK_SafeU16(WorldManagement->DevPointLights[1].Size));
    
    
    BinaryBuilder.Write(WorldFileHeader);
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_entity>* DevEntities = &WorldManagement->DevEntities[WorldIndex];
        AK_ForEach(DevEntity, DevEntities)
        {
            Internal__WriteName(&BinaryBuilder, DevEntity->Name, (ak_u8)AK_StringLength(DevEntity->Name));
            
            ak_char* LinkName = NULL;
            if(DevEntity->LinkID)
                LinkName = WorldManagement->DevEntities[!WorldIndex].Get(DevEntity->LinkID)->Name;
            
            ak_u8 LinkNameLength = LinkName ? (ak_u8)(AK_StringLength(LinkName)) : 0;
            BinaryBuilder.Write(LinkNameLength);
            if(LinkName)
                BinaryBuilder.Write(LinkName);
            
            BinaryBuilder.Write(DevEntity->Type);
            BinaryBuilder.Write(DevEntity->Transform.Translation);
            BinaryBuilder.Write(DevEntity->Transform.Orientation);
            BinaryBuilder.Write(DevEntity->Transform.Scale);
            Internal__WriteMaterial(&BinaryBuilder, Assets, &DevEntity->Material);
            
            mesh_info* MeshInfo = GetMeshInfo(Assets, DevEntity->MeshID);
            Internal__WriteName(&BinaryBuilder, MeshInfo->Name, (ak_u8)MeshInfo->Header.NameLength);
            
            if(DevEntity->Type == ENTITY_TYPE_BUTTON)
                BinaryBuilder.Write(DevEntity->IsToggled);
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_point_light>* DevPointLights = &WorldManagement->DevPointLights[WorldIndex];
        AK_ForEach(DevPointLight, DevPointLights)
        {
            Internal__WriteName(&BinaryBuilder, DevPointLight->Name, 
                                (ak_u8)AK_StringLength(DevPointLight->Name));
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

ak_bool Internal__CreateNewWorld(ak_arena* Scratch, world_management* WorldManagement, ak_string WorldName, dev_platform* DevPlatform, assets* Assets)
{
    AK_Assert(!AK_StringIsNullOrEmpty(WorldName), "WorldName cannot be null or empty");
    
    ak_string_builder WorldHeader = {};
    ak_string_builder WorldSource = {};
    
    ak_string WorldNameUpper = AK_ToUpper(WorldName, Scratch);
    ak_string WorldNameLower = AK_ToLower(WorldName, Scratch);
    ak_string HeaderGuard = AK_StringConcat(WorldNameUpper, "_H", Scratch);
    
    WorldHeader.WriteLine("#ifndef %.*s", HeaderGuard.Length, HeaderGuard.Data);
    WorldHeader.WriteLine("#define %.*s", HeaderGuard.Length, HeaderGuard.Data);
    WorldHeader.WriteLine("#include <game.h>");
    WorldHeader.WriteLine("#include \"generated.h\"");
    WorldHeader.NewLine();
    WorldHeader.WriteLine("struct %.*s : public world", WorldNameLower.Length, WorldNameLower.Data);
    WorldHeader.WriteLine("{");
    WorldHeader.WriteLine("\tentities_a WorldEntitiesA;");
    WorldHeader.WriteLine("\tentities_b WorldEntitiesB;");
    WorldHeader.WriteLine("\tpoint_lights_a PointLightsA;");
    WorldHeader.WriteLine("\tpoint_lights_b PointLightsB;");
    WorldHeader.WriteLine("};");
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_STARTUP(%.*s_Startup);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_UPDATE(%.*s_Update);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("#endif");
    
    WorldSource.WriteLine("#include \"%.*s.h\"", WorldName.Length, WorldName.Data);
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_STARTUP(%.*s_Startup)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("\t%.*s* World = (%.*s*)AK_Allocate(sizeof(%.*s));", WorldNameLower.Length, WorldNameLower.Data, WorldNameLower.Length, 
                          WorldNameLower.Data, WorldNameLower.Length, WorldNameLower.Data);
    //TODO(JJ): Generate some error handling when we have some proper error handling :)
    WorldSource.WriteLine("\tWorld->Update = %.*s_Update;", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("\tWorld->Shutdown = %.*s_Shutdown;", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("\tGame->World = World;");
    WorldSource.WriteLine("\treturn true;");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_UPDATE(%.*s_Update)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("\tGame->WorldShutdownCommon(Game);");
    WorldSource.WriteLine("\tGame->World = NULL;");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    //WorldSource.WriteLine("#include \"generated.cpp\"");
    WorldSource.WriteLine("#include <assets.cpp>");
    WorldSource.WriteLine("#include <game_common_source.cpp>");
    WorldSource.WriteLine("#define AK_COMMON_IMPLEMENTATION");
    WorldSource.WriteLine("#include <ak_common.h>");
    
    WorldManagement->DeleteAll();
    
    material PlayerMaterial = {CreateDiffuse(AK_Blue3()), InvalidNormal(), CreateSpecular(0.5f, 8)};
    dual_dev_entity DualPlayerEntity = WorldManagement->CreateDevEntityInBothWorlds("Player", ENTITY_TYPE_PLAYER, AK_V3(0.0f, 0.0f, 0.0f), AK_XAxis(), 0.0f, AK_V3(1.0f, 1.0f, 1.0f), PlayerMaterial, MESH_ASSET_ID_PLAYER);
    
    WorldManagement->EntityNameCollisionMap[0].Insert(DualPlayerEntity.EntityA->Name, true);
    WorldManagement->EntityNameCollisionMap[1].Insert(DualPlayerEntity.EntityB->Name, true);
    
    material FloorMaterial = { CreateDiffuse(AK_White3()) };
    dual_dev_entity DualStaticEntity = WorldManagement->CreateDevEntityInBothWorlds("Default_Floor", ENTITY_TYPE_STATIC, AK_V3(0.0f, 0.0f, -1.0f), AK_XAxis(), 0.0f, AK_V3(10.0f, 10.0f, 1.0f), FloorMaterial, MESH_ASSET_ID_BOX);
    WorldManagement->EntityNameCollisionMap[0].Insert(DualStaticEntity.EntityA->Name, true);
    WorldManagement->EntityNameCollisionMap[1].Insert(DualStaticEntity.EntityB->Name, true);
    
    dual_dev_point_light DualPointLights = WorldManagement->CreateDevPointLightInBothWorlds("Default_Light", AK_V3(0.0f, 0.0f, 10.0f), 20.0f, AK_White3(), 1.0f);
    
    WorldManagement->LightNameCollisionMap[0].Insert(DualPointLights.PointLightA->Name, true);
    WorldManagement->LightNameCollisionMap[1].Insert(DualPointLights.PointLightB->Name, true);
    
    ak_string NewWorldDirectoryPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
    
    ak_string WorldHeaderString = WorldHeader.PushString(Scratch);
    ak_string WorldSourceString = WorldSource.PushString(Scratch);
    
    ak_string WorldHeaderPath = AK_FormatString(Scratch, "%.*s%.*s.h", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    ak_string WorldSourcePath = AK_FormatString(Scratch, "%.*s%.*s.cpp", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    
    if(!AK_DirectoryExists(NewWorldDirectoryPath))
        AK_CreateDirectory(NewWorldDirectoryPath);
    
    AK_WriteEntireFile(WorldHeaderPath, WorldHeaderString.Data, WorldHeaderString.Length);
    AK_WriteEntireFile(WorldSourcePath, WorldSourceString.Data, WorldSourceString.Length);
    
    WorldManagement->CurrentWorldName = AK_PushString(WorldName);
    WorldManagement->CurrentWorldPath = AK_PushString(NewWorldDirectoryPath);
    
    ak_string ErrorMessage = Internal__BuildWorld(Scratch, WorldManagement, DevPlatform, Assets);
    if(!AK_StringIsNullOrEmpty(ErrorMessage))
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to create world. Building of the world failed.\n Compiler errors: \n%.*s", ErrorMessage.Length, ErrorMessage.Data);
        AK_MessageBoxOk("Failed to create world", Message);
        Editor_DebugLog(ErrorMessage);
        WorldManagement->DeleteAll();
        AK_FileRemove(WorldHeaderPath);
        AK_FileRemove(WorldSourcePath);
        AK_DirectoryRemove(NewWorldDirectoryPath);
    }
    
    WorldHeader.ReleaseMemory();
    WorldSource.ReleaseMemory();
    
    return AK_StringIsNullOrEmpty(ErrorMessage);
}

ak_bool Internal__LoadWorld(ak_arena* Scratch, world_management* WorldManagement, ak_string WorldName, assets* Assets, dev_platform* DevPlatform)
{
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
AK_DeletePool(&DevEntitiesArray[0]); \
AK_DeletePool(&DevEntitiesArray[1]); \
AK_DeletePool(&DevPointLightsArray[0]); \
AK_DeletePool(&DevPointLightsArray[1]); \
AK_DeleteHashMap(&EntityNameCollisionMaps[0]); \
AK_DeleteHashMap(&EntityNameCollisionMaps[1]); \
AK_DeleteHashMap(&LightNameCollisionMaps[0]); \
AK_DeleteHashMap(&LightNameCollisionMaps[1]); \
AK_DeleteHashMap(&LinkHashMaps[0]); \
AK_DeleteHashMap(&LinkHashMaps[1]); \
} while(0)
    
    ak_pool<dev_entity> DevEntitiesArray[2];
    DevEntitiesArray[0] = AK_CreatePool<dev_entity>(Header->EntityCount[0]);
    DevEntitiesArray[1] = AK_CreatePool<dev_entity>(Header->EntityCount[1]);
    
    ak_pool<dev_point_light> DevPointLightsArray[2];
    DevPointLightsArray[0] = AK_CreatePool<dev_point_light>(Header->PointLightCount[0]);
    DevPointLightsArray[1] = AK_CreatePool<dev_point_light>(Header->PointLightCount[1]);
    
    ak_hash_map<ak_char*, ak_bool> EntityNameCollisionMaps[2] = {};
    ak_hash_map<ak_char*, ak_bool> LightNameCollisionMaps[2] = {};
    
    ak_hash_map<ak_char*, ak_u64> LinkHashMaps[2] = {};
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_entity>* DevEntities = &DevEntitiesArray[WorldIndex];
        ak_hash_map<ak_char*, ak_u64>* LinkHashMap = &LinkHashMaps[WorldIndex];
        ak_hash_map<ak_char*, ak_bool>* EntityNameCollisionMap = &EntityNameCollisionMaps[WorldIndex];
        ak_hash_map<ak_char*, ak_bool>* LightNameCollisionMap = &LightNameCollisionMaps[WorldIndex];
        
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
            
            dev_entity* DevEntity = Internal__CreateDevEntity(DevEntities, Name.Data, Type, Position, Orientation, Scale, Material, *MeshID);
            
            EntityNameCollisionMap->Insert(DevEntity->Name, true);
            if(!AK_StringIsNullOrEmpty(LinkName))
            {
                LinkHashMap->Insert(LinkName.Data, DevEntity->ID);
            }
            
            if(Type == ENTITY_TYPE_BUTTON)
                DevEntity->IsToggled = WorldFileStream.CopyConsume<ak_bool>();
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_hash_map<ak_char*, ak_u64>* LinkHashMap = &LinkHashMaps[!WorldIndex];
        ak_pool<dev_entity>* DevEntities = &DevEntitiesArray[WorldIndex];
        AK_ForEach(DevEntity, DevEntities)
        {
            ak_u64* LinkID = LinkHashMap->Find(DevEntity->Name);
            if(LinkID) DevEntity->LinkID = *LinkID;
        }
    }
    
    AK_DeleteHashMap(&LinkHashMaps[0]);
    AK_DeleteHashMap(&LinkHashMaps[1]);
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_pool<dev_point_light>* DevPointLights = &DevPointLightsArray[WorldIndex];
        ak_hash_map<ak_char*, ak_bool>* LightNameCollisionMap = &LightNameCollisionMaps[WorldIndex];
        for(ak_u32 PointLightIndex = 0; PointLightIndex < Header->PointLightCount[WorldIndex]; 
            PointLightIndex++)
        {
            ak_string Name = Internal__ReadAssetName(&WorldFileStream, Scratch);
            ak_color3f Color = WorldFileStream.CopyConsume<ak_color3f>();
            ak_f32 Intensity = WorldFileStream.CopyConsume<ak_f32>();
            ak_v3f Position = WorldFileStream.CopyConsume<ak_v3f>();
            ak_f32 Radius = WorldFileStream.CopyConsume<ak_f32>();
            LightNameCollisionMap->Insert(Name.Data, true);
            
            Internal__CreateDevPointLight(DevPointLights, Name.Data, Position, Radius, Color, Intensity);
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
    
    WorldManagement->DeleteAll();
    
    
    WorldManagement->DevEntities[0] = DevEntitiesArray[0];
    WorldManagement->DevEntities[1] = DevEntitiesArray[1];
    WorldManagement->DevPointLights[0] = DevPointLightsArray[0];
    WorldManagement->DevPointLights[1] = DevPointLightsArray[1];
    
    WorldManagement->EntityNameCollisionMap[0] = EntityNameCollisionMaps[0];
    WorldManagement->EntityNameCollisionMap[1] = EntityNameCollisionMaps[1];
    WorldManagement->LightNameCollisionMap[0] = LightNameCollisionMaps[0];
    WorldManagement->LightNameCollisionMap[1] = LightNameCollisionMaps[1];
    
    WorldManagement->CurrentWorldPath = AK_PushString(WorldDirectoryPath);
    WorldManagement->CurrentWorldName = AK_PushString(WorldName);
    
    ak_string ErrorMessage = Internal__BuildWorld(Scratch, WorldManagement, DevPlatform, Assets);
    if(!AK_StringIsNullOrEmpty(ErrorMessage))
    {
        ak_string Message = AK_FormatString(Scratch, "Failed to load world. Building of the world failed.\n Compiler errors: \n%.*s", ErrorMessage.Length, ErrorMessage.Data);
        WorldManagement->DeleteAll();
        Editor_DebugLog(ErrorMessage);
        AK_MessageBoxOk("Failed to load world", Message);
        return false;
    }
    
    
    
#undef DELETE_TEMP_DATA
    
    return true;
}

void Internal__DeleteWorld(ak_arena* Scratch, ak_string WorldName, dev_platform* DevPlatform)
{
    ak_string WorldDirectoryPath = Internal__GetWorldDirectoryPath(Scratch, WorldName);
    
    ak_string WorldGeneratedPath = AK_StringConcat(WorldDirectoryPath, "generated.h", Scratch);
    ak_string WorldHeaderPath = AK_FormatString(Scratch, "%s%.*s.h", WorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    ak_string WorldSourcePath = 
        AK_FormatString(Scratch, "%s%.*s.cpp", WorldDirectoryPath.Data, WorldName.Length, 
                        WorldName.Data);
    ak_string WorldAssetPath = AK_FormatString(Scratch, "%s%.*s.world", WorldDirectoryPath.Data, 
                                               WorldName.Length, WorldName.Data);
    
    if(AK_FileExists(WorldGeneratedPath)) AK_FileRemove(WorldGeneratedPath);
    if(AK_FileExists(WorldHeaderPath)) AK_FileRemove(WorldHeaderPath);
    if(AK_FileExists(WorldSourcePath)) AK_FileRemove(WorldSourcePath);
    if(AK_FileExists(WorldAssetPath)) AK_FileRemove(WorldAssetPath);
    
    AK_DirectoryRemove(WorldDirectoryPath);
    
    DevPlatform->DeleteWorldFiles(WorldName);
}

void world_management::Update(editor* Editor, dev_platform* DevPlatform, assets* Assets)
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
                    if(Internal__CreateNewWorld(Scratch, this, WorldName, DevPlatform, Assets))
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
                    if(Internal__LoadWorld(Scratch, this, WorldName, Assets, DevPlatform))
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
            ak_string ErrorMessage = Internal__BuildWorld(Scratch, this, DevPlatform, Assets);
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
                    Internal__DeleteWorld(Scratch, WorldName, DevPlatform);
                    
                    NewState = WORLD_MANAGEMENT_STATE_NONE;
                    ImGui::CloseCurrentPopup();
                    
                    if(AK_StringEquals(WorldName, CurrentWorldName))
                        DeleteStrings();
                    
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

void world_management::DeleteStrings()
{
    AK_FreeString(CurrentWorldPath);
    AK_FreeString(CurrentWorldName);
    
    CurrentWorldPath = AK_CreateEmptyString();
    CurrentWorldName = AK_CreateEmptyString();
}

void world_management::DeleteAll()
{
    DeleteIndices();
    DeleteStrings();
    AK_DeletePool(&DevEntities[0]);
    AK_DeletePool(&DevEntities[1]);
    AK_DeletePool(&DevPointLights[0]);
    AK_DeletePool(&DevPointLights[1]);
    AK_DeleteHashMap(&EntityNameCollisionMap[0]);
    AK_DeleteHashMap(&EntityNameCollisionMap[1]);
    AK_DeleteHashMap(&LightNameCollisionMap[0]);
    AK_DeleteHashMap(&LightNameCollisionMap[1]);
    
}


dev_entity* world_management::CreateDevEntity(ak_u32 WorldIndex, ak_char* Name, entity_type Type, ak_v3f Position, 
                                              ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    return Internal__CreateDevEntity(&DevEntities[WorldIndex], Name, Type, Position, Axis, Angle, Scale, Material, MeshID);
}

dual_dev_entity world_management::CreateDevEntityInBothWorlds(ak_char* Name, entity_type Type, ak_v3f Position, 
                                                              ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    dual_dev_entity Result;
    Result.EntityA = CreateDevEntity(0, Name, Type, Position, Axis, Angle, Scale, Material, MeshID);
    Result.EntityB = CreateDevEntity(1, Name, Type, Position, Axis, Angle, Scale, Material, MeshID);
    return Result;
}

dev_point_light* world_management::CreateDevPointLight(ak_u32 WorldIndex, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    return Internal__CreateDevPointLight(&DevPointLights[WorldIndex], Name, Position, Radius, Color, Intensity);
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