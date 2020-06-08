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


void UpdatePlayer(game* Game, world_entity* PlayerEntity)
{
    input* Input = Game->Input;
    world* World = GetWorld(Game, PlayerEntity->ID);
    
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
    
    player* Player = (player*)PlayerEntity->UserData;
    
    PlayerEntity->Velocity.xy += MoveAcceleration*dt; 
    PlayerEntity->Velocity *= VelocityDamping;
    
    f32 GravityStrength = 60.0f;
    
    v3f MoveDelta = V3(PlayerEntity->Velocity.xy*dt);                   
    
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
                f32 Distance = Magnitude(ESpaceMoveDelta)*CollisionResult.t;
                f32 ShortenDistance = MaximumF32(Distance-VERY_CLOSE_DISTANCE, 0.0f);                                                
                ESpacePosition += (Normalize(ESpaceMoveDelta)*ShortenDistance);
                
                //NOTE(EVERYONE): Increase the gravity strength by a large amount to prevent climbing on walls that are too steep
                if(Dot(CollisionResult.SlidingPlane.Normal, Global_WorldZAxis) < 0.95f)
                    GravityStrength *= 4;
                
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
    
    PlayerEntity->Velocity.z -= (GravityStrength*dt);
    {
        v3f ESpaceMoveDelta = V3(0.0f, 0.0f, (PlayerEntity->Velocity.z*dt)/PlayerEllipsoid.Radius.z);
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