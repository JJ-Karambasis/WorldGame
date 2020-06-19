void 
CreatePlayer(game* Game, u32 WorldIndex, v3f Position, v3f Radius, c4 Color)
{    
    player* Player = GetPlayer(Game, WorldIndex);    
    //Player->AnimationController = CreateAnimationController(&Game->GameStorage, &Game->Assets->TestSkeleton);    
    //Player->AnimationController.PlayingAnimation.Clip = &Game->Assets->TestAnimation;
    Player->EntityID = CreateEntity(Game, WORLD_ENTITY_TYPE_PLAYER, WorldIndex, Position, V3(1.0f), V3(), Color, &Game->Assets->PlayerMesh, NULL, Player);    
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
                triangle3D Triangle = TransformTriangle3D(WalkableMesh->Triangles[TriangleIndex].P, TestEntity->Transform);
                
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

void UpdatePlayer(game* Game, world_entity* PlayerEntity)
{
    input* Input = Game->Input;
    world* World = GetWorld(Game, PlayerEntity->ID);
    
    player* Player = (player*)PlayerEntity->UserData;
    
    v2f MoveDirection = {};
    
    b32 StartJumping = false;    
    jumping_quad* JumpingQuad = NULL;
    for(u32 JumpingQuadIndex = 0; JumpingQuadIndex < ARRAYCOUNT(World->JumpingQuads); JumpingQuadIndex++)
    {
        JumpingQuad = World->JumpingQuads + JumpingQuadIndex;
        v2f HalfDim = JumpingQuad->Dimensions*0.5f;
        
        v2f Min = JumpingQuad->CenterP.xy - HalfDim;
        v2f Max = JumpingQuad->CenterP.xy + HalfDim;
        
        c4 QuadColor = Red4();
        if(PlayerEntity->Position.xy >= Min && PlayerEntity->Position.xy <= Max)
        {
            if(Abs(PlayerEntity->Position.z - JumpingQuad->CenterP.z) < 1e-2f)
            {
                if(IsPressed(Input->Action))
                {
                    StartJumping = true;
                    break;                    
                    
                }                
                QuadColor = Yellow4();
            }
        }       
        
        DEBUG_DRAW_QUAD(JumpingQuad->CenterP, Global_WorldZAxis, JumpingQuad->Dimensions, QuadColor);    
    }
    
    
    f32 dt = Game->dt;    
    f32 JumpGravity = 20.0f;        
    if(StartJumping)
    {                   
        jumping_quad* TargetQuad = JumpingQuad->OtherQuad;
        
        v2f XDirection = TargetQuad->CenterP.xy-JumpingQuad->CenterP.xy;
        f32 Displacement = Magnitude(XDirection);
        XDirection /= Displacement;                    
        
        f32 InitialVelocity = Sqrt(Displacement*JumpGravity);
        
        PlayerEntity->Velocity.xy = (InitialVelocity*SQRT2_2*XDirection);
        PlayerEntity->Velocity.z = InitialVelocity*SQRT2_2;
        
        Player->IsJumping = true;
    }
        
    if(Player->IsJumping)    
    {
        PlayerEntity->Velocity.z -= (JumpGravity*dt);            
    }
    else
    {
        
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
        
        v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;        
        f32 VelocityDamping = (1.0f / (1.0f + dt*MOVE_DAMPING));       
        
        PlayerEntity->Velocity.xy += MoveAcceleration*dt; 
        PlayerEntity->Velocity *= VelocityDamping;
        
        f32 GravityStrength = 60.0f;    
        PlayerEntity->Velocity.z -= GravityStrength*dt;
    }
    
    v3f MoveDelta = PlayerEntity->Velocity*dt;               
    
    b32 HasCollided = false;
    {
        ellipsoid3D PlayerEllipsoid = GetPlayerEllipsoid(Game, Player);    
        v3f ESpacePosition = PlayerEllipsoid.CenterP / PlayerEllipsoid.Radius;    
        
        {        
            v3f ESpaceMoveDelta = V3(MoveDelta.xy / PlayerEllipsoid.Radius.xy);                            
            v3f DestinationPoint = ESpacePosition + ESpaceMoveDelta;
            plane3D FirstPlane = InvalidPlane3D();        
            for(u32 Iterations = 0; Iterations < 3; Iterations++)
            {
                collision_result CollisionResult = WorldEllipsoidCollisions(World, ESpacePosition, PlayerEllipsoid.Radius, ESpaceMoveDelta);
                if(!CollisionResult.FoundCollision)
                {
                    ESpacePosition = DestinationPoint;
                    break;
                }
                else
                {
                    HasCollided = true;
                    
                    f32 Distance = Magnitude(ESpaceMoveDelta)*CollisionResult.t;
                    f32 ShortenDistance = MaximumF32(Distance-VERY_CLOSE_DISTANCE, 0.0f);                                                
                    ESpacePosition += (Normalize(ESpaceMoveDelta)*ShortenDistance);
                    
                    //NOTE(EVERYONE): Increase the gravity strength by a large amount to prevent climbing on walls that are too steep
                    if(Dot(CollisionResult.SlidingPlane.Normal, Global_WorldZAxis) < 0.95f)
                        MoveDelta.z *= 4;                    
                    
                    if(Iterations == 0)
                    {
                        f32 LongRadius = 1.0f + VERY_CLOSE_DISTANCE;
                        FirstPlane = CollisionResult.SlidingPlane;
                        
                        DestinationPoint -= (SignedDistance(DestinationPoint, FirstPlane) - LongRadius)*FirstPlane.Normal;
                        ESpaceMoveDelta = DestinationPoint - ESpacePosition;
                    }
                    else if(Iterations == 1)
                    {
                        plane3D SecondPlane = CollisionResult.SlidingPlane;
                        
                        v3f Crease = Cross(FirstPlane.Normal, SecondPlane.Normal);
                        f32 Displacement = Dot((DestinationPoint - ESpacePosition), Crease);
                        
                        ESpaceMoveDelta = Displacement*Crease;
                        DestinationPoint = ESpacePosition + ESpaceMoveDelta;
                    }
                }
            }                
        }
                
        {
            v3f ESpaceMoveDelta = V3(0.0f, 0.0f, MoveDelta.z/PlayerEllipsoid.Radius.z);
            v3f DestinationPoint = ESpacePosition + ESpaceMoveDelta;            
            plane3D FirstPlane = InvalidPlane3D();
            for(u32 Iterations = 0; Iterations < 3; Iterations++)
            {
                collision_result CollisionResult = WorldEllipsoidCollisions(World, ESpacePosition, PlayerEllipsoid.Radius, ESpaceMoveDelta);
                if(!CollisionResult.FoundCollision)
                {
                    ESpacePosition = DestinationPoint;
                    break;
                }
                else
                {
                    HasCollided = true;
                    f32 Distance = Magnitude(ESpaceMoveDelta)*CollisionResult.t;
                    f32 ShortenDistance = MaximumF32(Distance-VERY_CLOSE_DISTANCE, 0.0f);                
                    
                    //NOTE(EVERYONE): Only apply gravity when collide if the collider is a wall (wall is too steep)
                    if(Dot(CollisionResult.SlidingPlane.Normal, Global_WorldZAxis) < 0.95f)
                    {                                                            
                        if(Iterations == 0)
                        {
                            f32 LongRadius = 1.0f + VERY_CLOSE_DISTANCE;
                            FirstPlane = CollisionResult.SlidingPlane;
                            
                            DestinationPoint -= (SignedDistance(DestinationPoint, FirstPlane) - LongRadius)*FirstPlane.Normal;
                            ESpaceMoveDelta = DestinationPoint - ESpacePosition;
                        }
                        else if(Iterations == 1)
                        {
                            plane3D SecondPlane = CollisionResult.SlidingPlane;
                            
                            v3f Crease = Cross(FirstPlane.Normal, SecondPlane.Normal);
                            f32 Displacement = Dot((DestinationPoint - ESpacePosition), Crease);
                            
                            ESpaceMoveDelta = Displacement*Crease;
                            DestinationPoint = ESpacePosition + ESpaceMoveDelta;
                        }
                    }
                    else
                    {
                        ESpacePosition += (Normalize(ESpaceMoveDelta)*ShortenDistance);
                        break;
                    }
                }
            }
        }
        SetPlayerEllipsoidP(Game, Player, ESpacePosition*PlayerEllipsoid.Radius);    
    }
    
    if(HasCollided)
    {
        Player->IsJumping = false;
    }
    
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
}