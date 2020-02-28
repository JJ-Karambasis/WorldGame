/* Original Author: Armand (JJ) Karambasis */

#pragma pack(push, 1)
struct entity_data
{
    b32 IsBlocker;
    sqt Transform;
    c4 Color;
    v3f Velocity;
};
#pragma pack(pop)

inline ptr 
GetFrameOffsetSize(frame_offset_array* Array)
{
    ptr Result = sizeof(ptr)*Array->Count;
    return Result;
}

inline ptr 
GetFrameOffsetArraySize(frame_offset_array* Array)
{
    ptr Result = GetFrameOffsetSize(Array) + sizeof(u32);
    return Result;
}

inline ptr
GetFrameFileOffset(frame_offset_array* Array, u32 FrameIndex)
{
    ptr Result = GetFrameOffsetArraySize(Array) + Array->Ptr[FrameIndex];
    return Result;
}

void WriteGameState(development_game* Game)
{    
}

void ReadGameState(development_game* Game, u32 FrameIndex)
{
    
}

void DevelopmentTick(development_game* Game)
{
    development_input* Input = (development_input*)Game->Input;
    
    frame_recording* Recording = &Game->FrameRecordings;
    if(IsPressed(Input->RecordButton))
    {
        if(Recording->RecordingState == RECORDING_STATE_NONE)
        {
            Recording->RecordingState = RECORDING_STATE_RECORDING;
        }
        else if(Recording->RecordingState == RECORDING_STATE_RECORDING)
        {
            platform_file_handle* File = Global_Platform->OpenFile("frame_recording.data", PLATFORM_FILE_ATTRIBUTES_WRITE);
            ASSERT(File);
            
            Global_Platform->WriteFile(File, &Recording->FrameOffsets.Count, sizeof(u32), NO_OFFSET);
            Global_Platform->WriteFile(File, Recording->FrameOffsets.Ptr, (u32)GetFrameOffsetSize(&Recording->FrameOffsets), NO_OFFSET);
            
            for(arena_block* Block = Recording->FrameStream.First; Block; Block = Block->Next)
            {
                if(Block->Used)
                    Global_Platform->WriteFile(File, Block->Memory, (u32)Block->Used, NO_OFFSET);
            }
            
            Recording->CurrentFrameIndex = 0;                        
            Recording->FrameOffsets.Count = 0;
            Recording->NextFrameOffset = 0;                        
            ResetArena(&Recording->FrameStream);
            Global_Platform->CloseFile(File);
            Recording->RecordingState = RECORDING_STATE_NONE;
        }            
    }
    
    if(IsPressed(Input->PlaybackButton))
    {
        if(Recording->RecordingState == RECORDING_STATE_NONE)
        {            
            Recording->File = Global_Platform->OpenFile("frame_recording.data", PLATFORM_FILE_ATTRIBUTES_READ);
            ASSERT(Recording->File);
            
            Global_Platform->ReadFile(Recording->File, &Recording->FrameOffsets.Count, sizeof(u32), NO_OFFSET);
            ASSERT(Recording->FrameOffsets.Count < Recording->MaxFrameCount);
            Global_Platform->ReadFile(Recording->File, Recording->FrameOffsets.Ptr, sizeof(u32)*Recording->FrameOffsets.Count, NO_OFFSET);                        
            
            Recording->RecordingState = RECORDING_STATE_PLAYBACK;
        }
        else if((Recording->RecordingState == RECORDING_STATE_PLAYBACK) ||
                (Recording->RecordingState == RECORDING_STATE_CYCLE))
        {
            Recording->RecordingState = RECORDING_STATE_NONE;            
            Recording->CurrentFrameIndex = 0; 
        }
    }
    
    if(IsPressed(Input->CycleButton))
    {
        if(Recording->RecordingState == RECORDING_STATE_PLAYBACK)
        {
            Recording->RecordingState = RECORDING_STATE_CYCLE;
        }
        else if(Recording->RecordingState == RECORDING_STATE_CYCLE)
        {
            Recording->RecordingState = RECORDING_STATE_PLAYBACK;
            Recording->CurrentFrameIndex = 0; 
        }
    }
    
    if(IsPressed(Input->CycleLeft) && (Recording->RecordingState == RECORDING_STATE_CYCLE))
    {
        if(Recording->CurrentFrameIndex > 0)
            Recording->CurrentFrameIndex--;        
    }
    
    if(IsPressed(Input->CycleRight) && (Recording->RecordingState == RECORDING_STATE_CYCLE))
    {
        if(Recording->CurrentFrameIndex < Recording->FrameOffsets.Count)
            Recording->CurrentFrameIndex++;
    }
    
    switch(Recording->RecordingState)
    {
        case RECORDING_STATE_RECORDING:
        {
            WriteGameState(Game);
        } break;
        
        case RECORDING_STATE_PLAYBACK:
        {
            if(Recording->CurrentFrameIndex >= Recording->FrameOffsets.Count)
                Recording->CurrentFrameIndex = 0;
            ReadGameState(Game, Recording->CurrentFrameIndex++);
        } break;
        
        case RECORDING_STATE_CYCLE:
        {
            ReadGameState(Game, Recording->CurrentFrameIndex);
        } break;
    }
}