ak_array<ak_string> Internal__GetAllRecordingsPath(ak_arena* Scratch)
{
    ak_array<ak_string> Result = {};
    
    ak_array<ak_string> Files = AK_GetAllFilesInDirectory(RECORDINGS_PATH, Scratch);
    AK_ForEach(File, &Files)
    {
        ak_string Ext = AK_GetFileExtension(*File);
        ak_string Filename = AK_GetFilenameWithoutExtension(*File);
        if(AK_FileExists(*File) && AK_StringEquals(Ext, "recording"))
            Result.Add(Filename);
    }
    
    AK_DeleteArray(&Files);
    
    return Result;
}

ak_string Internal__GetRecordingPathFromName(ak_arena* Scratch, ak_string Name)
{
    ak_string RecordingPath = AK_StringConcat(RECORDINGS_PATH, Name, Scratch);
    RecordingPath = AK_StringConcat(RecordingPath, ".recording", Scratch);
    return RecordingPath;
}

ak_string Internal__ValidateRecordingFile(frame_reader* Reader, ak_arena* Scratch)
{
    ak_stream* Stream = &Reader->Stream;
    recording_file_header* FileHeader = Stream->PeekConsume<recording_file_header>();
    
    ak_u32 Current = Stream->At;
    
    ak_char Checksum[AK_Count(RECORDING_FILE_CHECKSUM)];
    Stream->SetOffsetFromEnd(AK_Count(RECORDING_FILE_CHECKSUM)-1);
    AK_MemoryCopy(Checksum, Stream->PeekConsume(AK_Count(RECORDING_FILE_CHECKSUM)), 
                  AK_Count(RECORDING_FILE_CHECKSUM));
    
    if(!AK_StringEquals(FileHeader->Signature, RECORDING_FILE_SIGNATURE))
        return AK_PushString("Failed to validate the frame recording signature", Scratch);
    
    if(!AK_StringEquals(Checksum, RECORDING_FILE_CHECKSUM))
        return AK_PushString("Failed to validate the frame recording checksum", Scratch);
    
    if(FileHeader->MajorVersion != RECORDING_FILE_MAJOR_VERSION ||
       FileHeader->MinorVersion != RECORDING_FILE_MINOR_VERSION)
    {
        return AK_FormatString(Scratch, "Cannot load frame recording of version %d.%d, current version of the editor only supports %d.%d", FileHeader->MajorVersion,
                               FileHeader->MinorVersion, 
                               RECORDING_FILE_MAJOR_VERSION, 
                               RECORDING_FILE_MINOR_VERSION);
    }
    
    Stream->SetOffset(Current);
    Reader->FileHeader = FileHeader;
    return AK_CreateEmptyString();
}

void frame_playback::Update(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform, dev_platform* DevPlatform)
{
    ak_arena* Scratch = Editor->Scratch;
    world_management* WorldManagement = &Editor->WorldManagement;
    
    ak_array<ak_string> Recordings = Internal__GetAllRecordingsPath(Scratch);
    
    frame_playback_state TempState = NewState;
    
    switch(NewState)
    {
        case FRAME_PLAYBACK_STATE_NONE:
        {
            if(ImGui::Button("Start Recording"))
            {
                NewState = FRAME_PLAYBACK_STATE_CREATE;
            }
            
            if(Recordings.Size == 0)
                UI_PushDisabledItem();
            
            if(ImGui::Button("Load Recording"))
            {
                NewState = FRAME_PLAYBACK_STATE_LOAD;
            }
            
            if(Recordings.Size == 0)
                UI_PopDisabledItem();
            
            if(AK_StringIsNullOrEmpty(CurrentRecordingName))
                UI_PushDisabledItem();
            
            ak_string RecordingPath = AK_CreateEmptyString();
            if(!AK_StringIsNullOrEmpty(CurrentRecordingName))
                RecordingPath = Internal__GetRecordingPathFromName(Scratch, CurrentRecordingName);
            
            if(ImGui::Button("Play Recording"))
            {
                if(!Reader.Arena)
                    Reader.Arena = AK_CreateArena();
                
                ak_buffer Buffer = AK_ReadEntireFile(RecordingPath, Reader.Arena);
                
                ak_string ErrorMessage = AK_CreateEmptyString();
                if(Buffer.IsValid())
                {
                    Reader.Stream = AK_BeginStream(Buffer);
                    ak_stream* Stream = &Reader.Stream;
                    ErrorMessage = Internal__ValidateRecordingFile(&Reader, Scratch);
                    
                    if(AK_StringIsNullOrEmpty(ErrorMessage))
                    {
                        ak_string Name = Internal__ReadAssetName(Stream, Scratch);
                        if(!AK_StringEquals(WorldManagement->CurrentWorldName, Name))
                        {
                            ak_string Message = AK_FormatString(Scratch, "World '%.*s' in recording file is not the same as the currently loaded world '%.*s'. Would you like to load the new one?", Name.Length, Name.Data, WorldManagement->CurrentWorldName.Length, 
                                                                WorldManagement->CurrentWorldName.Data);
                            
                            if(AK_MessageBoxYesNo("Message", Message))
                            {
                                Editor_StopGame(Editor, Platform);
                                if(Internal__LoadWorld(Editor->Scratch, WorldManagement, 
                                                       Name, Assets, DevPlatform))
                                {
                                    Editor_PlayGame(Editor, Graphics, Assets, Platform, DevPlatform);
                                    NewState = FRAME_PLAYBACK_STATE_PLAYING;
                                    Reader.FrameHeaders = Stream->Peek<frame_header>();
                                }
                            }
                        }
                        else
                        {
                            NewState = FRAME_PLAYBACK_STATE_PLAYING;
                            Reader.FrameHeaders = Stream->Peek<frame_header>();
                        }
                    }
                }
                else
                {
                    ErrorMessage = AK_FormatString(Scratch, "Failed to load recording file %.*s", RecordingPath.Length, RecordingPath.Data);
                }
                
                if(!AK_StringIsNullOrEmpty(ErrorMessage))
                {
                    AK_MessageBoxOk("Error", AK_FormatString(Scratch, "Error loading recording %.*s", ErrorMessage.Length, ErrorMessage.Data));
                }
                
                if(NewState == FRAME_PLAYBACK_STATE_NONE)
                    Reader.Clear();
            }
            
            ImGui::SameLine();
            ImGui::Text("Recording: %.*s", CurrentRecordingName.Length, CurrentRecordingName.Data);
            
            if(AK_StringIsNullOrEmpty(CurrentRecordingName))
                UI_PopDisabledItem();
        } break;
        
        case FRAME_PLAYBACK_STATE_LOAD:
        {
            if(!ImGui::IsPopupOpen("Load Recordings"))
                ImGui::OpenPopup("Load Recordings");
            
            local ak_u32 CurrentRecordingIndex = (ak_u32)-1;
            
            if(OldState != NewState)
                CurrentRecordingIndex = (ak_u32)-1;
            
            if(ImGui::BeginPopupModal("Load Recordings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Recording List");
                ImGui::Separator();
                
                if(!AK_StringIsNullOrEmpty(CurrentRecordingName) && 
                   CurrentRecordingIndex == (ak_u32)-1)
                {
                    ak_u32 Index = 0;
                    AK_ForEach(Recording, &Recordings)
                    {
                        if(AK_StringEquals(*Recording, CurrentRecordingName))
                            CurrentRecordingIndex = Index;
                        
                        Index++;
                    }
                }
                
                ak_u32 Index = 0;
                AK_ForEach(Recording, &Recordings)
                {
                    ak_char* Text = AK_FormatString(Editor->Scratch, "%d. %.*s", Index+1, 
                                                    Recording->Length, Recording->Data).Data;
                    ak_bool Selected = CurrentRecordingIndex == Index;
                    if(ImGui::Selectable(Text, Selected))
                    {
                        if(CurrentRecordingIndex == Index)
                            CurrentRecordingIndex = (ak_u32)-1;
                        else
                            CurrentRecordingIndex = Index;
                    }
                    
                    Index++;
                }
                
                ImGui::Separator();
                if(CurrentRecordingIndex == (ak_u32)-1)
                    UI_PushDisabledItem();
                
                if(ImGui::Button("Load"))
                {
                    ak_string RecordingName = Recordings[CurrentRecordingIndex];
                    
                    if(AK_StringIsNullOrEmpty(RecordingName))
                        AK_FreeString(RecordingName);
                    
                    CurrentRecordingName = AK_PushString(RecordingName);
                    
                    NewState = FRAME_PLAYBACK_STATE_NONE;
                    ImGui::CloseCurrentPopup();
                }
                
                if(CurrentRecordingIndex == (ak_u32)-1)
                    UI_PopDisabledItem();
                
                if(ImGui::Button("Close"))
                {
                    NewState = FRAME_PLAYBACK_STATE_NONE;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
        } break;
        
        case FRAME_PLAYBACK_STATE_CREATE:
        {
            if(!ImGui::IsPopupOpen("Create Recordings"))
                ImGui::OpenPopup("Create Recordings");
            
            if(ImGui::BeginPopupModal("Create Recordings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                local ak_char RecordingNameData[MAX_RECORDING_NAME] = {};
                if(OldState != NewState)
                    AK_MemoryClear(RecordingNameData, sizeof(RecordingNameData));
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Recording Name");
                ImGui::InputText("", RecordingNameData, MAX_RECORDING_NAME, ImGuiInputTextFlags_CharsNoBlank);
                
                ak_string RecordingName = AK_CreateString(RecordingNameData);
                if(AK_StringIsNullOrEmpty(RecordingName))
                    UI_PushDisabledItem();
                
                ak_bool ShowDuplicateRecordingsText = false;
                AK_ForEach(Recording, &Recordings)
                {
                    if(AK_StringEquals(*Recording, RecordingName)) ShowDuplicateRecordingsText = true; 
                }
                
                if(ImGui::Button("Create"))
                {
                    NewState = FRAME_PLAYBACK_STATE_RECORDING;
                    ImGui::CloseCurrentPopup();
                    
                    if(!AK_StringIsNullOrEmpty(CurrentRecordingName))
                        AK_FreeString(CurrentRecordingName);
                    
                    CurrentRecordingName = AK_PushString(RecordingName);
                    
                    recording_file_header* FileHeader = Writer.HeaderBuilder.Allocate<recording_file_header>();
                    
                    AK_MemoryCopy(FileHeader->Signature, RECORDING_FILE_SIGNATURE, sizeof(FileHeader->Signature));
                    FileHeader->MajorVersion = RECORDING_FILE_MAJOR_VERSION;
                    FileHeader->MinorVersion = RECORDING_FILE_MINOR_VERSION;
                    
                    Internal__WriteName(&Writer.HeaderBuilder, WorldManagement->CurrentWorldName.Data, (ak_u8)WorldManagement->CurrentWorldName.Length);
                    
                    Writer.FileHeader = FileHeader;
                }
                
                ImGui::SameLine();
                
                if(AK_StringIsNullOrEmpty(RecordingName))
                    UI_PopDisabledItem();
                
                if(ImGui::Button("Close"))
                {
                    NewState = FRAME_PLAYBACK_STATE_NONE;
                    ImGui::CloseCurrentPopup();
                }
                
                if(ShowDuplicateRecordingsText)
                {
                    UI_ErrorText("Recording with name '%.*s' already exists, cannot create recording without deleting already existing recording or renaming new one", RecordingName.Length, RecordingName.Data);
                }
                
                ImGui::EndPopup();
            }
        } break;
        
        case FRAME_PLAYBACK_STATE_RECORDING:
        {
            AK_Assert(!AK_StringIsNullOrEmpty(CurrentRecordingName), "Recording name must be present in order to be in the recording state");
            
            ak_string Filepath = Internal__GetRecordingPathFromName(Editor->Scratch, CurrentRecordingName);
            
            if(ImGui::Button("Stop Recording"))
            {
                ak_buffer HeaderBuffer = Writer.HeaderBuilder.PushBuffer(Editor->Scratch);
                ak_buffer DataBuffer = Writer.DataBuilder.PushBuffer(Editor->Scratch);
                
                frame_header* FrameHeader = (frame_header*)(HeaderBuffer.Data + sizeof(recording_file_header) + 
                                                            sizeof(ak_u8)+WorldManagement->CurrentWorldName.Length);
                
                ak_u32 OffsetToData = AK_SafeU32(HeaderBuffer.Size);
                for(ak_u32 FrameIndex = 0; FrameIndex < Writer.FileHeader->FrameCount; FrameIndex++)
                {
                    FrameHeader[FrameIndex].OffsetToData = OffsetToData;
                    OffsetToData += FrameHeader->DataSize;
                }
                
                ak_file_handle* FileHandle = AK_OpenFile(Filepath, AK_FILE_ATTRIBUTES_WRITE);
                AK_WriteFile(FileHandle, HeaderBuffer.Data, AK_SafeU32(HeaderBuffer.Size));
                AK_WriteFile(FileHandle, DataBuffer.Data, AK_SafeU32(DataBuffer.Size));
                AK_WriteFile(FileHandle, RECORDING_FILE_CHECKSUM, sizeof(RECORDING_FILE_CHECKSUM));
                AK_CloseFile(FileHandle);
                
                Writer.HeaderBuilder.ReleaseMemory();
                Writer.DataBuilder.ReleaseMemory();
                
                NewState = FRAME_PLAYBACK_STATE_NONE;
            }
            
            if(NewState != FRAME_PLAYBACK_STATE_NONE)
            {
                ImGui::SameLine();
                ImGui::Text("Recording: %.*s", CurrentRecordingName.Length, CurrentRecordingName.Data);
                ImGui::Text("Frames: %d", Writer.FileHeader->FrameCount);
            }
        } break;
        
        case FRAME_PLAYBACK_STATE_INSPECTING:
        {
            if(ImGui::Button("Stop Playing"))
            {
                Reader.CurrentFrameIndex = 0;
                PlayFrame(Editor);
                
                NewState = FRAME_PLAYBACK_STATE_NONE;
                AK_DeleteArena(Reader.Arena);
                Reader.Arena = NULL;
            }
            
            if(ImGui::Button("Stop Inspecting"))
                NewState = FRAME_PLAYBACK_STATE_INSPECTING;
            
            if(NewState != FRAME_PLAYBACK_STATE_NONE)
            {
                ImGui::SameLine();
                ImGui::Text("Recording: %.*s", CurrentRecordingName.Length, 
                            CurrentRecordingName.Data);
                ImGui::Text("Frames %d/%d", Reader.CurrentFrameIndex, Reader.FileHeader->FrameCount-1);
            }
            
            ImGuiIO* IO = &ImGui::GetIO();
            
            if(ImGui::IsKeyPressedMap(ImGuiKey_LeftArrow))
            {
                if(Reader.CurrentFrameIndex == 0)
                    Reader.CurrentFrameIndex = Reader.FileHeader->FrameCount-1;
                else
                    Reader.CurrentFrameIndex--;
            }
            
            if(ImGui::IsKeyPressedMap(ImGuiKey_RightArrow))
            {
                if(Reader.CurrentFrameIndex == (Reader.FileHeader->FrameCount-1))
                    Reader.CurrentFrameIndex = 0;
                else
                    Reader.CurrentFrameIndex++;
            }
            
        } break;
        
        case FRAME_PLAYBACK_STATE_PLAYING:
        {
            if(ImGui::Button("Stop Playing"))
            {
                Reader.CurrentFrameIndex = 0;
                PlayFrame(Editor);
                
                NewState = FRAME_PLAYBACK_STATE_NONE;
                AK_DeleteArena(Reader.Arena);
                Reader.Arena = NULL;
            }
            
            if(ImGui::Button("Start Inspecting"))
                NewState = FRAME_PLAYBACK_STATE_INSPECTING;
            
            if(NewState != FRAME_PLAYBACK_STATE_NONE)
            {
                ImGui::SameLine();
                ImGui::Text("Recording: %.*s", CurrentRecordingName.Length, 
                            CurrentRecordingName.Data);
                ImGui::Text("Frames %d/%d", Reader.CurrentFrameIndex, Reader.FileHeader->FrameCount-1);
            }
        } break;
    }
    
    AK_DeleteArray(&Recordings);
    OldState = TempState;
}

void frame_playback::RecordFrame(editor* Editor)
{
    game* Game = Editor->Game;
    
    
    ak_binary_builder* HeaderBuilder = &Writer.HeaderBuilder;
    ak_binary_builder* DataBuilder = &Writer.DataBuilder;
    
    world* World = Game->World;
    
    frame_header* FrameHeader = HeaderBuilder->Allocate<frame_header>();
    FrameHeader->Input = Game->Input;
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        AK_ForEach(Entity, &World->EntityStorage[WorldIndex])
        {
            if(Entity->Type != ENTITY_TYPE_STATIC)
            {
                ak_u32 Index = AK_PoolIndex(Entity->ID);
                ak_char* Name = Editor->GameEntityNames[WorldIndex][Index];
                physics_object* PhysicsObject = &World->PhysicsObjects[WorldIndex][Index];
                
                ak_u32 NameSize = Internal__WriteName(DataBuilder, Name, (ak_u8)AK_StringLength(Name));
                
                ak_u32* ObjectSize = DataBuilder->Allocate<ak_u32>();
                *ObjectSize += DataBuilder->Write(PhysicsObject->Position);
                *ObjectSize += DataBuilder->Write(PhysicsObject->Orientation);
                *ObjectSize += DataBuilder->Write(PhysicsObject->Scale);
                *ObjectSize += DataBuilder->Write(PhysicsObject->Velocity);
                
                switch(Entity->Type)
                {
                    case ENTITY_TYPE_PLAYER:
                    {
                        player* Player = World->Players + WorldIndex;
                        *ObjectSize += DataBuilder->Write(Player->GravityVelocity);
                    } break;
                }
                
                FrameHeader->DataSize += *ObjectSize + NameSize + sizeof(ak_u32);
                FrameHeader->EntityCount[WorldIndex]++;
            }
        }
    }
    
    Writer.FileHeader->FrameCount++;
}

void frame_playback::PlayFrame(editor* Editor)
{
    frame_header* FrameHeader = Reader.FrameHeaders + Reader.CurrentFrameIndex;
    ak_stream* Stream = &Reader.Stream;
    
    world_management* WorldManagement = &Editor->WorldManagement;
    
    Stream->SetOffset(FrameHeader->OffsetToData);
    
    Editor->Game->Input = FrameHeader->Input;
    world* World = Editor->Game->World;
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        for(ak_u32 EntityIndex = 0; EntityIndex < FrameHeader->EntityCount[WorldIndex]; EntityIndex++)
        {
            ak_string Name = Internal__ReadAssetName(Stream, Editor->Scratch);
            ak_u32 ObjectSize = Stream->CopyConsume<ak_u32>();
            
            ak_u64* EntityID = Editor->GameEntityNameHash[WorldIndex].Find(Name.Data);
            if(!EntityID)
            {
                Stream->Consume(ObjectSize);
            }
            else
            {
                ak_u32 Index = AK_PoolIndex(*EntityID);
                physics_object* PhysicsObject = World->PhysicsObjects[WorldIndex].Get(Index);
                
                PhysicsObject->Position = Stream->CopyConsume<ak_v3f>();
                PhysicsObject->Orientation = Stream->CopyConsume<ak_quatf>();
                PhysicsObject->Scale = Stream->CopyConsume<ak_v3f>();
                PhysicsObject->Velocity = Stream->CopyConsume<ak_v3f>();
                
                entity_type Type = World->EntityStorage[WorldIndex].Get(*EntityID)->Type;
                switch(Type)
                {
                    case ENTITY_TYPE_PLAYER:
                    {
                        player* Player = World->Players + WorldIndex;
                        Player->GravityVelocity = Stream->CopyConsume<ak_v3f>();
                    } break;
                }
            }
        }
    }
    
    if(NewState == FRAME_PLAYBACK_STATE_PLAYING)
    {
        Reader.CurrentFrameIndex++;
        if(Reader.CurrentFrameIndex == Reader.FileHeader->FrameCount)
            Reader.CurrentFrameIndex = 0;
    }
}