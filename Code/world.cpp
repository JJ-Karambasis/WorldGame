inline world* 
GetWorld(game* Game, u32 WorldIndex)
{
    world* World = Game->Worlds + WorldIndex;
    return World;
}

inline world* 
GetCurrentWorld(game* Game)
{
    world* World = GetWorld(Game, Game->CurrentWorldIndex);
    return World;
}

inline b32 
IsCurrentWorldIndex(game* Game, u32 WorldIndex)
{
    b32 Result = Game->CurrentWorldIndex == WorldIndex;
    return Result;
}

inline player* 
GetPlayer(game* Game, u32 WorldIndex)
{
    player* Player = &Game->Worlds[WorldIndex].Player;
    return Player;
}

world_entity* AllocateEntity(world_entity_pool* Pool)
{
    i32 Index;
    if(Pool->FreeHead != -1)
    {
        Index = Pool->FreeHead;
    }
    else
    {
        //CONFIRM(JJ): Should we handle a dynamically grow entity pool? Doubt it
        ASSERT(Pool->MaxUsed < MAX_WORLD_ENTITIES);
        Index = Pool->MaxUsed++;
    }
    
    world_entity* Result = Pool->Entities + Index;    
    if(Pool->FreeHead != -1)    
        Pool->FreeHead = Result->ID & 0xFFFFFFFF;
    
    Result->ID = (Pool->NextKey++ << 32) | Index;
    return Result;
}

b32 IsEntityAllocated(world_entity* Entity)
{
    b32 Result = (Entity->ID & 0xFFFFFFFF00000000) != 0;
    return Result;
}

world_entity* GetEntity(world_entity_pool* Pool, i64 ID)
{
    i64 Index = ID & 0xFFFFFFFF;
    ASSERT(Index <= Pool->MaxUsed);
    world_entity* Result = Pool->Entities + Index;
    return Result;
}

world_entity* GetEntity(world* World, i64 ID)
{    
    world_entity* Result = GetEntity(&World->EntityPool, ID);
    return Result;
}

i64 CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color)
{
    world* World = GetWorld(Game, WorldIndex);
    world_entity* Entity = AllocateEntity(&World->EntityPool);
    
    Entity->Type = Type;
    Entity->WorldIndex = WorldIndex;
    Entity->Transform = CreateSQT(Position, Scale, Euler);
    Entity->Color = Color;
        
    return Entity->ID;
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1);    
}

inline void
CreateDualLinkedEntities(game* Game, world_entity_type Type, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1)
{
    i64 AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    i64 BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
    GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
    GetEntity(&Game->Worlds[1], BID)->LinkID = AID;    
}

inline void
CreateDualLinkedEntities(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)                      
{
    CreateDualLinkedEntities(Game, Type, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1);    
}

inline void
CreateSingleLinkedEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1)
{
    i64 AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    i64 BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
    ASSERT((LinkWorldIndex == 0) || (LinkWorldIndex == 1));
    
    if(LinkWorldIndex == 0)
        GetEntity(&Game->Worlds[1], BID)->LinkID = AID;
    else
        GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
}

inline void
CreateSingleLinkedEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)
{
    CreateSingleLinkedEntities(Game, Type, LinkWorldIndex, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1);        
}

inline box_entity* CreateEntity(game* Game, box_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color)
{
    box_entity* Result = PushStruct(&Game->GameStorage, box_entity, Clear, 0);        
    Result->Transform = CreateSQT(Position, Scale, Euler);
    Result->Type = Type;
    Result->Color = Color;        
    Result->WorldIndex = WorldIndex;
    
    AddToList(&Game->Worlds[WorldIndex].Entities, Result);
    
    return Result;
}

inline void
CreateEntityInBothWorlds(game* Game, box_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1);
}

inline void
CreateDualLinkedEntities(game* Game, box_entity_type Type, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1)
{
    box_entity* A = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    box_entity* B = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
    A->Link = B;
    B->Link = A;
}

inline void
CreateSingleLinkedEntities(game* Game, box_entity_type Type, u32 LinkWorldIndex, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1)
{
    box_entity* A = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    box_entity* B = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
    ASSERT((LinkWorldIndex == 0) || (LinkWorldIndex == 1));
        
    if(LinkWorldIndex == 0)
        B->Link = A;
    else
        A->Link = B;    
}

inline void
CreateDualLinkedEntities(game* Game, box_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)                      
{
    CreateDualLinkedEntities(Game, Type, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1);    
}

inline void
CreateSingleLinkedEntities(game* Game, box_entity_type Type, u32 LinkWorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1)
{
    CreateSingleLinkedEntities(Game, Type, LinkWorldIndex, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1);        
}

inline blocker*
CreateBlocker(game* Game, u32 WorldIndex, v3f P0, f32 Height0, v3f P1, f32 Height1)
{
    blocker* Blocker = PushStruct(&Game->GameStorage, blocker, Clear, 0);
    Blocker->P0 = P0;
    Blocker->P1 = P1;
    Blocker->Height0 = Height0;
    Blocker->Height1 = Height1;
    
    AddToList(&Game->Worlds[WorldIndex].Blockers, Blocker);
    
    return Blocker;
}

inline void CreateBlockersInBothWorlds(game* Game, v3f P0, f32 Height0, v3f P1, f32 Height1)
{
    CreateBlocker(Game, 0, P0, Height0, P1, Height1);
    CreateBlocker(Game, 1, P0, Height0, P1, Height1);
}

void IntegrateWorld(game* Game, u32 WorldIndex)
{
    world* World = GetWorld(Game, WorldIndex);
    
}

inline aabb3D 
GetAABB(box_entity* Entity)
{
    aabb3D Result = CreateAABB3DCenterDim(V3(Entity->Position.xy, Entity->Position.z+(Entity->Scale.z*0.5f)), Entity->Scale);            
    return Result;
}
inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

inline b32
TimeResultTest(f32* tMin, time_result_2D TimeResult)
{    
    //TODO(JJ): Need to find a more robust replacement for this epsilon    
    
    if(!IsInvalidTimeResult2D(TimeResult))
    {
        if(*tMin > TimeResult.Time)        
        {
            *tMin = TimeResult.Time;
            return true;                    
        }
    }
    
    return false;
}

inline void
ApplyVelocityXY(box_entity* Entity, v2f MoveAcceleration, f32 dt, f32 Damping)
{
    Entity->Velocity.xy += MoveAcceleration*dt;
    Entity->Velocity *= Damping;
}

inline b32 IsPlayerPushingObject(player* Player, v2f MoveDirection)
{
#define DIRECTION_BIAS 1e-4f
    b32 Result = ((Player->State == PLAYER_STATE_PUSHING) && 
                  Player->Pushing.Object && 
                  Player->Pushing.Object->Type == BOX_ENTITY_TYPE_PUSHABLE && 
                  AreEqual(Player->Pushing.Direction, MoveDirection, DIRECTION_BIAS));
    return Result;
}

inline b32 IsInRangeOfBlockerZ(blocker* Blocker, f32 BottomZ, f32 TopZ)
{
    f32 BlockerZ0 = Blocker->P0.z;
    f32 BlockerHeight0 = BlockerZ0 + Blocker->Height0;
    f32 BlockerZ1 = Blocker->P1.z;
    f32 BlockerHeight1 = BlockerZ1 + Blocker->Height1;
    
    b32 Result = (IsRangeInInterval(BlockerZ0, BlockerHeight0, BottomZ, TopZ) ||
                  IsRangeInInterval(BlockerZ1, BlockerHeight1, BottomZ, TopZ));
    return Result;
}

void OnWorldSwitch(game* Game, u32 LastWorldIndex, u32 CurrentWorldIndex)
{        
    player* LastPlayer = GetPlayer(Game, LastWorldIndex);
    LastPlayer->Velocity = {};
}

void UpdateEntity(game* Game, box_entity* Object, v2f Acceleration)
{   
    u32 ObjectCount = 2;
    
    v2f MoveDeltas[2] = {};
    box_entity* Objects[2] = {};            
    Objects[0] = Object;        
    
    if(Object->Link)        
    {
        Objects[1] = Object->Link;            
        ASSERT(Object->Link->Link == NULL);
    }
            
    b32 StopProcessing[2] = {};
    
    f32 dt = Game->dt;
    f32 VelocityDamping = (1.0f / (1.0f + dt*MOVE_DAMPING));    
    for(u32 ObjectIndex = 0; ObjectIndex < ObjectCount; ObjectIndex++)
    {
        if(Objects[ObjectIndex])
        {
            ApplyVelocityXY(Objects[ObjectIndex], Acceleration, dt, VelocityDamping);
            MoveDeltas[ObjectIndex] = Objects[ObjectIndex]->Velocity.xy*dt;
        }
        else
            StopProcessing[ObjectIndex] = true;
    }
    
#define MOVE_DELTA_EPSILON 1e-6f
    
    for(u32 Iterations=0; ; Iterations++)
    {
        DEVELOPER_MAX_TIME_ITERATIONS(Iterations);                
        time_result_2D BestResults[2] = {InvalidTimeResult2D(), InvalidTimeResult2D()};
        
        for(u32 ObjectIndex = 0; ObjectIndex < ObjectCount; ObjectIndex++)
        {
            if(!StopProcessing[ObjectIndex])
            {
                if(SquareMagnitude(MoveDeltas[ObjectIndex]) > MOVE_DELTA_EPSILON)
                {   
                    world* ObjectWorld = GetWorld(Game, Objects[ObjectIndex]->WorldIndex);
                    v2f TargetPosition = Objects[ObjectIndex]->Position.xy + MoveDeltas[ObjectIndex];
                    
                    for(blocker* Blocker = ObjectWorld->Blockers.First; Blocker; Blocker = Blocker->Next)
                    {
                        if(IsInRangeOfBlockerZ(Blocker, Objects[ObjectIndex]->Position.z, Objects[ObjectIndex]->Position.z + Objects[ObjectIndex]->Scale.z))
                        {
                            time_result_2D TimeResult = MovingRectangleEdgeIntersectionTime2D(Objects[ObjectIndex]->Position.xy, TargetPosition, Objects[ObjectIndex]->Scale.xy, Blocker->P0.xy, Blocker->P1.xy);
                            if(!IsInvalidTimeResult2D(TimeResult) && (TimeResult.Time < BestResults[ObjectIndex].Time))
                                BestResults[ObjectIndex] = TimeResult;                        
                        }
                    }         
                    
                    for(box_entity* BoxEntity = ObjectWorld->Entities.First; BoxEntity; BoxEntity = BoxEntity->Next)
                    {
                        if((BoxEntity->Type != BOX_ENTITY_TYPE_WALKABLE) && (BoxEntity != Objects[ObjectIndex]))
                        {
                            if(IsRangeInInterval(BoxEntity->Position.z, BoxEntity->Position.z+BoxEntity->Scale.z, Objects[ObjectIndex]->Position.z, Objects[ObjectIndex]->Position.z+Objects[ObjectIndex]->Scale.z))
                            {                                    
                                time_result_2D TimeResult = MovingRectangleRectangleIntersectionTime2D(Objects[ObjectIndex]->Position.xy, TargetPosition, Objects[ObjectIndex]->Scale.xy,
                                                                                                       BoxEntity->Position.xy, BoxEntity->Scale.xy);
                                if(!IsInvalidTimeResult2D(TimeResult) && (TimeResult.Time < BestResults[ObjectIndex].Time))
                                    BestResults[ObjectIndex] = TimeResult;                                    
                            }
                        }
                    }                        
                }
                else
                    StopProcessing[ObjectIndex] = true;
            }
        }
        
        for(u32 ObjectIndex = 0; ObjectIndex < 2; ObjectIndex++)
        {
            if(!StopProcessing[ObjectIndex])
            {
                player* Player = GetPlayer(Game, Objects[ObjectIndex]->WorldIndex);
                
                v2f TargetPosition = Objects[ObjectIndex]->Position.xy + MoveDeltas[ObjectIndex];
                
                if(!IsInvalidTimeResult2D(BestResults[ObjectIndex]))
                {                    
                    v2f Delta = BestResults[ObjectIndex].ContactPoint - Objects[ObjectIndex]->Position.xy;
                    Objects[ObjectIndex]->Position.xy = BestResults[ObjectIndex].ContactPoint;                            
                    
                    MoveDeltas[ObjectIndex] = TargetPosition - BestResults[ObjectIndex].ContactPoint;
                    if(BestResults[ObjectIndex].Normal != 0)
                    {                        
                        MoveDeltas[ObjectIndex] = MoveDeltas[ObjectIndex] - Dot(MoveDeltas[ObjectIndex], BestResults[ObjectIndex].Normal)*BestResults[ObjectIndex].Normal;
                        Objects[ObjectIndex]->Velocity.xy = Objects[ObjectIndex]->Velocity.xy - Dot(Objects[ObjectIndex]->Velocity.xy, BestResults[ObjectIndex].Normal)*BestResults[ObjectIndex].Normal;                                            
                    }
                    
                    if(Player->Pushing.Object == Objects[ObjectIndex])                                
                        Player->Position.xy += Delta;
                }
                else
                {
                    Objects[ObjectIndex]->Position.xy = TargetPosition;
                    if(Player->Pushing.Object == Objects[ObjectIndex])
                        Player->Position.xy += MoveDeltas[ObjectIndex];
                    
                    StopProcessing[ObjectIndex] = true;
                }
            }                                                               
        }           
        
        if(StopProcessing[0] && StopProcessing[1])
            break;        
    }                
}

void UpdateWorld(game* Game)
{    
    world* World = GetWorld(Game, Game->CurrentWorldIndex);
    player* Player = &World->Player;
    
    f32 PlayerHeight = TrueHeight(Game->PlayerHeight, Game->PlayerRadius);
    f32 PlayerWalkHeight = (Player->Position.z + PlayerHeight*0.25f);
    
    input* Input = Game->Input;
    
    v2f MoveDirection = {};
    
    if(IsDown(Input->MoveForward))
        MoveDirection.y = 1.0f;
    
    if(IsDown(Input->MoveBackward))
        MoveDirection.y = -1.0f;
    
    if(IsDown(Input->MoveRight))
        MoveDirection.x = 1.0f;
    
    if(IsDown(Input->MoveLeft))
        MoveDirection.x = -1.0f;
    
    if(MoveDirection != 0)
        MoveDirection = Normalize(MoveDirection);
        
    f32 dt = Game->dt;    
    v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;    
    
    f32 VelocityDamping = (1.0f / (1.0f + dt*MOVE_DAMPING));
    
    pushing_state Pushing = {};
    if(IsPlayerPushingObject(Player, MoveDirection))
    {                     
        Player->Velocity = {};        
        box_entity* Object = Player->Pushing.Object;                
        
        UpdateEntity(Game, Object, MoveAcceleration);
        
        Pushing = Player->Pushing;
    }
    else
    {                   
        if(Player->Pushing.Object)
        {            
            Player->Pushing.Object->Velocity = {};
            if(Player->Pushing.Object->Link)
                Player->Pushing.Object->Link->Velocity = {};
        }
        
        player_state PlayerState = {};                        
        
        Player->Velocity.xy += MoveAcceleration*dt;
        Player->Velocity.xy *= VelocityDamping;
        
        v2f MoveDelta = Player->Velocity.xy*dt;   
        
        //TODO(JJ): Cap this out
        for(u32 Iterations = 0; ; Iterations++)
        {
            DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
            if(SquareMagnitude(MoveDelta) > MOVE_DELTA_EPSILON)
            {        
                v2f TargetPosition = Player->Position.xy + MoveDelta;    
                
                time_result_2D BestResult = InvalidTimeResult2D();
                for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
                {            
                    if(IsInRangeOfBlockerZ(Blocker, Player->Position.z, Player->Position.z+PlayerHeight))
                    {           
                        time_result_2D TimeResult = MovingCircleEdgeIntersectionTime2D(Player->Position.xy, TargetPosition, Game->PlayerRadius, Blocker->P0.xy, Blocker->P1.xy);
                        if(!IsInvalidTimeResult2D(TimeResult) && (BestResult.Time > TimeResult.Time))                
                            BestResult = TimeResult;                                
                    }
                }
                
                box_entity* HitEntity = NULL;
                for(box_entity* BoxEntity = World->Entities.First; BoxEntity; BoxEntity = BoxEntity->Next)
                {
                    if(BoxEntity->Type != BOX_ENTITY_TYPE_WALKABLE)
                    {                        
                        if(IsRangeInInterval(BoxEntity->Position.z, BoxEntity->Position.z+BoxEntity->Scale.z, Player->Position.z, Player->Position.z+PlayerHeight))
                        {
                            v2f HalfDim = BoxEntity->Scale.xy*0.5f;                           
                            v2f Min = BoxEntity->Position.xy - HalfDim;
                            v2f Max = BoxEntity->Position.xy + HalfDim;
                                                        
                            time_result_2D TimeResult = MovingCircleRectangleIntersectionTime2D(Player->Position.xy, TargetPosition, Game->PlayerRadius, Min, Max);
                            
                            if(!IsInvalidTimeResult2D(TimeResult) && (BestResult.Time > TimeResult.Time))
                            {
                                BestResult = TimeResult;
                                HitEntity = BoxEntity;
                            }
                        }
                    }
                }                        
                
                if(!IsInvalidTimeResult2D(BestResult))    
                {                
                    Player->Position.xy = BestResult.ContactPoint;                
                    MoveDelta = TargetPosition-BestResult.ContactPoint;        
                    
                    if(BestResult.Normal != 0)
                    {                    
                        MoveDelta = MoveDelta - Dot(MoveDelta, BestResult.Normal)*BestResult.Normal;
                        Player->Velocity.xy = Player->Velocity.xy - Dot(Player->Velocity.xy, BestResult.Normal)*BestResult.Normal;                    
                        
                        if(AreEqual(MoveDirection, -BestResult.Normal, 1e-4f))
                        {
                            PlayerState = PLAYER_STATE_PUSHING;
                            Pushing.Object = HitEntity;
                            Pushing.Direction = MoveDirection;                            
                        }
                    }
                }
                else
                {
                    Player->Position.xy = TargetPosition;                    
                    break;
                }
            }
            else
            {
                break;
            }
        }
        
        Player->State = PlayerState;
    }
    
    Player->Pushing = Pushing;
    
    
    f32 BestPointZ = -FLT_MAX;
    triangle3D BestTriangle = InvalidTriangle3D();          
    for(box_entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->Type == BOX_ENTITY_TYPE_WALKABLE)
        {                        
            triangle3D_mesh* Mesh = &Game->Assets->BoxTriangleMesh;                        
            
            for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
            {
                triangle3D Triangle = TransformTriangle3D(Mesh->Triangles[TriangleIndex], Entity->Transform);                        
                
                if(!IsDegenerateTriangle2D(Triangle))
                {                                                               
                    if(IsPointInTriangle2D(Triangle, Player->Position.xy))
                    {
                        f32 TriangleZ = FindTriangleZ(Triangle, Player->Position.xy);
                        if((TriangleZ > BestPointZ) && (TriangleZ <= PlayerWalkHeight))
                        {
                            BestPointZ = TriangleZ;       
                            BestTriangle = Triangle;
                        }
                    }
                }
                DEVELOPER_INCREMENT_WALKING_TRIANGLE();
            }
        }
    }
    
    ASSERT(BestPointZ != -FLT_MAX);    
    Player->Position.z = BestPointZ;                        
}

#include "player.cpp"