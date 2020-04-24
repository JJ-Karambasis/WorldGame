inline pushing_state
InitPushingState()
{
    pushing_state Result;
    Result.EntityID = InvalidEntityID();
    Result.Direction = {};
    return Result;
}

inline b32 
AreEqualIDs(world_entity_id AID, world_entity_id BID)
{
    b32 Result = (AID.ID == BID.ID) && (AID.WorldIndex == BID.WorldIndex);
    return Result;
}

inline world* 
GetWorld(game* Game, u32 WorldIndex)
{
    ASSERT((WorldIndex == 0) || (WorldIndex == 1));
    world* World = Game->Worlds + WorldIndex;
    return World;
}

inline world* 
GetWorld(game* Game, world_entity_id ID)
{    
    world* World = GetWorld(Game, ID.WorldIndex);
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

inline world_entity* 
GetEntity(world_entity_pool* Pool, world_entity_id EntityID)
{
    i64 Index = GetIndex(Pool, EntityID.ID);
    world_entity* Result = Pool->Entities + Index;
    return Result;
}

inline world_entity* 
GetEntity(world* World, world_entity_id EntityID)
{    
    world_entity* Result = GetEntity(&World->EntityPool, EntityID);
    return Result;
}

inline world_entity* 
GetEntity(game* Game, world_entity_id EntityID)
{    
    world_entity* Result = GetEntity(GetWorld(Game, EntityID.WorldIndex), EntityID);
    return Result;
}

inline world_entity* 
GetEntityOrNull(world_entity_pool* Pool, world_entity_id EntityID)
{
    if(IsInvalidEntityID(EntityID))
        return NULL;
    
    world_entity* Result = GetEntity(Pool, EntityID);
    return Result;
}

inline world_entity* 
GetEntityOrNull(world* World, world_entity_id EntityID)
{    
    world_entity* Result = GetEntityOrNull(&World->EntityPool, EntityID);
    return Result;    
}

inline world_entity* 
GetEntityOrNull(game* Game, world_entity_id EntityID)
{
    if(IsInvalidEntityID(EntityID))
        return NULL;
    
    world_entity* Result = GetEntityOrNull(GetWorld(Game, EntityID.WorldIndex), EntityID);
    return Result;    
}

inline world_entity*
GetPlayerEntity(game* Game, u32 WorldIndex)
{
    world* World = GetWorld(Game, WorldIndex);    
    world_entity* Result = GetEntity(World, World->Player.EntityID);
    return Result;
}

world_entity* 
AllocateEntity(world_entity_pool* Pool)
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
        Pool->FreeHead = Result->ID.ID & 0xFFFFFFFF;
    
    Result->ID.ID = (Pool->NextKey++ << 32) | Index;
    return Result;
}

inline b32 
IsEntityAllocated(world_entity* Entity)
{
    b32 Result = (Entity->ID.ID & 0xFFFFFFFF00000000) != 0;
    return Result;
}

world_entity* 
GetFirstEntity(world_entity_pool* Pool)
{
    for(u32 Index = 0; Index < MAX_WORLD_ENTITIES; Index++)
    {
        world_entity* Result = Pool->Entities + Index;
        if(IsEntityAllocated(Result))
            return Result;
    }
    
    return NULL;
}

world_entity* 
GetNextEntity(world_entity_pool* Pool, world_entity* Entity)
{
    ASSERT(IsEntityAllocated(Entity));    
    i32 Index = SafeI32(GetIndex(Pool, Entity->ID.ID)+1);
    
    for(Index; Index < MAX_WORLD_ENTITIES; Index++)
    {
        world_entity* Result = Pool->Entities + Index;
        if(IsEntityAllocated(Result))
            return Result;
    }
    
    return NULL;
}

world_entity_id 
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    world_entity* Entity = AllocateEntity(&World->EntityPool);
    
    Entity->ID = MakeEntityID(Entity->ID.ID, WorldIndex);
    
    Entity->Type = Type;    
    Entity->Transform = CreateSQT(Position, Scale, Euler);
    Entity->Color = Color;
    Entity->UserData = UserData;
    Entity->LinkID = InvalidEntityID();
    
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
    world_entity_id AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    world_entity_id BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
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
    world_entity_id AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0);
    world_entity_id BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1);
    
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

void 
CreatePlayer(game* Game, u32 WorldIndex, v3f Position, f32 Radius, f32 Height, c4 Color)
{    
    player* Player = GetPlayer(Game, WorldIndex);
    Player->Pushing = InitPushingState();
    
    Player->EntityID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f), V3(), Color, Player);
    
    world_entity* Entity = GetEntity(Game, Player->EntityID);    
    
    Entity->Collider.Type = COLLIDER_TYPE_VERTICAL_CAPSULE;
    Entity->Collider.VerticalCapsule.P = {};
    Entity->Collider.VerticalCapsule.Radius = Radius;
    Entity->Collider.VerticalCapsule.Height = Height;    
}

world_entity_id 
CreateBoxEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Dim, c4 Color)
{
    world_entity_id Result = CreateEntity(Game, Type, WorldIndex, Position, V3(1.0f), V3(), Color);    
    world_entity* Entity = GetEntity(Game, Result);
    
    Entity->Collider.Type = COLLIDER_TYPE_ALIGNED_BOX;
    Entity->Collider.AlignedBox.CenterP = {};
    Entity->Collider.AlignedBox.CenterP.z = Dim.z*0.5f;
    Entity->Collider.AlignedBox.Dim = Dim;    
    
    return Result;
}

void 
CreateBoxEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Dim, c4 Color0, c4 Color1)
{    
    CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    CreateBoxEntity(Game, Type, 1, Position, Dim, Color1); 
}

void 
CreateSingleLinkedBoxEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position, v3f Dim, c4 Color0, c4 Color1)
{
    world_entity_id AID = CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    world_entity_id BID = CreateBoxEntity(Game, Type, 1, Position, Dim, Color1);
    
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

inline void 
CreateBlockersInBothWorlds(game* Game, v3f P0, f32 Height0, v3f P1, f32 Height1)
{
    CreateBlocker(Game, 0, P0, Height0, P1, Height1);
    CreateBlocker(Game, 1, P0, Height0, P1, Height1);
}

inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

inline world_entity* 
IsPlayerPushingObject(game* Game, player* Player, v2f MoveDirection)
{
    f32 Epsilon = 1e-4f;
    
    if((Player->State == PLAYER_STATE_PUSHING) && 
       !IsInvalidEntityID(Player->Pushing.EntityID) && 
       AreEqual(Player->Pushing.Direction, MoveDirection, Epsilon))
    {        
        world_entity* Entity = GetEntity(Game, Player->Pushing.EntityID);                        
        b32 Result = (Entity->Type == WORLD_ENTITY_TYPE_PUSHABLE);
        if(Result)        
            return Entity;                
    }
    
    return NULL;        
}

inline b32 
IsInRangeOfBlockerZ(blocker* Blocker, f32 BottomZ, f32 TopZ)
{
    f32 BlockerZ0 = Blocker->P0.z;
    f32 BlockerHeight0 = BlockerZ0 + Blocker->Height0;
    f32 BlockerZ1 = Blocker->P1.z;
    f32 BlockerHeight1 = BlockerZ1 + Blocker->Height1;
    
    b32 Result = (IsRangeInInterval(BlockerZ0, BlockerHeight0, BottomZ, TopZ) ||
                  IsRangeInInterval(BlockerZ1, BlockerHeight1, BottomZ, TopZ));
    return Result;
}

vertical_capsule 
GetWorldSpaceVerticalCapsule(vertical_capsule Capsule, sqt Transform)
{
    Transform.Orientation = IdentityQuaternion();
    
    vertical_capsule Result;
    Result.P = TransformV3(Capsule.P, Transform);
    Result.Radius = Capsule.Radius * MaximumF32(Transform.Scale.x, Transform.Scale.y);
    Result.Height = Capsule.Height * Transform.Scale.z;
    return Result;
}

aligned_box 
GetWorldSpaceAlignedBox(aligned_box Box, sqt Transform)
{
    Transform.Orientation = IdentityQuaternion();
    
    aligned_box Result;
    Result.CenterP = TransformV3(Box.CenterP, Transform);
    Result.Dim = Box.Dim*Transform.Scale;
    return Result;
}

world_entity* 
ClearEntityVelocity(game* Game, world_entity_id ID)
{
    if(!IsInvalidEntityID(ID))
    {        
        world_entity* Entity = GetEntity(Game, ID);
        ASSERT(IsEntityAllocated(Entity));                        
        Entity->Velocity = {};                        
        return Entity;
    }
    
    return NULL;
}

void 
OnWorldSwitch(game* Game, u32 LastWorldIndex, u32 CurrentWorldIndex)
{
    player* LastPlayer = GetPlayer(Game, LastWorldIndex);    
    ClearEntityVelocity(Game, LastPlayer->EntityID);            
    world_entity* PushingEntity = ClearEntityVelocity(Game, LastPlayer->Pushing.EntityID);
    if(PushingEntity)
        ClearEntityVelocity(Game, PushingEntity->LinkID);
            
}

void 
ResolveImpact(world_entity* Entity, v2f* MoveDelta, time_result_2D TimeResult)
{       
    v2f TargetPosition = Entity->Position.xy + *MoveDelta;    
    
    Entity->Position.xy = TimeResult.ContactPoint;
    *MoveDelta = TargetPosition - TimeResult.ContactPoint;
    
    if(TimeResult.Normal != 0)
    {
        *MoveDelta -= Dot(*MoveDelta, TimeResult.Normal)*TimeResult.Normal;
        Entity->Velocity.xy -= Dot(Entity->Velocity.xy, TimeResult.Normal)*TimeResult.Normal;                        
    }
}

time_of_impact_result 
FindTOI(game* Game, world_entity* Entity, v2f MoveDelta)
{    
    time_of_impact_result Result;
    Result.TimeResult = InvalidTimeResult2D();
    Result.HitEntityID = InvalidEntityID();    
    
    world* World = GetWorld(Game, Entity->ID);
    
    switch(Entity->Collider.Type)
    {
        case COLLIDER_TYPE_ALIGNED_BOX:
        {
            aligned_box EntityBox = GetWorldSpaceAlignedBox(Entity->Collider.AlignedBox, Entity->Transform);
            
            v2f TargetPosition = EntityBox.CenterP.xy + MoveDelta;
                        
            f32 HalfDimZ = EntityBox.Dim.z*0.5f;
            f32 MinZ = EntityBox.CenterP.z-HalfDimZ;
            f32 MaxZ = EntityBox.CenterP.z+HalfDimZ;            
            
            for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
            {
                if(IsInRangeOfBlockerZ(Blocker, MinZ, MaxZ))
                {
                    time_result_2D TimeResult = MovingRectangleEdgeIntersectionTime2D(EntityBox.CenterP.xy, TargetPosition, EntityBox.Dim.xy, Blocker->P0.xy, Blocker->P1.xy);
                    if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))
                        Result.TimeResult = TimeResult;
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
                            
                            f32 TestHalfDimZ = TestEntityBox.Dim.z*0.5f;
                            f32 TestMinZ = TestEntityBox.CenterP.z-HalfDimZ;
                            f32 TestMaxZ = TestEntityBox.CenterP.z+HalfDimZ;
                            
                            if(IsRangeInInterval(TestMinZ, TestMaxZ, MinZ, MaxZ))
                            {
                                time_result_2D TimeResult = MovingRectangleRectangleIntersectionTime2D(EntityBox.CenterP.xy, TargetPosition, EntityBox.Dim.xy, 
                                                                                                       TestEntityBox.CenterP.xy, TestEntityBox.Dim.xy);
                                if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))
                                    Result.TimeResult = TimeResult;
                            }                            
                        } break;
                        
                        case COLLIDER_TYPE_VERTICAL_CAPSULE:
                        {
                            //NOT_IMPLEMENTED;
                        } break;
                        
                        INVALID_DEFAULT_CASE;
                    }
                }
            }            
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
                    if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))
                        Result.TimeResult = TimeResult;
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
                                if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))
                                {
                                    Result.TimeResult = TimeResult;
                                    Result.HitEntityID = TestEntity->ID;                                    
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

void 
UpdateWorld(game* Game)
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
                
                if(IsPlayerPushingObject(Game, Player, MoveDirection))
                    continue;
                
                world_entity* PushingEntity = ClearEntityVelocity(Game, Player->Pushing.EntityID);
                if(PushingEntity)                    
                    ClearEntityVelocity(Game, PushingEntity->LinkID);        
                
                APPLY_VELOCITY(Entity);                
                
                //DetectCollisions(Game, Entity);
                
                v2f MoveDelta = Entity->Velocity.xy*dt;
                
                for(u32 Iterations = 0; ; Iterations++)
                {
                    DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                    
                    if(SquareMagnitude(MoveDelta) <= MOVE_DELTA_EPSILON)
                        break;
                    
                    time_of_impact_result TOI = FindTOI(Game, Entity, MoveDelta);
                    if(IsInvalidTimeResult2D(TOI.TimeResult))
                    {
                        Entity->Position.xy += MoveDelta;
                        break;
                    }
                    
                    ResolveImpact(Entity, &MoveDelta, TOI.TimeResult);
                    
                    if((TOI.TimeResult.Normal != 0) && AreEqual(MoveDirection, -TOI.TimeResult.Normal, 1e-4f))
                    {
                        PlayerState = PLAYER_STATE_PUSHING;
                        Pushing.EntityID = TOI.HitEntityID;
                        Pushing.Direction = MoveDirection;
                    }                    
                }
            } break;
            
            case WORLD_ENTITY_TYPE_PUSHABLE:
            {
                player* Player = GetPlayer(Game, Entity->ID.WorldIndex);
                if(IsPlayerPushingObject(Game, Player, MoveDirection) == Entity)
                {
                    world_entity* PlayerEntity = GetEntity(Game, Player->EntityID);
                    PlayerEntity->Velocity = {};
                    
                    world_entity* TestEntities[2] = {Entity, GetEntityOrNull(Game, Entity->LinkID)};                    
                    b32 StopProcessing[2] = {};
                    v2f MoveDeltas[2];
                    
                    APPLY_VELOCITY(TestEntities[0]);
                    MoveDeltas[0] = TestEntities[0]->Velocity.xy*dt;
                    
                    if(TestEntities[1])
                    {
                        APPLY_VELOCITY(TestEntities[1]);
                        MoveDeltas[1] = TestEntities[1]->Velocity.xy*dt;
                    }
                    else
                        StopProcessing[1] = true;
                    
                    
                    for(u32 Iterations = 0; ; Iterations++)
                    {
                        DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                        
                        time_of_impact_result TOIs[2] = {};
                        
                        for(u32 ObjectIndex = 0; ObjectIndex < 2; ObjectIndex++)
                        {
                            if(StopProcessing[ObjectIndex])
                                continue;
                            
                            //TODO(JJ): Need to find someway of pruning out collisions that we don't want to check. In this instance, the player and pushable object
                            if(SquareMagnitude(MoveDeltas[ObjectIndex]) <= MOVE_DELTA_EPSILON)                            
                                StopProcessing[ObjectIndex] = true;                                                            
                            else                            
                                TOIs[ObjectIndex] = FindTOI(Game, TestEntities[ObjectIndex], MoveDeltas[ObjectIndex]);
                        }
                        
                        for(u32 ObjectIndex = 0; ObjectIndex < 2; ObjectIndex++)
                        {
                            if(StopProcessing[ObjectIndex])
                                continue;
                            
                            v2f Delta = MoveDeltas[ObjectIndex];
                            
                            if(IsInvalidTimeResult2D(TOIs[ObjectIndex].TimeResult))
                            {
                                TestEntities[ObjectIndex]->Position.xy += MoveDeltas[ObjectIndex];                                
                                StopProcessing[ObjectIndex] = true;                                
                            }
                            else
                            {                                
                                Delta = TOIs[ObjectIndex].TimeResult.ContactPoint - TestEntities[ObjectIndex]->Position.xy;                                
                                ResolveImpact(TestEntities[ObjectIndex], MoveDeltas+ObjectIndex, TOIs[ObjectIndex].TimeResult);                                                                    
                            }   
                            
                            if(AreEqualIDs(Player->Pushing.EntityID, TestEntities[ObjectIndex]->ID))
                                PlayerEntity->Position.xy += Delta;                            
                        }
                        
                        if(StopProcessing[0] && StopProcessing[1])
                            break;
                    }
                    
                    Pushing = Player->Pushing;
                    PlayerState = Player->State;
                }
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
                DEVELOPER_MAX_WALKING_TRIANGLE();
            }
        }
    }
    
    ASSERT(BestPointZ != -FLT_MAX);    
    PlayerEntity->Position.z = BestPointZ;                        
}

