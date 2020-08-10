void ReadFrame(dev_context* DevContext, u32 FrameIndex)
{
    game* Game = DevContext->Game;
    frame_recording* Recording = &DevContext->FrameRecording;
    
    ASSERT(FrameIndex < Recording->TotalLoadedFrames);
    
    frame* Frames = (frame*)(Recording->RecordingBuffer.Data + sizeof(u32));        
    frame* Frame = Frames + FrameIndex;
    
    ASSERT(Frame->dt != 0);
    
    Game->dt = Frame->dt;
    *Game->Input = Frame->Input;
    
    world_entity* PlayerEntity0 = GetWorld(Game, 0)->PlayerEntity;
    world_entity* PlayerEntity1 = GetWorld(Game, 1)->PlayerEntity;
    
    PlayerEntity0->Position       = Frame->PlayerPosition[0];
    PlayerEntity0->Orientation    = Frame->PlayerOrientation[0];
    PlayerEntity0->Velocity       = Frame->PlayerVelocity[0];
    PlayerEntity0->CollidedNormal = Frame->CollidedNormal[0];
    
    PlayerEntity1->Position       = Frame->PlayerPosition[1];
    PlayerEntity1->Orientation    = Frame->PlayerOrientation[1];
    PlayerEntity1->Velocity       = Frame->PlayerVelocity[1];
    PlayerEntity1->CollidedNormal = Frame->CollidedNormal[1];
}

void DevelopmentFrameRecording(dev_context* DevContext)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;    
    
    frame_recording* FrameRecording = &DevContext->FrameRecording;
    {        
        frame_recording_state LastState = FrameRecording->RecordingState;
        if(FrameRecording->RecordingState == FRAME_RECORDING_STATE_NONE || FrameRecording->RecordingState == FRAME_RECORDING_STATE_RECORDING)
        {                           
            char* RecordingText = (FrameRecording->RecordingState == FRAME_RECORDING_STATE_RECORDING) ? "Stop Recording" : "Start Recording";                    
            if(ImGui::Button(RecordingText)) 
            {                
                FrameRecording->RecordingState = (FrameRecording->RecordingState == FRAME_RECORDING_STATE_RECORDING) ? FRAME_RECORDING_STATE_NONE : FRAME_RECORDING_STATE_RECORDING;                
                if(FrameRecording->RecordingState == FRAME_RECORDING_STATE_RECORDING)
                {
                    string RecordingPath = Platform_FindNewFrameRecordingPath();
                    if(!IsInvalidString(RecordingPath))
                    {
                        CopyToStorage(&FrameRecording->RecordingPath, RecordingPath);
                    }
                }
                else
                {                    
                    ptr DataSize = sizeof(u32)+sizeof(frame)*FrameRecording->RecordedFrames.Size;
                    void* Data = PushSize(DataSize, NoClear, 0);                    
                    *(u32*)Data = FrameRecording->RecordedFrames.Size;
                    CopyMemory((u32*)Data+1, FrameRecording->RecordedFrames.Data, sizeof(frame)*FrameRecording->RecordedFrames.Size);                    
                    Global_Platform->WriteEntireFile(FrameRecording->RecordingPath.String.Data, Data, (u32)DataSize);
                    
                    FrameRecording->RecordedFrames.Size = 0;
                    
                    if(!IsInvalidBuffer(FrameRecording->RecordingBuffer))
                        Global_Platform->FreeFileMemory(&FrameRecording->RecordingBuffer);            
                    
                    FrameRecording->RecordingBuffer = Global_Platform->ReadEntireFile(FrameRecording->RecordingPath.String.Data);
                }
            }            
            
            
            if(FrameRecording->RecordingState == FRAME_RECORDING_STATE_RECORDING)
            {
                ImGui::SameLine();
                ImGui::Text("Recording File: %s\n", FrameRecording->RecordingPath.String.Data);                    
            }
        }
        
        if(FrameRecording->RecordingState == FRAME_RECORDING_STATE_NONE)
        {            
            if(ImGui::Button("Load Recording"))
            {
                string RecordingPath = Platform_OpenFileDialog("arc_recording");
                if(!IsInvalidString(RecordingPath))
                {
                    if(!IsInvalidBuffer(FrameRecording->RecordingBuffer))
                        Global_Platform->FreeFileMemory(&FrameRecording->RecordingBuffer);            
                    
                    FrameRecording->RecordingBuffer = Global_Platform->ReadEntireFile(RecordingPath.Data);
                    CopyToStorage(&FrameRecording->RecordingPath, RecordingPath);
                }
            }
            
            if(!IsInvalidBuffer(FrameRecording->RecordingBuffer) && !IsInvalidString(FrameRecording->RecordingPath.String))
            {
                ImGui::SameLine();
                ImGui::Text("Recording File: %s\n", FrameRecording->RecordingPath.String.Data);
            }
        }
        
        if(!IsInvalidBuffer(FrameRecording->RecordingBuffer) && (FrameRecording->RecordingState != FRAME_RECORDING_STATE_RECORDING))
        {                
            char* PlayRecordingText = (((FrameRecording->RecordingState == FRAME_RECORDING_STATE_PLAYING) || 
                                        (FrameRecording->RecordingState == FRAME_RECORDING_STATE_INSPECT_FRAMES))
                                       ? "Stop Playing" : "Start Playing");
            
            if(ImGui::Button(PlayRecordingText)) 
            {                
                if((FrameRecording->RecordingState != FRAME_RECORDING_STATE_PLAYING) &&
                   (FrameRecording->RecordingState != FRAME_RECORDING_STATE_INSPECT_FRAMES))
                {            
                    FrameRecording->RecordingState = FRAME_RECORDING_STATE_PLAYING;
                    FrameRecording->TotalLoadedFrames = *(u32*)FrameRecording->RecordingBuffer.Data;                                                
                }
                else
                {                    
                    FrameRecording->RecordingState = FRAME_RECORDING_STATE_NONE;
                    ReadFrame(DevContext, 0);                        
                    *Game->Input = {};
                    FrameRecording->CurrentFrameIndex = 0;                                                                                             
                }
            }
            
            if((FrameRecording->RecordingState == FRAME_RECORDING_STATE_PLAYING) || (FrameRecording->RecordingState == FRAME_RECORDING_STATE_INSPECT_FRAMES))
            {
                ImGui::SameLine();
                ImGui::Text("Recording File: %s\n", FrameRecording->RecordingPath.String.Data);                   
                
                char* InspectText = (FrameRecording->RecordingState == FRAME_RECORDING_STATE_INSPECT_FRAMES) ? "Stop Inspecting" : "Start Inspecting";
                if(ImGui::Button(InspectText))
                {
                    if(FrameRecording->RecordingState == FRAME_RECORDING_STATE_INSPECT_FRAMES)
                        FrameRecording->RecordingState = FRAME_RECORDING_STATE_PLAYING;
                    else
                        FrameRecording->RecordingState = FRAME_RECORDING_STATE_INSPECT_FRAMES;
                }
                
                ImGui::SameLine();
                ImGui::Text("Frame %d/%d", FrameRecording->CurrentFrameIndex, FrameRecording->TotalLoadedFrames-1);                 
                
                ImGuiIO* IO = &ImGui::GetIO();
                
                if(ImGui::IsKeyPressedMap(ImGuiKey_LeftArrow))
                {
                    if(FrameRecording->CurrentFrameIndex > 0)
                        FrameRecording->CurrentFrameIndex--;                    
                }
                
                if(ImGui::IsKeyPressedMap(ImGuiKey_RightArrow))
                {
                    FrameRecording->CurrentFrameIndex++;                    
                    if(FrameRecording->CurrentFrameIndex == FrameRecording->TotalLoadedFrames)
                        FrameRecording->CurrentFrameIndex = FrameRecording->TotalLoadedFrames-1;
                }
            }
        }                
    }
    
}
void DevelopmentRecordFrame(dev_context* DevContext, game* Game)
{
    frame_recording* Recording = &DevContext->FrameRecording;
    if(Recording->RecordingState == FRAME_RECORDING_STATE_RECORDING)
    {   
        frame Frame;        
        
        input* Input = Game->Input;
        
        ASSERT(Game->dt != 0);
        
        Frame.dt = Game->dt;
        Frame.Input = *Input; 
        
        world_entity* PlayerEntity0 = GetWorld(Game, 0)->PlayerEntity;
        world_entity* PlayerEntity1 = GetWorld(Game, 1)->PlayerEntity;
        
        Frame.PlayerPosition[0] = PlayerEntity0->Position;
        Frame.PlayerPosition[1] = PlayerEntity1->Position;
        
        Frame.PlayerOrientation[0] = PlayerEntity0->Orientation;
        Frame.PlayerOrientation[1] = PlayerEntity1->Orientation;
        
        Frame.PlayerVelocity[0] = PlayerEntity0->Velocity;
        Frame.PlayerVelocity[1] = PlayerEntity1->Velocity;
        
        Frame.CollidedNormal[0] = PlayerEntity0->CollidedNormal;
        Frame.CollidedNormal[1] = PlayerEntity1->CollidedNormal;
        
        Append(&Recording->RecordedFrames, Frame);
    }
}

void DevelopmentPlayFrame(dev_context* DevContext, game* Game)
{
    frame_recording* Recording = &DevContext->FrameRecording;
    if((Recording->RecordingState == FRAME_RECORDING_STATE_PLAYING) ||
       (Recording->RecordingState == FRAME_RECORDING_STATE_INSPECT_FRAMES))
    {        
        ReadFrame(DevContext, Recording->CurrentFrameIndex);                
        if(Recording->RecordingState == FRAME_RECORDING_STATE_PLAYING)
        {        
            Recording->CurrentFrameIndex++;
            if(Recording->CurrentFrameIndex >= Recording->TotalLoadedFrames)
                Recording->CurrentFrameIndex = 0;        
        }
    }    
}