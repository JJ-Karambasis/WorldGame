void UpdatePlayer(game* Game, world_entity* PlayerEntity)
{
    input* Input = Game->Input;
    world* World = GetWorld(Game, PlayerEntity->ID);
    
    v2f MoveDirection = {};
    
    b32 StartJumping = false;    
    jumping_quad* JumpingQuad = NULL;
    for(u32 JumpingQuadIndex = 0; JumpingQuadIndex < ARRAYCOUNT(World->JumpingQuads); JumpingQuadIndex++)
    {
        JumpingQuad = World->JumpingQuads + JumpingQuadIndex;
        v2f HalfDim = JumpingQuad->Dimensions*0.5f;
        
        v2f Min = JumpingQuad->CenterP.xy - HalfDim;
        v2f Max = JumpingQuad->CenterP.xy + HalfDim;
        
        c3 QuadColor = Red3();
        if(PlayerEntity->Position.xy >= Min && PlayerEntity->Position.xy <= Max)
        {
            if(Abs(PlayerEntity->Position.z - JumpingQuad->CenterP.z) < 1e-2f)
            {
                if(IsPressed(Input->Action))
                {
                    StartJumping = true;
                    break;                    
                    
                }                
                QuadColor = Yellow3();
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
        
        PlayerEntity->State = WORLD_ENTITY_STATE_JUMPING;        
    }
    
    if(PlayerEntity->State == WORLD_ENTITY_STATE_JUMPING)    
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
    
    PlayerEntity->MoveDelta = V3(PlayerEntity->Velocity.xy*dt);                  
    for(u32 Iterations = 0; Iterations < 4; Iterations++)
    {            
        f32 DeltaLength = Magnitude(PlayerEntity->MoveDelta);
        
        if(DeltaLength > VERY_CLOSE_DISTANCE)
        {            
            v3f TargetPosition = PlayerEntity->Position + PlayerEntity->MoveDelta;                           
            collision_result CollisionResult = DetectCollisions(World, PlayerEntity);
            
            if(!CollisionResult.HitEntity)
            {
                PlayerEntity->Position = TargetPosition;                                
                break;
            }
            else
            {   
                penetration* Penetration = &CollisionResult.Penetration;
                
                PlayerEntity->Position += PlayerEntity->MoveDelta*CollisionResult.t;
                PlayerEntity->Position += Penetration->Normal*1e-4f;
                
                PlayerEntity->MoveDelta = TargetPosition - PlayerEntity->Position;
                
                PlayerEntity->MoveDelta -= Dot(PlayerEntity->MoveDelta, Penetration->Normal)*Penetration->Normal;
                PlayerEntity->Velocity -= Dot(PlayerEntity->Velocity, Penetration->Normal)*Penetration->Normal;                
            }
        }
        else
        {
            break;
        }
    }   
    
    PlayerEntity->MoveDelta = V3(0, 0, PlayerEntity->Velocity.z*dt);                  
    for(u32 Iterations = 0; Iterations < 4; Iterations++)
    {            
        f32 DeltaLength = Magnitude(PlayerEntity->MoveDelta);
        
        if(DeltaLength > VERY_CLOSE_DISTANCE)
        {            
            v3f TargetPosition = PlayerEntity->Position + PlayerEntity->MoveDelta;                
            collision_result CollisionResult = DetectCollisions(World, PlayerEntity);
            
            if(!CollisionResult.HitEntity)
            {
                PlayerEntity->Position = TargetPosition;                                
                break;
            }
            else
            {   
                penetration* Penetration = &CollisionResult.Penetration;
                
                PlayerEntity->Position += PlayerEntity->MoveDelta*CollisionResult.t;                
                
                PlayerEntity->MoveDelta = TargetPosition - PlayerEntity->Position;
                
                PlayerEntity->MoveDelta -= Dot(PlayerEntity->MoveDelta, Penetration->Normal)*Penetration->Normal;
                PlayerEntity->Velocity -= Dot(PlayerEntity->Velocity, Penetration->Normal)*Penetration->Normal;                
            }
        }
        else
        {
            break;
        }
    }   
    
#if 0         
    if(HasCollided)
    {
        Player->IsJumping = false;
    }
    
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
