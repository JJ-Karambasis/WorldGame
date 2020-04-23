inline pushing_state
InitPushingState()
{
    pushing_state Result;
    Result.EntityID = -1;
    Result.Direction = {};
    return Result;
}

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

inline i64 
GetIndex(world_entity_pool* Pool, i64 ID)
{
    i64 Result = ID & 0xFFFFFFFF;
    ASSERT(Result <= Pool->MaxUsed);
    return Result;
}

world_entity* GetEntity(world_entity_pool* Pool, i64 ID)
{
    i64 Index = GetIndex(Pool, ID);
    world_entity* Result = Pool->Entities + Index;
    return Result;
}

world_entity* GetEntity(world* World, i64 ID)
{    
    world_entity* Result = GetEntity(&World->EntityPool, ID);
    return Result;
}

world_entity* GetEntity(game* Game, u32 WorldIndex, i64 ID)
{    
    world_entity* Result = GetEntity(GetWorld(Game, WorldIndex), ID);
    return Result;
}

inline world_entity*
GetPlayerEntity(game* Game, u32 WorldIndex)
{
    world* World = GetWorld(Game, WorldIndex);    
    world_entity* Result = GetEntity(World, World->Player.EntityID);
    return Result;
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

world_entity* GetFirstEntity(world_entity_pool* Pool)
{
    for(u32 Index = 0; Index < MAX_WORLD_ENTITIES; Index++)
    {
        world_entity* Result = Pool->Entities + Index;
        if(IsEntityAllocated(Result))
            return Result;
    }
    
    return NULL;
}

world_entity* GetNextEntity(world_entity_pool* Pool, world_entity* Entity)
{
    ASSERT(IsEntityAllocated(Entity));    
    i32 Index = SafeI32(GetIndex(Pool, Entity->ID)+1);
    
    for(Index; Index < MAX_WORLD_ENTITIES; Index++)
    {
        world_entity* Result = Pool->Entities + Index;
        if(IsEntityAllocated(Result))
            return Result;
    }
    
    return NULL;
}

i64 CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    world_entity* Entity = AllocateEntity(&World->EntityPool);
    
    Entity->Type = Type;
    Entity->WorldIndex = WorldIndex;
    Entity->Transform = CreateSQT(Position, Scale, Euler);
    Entity->Color = Color;
    Entity->UserData = UserData;
    Entity->LinkID = -1;
    
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

void CreatePlayer(game* Game, u32 WorldIndex, v3f Position, f32 Radius, f32 Height, c4 Color)
{    
    player* Player = GetPlayer(Game, WorldIndex);
    Player->Pushing = InitPushingState();
    
    i64 ID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f), V3(), Color, Player);
    
    world_entity* Entity = GetEntity(Game, WorldIndex, ID);    
    
    Entity->Collider.Type = COLLIDER_TYPE_VERTICAL_CAPSULE;
    Entity->Collider.VerticalCapsule.P = {};
    Entity->Collider.VerticalCapsule.Radius = Radius;
    Entity->Collider.VerticalCapsule.Height = Height;    
}

i64 CreateBoxEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Dim, c4 Color)
{
    i64 Result = CreateEntity(Game, Type, WorldIndex, Position, V3(1.0f), V3(), Color);    
    world_entity* Entity = GetEntity(Game, WorldIndex, Result);
    
    Entity->Collider.Type = COLLIDER_TYPE_ALIGNED_BOX;
    Entity->Collider.AlignedBox.CenterP = {};
    Entity->Collider.AlignedBox.CenterP.z = Dim.z*0.5f;
    Entity->Collider.AlignedBox.Dim = Dim;    
    
    return Result;
}

void CreateBoxEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Dim, c4 Color0, c4 Color1)
{    
    CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    CreateBoxEntity(Game, Type, 1, Position, Dim, Color1); 
}

void CreateSingleLinkedBoxEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position, v3f Dim, c4 Color0, c4 Color1)
{
    i64 AID = CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    i64 BID = CreateBoxEntity(Game, Type, 1, Position, Dim, Color1);
    
    ASSERT((LinkWorldIndex == 0) || (LinkWorldIndex == 1));
    
    if(LinkWorldIndex == 0)
        GetEntity(&Game->Worlds[1], BID)->LinkID = AID;
    else
        GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
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

inline b32 IsPlayerPushingObject(world* World, player* Player, v2f MoveDirection)
{
    f32 Epsilon = 1e-4f;
    
    if((Player->State == PLAYER_STATE_PUSHING) && 
       (Player->Pushing.EntityID != -1) && 
       AreEqual(Player->Pushing.Direction, MoveDirection, Epsilon))
    {        
        world_entity* Entity = GetEntity(World, Player->Pushing.EntityID);                        
        b32 Result = (Entity->Type == WORLD_ENTITY_TYPE_PUSHABLE);
        return Result;
    }
    
    return false;        
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

vertical_capsule GetWorldSpaceVerticalCapsule(vertical_capsule Capsule, sqt Transform)
{
    Transform.Orientation = IdentityQuaternion();
    
    vertical_capsule Result;
    Result.P = TransformV3(Capsule.P, Transform);
    Result.Radius = Capsule.Radius * MaximumF32(Transform.Scale.x, Transform.Scale.y);
    Result.Height = Capsule.Height * Transform.Scale.z;
    return Result;
}

aligned_box GetWorldSpaceAlignedBox(aligned_box Box, sqt Transform)
{
    Transform.Orientation = IdentityQuaternion();
    
    aligned_box Result;
    Result.CenterP = TransformV3(Box.CenterP, Transform);
    Result.Dim = Box.Dim*Transform.Scale;
    return Result;
}

void OnWorldSwitch(game* Game, u32 LastWorldIndex, u32 CurrentWorldIndex)
{        
    world_entity* LastPlayer = GetPlayerEntity(Game, LastWorldIndex);    
    LastPlayer->Velocity = {};
}

#if 0 

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
#endif

world_entity* ClearEntityVelocity(world* World, i64 ID)
{
    if(ID != -1)
    {
        ASSERT(ID != 0);
        world_entity* Entity = GetEntity(World, ID);
        ASSERT(IsEntityAllocated(Entity));                        
        Entity->Velocity = {};                        
        return Entity;
    }
    
    return NULL;
}

time_result_2D FindTOI(world* World, world_entity* Entity, v2f MoveDelta, world_entity** HitEntity)
{    
    time_result_2D Result = InvalidTimeResult2D();
    
    switch(Entity->Collider.Type)
    {
        case COLLIDER_TYPE_ALIGNED_BOX:
        {
            NOT_IMPLEMENTED;
        } break;
        
        case COLLIDER_TYPE_VERTICAL_CAPSULE:
        {
            vertical_capsule EntityCapsule = GetWorldSpaceVerticalCapsule(Entity->Collider.VerticalCapsule, Entity->Transform);                                                                
            
            v2f TargetPosition = EntityCapsule.P.xy+MoveDelta;            
            f32 CapsuleHeight = EntityCapsule.P.z+EntityCapsule.Height+(EntityCapsule.Radius*2);
            
            for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
            {
                if(IsInRangeOfBlockerZ(Blocker, EntityCapsule.P.z, CapsuleHeight))
                {
                    time_result_2D TimeResult = MovingCircleEdgeIntersectionTime2D(EntityCapsule.P.xy, TargetPosition, EntityCapsule.Radius, Blocker->P0.xy, Blocker->P1.xy);
                    if(!IsInvalidTimeResult2D(TimeResult) && (Result.Time > TimeResult.Time))
                        Result = TimeResult;
                }                                    
            }
            
            for(world_entity* TestEntity = GetFirstEntity(&World->EntityPool); TestEntity; TestEntity = GetNextEntity(&World->EntityPool, TestEntity))
            {
                if((TestEntity->Type != WORLD_ENTITY_TYPE_WALKABLE) && (TestEntity != Entity))
                {                                        
                    switch(TestEntity->Collider.Type)
                    {
                        case COLLIDER_TYPE_ALIGNED_BOX:
                        {
                            aligned_box TestEntityBox = GetWorldSpaceAlignedBox(TestEntity->Collider.AlignedBox, TestEntity->Transform);
                            
                            v3f HalfDim = TestEntityBox.Dim*0.5f;
                            v3f Min = TestEntityBox.CenterP-HalfDim;
                            v3f Max = TestEntityBox.CenterP+HalfDim;
                            
                            if(IsRangeInInterval(Min.z, Max.z, EntityCapsule.P.z, CapsuleHeight))
                            {                                                    
                                time_result_2D TimeResult = MovingCircleRectangleIntersectionTime2D(EntityCapsule.P.xy, TargetPosition, EntityCapsule.Radius, Min.xy, Max.xy);
                                if(!IsInvalidTimeResult2D(TimeResult) && (Result.Time > TimeResult.Time))
                                {
                                    Result = TimeResult;
                                    *HitEntity = TestEntity;
                                }
                            }
                        } break;
                        
                        case COLLIDER_TYPE_VERTICAL_CAPSULE:
                        {
                            NOT_IMPLEMENTED;
                        } break;
                    }
                }                                    
            }
            
        } break;
        
        INVALID_DEFAULT_CASE;
    }      
    
    return Result;
}

void UpdateWorld(game* Game)
{    
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
        
    player_state PlayerState = {};
    pushing_state Pushing = InitPushingState();
    
#define APPLY_VELOCITY(Entity) \
    Entity->Velocity.xy += MoveAcceleration*dt; \
    Entity->Velocity.xy *= VelocityDamping
        
    #define MOVE_DELTA_EPSILON 1e-6f
    
    world* World = GetWorld(Game, Game->CurrentWorldIndex);        
    for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
    {        
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
            {
                ASSERT(Entity->UserData);
                player* Player = (player*)Entity->UserData;
                
                if(IsPlayerPushingObject(World, Player, MoveDirection))
                    continue;
                
                world_entity* PushingEntity = ClearEntityVelocity(World, Player->Pushing.EntityID);
                if(PushingEntity)                    
                    ClearEntityVelocity(World, PushingEntity->LinkID);        
                
                Entity->Velocity.xy += MoveAcceleration*dt;
                Entity->Velocity.xy *= VelocityDamping;                    
                                                
                //DetectCollisions(Game, Entity);
                
                v2f MoveDelta = Entity->Velocity.xy*dt;
                
                for(u32 Iterations = 0; ; Iterations++)
                {
                    DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                    
                    if(SquareMagnitude(MoveDelta) <= MOVE_DELTA_EPSILON)
                        break;
                    
                    world_entity* HitEntity = NULL;
                    time_result_2D TimeResult = FindTOI(World, Entity, MoveDelta, &HitEntity);
                    if(IsInvalidTimeResult2D(TimeResult))
                    {
                        Entity->Position.xy += MoveDelta;
                        break;
                    }
                                                            
                    v2f TargetPosition = Entity->Position.xy + MoveDelta;
                    
                    Entity->Position.xy = TimeResult.ContactPoint;
                    MoveDelta = TargetPosition - TimeResult.ContactPoint;
                    
                    if(TimeResult.Normal != 0)
                    {
                        MoveDelta -= Dot(MoveDelta, TimeResult.Normal)*TimeResult.Normal;
                        Entity->Velocity.xy -= Dot(Entity->Velocity.xy, TimeResult.Normal)*TimeResult.Normal;
                        
                    }
                }
            } break;
            
            case WORLD_ENTITY_TYPE_PUSHABLE:
            {
            } break;
        }                        
    }    
    
    World->Player.State = PlayerState;
    World->Player.Pushing = Pushing;
        
    world_entity* PlayerEntity = GetEntity(World, World->Player.EntityID);
    
    f32 PlayerHeight = TrueHeight(Game->PlayerHeight, Game->PlayerRadius);
    f32 PlayerWalkHeight = (PlayerEntity->Position.z + PlayerHeight*0.25f);
    
    f32 BestPointZ = -FLT_MAX;
    triangle3D BestTriangle = InvalidTriangle3D();          
    
    for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
    {
        if(Entity->Type == WORLD_ENTITY_TYPE_WALKABLE)
        {            
            triangle3D_mesh* Mesh = &Game->Assets->BoxTriangleMesh;                        
            
            for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
            {
                triangle3D Triangle = TransformTriangle3D(Mesh->Triangles[TriangleIndex], Entity->Transform);                        
                
                if(!IsDegenerateTriangle2D(Triangle))
                {                                                               
                    if(IsPointInTriangle2D(Triangle, PlayerEntity->Position.xy))
                    {
                        f32 TriangleZ = FindTriangleZ(Triangle, PlayerEntity->Position.xy);
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
    PlayerEntity->Position.z = BestPointZ;                        
}

#if 0 

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
#endif

#include "player.cpp"