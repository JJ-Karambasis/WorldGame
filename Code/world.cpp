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

inline world* 
GetNotCurrentWorld(game* Game)
{
    world* World = GetWorld(Game, !Game->CurrentWorldIndex);
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
    world_entity* Result = GetByID(Pool, EntityID.ID);    
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
GetPlayerEntity(world* World)
{    
    world_entity* Result = GetEntity(World, World->Player.EntityID);
    return Result;
}

inline world_entity*
GetPlayerEntity(game* Game, u32 WorldIndex)
{
    world* World = GetWorld(Game, WorldIndex);        
    world_entity* Result = GetPlayerEntity(World);
    return Result;
}

inline v3f
GetPlayerPosition(game* Game, player* Player)
{
    v3f Result = GetEntity(Game, Player->EntityID)->Position;
    return Result;
}

void FreeEntity(game* Game, world_entity_id ID)
{
    world* World = GetWorld(Game, ID);
    FreeFromPool(&World->EntityPool, ID.ID);
}

world_entity_id
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color, mesh* Mesh, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    
    i64 EntityID = AllocateFromPool(&World->EntityPool);
    world_entity* Entity = GetByID(&World->EntityPool, EntityID);
    
    Entity->ID = MakeEntityID(EntityID, WorldIndex);
    
    Entity->Type = Type;    
    Entity->Transform = CreateSQT(Position, Scale, Euler);
    Entity->Color = Color;
    Entity->Mesh = Mesh;
    Entity->UserData = UserData;    
    Entity->LinkID = InvalidEntityID();
    
    return Entity->ID;
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh0, mesh* Mesh1)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0, Mesh0);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1, Mesh1);    
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0, Mesh);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1, Mesh);    
}

inline void
CreateDualLinkedEntities(game* Game, world_entity_type Type, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1, 
                         mesh* Mesh0, mesh* Mesh1)
{
    world_entity_id AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0, Mesh0);
    world_entity_id BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1, Mesh1);
    
    GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
    GetEntity(&Game->Worlds[1], BID)->LinkID = AID;    
}

inline void
CreateDualLinkedEntities(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh)                      
{
    CreateDualLinkedEntities(Game, Type, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1, Mesh, Mesh);    
}

inline void
CreateSingleLinkedEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position0, v3f Position1, v3f Scale0, v3f Scale1, v3f Euler0, v3f Euler1, c4 Color0, c4 Color1,
                           mesh* Mesh0, mesh* Mesh1)
{
    world_entity_id AID = CreateEntity(Game, Type, 0, Position0, Scale0, Euler0, Color0, Mesh0);
    world_entity_id BID = CreateEntity(Game, Type, 1, Position1, Scale1, Euler1, Color1, Mesh1);
    
    ASSERT((LinkWorldIndex == 0) || (LinkWorldIndex == 1));
    
    if(LinkWorldIndex == 0)
        GetEntity(&Game->Worlds[1], BID)->LinkID = AID;
    else
        GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
}

inline void
CreateSingleLinkedEntities(game* Game, world_entity_type Type, u32 LinkWorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh0, mesh* Mesh1)
{
    CreateSingleLinkedEntities(Game, Type, LinkWorldIndex, Position, Position, Scale, Scale, Euler, Euler, Color0, Color1, Mesh0, Mesh1);        
}

void 
CreatePlayer(game* Game, u32 WorldIndex, v3f Position, v3f Radius, c4 Color)
{    
    player* Player = GetPlayer(Game, WorldIndex);
    Player->Pushing = InitPushingState();
    //Player->AnimationController = CreateAnimationController(&Game->GameStorage, &Game->Assets->TestSkeleton);    
    //Player->AnimationController.PlayingAnimation.Clip = &Game->Assets->TestAnimation;
    Player->EntityID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f), V3(), Color, &Game->Assets->PlayerMesh, Player);
    
    Player->Radius = Radius;    
}

ellipsoid3D
GetPlayerEllipsoid(game* Game, player* Player)
{   
    world_entity* PlayerEntity = GetEntity(Game, Player->EntityID);
    
    ellipsoid3D Result;
    Result.Radius = Player->Radius * PlayerEntity->Scale;    
    Result.CenterP = V3(PlayerEntity->Position.xy, PlayerEntity->Position.z + Result.Radius.z);        
    return Result;
}

void SetPlayerEllipsoidP(game* Game, player* Player, v3f CenterP)
{
    world_entity* PlayerEntity = GetEntity(Game, Player->EntityID);
    f32 ZDim = Player->Radius.z * PlayerEntity->Scale.z;    
    PlayerEntity->Position = V3(CenterP.xy, CenterP.z - ZDim);    
}

world_entity_id 
CreateBoxEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Dim, c4 Color)
{
    world_entity_id Result = CreateEntity(Game, Type, WorldIndex, Position, Dim, V3(), Color, &Game->Assets->BoxGraphicsMesh);    
    world_entity* Entity = GetEntity(Game, Result);
    
    Entity->Collider.Type = COLLIDER_TYPE_ALIGNED_BOX;
    Entity->Collider.AlignedBox.CenterP = {};
    Entity->Collider.AlignedBox.CenterP.z = 0.5f;
    Entity->Collider.AlignedBox.Dim = V3(1.0f, 1.0f, 1.0f);    
    
    return Result;
}

void 
CreateBoxEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Dim, c4 Color0, c4 Color1, world_entity_id* IDs=NULL)
{    
    world_entity_id A = CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    world_entity_id B = CreateBoxEntity(Game, Type, 1, Position, Dim, Color1); 
    
    if(IDs)
    {
        IDs[0] = A;
        IDs[1] = B;
    }
}

void 
CreateBoxEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Dim, c4 Color, world_entity_id* IDs=NULL)
{    
    CreateBoxEntityInBothWorlds(Game, Type, Position, Dim, Color, Color, IDs);    
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

void 
CreateDualLinkedBoxEntities(game* Game, world_entity_type Type, v3f Position, v3f Dim, c4 Color0, c4 Color1)
{
    world_entity_id AID = CreateBoxEntity(Game, Type, 0, Position, Dim, Color0);
    world_entity_id BID = CreateBoxEntity(Game, Type, 1, Position, Dim, Color1);
    
    GetEntity(&Game->Worlds[0], AID)->LinkID = BID;
    GetEntity(&Game->Worlds[1], BID)->LinkID = AID;    
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

vertical_capsule 
GetWorldSpaceVerticalCapsule(world_entity* Entity)
{
    ASSERT(Entity->Collider.Type == COLLIDER_TYPE_VERTICAL_CAPSULE);
    vertical_capsule Result = GetWorldSpaceVerticalCapsule(Entity->Collider.VerticalCapsule, Entity->Transform);
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

aligned_box 
GetWorldSpaceAlignedBox(world_entity* Entity)
{
    ASSERT(Entity->Collider.Type == COLLIDER_TYPE_ALIGNED_BOX);
    aligned_box Result = GetWorldSpaceAlignedBox(Entity->Collider.AlignedBox, Entity->Transform);
    return Result;    
}

world_entity* 
ClearEntityVelocity(game* Game, world_entity_id ID)
{
    if(!IsInvalidEntityID(ID))
    {        
        world_entity* Entity = GetEntity(Game, ID);
        ASSERT(IsAllocatedID(ID.ID));
        Entity->Velocity = {};                        
        return Entity;
    }
    
    return NULL;
}

inline f32
GetCapsuleHeight(vertical_capsule* Capsule)
{
    f32 Result = Capsule->P.z+Capsule->Height+(Capsule->Radius*2);
    return Result;
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
ResolveImpact(world_entity* Entity, v2f NewPosition, v2f Normal, v2f* MoveDelta)
{       
    v2f TargetPosition = Entity->Position.xy + *MoveDelta;    
    
    NewPosition += Normal*0.00001f;
    
    Entity->Position.xy = NewPosition;
    *MoveDelta = TargetPosition - NewPosition;
    
    if(Normal != 0)
    {
        *MoveDelta -= Dot(*MoveDelta, Normal)*Normal;
        Entity->Velocity.xy -= Dot(Entity->Velocity.xy, Normal)*Normal;                        
    }
}

void ResolvePushableImpact(world_entity* PlayerEntity, world_entity* Entity, v2f NewPosition, v2f Normal, v2f* MoveDelta)
{
    ASSERT(PlayerEntity->Type == WORLD_ENTITY_TYPE_PLAYER);
    v2f OldP = Entity->Position.xy;
    
    ResolveImpact(Entity, NewPosition, Normal, MoveDelta);
    
    player* Player = (player*)PlayerEntity->UserData;
    
    v2f Delta = Entity->Position.xy - OldP;
    if(AreEqualIDs(Player->Pushing.EntityID, Entity->ID))
        PlayerEntity->Position.xy += Delta;                            
}

b32 
ResolveEntity(world_entity* Entity, time_result_2D TimeResult, v2f* MoveDelta)
{    
    if(IsInvalidTimeResult2D(TimeResult))
    {
        Entity->Position.xy += (*MoveDelta);                                
        return true;        
    }
    
    ResolveImpact(Entity, TimeResult.ContactPoint, TimeResult.Normal, MoveDelta);                                                                                                    
    return false;
}

b32 ResolvePushableEntity(world_entity* PlayerEntity, world_entity* Entity, time_result_2D TimeResult, v2f* MoveDelta)
{    
    ASSERT(PlayerEntity->Type == WORLD_ENTITY_TYPE_PLAYER);
    v2f OldP = Entity->Position.xy;
    
    b32 Result = ResolveEntity(Entity, TimeResult, MoveDelta);    
    
    player* Player = (player*)PlayerEntity->UserData;
    
    v2f Delta = Entity->Position.xy - OldP;
    if(AreEqualIDs(Player->Pushing.EntityID, Entity->ID))
        PlayerEntity->Position.xy += Delta;                            
    
    return Result;
}

time_of_impact_result 
FindTOI(game* Game, world_entity* Entity, v2f MoveDelta, world_entity_id CullID)
{    
    time_of_impact_result Result;
    Result.TimeResult = InvalidTimeResult2D();
    Result.HitEntityID = InvalidEntityID();    
    
    world* World = GetWorld(Game, Entity->ID);
    
    switch(Entity->Collider.Type)
    {
        case COLLIDER_TYPE_ALIGNED_BOX:
        {
            aligned_box EntityBox = GetWorldSpaceAlignedBox(Entity);
            
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
            
            pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);                                    
            for(world_entity* TestEntity = GetFirst(&Iter); TestEntity; TestEntity = GetNext(&Iter))
            {                             
                if((TestEntity->Type != WORLD_ENTITY_TYPE_WALKABLE) && (TestEntity != Entity))
                {   
                    if(AreEqualIDs(TestEntity->ID, CullID))
                        continue;
                    
                    switch(TestEntity->Collider.Type)
                    {
                        case COLLIDER_TYPE_ALIGNED_BOX:
                        {
                            aligned_box TestEntityBox = GetWorldSpaceAlignedBox(TestEntity);
                            
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
                            vertical_capsule TestEntityCapsule = GetWorldSpaceVerticalCapsule(TestEntity);                            
                            f32 CapsuleHeight = GetCapsuleHeight(&TestEntityCapsule);                            
                            
                            if(IsRangeInInterval(MinZ, MaxZ, TestEntityCapsule.P.z, CapsuleHeight))
                            {
                                time_result_2D TimeResult = MovingRectangleCircleIntersectionTime2D(EntityBox.CenterP.xy, TargetPosition, EntityBox.Dim.xy, TestEntityCapsule.P.xy, TestEntityCapsule.Radius);
                                if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))
                                {                                                                   
                                    Result.TimeResult = TimeResult;
                                    Result.HitEntityID = TestEntity->ID;
                                }
                            }                            
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
            f32 CapsuleHeight = GetCapsuleHeight(&EntityCapsule);
            
            for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
            {
                if(IsInRangeOfBlockerZ(Blocker, EntityCapsule.P.z, CapsuleHeight))
                {                    
                    time_result_2D TimeResult = MovingCircleEdgeIntersectionTime2D(EntityCapsule.P.xy, TargetPosition, EntityCapsule.Radius, Blocker->P0.xy, Blocker->P1.xy);                    
                    if(!IsInvalidTimeResult2D(TimeResult) && (Result.TimeResult.Time > TimeResult.Time))                    
                        Result.TimeResult = TimeResult;                                                                    
                }                                    
            }
            
            pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);
            for(world_entity* TestEntity = GetFirst(&Iter); TestEntity; TestEntity = GetNext(&Iter))
            {
                if((TestEntity->Type != WORLD_ENTITY_TYPE_WALKABLE) && (TestEntity != Entity))
                {   
                    if(AreEqualIDs(TestEntity->ID, CullID))
                        continue;
                    
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
                            //CONFIRM(JJ): Will we even need to support this case?
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

struct time_result
{    
    v3f ContactPoint;    
    f32 t;
    v3f Normal;    
    b32 Intersected;    
};

b32 SolveSphereSweepRoot(f32 a, f32 b, f32 c, f32 tCurrent, f32* tOut)
{    
    quadratic_equation_result RootSolver = SolveQuadraticEquation(a, b, c);
    if(RootSolver.RootCount > 0)
    {
        if(RootSolver.RootCount == 1)
        {
            if((RootSolver.Roots[0] > 0.0f) && (RootSolver.Roots[0] < tCurrent))                                                
            {
                *tOut = RootSolver.Roots[0];
                return true;
            }
        }
        else
        {
            if(RootSolver.Roots[0] > RootSolver.Roots[1])
                SWAP(RootSolver.Roots[0], RootSolver.Roots[1]);
            
            if((RootSolver.Roots[0] > 0) && (RootSolver.Roots[0] < tCurrent))
            {
                *tOut = RootSolver.Roots[0];
                return true;
            }
            
            if((RootSolver.Roots[1] > 0) && (RootSolver.Roots[1] < tCurrent))
            {
                *tOut = RootSolver.Roots[1];                                            
                return true;
            }
        }                                                                                
    }
    
    return false;
}

time_result HandleEllipsoidCollisions(world* World, assets* Assets, ellipsoid3D Ellipsoid, v3f MoveDelta)
{   
    time_result Result = {};
    
    v3f InvRadius = 1.0f/Ellipsoid.Radius;
    
    v3f ESpacePosition = Ellipsoid.CenterP*InvRadius;    
    v3f ESpaceDelta = MoveDelta*InvRadius;    
    
    f32 tMin = INFINITY;
    v3f ESpaceContactPoint = InvalidV3();
    
    FOR_EACH(TestEntity, &World->EntityPool)
    {
        if(TestEntity->Type == WORLD_ENTITY_TYPE_WALKABLE)
        {
            walkable_mesh* TriangleMesh = &Assets->BoxWalkableMesh;
            
            for(u32 TriangleIndex = 0; TriangleIndex < TriangleMesh->TriangleCount; TriangleIndex++)
            {   
                //NOTE(EVERYONE): Please see https://www.peroxide.dk/papers/collision/collision.pdf for the algorithm
                triangle3D Triangle = TransformTriangle3D(TriangleMesh->Triangles[TriangleIndex], TestEntity->Transform);
                
                v3f ESpaceTriangle[3] = 
                {
                    Triangle.P[0]*InvRadius, 
                    Triangle.P[1]*InvRadius,
                    Triangle.P[2]*InvRadius
                };
                
                plane3D ESpaceTrianglePlane = CreatePlane3D(ESpaceTriangle);    
                
                f32 Denominator = Dot(ESpaceTrianglePlane.Normal, ESpaceDelta);                            
                f32 SignedDistanceToPlane = SignedDistance(ESpacePosition, ESpaceTrianglePlane);
                
                f32 t0, t1;
                
                b32 IsEmbedded = false;
                if(Denominator == 0)
                {
                    if(Abs(SignedDistanceToPlane) >= 1.0f)
                        continue;                                                                
                    
                    t0 = 0.0f;
                    t1 = 1.0f;
                    IsEmbedded = true;
                }
                else
                {
                    f32 InvDenominator = 1.0f/Denominator;
                    t0 =  ( 1.0f - SignedDistanceToPlane)*InvDenominator;
                    t1 =  (-1.0f - SignedDistanceToPlane)*InvDenominator;                                            
                    
                    if(t0 > t1) SWAP(t0, t1);                                            
                    
                    if(t0 > 1.0f || t1 < 0.0f)
                        continue;                                            
                    
                    t0 = SaturateF32(t0);
                    t1 = SaturateF32(t1);                                                                                        
                }
                
                b32 FoundCollision = false;
                
                b32 HasIntersected = false;
                v3f IntersectionPoint = InvalidV3();
                f32 t = 1.0f;
                
                if(!IsEmbedded)
                {
                    v3f PlaneIntersectionPoint = (ESpacePosition - ESpaceTrianglePlane.Normal) + t0*ESpaceDelta;
                    if(IsPointProjectedInTriangle3D(ESpaceTriangle, PlaneIntersectionPoint))
                    {
                        IntersectionPoint = PlaneIntersectionPoint;
                        t = t0;
                        HasIntersected = true;
                    }
                }
                
                if(!HasIntersected)
                {
                    f32 ESpaceDeltaSqrLength = SquareMagnitude(ESpaceDelta);
                    
                    //NOTE(EVERYONE): Perform a sphere sweep test against the vertices
                    {
                        f32 tVertex;                                        
                        f32 a = ESpaceDeltaSqrLength;
                        
                        f32 b = 2.0f*(Dot(ESpaceDelta, ESpacePosition-ESpaceTriangle[0]));
                        f32 c = SquareMagnitude(ESpaceTriangle[0]-ESpacePosition) - 1.0f;                                                                        
                        if(SolveSphereSweepRoot(a, b, c, t, &tVertex))
                        {
                            IntersectionPoint = ESpaceTriangle[0];
                            t = tVertex;                                        
                            HasIntersected = true;
                        }
                        
                        b = 2.0f*(Dot(ESpaceDelta, ESpacePosition-ESpaceTriangle[1]));
                        c = SquareMagnitude(ESpaceTriangle[1]-ESpacePosition) - 1.0f;
                        if(SolveSphereSweepRoot(a, b, c, t, &tVertex))
                        {
                            IntersectionPoint = ESpaceTriangle[1];
                            t = tVertex;
                            HasIntersected = true;
                        }
                        
                        b = 2.0f*(Dot(ESpaceDelta, ESpacePosition-ESpaceTriangle[2]));
                        c = SquareMagnitude(ESpaceTriangle[2]-ESpacePosition)-1.0f;
                        if(SolveSphereSweepRoot(a, b, c, t, &tVertex))
                        {
                            IntersectionPoint = ESpaceTriangle[2];
                            t = tVertex;
                            HasIntersected = true;
                        }
                    }                                    
                    
                    //NOTE(EVERYONE): Perform a sphere sweep test against the edges
                    {
                        f32 tEdge;
                        
                        v3f Edge = ESpaceTriangle[1]-ESpaceTriangle[0];
                        v3f BaseToVertex = ESpaceTriangle[0] - ESpacePosition;
                        
                        f32 EdgeSqrLength = SquareMagnitude(Edge);
                        f32 EdgeDotVelocity = Dot(Edge, ESpaceDelta);
                        f32 EdgeDotBaseToVertex = Dot(Edge, BaseToVertex);
                        
                        f32 a = (EdgeSqrLength * -ESpaceDeltaSqrLength) + Square(EdgeDotVelocity);
                        f32 b = (EdgeSqrLength * 2*Dot(ESpaceDelta, BaseToVertex)) - 2.0f*EdgeDotVelocity*EdgeDotBaseToVertex;
                        f32 c = (EdgeSqrLength * (1-SquareMagnitude(BaseToVertex))) + Square(EdgeDotBaseToVertex);
                        
                        if(SolveSphereSweepRoot(a, b, c, t, &tEdge))
                        {
                            f32 f = ((EdgeDotVelocity*tEdge) - EdgeDotBaseToVertex) / EdgeSqrLength;
                            if((f >= 0.0f) && (f <= 1.0f))
                            {
                                IntersectionPoint = ESpaceTriangle[0] + f*Edge;
                                t = tEdge;
                                HasIntersected = true;
                            }
                        }
                        
                        Edge = ESpaceTriangle[2] - ESpaceTriangle[1];
                        BaseToVertex = ESpaceTriangle[1] - ESpacePosition;
                        
                        EdgeSqrLength = SquareMagnitude(Edge);
                        EdgeDotVelocity = Dot(Edge, ESpaceDelta);
                        EdgeDotBaseToVertex = Dot(Edge, BaseToVertex);
                        
                        a = (EdgeSqrLength * -ESpaceDeltaSqrLength) + Square(EdgeDotVelocity);
                        b = (EdgeSqrLength * 2*Dot(ESpaceDelta, BaseToVertex)) - 2.0f*EdgeDotVelocity*EdgeDotBaseToVertex;
                        c = (EdgeSqrLength * (1-SquareMagnitude(BaseToVertex))) + Square(EdgeDotBaseToVertex);                                            
                        
                        if(SolveSphereSweepRoot(a, b, c, t, &tEdge))
                        {
                            f32 f = ((EdgeDotVelocity*tEdge) - EdgeDotBaseToVertex) / EdgeSqrLength;
                            if((f >= 0.0f) && (f <= 1.0f))
                            {
                                IntersectionPoint = ESpaceTriangle[1] + f*Edge;
                                t = tEdge;
                                HasIntersected = true;
                            }
                        }
                        
                        Edge = ESpaceTriangle[0] - ESpaceTriangle[2];
                        BaseToVertex = ESpaceTriangle[2] - ESpacePosition;
                        
                        EdgeSqrLength = SquareMagnitude(Edge);
                        EdgeDotVelocity = Dot(Edge, ESpaceDelta);
                        EdgeDotBaseToVertex = Dot(Edge, BaseToVertex);
                        
                        a = (EdgeSqrLength * -ESpaceDeltaSqrLength) + Square(EdgeDotVelocity);
                        b = (EdgeSqrLength * 2*Dot(ESpaceDelta, BaseToVertex)) - 2.0f*EdgeDotVelocity*EdgeDotBaseToVertex;
                        c = (EdgeSqrLength * (1-SquareMagnitude(BaseToVertex))) + Square(EdgeDotBaseToVertex);                                            
                        
                        if(SolveSphereSweepRoot(a, b, c, t, &tEdge))
                        {
                            f32 f = ((EdgeDotVelocity*tEdge) - EdgeDotBaseToVertex) / EdgeSqrLength;
                            if((f >= 0.0f) && (f <= 1.0f))
                            {
                                IntersectionPoint = ESpaceTriangle[2] + f*Edge;
                                t = tEdge;
                                HasIntersected = true;
                            }
                        }                                            
                    }
                }
                
                if(HasIntersected && (tMin > t))
                {
                    Result.Intersected = true;                                                
                    tMin = t;
                    ESpaceContactPoint = IntersectionPoint;                    
                }    
                
            }
        }                                        
    }               
    
    if(Result.Intersected)
    {        
        Result.t = tMin;
        Result.ContactPoint = ESpaceContactPoint*Ellipsoid.Radius;        
        Result.Normal = Normalize(((ESpacePosition+ESpaceDelta*tMin) - ESpaceContactPoint)*InvRadius);                        
    }    
    
    return Result;
}

#define APPLY_VELOCITY(Entity) \
Entity->Velocity.xy += MoveAcceleration*dt; \
Entity->Velocity.xy *= VelocityDamping

#if 0 
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
    
#define MOVE_DELTA_EPSILON 1e-4f    
    
    world* World = GetWorld(Game, Game->CurrentWorldIndex);         
    
    pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);
    for(world_entity* Entity = GetFirst(&Iter); Entity; Entity = GetNext(&Iter))
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
                
                v2f MoveDelta = Entity->Velocity.xy*dt;                                
                
                for(u32 Iterations = 0; ; Iterations++)
                {
                    DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                    
                    if(SquareMagnitude(MoveDelta) <= MOVE_DELTA_EPSILON)
                        break;
                    
                    time_of_impact_result TOI = FindTOI(Game, Entity, MoveDelta, InvalidEntityID());
                    
                    if(ResolveEntity(Entity, TOI.TimeResult, &MoveDelta))
                        break;
                    
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
                    v2f MoveDeltas[2];
                    
                    APPLY_VELOCITY(TestEntities[0]);
                    MoveDeltas[0] = TestEntities[0]->Velocity.xy*dt;
                    
                    b32 DualLinkedEntities = false;
                    if(TestEntities[1])
                    {
                        APPLY_VELOCITY(TestEntities[1]);
                        MoveDeltas[1] = TestEntities[1]->Velocity.xy*dt;                        
                        DualLinkedEntities = AreEqualIDs(TestEntities[1]->LinkID, TestEntities[0]->ID);
                    }                                        
                    
                    if(DualLinkedEntities)
                    {
                        for(u32 Iterations = 0; ; Iterations++)
                        {
                            DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                            
                            time_of_impact_result TOIs[2] = {};
                            
                            
                            if((SquareMagnitude(MoveDeltas[0]) <= MOVE_DELTA_EPSILON) || (SquareMagnitude(MoveDeltas[1]) <= MOVE_DELTA_EPSILON))
                                break;
                            
                            TOIs[0] = FindTOI(Game, TestEntities[0], MoveDeltas[0], PlayerEntity->ID);
                            TOIs[1] = FindTOI(Game, TestEntities[1], MoveDeltas[1], PlayerEntity->ID);                            
                            
                            u32 Index = 0;
                            if(IsInvalidTimeResult2D(TOIs[0].TimeResult) && IsInvalidTimeResult2D(TOIs[1].TimeResult))
                            {
                                TestEntities[0]->Position.xy += MoveDeltas[0];
                                TestEntities[1]->Position.xy += MoveDeltas[1];
                                
                                if(AreEqualIDs(Player->Pushing.EntityID, TestEntities[0]->ID))
                                    PlayerEntity->Position.xy += MoveDeltas[0];
                                else if(AreEqualIDs(Player->Pushing.EntityID, TestEntities[1]->ID))
                                    PlayerEntity->Position.xy += MoveDeltas[1];                                
                                break;
                            }                            
                            else if(TOIs[0].TimeResult.Time == 0.0f || TOIs[1].TimeResult.Time == 0.0f)
                            {   
                                if(TOIs[0].TimeResult.Time == 0)
                                    ResolvePushableImpact(PlayerEntity, TestEntities[0], TOIs[0].TimeResult.ContactPoint, TOIs[0].TimeResult.Normal, &MoveDeltas[0]);
                                
                                if(TOIs[1].TimeResult.Time == 0)
                                    ResolvePushableImpact(PlayerEntity, TestEntities[1], TOIs[1].TimeResult.ContactPoint, TOIs[1].TimeResult.Normal, &MoveDeltas[1]);                                                                
                            }
                            else
                            {
                                u32 SmallestIndex = (TOIs[0].TimeResult.Time <= TOIs[1].TimeResult.Time) ? 0 : 1;
                                u32 NotSmallestIndex = !SmallestIndex;
                                f32 SmallestT = TOIs[SmallestIndex].TimeResult.Time;
                                
                                v2f ResolveNormal = TOIs[SmallestIndex].TimeResult.Normal;
                                
                                ResolvePushableImpact(PlayerEntity, TestEntities[SmallestIndex], TestEntities[SmallestIndex]->Position.xy + MoveDeltas[SmallestIndex]*SmallestT, 
                                                      ResolveNormal, &MoveDeltas[SmallestIndex]);                                                                
                                
                                ResolvePushableImpact(PlayerEntity, TestEntities[NotSmallestIndex], TestEntities[NotSmallestIndex]->Position.xy + MoveDeltas[NotSmallestIndex]*SmallestT,
                                                      ResolveNormal, &MoveDeltas[NotSmallestIndex]);                                                                                                                                
                            }                                                                     
                        }
                    }
                    else
                    {           
                        b32 StopProcessing[2] = {};
                        if(!TestEntities[1]) StopProcessing[1] = true;
                        
                        for(u32 Iterations = 0; ; Iterations++)
                        {
                            DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                            
                            time_of_impact_result TOIs[2] = {};
                            
                            for(u32 ObjectIndex = 0; ObjectIndex < 2; ObjectIndex++)
                            {
                                if(StopProcessing[ObjectIndex])
                                    continue;
                                
                                if(SquareMagnitude(MoveDeltas[ObjectIndex]) <= MOVE_DELTA_EPSILON)                            
                                    StopProcessing[ObjectIndex] = true;                                                            
                                else                            
                                    TOIs[ObjectIndex] = FindTOI(Game, TestEntities[ObjectIndex], MoveDeltas[ObjectIndex], PlayerEntity->ID);
                            }                                                
                            
                            for(u32 ObjectIndex = 0; ObjectIndex < 2; ObjectIndex++)
                            {
                                if(StopProcessing[ObjectIndex])
                                    continue;
                                
                                if(ResolvePushableEntity(PlayerEntity, TestEntities[ObjectIndex], TOIs[ObjectIndex].TimeResult, &MoveDeltas[ObjectIndex]))
                                    StopProcessing[ObjectIndex] = true;                                                            
                            }
                            
                            if(StopProcessing[0] && StopProcessing[1])
                                break;                            
                        }
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
    
    Iter = BeginIter(&World->EntityPool);
    for(world_entity* Entity = GetFirst(&Iter); Entity; Entity = GetNext(&Iter))
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
    
    player* Player = (player*)PlayerEntity->UserData;
    
#if 0 
    animation_controller* Controller = &Player->AnimationController;
    playing_animation* PlayingAnimation = &Controller->PlayingAnimation;    
    skeleton* Skeleton = Controller->Skeleton;
    
    u32 PrevFrameIndex = FloorU32(PlayingAnimation->t*ANIMATION_FPS);
    u32 NextFrameIndex = PrevFrameIndex+1;
    
    f32 PrevFrameT = PrevFrameIndex*ANIMATION_HZ;
    f32 NextFrameT = NextFrameIndex*ANIMATION_HZ;
    
    f32 BlendFactor = (PlayingAnimation->t-PrevFrameT)/(NextFrameT-PrevFrameT);
    
    ASSERT((BlendFactor >= 0) && (BlendFactor <= 1));
    
    animation_frame* PrevFrame = PlayingAnimation->Clip->Frames + PrevFrameIndex;
    
    if(NextFrameIndex == PlayingAnimation->Clip->FrameCount)
        NextFrameIndex = 0;
    animation_frame* NextFrame = PlayingAnimation->Clip->Frames + NextFrameIndex;        
    
    for(u32 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {
        Controller->JointPoses[JointIndex] = InterpolatePose(PrevFrame->JointPoses[JointIndex], BlendFactor, NextFrame->JointPoses[JointIndex]);                        
    }
    
    PlayingAnimation->t += Game->dt;    
    if(PlayingAnimation->t >= GetClipDuration(PlayingAnimation->Clip))
        PlayingAnimation->t = 0.0;    
    
    GenerateGlobalPoses(Controller);
#endif
    ASSERT(BestPointZ != -FLT_MAX);    
    PlayerEntity->Position.z = BestPointZ;                        
    
    camera* Camera = &World->Camera;
    
    Camera->Position = PlayerEntity->Position;
    Camera->FocalPoint = PlayerEntity->Position;
    Camera->Position.z += 6.0f;
    Camera->Orientation = IdentityM3();    
}
#endif

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
    
    world* World = GetWorld(Game, Game->CurrentWorldIndex);         
    
    FOR_EACH(Entity, &World->EntityPool)
    {        
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
            {
                ASSERT(Entity->UserData);
                player* Player = (player*)Entity->UserData;
                
                Entity->Velocity.xy += MoveAcceleration*dt; 
                Entity->Velocity *= VelocityDamping;                
                v3f MoveDelta = Entity->Velocity*dt;   
                
                ellipsoid3D PlayerEllipsoid = GetPlayerEllipsoid(Game, Player);                
                                
                for(u32 Iterations = 0; ; Iterations++)
                {   
                    DEVELOPER_MAX_TIME_ITERATIONS(Iterations);
                    
#define MOVE_DELTA_EPSILON 1e-4f                        
                    if(SquareMagnitude(MoveDelta) <= MOVE_DELTA_EPSILON)
                        break;                                                            
                    
                    time_result CollisionResult = HandleEllipsoidCollisions(World, Game->Assets, PlayerEllipsoid, MoveDelta);
                    if(CollisionResult.Intersected)
                    {
                        v3f TargetPosition = PlayerEllipsoid.CenterP + MoveDelta;
                        
                        PlayerEllipsoid.CenterP += (MoveDelta*CollisionResult.t);
                        PlayerEllipsoid.CenterP += (CollisionResult.Normal*1e-5f);
                        MoveDelta = TargetPosition - PlayerEllipsoid.CenterP;
                        MoveDelta -= Dot(MoveDelta, CollisionResult.Normal)*CollisionResult.Normal;
                        Entity->Velocity -= Dot(Entity->Velocity, CollisionResult.Normal)*CollisionResult.Normal;
                    }
                    else
                    {
                        PlayerEllipsoid.CenterP += MoveDelta;
                        break;                                                            
                    }
                }
                
                SetPlayerEllipsoidP(Game, Player, PlayerEllipsoid.CenterP);                
            } break;                        
        }                        
    }    
    
    world_entity* PlayerEntity = GetEntity(World, World->Player.EntityID);    
    
    camera* Camera = &World->Camera;
    
    Camera->Position = PlayerEntity->Position;
    Camera->FocalPoint = PlayerEntity->Position;
    Camera->Position.z += 6.0f;
    Camera->Orientation = IdentityM3();    
}