#define PLAYER_SIZE sizeof(recording_entity_header) + sizeof(player)
#define RIGID_BODY_SIZE sizeof(recording_entity_header)
#define PUSHABLE_SIZE sizeof(recording_entity_header) + sizeof(pushing_object)

inline b32 
ValidateSignature(frame_recording_header Header)
{
    b32 Result = (Header.MajorVersion == FRAME_RECORDING_MAJOR_VERSION) && (Header.MinorVersion == FRAME_RECORDING_MINOR_VERSION);
    return Result;
}

inline b32 
ValidateVersion(frame_recording_header Header)
{
    b32 Result = StringEquals(Header.Signature, FRAME_RECORDING_SIGNATURE);
    return Result;
}

void frame_recording::StartRecording()
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_NONE);
    RecordingState = FRAME_RECORDING_STATE_RECORDING;
    
    if(!WrittenFrameInfos.IsInitialized())
        WrittenFrameInfos = CreateDynamicArray<frame_info>(2048);
    
    if(!WrittenFrameDatas.IsInitialized())
        WrittenFrameDatas = CreateDynamicArray<frame_data>(2048);        
    
    if(!IsInitialized(&RecordingArena))
        RecordingArena = CreateArena(MEGABYTE(4));                
}

void frame_recording::RecordFrame(game* Game)
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_RECORDING);
    
    u32 Size = 0;
    u32 EntityCount = 0;
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
        {
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {                    
                    Size += PLAYER_SIZE;
                    EntityCount++;
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    Size += RIGID_BODY_SIZE;
                    EntityCount++;
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {
                    Size += PUSHABLE_SIZE;                    
                    EntityCount++;
                } break;
            }
        }
    }
    
    Size += sizeof(input);
    
    frame_info FrameInfo = {};    
    FrameInfo.Size = Size;
    FrameInfo.RecordedEntityCount = EntityCount;
    
    u32 EntityBufferSize = Size-sizeof(input);
    
    frame_data FrameData;
    FrameData.Input = *Game->Input;
    FrameData.EntityBuffer = PushSize(&RecordingArena, EntityBufferSize, Clear, 0);
    
    u8* At = (u8*)FrameData.EntityBuffer;
    u8* End = At + EntityBufferSize;
    for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
        {
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    sim_entity* SimEntity = GetSimEntity(Game, Entity->ID);
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = SimEntity->Velocity;
                    Header->Acceleration = SimEntity->Acceleration;
                    
                    player* Player = (player*)(Header+1);
                    *Player = *GetUserData(Entity, player);
                    
                    At += PLAYER_SIZE;
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    sim_entity* SimEntity = GetSimEntity(Game, Entity->ID);
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = SimEntity->Velocity;
                    Header->Acceleration = SimEntity->Acceleration;
                    
                    At += RIGID_BODY_SIZE;                    
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {
                    sim_entity* SimEntity = GetSimEntity(Game, Entity->ID);
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = SimEntity->Velocity;
                    Header->Acceleration = SimEntity->Acceleration;
                    
                    pushing_object* PushingObject = (pushing_object*)(Header+1);
                    *PushingObject = *GetUserData(Entity, pushing_object);
                    
                    At += PUSHABLE_SIZE;                    
                } break;
            }
        }
    }
    
    ASSERT(At == End);
    
    WrittenFrameInfos.Add(FrameInfo);
    WrittenFrameDatas.Add(FrameData);
    FrameCount++;
}

void frame_recording::EndRecording(char* Path)
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_RECORDING);
    RecordingState = FRAME_RECORDING_STATE_NONE;
    
    u32 DataSize = 0;
    DataSize += sizeof(frame_recording_header);
    DataSize += sizeof(frame_info)*FrameCount;
    FOR_EACH(FrameInfo, &WrittenFrameInfos)    
        DataSize += FrameInfo->Size;    
    
    void* Data = PushSize(DataSize, Clear, 0);
    u8* DataAt = (u8*)Data;
    
    frame_recording_header* Header = (frame_recording_header*)DataAt;
    CopyMemory((char*)Header->Signature, (char*)FRAME_RECORDING_SIGNATURE, sizeof(FRAME_RECORDING_SIGNATURE));
    Header->MajorVersion = FRAME_RECORDING_MAJOR_VERSION;
    Header->MinorVersion = FRAME_RECORDING_MINOR_VERSION;
    Header->FrameCount = FrameCount;    
    DataAt += sizeof(frame_recording_header);
    
    u32 OffsetToFrame = sizeof(frame_recording_header)+sizeof(frame_info)*FrameCount;
    FOR_EACH(FrameInfo, &WrittenFrameInfos)
    {
        FrameInfo->OffsetToFrame = OffsetToFrame;
        OffsetToFrame += FrameInfo->Size;        
        *((frame_info*)DataAt) = *FrameInfo;
        DataAt += sizeof(frame_info);
    }
    ASSERT(OffsetToFrame == DataSize);
           
    for(u32 FrameIndex = 0; FrameIndex < FrameCount; FrameIndex++)
    {
        frame_info* FrameInfo = &WrittenFrameInfos[FrameIndex];        
        frame_data* FrameData = &WrittenFrameDatas[FrameIndex];
        *(input*)DataAt = FrameData->Input;
        CopyMemory(DataAt + sizeof(input), FrameData->EntityBuffer, FrameInfo->Size-sizeof(input));
        DataAt += FrameInfo->Size;        
    }        
    
    WriteEntireFile(Path, Data, DataSize);    
    
    WrittenFrameInfos.Reset();
    WrittenFrameDatas.Reset();
    ResetArena(&RecordingArena);
    FrameCount = 0;
}

b32 frame_recording::StartPlaying(char* Path)
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_NONE);
    ReadRecordingBuffer = ReadEntireFile(Path);
    if(!ReadRecordingBuffer.IsValid()) return false;    
    
    frame_recording_header* Header = (frame_recording_header*)ReadRecordingBuffer.Data;
    
    if(!ValidateSignature(*Header)) return false;
    if(!ValidateVersion(*Header))   return false;    
    
    FrameCount = Header->FrameCount;        
    RecordingState = FRAME_RECORDING_STATE_PLAYING;
    
    return true;
}

void frame_recording::PlayFrame(game* Game, u32 FrameIndex)
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_PLAYING);
    
    frame_info* FrameInfoArray = (frame_info*)(ReadRecordingBuffer.Data + sizeof(frame_recording_header));   
    frame_info FrameInfo = FrameInfoArray[FrameIndex];
    
    u8* FrameAt = ReadRecordingBuffer.Data + FrameInfo.OffsetToFrame;
    u8* FrameEnd = FrameAt+FrameInfo.Size;
    
    *Game->Input = *(input*)FrameAt;
    FrameAt += sizeof(input);
    for(u32 EntityIndex = 0; EntityIndex < FrameInfo.RecordedEntityCount; EntityIndex++)
    {
        recording_entity_header* Header = (recording_entity_header*)FrameAt;        
        entity* Entity = GetEntity(Game, Header->ID);        
        *GetEntityTransform(Game, Entity->ID) = Header->Transform;
        
        sim_entity* SimEntity = GetSimEntity(Game, Header->ID);
        SimEntity->Velocity = Header->Velocity;
        SimEntity->Acceleration = Header->Acceleration;
        
        FrameAt += sizeof(recording_entity_header);
        switch(Entity->Type)
        {
            case ENTITY_TYPE_PLAYER:
            {
                *GetUserData(Entity, player) = *(player*)FrameAt;
                FrameAt += sizeof(player);
            } break;
            
            case ENTITY_TYPE_PUSHABLE:
            {
                *GetUserData(Entity, pushing_object) = *(pushing_object*)FrameAt;
                FrameAt += sizeof(pushing_object);
            } break;
        }
    }        
    ASSERT(FrameAt == FrameEnd);
}

void frame_recording::EndPlaying()
{
    ASSERT(RecordingState == FRAME_RECORDING_STATE_PLAYING);
    RecordingState = FRAME_RECORDING_STATE_NONE;
    FreeFileMemory(&ReadRecordingBuffer);
    FrameCount = 0;
}