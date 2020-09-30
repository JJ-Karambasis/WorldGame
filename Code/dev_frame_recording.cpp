#define PLAYER_SIZE sizeof(recording_entity_header) + sizeof(player)
#define RIGID_BODY_SIZE sizeof(recording_entity_header)
#define PUSHABLE_SIZE sizeof(recording_entity_header) + sizeof(pushing_object)

inline ak_bool 
ValidateSignature(frame_recording_header Header)
{
    ak_bool Result = (Header.MajorVersion == FRAME_RECORDING_MAJOR_VERSION) && (Header.MinorVersion == FRAME_RECORDING_MINOR_VERSION);
    return Result;
}

inline ak_bool
ValidateVersion(frame_recording_header Header)
{
    ak_bool Result = AK_StringEquals(Header.Signature, FRAME_RECORDING_SIGNATURE);
    return Result;
}

void frame_recording::StartRecording()
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_NONE, "Recording state should be set to none before you start recording");
    RecordingState = FRAME_RECORDING_STATE_RECORDING;
    
    if(!RecordingArena)        
        RecordingArena = AK_CreateArena(AK_Megabyte(4));                
}

void frame_recording::RecordFrame(game* Game)
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_RECORDING, "Recording state should be recording before you record a frame");
    
    ak_u32 Size = 0;
    ak_u32 EntityCount = 0;
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
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
    
    ak_u32 EntityBufferSize = Size-sizeof(input);
    
    frame_data FrameData;
    FrameData.Input = *Game->Input;
    FrameData.EntityBuffer = RecordingArena->Push(EntityBufferSize);
    
    ak_u8* At = (ak_u8*)FrameData.EntityBuffer;
    ak_u8* End = At + EntityBufferSize;
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        simulation* Simulation = GetSimulation(Game, WorldIndex);
        AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
        {
            switch(Entity->Type)
            {
                case ENTITY_TYPE_PLAYER:
                {                    
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = RigidBody->Velocity;
                    Header->Acceleration = RigidBody->Acceleration;
                    
                    player* Player = (player*)(Header+1);
                    *Player = *GetUserData(Entity, player);
                    
                    At += PLAYER_SIZE;
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = RigidBody->Velocity;
                    Header->Acceleration = RigidBody->Acceleration;
                    
                    At += RIGID_BODY_SIZE;                    
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {
                    rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    recording_entity_header* Header = (recording_entity_header*)At;
                    Header->ID = Entity->ID;
                    Header->Transform = *GetEntityTransform(Game, Entity->ID);
                    Header->Velocity = RigidBody->Velocity;
                    Header->Acceleration = RigidBody->Acceleration;
                    
                    pushing_object* PushingObject = (pushing_object*)(Header+1);                    
                    *PushingObject = *GetUserData(Entity, pushing_object);
                    
                    At += PUSHABLE_SIZE;                    
                } break;
            }
        }
    }
    
    AK_Assert(At == End, "Frame recording is corrupted. This is a programming error");
    
    WrittenFrameInfos.Add(FrameInfo);
    WrittenFrameDatas.Add(FrameData);
    FrameCount++;
}

void frame_recording::EndRecording(char* Path)
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_RECORDING, "Recording state should be recording before you end recording");
    RecordingState = FRAME_RECORDING_STATE_NONE;
    
    ak_u32 DataSize = 0;
    DataSize += sizeof(frame_recording_header);
    DataSize += sizeof(frame_info)*FrameCount;
    AK_ForEach(FrameInfo, &WrittenFrameInfos)    
        DataSize += FrameInfo->Size;    
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    void* Data = GlobalArena->Push(DataSize);
    ak_u8* DataAt = (ak_u8*)Data;
    
    frame_recording_header* Header = (frame_recording_header*)DataAt;
    AK_MemoryCopy((char*)Header->Signature, (char*)FRAME_RECORDING_SIGNATURE, sizeof(FRAME_RECORDING_SIGNATURE));
    Header->MajorVersion = FRAME_RECORDING_MAJOR_VERSION;
    Header->MinorVersion = FRAME_RECORDING_MINOR_VERSION;
    Header->FrameCount = FrameCount;    
    DataAt += sizeof(frame_recording_header);
    
    ak_u32 OffsetToFrame = sizeof(frame_recording_header)+sizeof(frame_info)*FrameCount;
    AK_ForEach(FrameInfo, &WrittenFrameInfos)
    {
        FrameInfo->OffsetToFrame = OffsetToFrame;
        OffsetToFrame += FrameInfo->Size;        
        *((frame_info*)DataAt) = *FrameInfo;
        DataAt += sizeof(frame_info);
    }
    AK_Assert(OffsetToFrame == DataSize, "Frame recording is corrupted. This is a programming error");
    
    for(ak_u32 FrameIndex = 0; FrameIndex < FrameCount; FrameIndex++)
    {
        frame_info* FrameInfo = &WrittenFrameInfos[FrameIndex];        
        frame_data* FrameData = &WrittenFrameDatas[FrameIndex];
        *(input*)DataAt = FrameData->Input;
        AK_MemoryCopy(DataAt + sizeof(input), FrameData->EntityBuffer, FrameInfo->Size-sizeof(input));
        DataAt += FrameInfo->Size;        
    }        
    
    AK_WriteEntireFile(Path, Data, DataSize);    
    
    WrittenFrameInfos.Clear();
    WrittenFrameDatas.Clear();
    RecordingArena->Clear();
    GlobalArena->EndTemp(&TempArena);
    FrameCount = 0;
}

ak_bool frame_recording::StartPlaying(ak_char* Path)
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_NONE, "Recording state should be set to none before you start playing");
    ReadRecordingBuffer = AK_ReadEntireFile(Path, RecordingArena);
    if(!ReadRecordingBuffer.IsValid()) return false;    
    
    frame_recording_header* Header = (frame_recording_header*)ReadRecordingBuffer.Data;
    
    if(!ValidateSignature(*Header)) return false;
    if(!ValidateVersion(*Header))   return false;    
    
    FrameCount = Header->FrameCount;        
    RecordingState = FRAME_RECORDING_STATE_PLAYING;
    
    return true;
}

void frame_recording::PlayFrame(game* Game, ak_u32 FrameIndex)
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_PLAYING, "Recording state should be playing before you play a frame");
    
    frame_info* FrameInfoArray = (frame_info*)(ReadRecordingBuffer.Data + sizeof(frame_recording_header));   
    frame_info FrameInfo = FrameInfoArray[FrameIndex];
    
    ak_u8* FrameAt = ReadRecordingBuffer.Data + FrameInfo.OffsetToFrame;
    ak_u8* FrameEnd = FrameAt+FrameInfo.Size;
    
    *Game->Input = *(input*)FrameAt;
    FrameAt += sizeof(input);
    for(ak_u32 EntityIndex = 0; EntityIndex < FrameInfo.RecordedEntityCount; EntityIndex++)
    {
        recording_entity_header* Header = (recording_entity_header*)FrameAt;        
        entity* Entity = GetEntity(Game, Header->ID);        
        *GetEntityTransform(Game, Entity->ID) = Header->Transform;
        
        simulation* Simulation = GetSimulation(Game, Header->ID);
        
        rigid_body* RigidBody = Simulation->GetSimEntity(Entity->SimEntityID)->ToRigidBody();
        RigidBody->Velocity = Header->Velocity;
        RigidBody->Acceleration = Header->Acceleration;
        
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
    AK_Assert(FrameAt == FrameEnd, "Frame is courrpted. This is a programming error or the file is corrupted");
}

void frame_recording::EndPlaying()
{
    AK_Assert(RecordingState == FRAME_RECORDING_STATE_PLAYING, "Recording state should be playing before end playing");
    RecordingState = FRAME_RECORDING_STATE_NONE;
    RecordingArena->Clear();    
    FrameCount = 0;
}