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

inline v3f
GetPlayerVelocity(game* Game, player* Player)
{
    v3f Result = GetEntity(Game, Player->EntityID)->Velocity;
    return Result;
}

void FreeEntity(game* Game, world_entity_id ID)
{
    world* World = GetWorld(Game, ID);
    FreeFromPool(&World->EntityPool, ID.ID);
}

world_entity_id
CreateEntity(game* Game, world_entity_type Type, u32 WorldIndex, v3f Position, v3f Scale, v3f Euler, c4 Color, mesh* Mesh, walkable_mesh* WalkableMesh = NULL, void* UserData=NULL)
{
    world* World = GetWorld(Game, WorldIndex);
    
    i64 EntityID = AllocateFromPool(&World->EntityPool);
    world_entity* Entity = GetByID(&World->EntityPool, EntityID);
    
    Entity->ID = MakeEntityID(EntityID, WorldIndex);
    
    Entity->Type = Type;    
    Entity->Transform = CreateSQT(Position, Scale, Euler);
    Entity->Color = Color;
    Entity->Mesh = Mesh;
    Entity->WalkableMesh = WalkableMesh;
    Entity->UserData = UserData;    
    Entity->LinkID = InvalidEntityID();
    
    return Entity->ID;
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh0, mesh* Mesh1, walkable_mesh* WalkableMesh0=NULL, walkable_mesh* WalkableMesh1=NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0, Mesh0, WalkableMesh0);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1, Mesh1, WalkableMesh1);    
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color0, c4 Color1, mesh* Mesh, walkable_mesh* WalkableMesh = NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color0, Mesh, WalkableMesh);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color1, Mesh, WalkableMesh);    
}

inline void
CreateEntityInBothWorlds(game* Game, world_entity_type Type, v3f Position, v3f Scale, v3f Euler, c4 Color, mesh* Mesh, walkable_mesh* WalkableMesh = NULL)
{
    CreateEntity(Game, Type, 0, Position, Scale, Euler, Color, Mesh, WalkableMesh);
    CreateEntity(Game, Type, 1, Position, Scale, Euler, Color, Mesh, WalkableMesh);    
}

void 
CreatePlayer(game* Game, u32 WorldIndex, v3f Position, v3f Radius, c4 Color)
{    
    player* Player = GetPlayer(Game, WorldIndex);
    Player->Pushing = InitPushingState();
    //Player->AnimationController = CreateAnimationController(&Game->GameStorage, &Game->Assets->TestSkeleton);    
    //Player->AnimationController.PlayingAnimation.Clip = &Game->Assets->TestAnimation;
    Player->EntityID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f), V3(), Color, &Game->Assets->PlayerMesh, NULL, Player);
    
    Player->Radius = Radius;    
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

inline blocker*
CreateBlocker(game* Game, u32 WorldIndex, v3f P0, f32 Height0, v3f P1, f32 Height1)
{
    blocker* Blocker = AllocateListEntry(&GetWorld(Game, WorldIndex)->Blockers, &Game->GameStorage);
    Blocker->P0 = P0;
    Blocker->P1 = P1;
    Blocker->Height0 = Height0;
    Blocker->Height1 = Height1;
    
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

void 
OnWorldSwitch(game* Game, u32 LastWorldIndex, u32 CurrentWorldIndex)
{
    player* LastPlayer = GetPlayer(Game, LastWorldIndex);    
    ClearEntityVelocity(Game, LastPlayer->EntityID);            
    world_entity* PushingEntity = ClearEntityVelocity(Game, LastPlayer->Pushing.EntityID);
    if(PushingEntity)
        ClearEntityVelocity(Game, PushingEntity->LinkID);
    
}

b32 GetLowestRoot(f32 a, f32 b, f32 c, f32 MinRoot, f32* t)
{
    f32 Epsilon = 0.0f;
    
    quadratic_equation_result Result = SolveQuadraticEquation(a, b, c);
    if(Result.RootCount > 0)
    {
        if(Result.RootCount == 1)
        {
            if(Result.Roots[0] > 0 && Result.Roots[0] < MinRoot)
            {
                *t = Result.Roots[0];
                return true;
            }
        }
        else
        {                           
            if(Result.Roots[0] > Result.Roots[1])
                SWAP(Result.Roots[0], Result.Roots[1]);            
            
            if((Result.Roots[0]+Epsilon) > 0 && Result.Roots[0] < MinRoot)
            {
                *t = MaximumF32(Result.Roots[0], 0.0f);
                return true;
            }
            
            if((Result.Roots[1]+Epsilon) > 0 && Result.Roots[1] < MinRoot)
            {
                *t = MaximumF32(Result.Roots[1], 0.0f);
                return true;
            }                        
        }
    }    
    return false;
}

b32
SphereVertexSweepTest(v3f Vertex, v3f BasePoint, v3f Delta, f32 DeltaSquare, f32* t, v3f* ContactPoint)
{
    f32 a = DeltaSquare;
    f32 b = 2*(Dot(Delta, BasePoint-Vertex));
    f32 c = SquareMagnitude(Vertex-BasePoint) - 1;    
    
    f32 tVertex;
    b32 Result = GetLowestRoot(a, b, c, *t, &tVertex);
    if(Result)
    {
        *t = tVertex;
        *ContactPoint = Vertex;
    }
    
    return Result;    
}

b32
SphereEdgeSweepTest(v3f P0, v3f P1, v3f BasePoint, v3f Delta, f32 DeltaSquare, f32* t, v3f* ContactPoint)
{    
    v3f Edge = P1 - P0;
    v3f BaseToVertex = P0 - BasePoint;
    
    f32 EdgeSquare = SquareMagnitude(Edge);
    f32 EdgeDotDelta = Dot(Edge, Delta);
    f32 EdgeDotVertex = Dot(Edge, BaseToVertex);
    
    f32 a = EdgeSquare * -DeltaSquare + Square(EdgeDotDelta);
    f32 b = EdgeSquare * (2*Dot(Delta, BaseToVertex)) - 2*(EdgeDotDelta*EdgeDotVertex);
    f32 c = EdgeSquare * (1 - SquareMagnitude(BaseToVertex)) + Square(EdgeDotVertex);
    
    f32 tEdge;
    
    b32 Result = false;
    if(GetLowestRoot(a, b, c, *t, &tEdge))
    {
        f32 f = (EdgeDotDelta*tEdge - EdgeDotVertex) / EdgeSquare;
        Result = (f >= 0 && f <= 1);
        if(Result)
        {
            *t = tEdge;
            *ContactPoint = P0 + f*Edge;
        }
    }
    
    return Result;
}

struct collision_result
{
    b32 FoundCollision;    
    v3f ContactPoint;
    plane3D SlidingPlane;
    f32 t;
};

collision_result WorldEllipsoidCollisions(world* World, v3f Position, v3f Radius, v3f Delta)
{
    collision_result Result = {};
    
    v3f UnitDelta = Normalize(Delta);
    FOR_EACH(TestEntity, &World->EntityPool)
    {                        
        if(TestEntity->Type == WORLD_ENTITY_TYPE_WALKABLE)
        {
            ASSERT(TestEntity->WalkableMesh);
            walkable_mesh* WalkableMesh = TestEntity->WalkableMesh;
            for(u32 TriangleIndex = 0; TriangleIndex < WalkableMesh->TriangleCount; TriangleIndex++)
            {                                                                
                triangle3D Triangle = TransformTriangle3D(WalkableMesh->Triangles[TriangleIndex], TestEntity->Transform);
                
                v3f ESpaceTriangle[3] = 
                {
                    Triangle.P[0] / Radius,
                    Triangle.P[1] / Radius,
                    Triangle.P[2] / Radius
                };
                
                plane3D ESpaceTrianglePlane = CreatePlane3D(ESpaceTriangle);
                
                //NOTE(EVERYONE): Only collide with triangles that are front-facing
                if(Dot(ESpaceTrianglePlane.Normal, UnitDelta) <= 0.0f)
                {                                    
                    f32 Denominator = Dot(ESpaceTrianglePlane.Normal, Delta);
                    f32 DistanceToPlane = SignedDistance(Position, ESpaceTrianglePlane);
                    
                    f32 t0, t1;
                    
                    b32 IsEmbedded = false;
                    if(Abs(Denominator) <= 1e-6f)
                    {
                        if(Abs(DistanceToPlane) >= 1)
                            continue;
                        
                        t0 = 0.0f;
                        t1 = 1.0f;
                        IsEmbedded = true;
                    }
                    else
                    {
                        t0 = ( 1 - DistanceToPlane) / Denominator;
                        t1 = (-1 - DistanceToPlane) / Denominator;
                        
                        if(t0 > t1)
                            SWAP(t0, t1);
                        
                        if(t0 > 1 || t1 < 0)
                            continue;
                        
                        t0 = SaturateF32(t0);
                        t1 = SaturateF32(t1);
                    }                                                                        
                    
                    f32 t = 1.0f;
                    v3f ContactPoint = InvalidV3();
                    b32 HasIntersected = false;
                    
                    if(!IsEmbedded)
                    {
                        v3f PlaneIntersectionPoint = (Position - ESpaceTrianglePlane.Normal) + (t0*Delta);
                        
                        if(IsPointInTriangle3D(ESpaceTriangle, PlaneIntersectionPoint))
                        {                                            
                            t = t0;
                            ContactPoint = PlaneIntersectionPoint;
                            HasIntersected = true;                                            
                        }
                    }
                    
                    if(!HasIntersected)
                    {
                        f32 DeltaSquare = SquareMagnitude(Delta);
                        
                        if(SphereVertexSweepTest(ESpaceTriangle[0], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                    
                        
                        if(SphereVertexSweepTest(ESpaceTriangle[1], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                    
                        
                        if(SphereVertexSweepTest(ESpaceTriangle[2], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                    
                        
                        if(SphereEdgeSweepTest(ESpaceTriangle[0], ESpaceTriangle[1], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                    
                        
                        if(SphereEdgeSweepTest(ESpaceTriangle[1], ESpaceTriangle[2], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                    
                        
                        if(SphereEdgeSweepTest(ESpaceTriangle[2], ESpaceTriangle[0], Position, Delta, DeltaSquare, &t, &ContactPoint))                                        
                            HasIntersected = true;                                                                                                                            
                    }
                    
                    if(HasIntersected)
                    {                        
                        if(!Result.FoundCollision || (t < Result.t))
                        {
                            Result.t = t;
                            Result.ContactPoint = ContactPoint;
                            Result.FoundCollision = true;                            
                        }                                        
                    }
                }
            }
        }
    }    
    
    if(Result.FoundCollision)    
        Result.SlidingPlane = CreatePlane3D(Result.ContactPoint, (Position + Delta*Result.t)-Result.ContactPoint);    
    
    return Result;
}

void 
UpdateWorld(game* Game)
{                
    world* World = GetWorld(Game, Game->CurrentWorldIndex);         
    
    FOR_EACH(Entity, &World->EntityPool)
    {        
        switch(Entity->Type)
        {
            case WORLD_ENTITY_TYPE_PLAYER:
            {
                ASSERT(Entity->UserData);
                UpdatePlayer(Game, Entity);                
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