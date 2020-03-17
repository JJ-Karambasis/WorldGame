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
GetFrameOffsetSize(u32 FrameCount)
{
    ptr Result = sizeof(ptr)*FrameCount;
    return Result;
}

inline ptr 
GetFrameOffsetArraySize(u32 FrameCount)
{
    ptr Result = GetFrameOffsetSize(FrameCount) + sizeof(u32);
    return Result;
}

inline ptr
GetFrameFileOffset(ptr* Array, u32 FrameCount, u32 FrameIndex)
{
    ptr Result = GetFrameOffsetArraySize(FrameCount) + Array[FrameIndex];
    return Result;
}

void WriteGameState(development_game* Game)
{
    frame_recording* FrameRecording = &Game->FrameRecordings;
    FrameRecording->FrameOffsets[FrameRecording->FrameCount++] = FrameRecording->NextFrameOffset;
    
    player* Player = &Game->Player;    
    input* Input = Game->Input;
    
    ptr EntrySize = 0;
    PushWriteStruct(&FrameRecording->FrameStream, &Player->Position, v3f, 0); EntrySize += sizeof(v3f);
    PushWriteStruct(&FrameRecording->FrameStream, &Player->Velocity, v3f, 0); EntrySize += sizeof(v3f);
    PushWriteStruct(&FrameRecording->FrameStream, &Input->dt, f32, 0); EntrySize += sizeof(f32);
    PushWriteArray(&FrameRecording->FrameStream, Input->Buttons, ARRAYCOUNT(Input->Buttons), button, 0); EntrySize += sizeof(Input->Buttons);         
    FrameRecording->NextFrameOffset += EntrySize;
}

void ReadGameState(development_game* Game, u32 FrameIndex)
{
    frame_recording* FrameRecording = &Game->FrameRecordings;    
    ptr Offset = GetFrameFileOffset(FrameRecording->FrameOffsets, FrameRecording->FrameCount, FrameIndex);
    
    player* Player = &Game->Player;
    input* Input = Game->Input;
    
#pragma pack(push, 1)
    struct
    {
        v3f Position;
        v3f Velocity;
        f32 dt;
        button Buttons[ARRAYCOUNT(input::Buttons)];
    } Context;
    
#pragma pack(pop)
    platform_file_handle* File = FrameRecording->File;
    ASSERT(Global_Platform->ReadFile(File, &Context, sizeof(Context), (u64)Offset));
    
    Player->Position = Context.Position;
    Player->Velocity = Context.Velocity;
    Input->dt = Context.dt;
    CopyArray(Input->Buttons, Context.Buttons, ARRAYCOUNT(input::Buttons), button);
}

void DrawWalkables(walking_system_recording* WalkingSystem)
{
    const f32 PointSize = 0.05f;
    const f32 LineSize = 0.025f;
    
    WalkingSystem->EventRecordings[0].ShouldRender = true;
    WalkingSystem->EventRecordings[1].ShouldRender = true;
    for(u32 Recording = 0; Recording < ARRAYCOUNT(WalkingSystem->EventRecordings); Recording++)
    {
        walking_event_recording* EventRecording = WalkingSystem->EventRecordings + Recording;
        if(EventRecording->ShouldRender)
        {
            for(u32 EventIndex = 0; EventIndex < EventRecording->EventCount; EventIndex++)
            {
                walking_event* Event = &EventRecording->Events[EventIndex];
                switch(Event->Type)
                {
                    case WALKING_EVENT_TYPE_DRAW_POINT:
                    {
                        DRAW_POINT(Event->DebugPoint.Point, PointSize, Event->DebugPoint.Color);
                    } break;
                    
                    case WALKING_EVENT_TYPE_DRAW_EDGE:
                    {
                        DRAW_LINE(Event->DebugEdge.Point0, Event->DebugEdge.Point1, LineSize, LineSize, Event->DebugEdge.Color);
                    } break;
                }
            }                        
        }        
    }    
}

void DevelopmentTick(development_game* Game)
{            
    if(!Game->DevInitialized)
    {
        Game->FrameRecordings.FrameStream = CreateArena(MEGABYTE(1));
        Game->DevInitialized = true;
    }
    
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
            
            Global_Platform->WriteFile(File, &Recording->FrameCount, sizeof(u32), NO_OFFSET);
            Global_Platform->WriteFile(File, Recording->FrameOffsets, (u32)GetFrameOffsetSize(Recording->FrameCount), NO_OFFSET);
            
            for(arena_block* Block = Recording->FrameStream.First; Block; Block = Block->Next)
            {
                if(Block->Used)
                    Global_Platform->WriteFile(File, Block->Memory, (u32)Block->Used, NO_OFFSET);
            }
            
            Recording->CurrentFrameIndex = 0;                        
            Recording->FrameCount = 0;
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
            
            Global_Platform->ReadFile(Recording->File, &Recording->FrameCount, sizeof(u32), NO_OFFSET);
            ASSERT(Recording->FrameCount < Recording->MaxFrameCount);
            Global_Platform->ReadFile(Recording->File, Recording->FrameOffsets, sizeof(ptr)*Recording->FrameCount, NO_OFFSET);                        
            
            Recording->RecordingState = RECORDING_STATE_PLAYBACK;
        }
        else if((Recording->RecordingState == RECORDING_STATE_PLAYBACK) ||
                (Recording->RecordingState == RECORDING_STATE_CYCLE))
        {
            ReadGameState(Game, 0);
            Recording->RecordingState = RECORDING_STATE_NONE;            
            Recording->CurrentFrameIndex = 0; 
            Recording->FrameCount = 0;            
            Global_Platform->CloseFile(Recording->File);
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
        if(Recording->CurrentFrameIndex < Recording->FrameCount)
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
            if(Recording->CurrentFrameIndex >= Recording->FrameCount)
                Recording->CurrentFrameIndex = 0;
            ReadGameState(Game, Recording->CurrentFrameIndex++);
        } break;
        
        case RECORDING_STATE_CYCLE:
        {
            ReadGameState(Game, Recording->CurrentFrameIndex);                        
            DrawWalkables(&Recording->WalkingSystemRecording);
        } break;
    }                
    
    DrawWalkables(&Recording->WalkingSystemRecording);
}