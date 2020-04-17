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

void UpdateWorld(game* Game, u32 WorldIndex)
{   
#define DIRECTION_BIAS 0.0001f
    
    world* World = GetWorld(Game, WorldIndex);
    
    player* Player = &World->Player;
    
    Player->Radius = 0.35f;
    Player->Height = 1.0f;
    
    v2f MoveDirection = {};        
    input* Input = Game->Input;
    
    f32 PlayerHeight = TrueHeight(Player->Height, Player->Radius);
    f32 PlayerTop = Player->Position.z + PlayerHeight;
    
    f32 dt = Game->dt;
    for(box_entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(!Entity->Walkable && (Entity != Player->PushingBlock))
        {            
            Entity->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));    
            //Entity->Velocity.z -= dt*10.0f;            
            Entity->Position += Entity->Velocity*dt;
        }
    }    
    
    if(WorldIndex == Game->CurrentWorldIndex)
    {
        if(IsDown(Input->MoveForward))    
            MoveDirection.y =  1.0f;    
        
        if(IsDown(Input->MoveBackward))    
            MoveDirection.y = -1.0f;        
        
        if(IsDown(Input->MoveRight))   
            MoveDirection.x =  1.0f;    
        
        if(IsDown(Input->MoveLeft))    
            MoveDirection.x = -1.0f;    
        
        if(MoveDirection != 0.0f)
            MoveDirection = Normalize(MoveDirection);
    }
    
    player_state PlayerState = {};    
    box_entity* PushingBlock = NULL;
    v2f PushDirection = {};
    
    //TODO(JJ): Fix this janky ass epsilon. It's really scary
#define TEPSILON 0.1f
    
    if((Player->State == PLAYER_STATE_PUSHING) && Player->PushingBlock && AreEqual(Player->PushDirection, MoveDirection, DIRECTION_BIAS))        
    {                       
        box_entity* PushingEntity = Player->PushingBlock;
        v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;                        
        
        Player->Velocity.xy = {};
        
        v2f Delta = PushingEntity->Velocity.xy*dt;        
        PushingEntity->Velocity.xy += (MoveAcceleration*dt);
        PushingEntity->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));                
        
        f32 BlockTop = PushingEntity->Position.z + PushingEntity->Scale.z;
        
        v2f OldPlayerP = Player->Position.xy;
        v2f OldBlockP = PushingEntity->Position.xy;
        
        CONSOLE_LOG("Delta (%f, %f)\n", Delta.x, Delta.y);
        
        f32 tRemaining = 1.0f;
        for(u32 Iterations = 0; (Iterations < 4) && (tRemaining > 0.0f); Iterations++)
        {
            f32 tMin = 1.0f;
            
            v2f Normal = {};
            for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
            {                
                if(IsInRangeOfBlockerZ(Blocker, PushingEntity->Position.z, BlockTop))
                {
                    v2f BlockerNormal = {};                    
                    time_result_2D TimeResult = MovingRectangleEdgeIntersectionTime(PushingEntity->Position.xy, PushingEntity->Position.xy+Delta, PushingEntity->Scale.xy, Blocker->P0.xy, Blocker->P1.xy, NULL);                    
                    if(TimeResultTest(&tMin, TimeResult))
                        Normal = BlockerNormal;
                }                
            }
            
            tMin = MaximumF32(0.0f, tMin-TEPSILON);
            
            v2f DeltaIter = tMin*Delta;            
            Player->Position.xy += DeltaIter;
            PushingEntity->Position.xy += DeltaIter;      
            
            if(Normal == V2())
                break;
            
            Delta = (PushingEntity->Position.xy+Delta)-PushingEntity->Position.xy;
            Delta -= Dot(Delta, Normal)*Normal;            
            PushingEntity->Velocity.xy -= Dot(PushingEntity->Velocity.xy, Normal)*Normal;            
            tRemaining -= tMin*tRemaining;
        }
        
        PushingBlock = PushingEntity;
        PlayerState = PLAYER_STATE_PUSHING;        
        PushDirection = Player->PushDirection;
    }    
    else
    {   
        if(Player->PushingBlock)
            Player->PushingBlock->Velocity = {};
        
        v2f OldPlayerP = Player->Position.xy;
        
        v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;      
        v2f Delta = Player->Velocity.xy*dt;
        Player->Velocity.xy += (MoveAcceleration*dt);
        Player->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));                
        
        f32 tRemaining = 1.0f;    
        for(u32 Iterations = 0; (Iterations < 4) && (tRemaining > 0.0f); Iterations++)
        {
            f32 tMin = 1.0f;
            v2f Normal = {};
            
            for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
            {
                f32 BlockerZ0 = Blocker->P0.z;
                f32 BlockerHeight0 = BlockerZ0 + Blocker->Height0;
                f32 BlockerZ1 = Blocker->P1.z;
                f32 BlockerHeight1 = BlockerZ1 + Blocker->Height1;
                
                if(IsInRangeOfBlockerZ(Blocker, Player->Position.z, PlayerTop))
                {                        
                    v2f BlockerNormal = {};
                    
                    time_result_2D TimeResult = MovingCircleEdgeIntersectionTime(Player->Position.xy, Player->Position.xy+Delta, Player->Radius, Blocker->P0.xy, Blocker->P1.xy, &BlockerNormal);        
                    if(TimeResultTest(&tMin, TimeResult))                        
                        Normal = BlockerNormal;                                                                
                }            
            }
            
            box_entity* HitEntity = NULL;
            for(box_entity* BoxEntity = World->Entities.First; BoxEntity; BoxEntity = BoxEntity->Next)
            {
                if(!BoxEntity->Walkable)
                {
                    f32 Z0 = BoxEntity->Position.z;
                    f32 ZHeight = Z0 + BoxEntity->Scale.z;
                    
                    if(IsRangeInInterval(BoxEntity->Position.z, ZHeight, Player->Position.z, PlayerTop))
                    {
                        v2f HalfDim = BoxEntity->Scale.xy*0.5f;
                        
                        v2f P0 = Player->Position.xy;
                        v2f P1 = Player->Position.xy + Delta;
                        
                        v2f Min = BoxEntity->Position.xy - HalfDim;
                        v2f Max = BoxEntity->Position.xy + HalfDim;
                        
                        v2f BlockNormal = {};
                        time_result_2D TimeResult = MovingCircleRectangleIntersectionTime2D(P0, P1, Player->Radius, Min, Max, &BlockNormal);
                        if(TimeResultTest(&tMin, TimeResult))
                        {
                            Normal = BlockNormal;
                            HitEntity = BoxEntity;                            
                        }
                    }
                }
            }
            
            tMin = MaximumF32(0.0f, tMin-TEPSILON);
            
            Player->Position.xy += tMin*Delta;        
            if(Normal == V2())
                break;
            
            Delta = (Player->Position.xy+Delta)-Player->Position.xy;
            Delta -= Dot(Delta, Normal)*Normal;
            Player->Velocity.xy -= Dot(Player->Velocity.xy, Normal)*Normal;            
            tRemaining -= tMin*tRemaining;                            
            
            if(AreEqual(MoveDirection, -Normal, DIRECTION_BIAS))
            {
                PlayerState = PLAYER_STATE_PUSHING;                
                PushingBlock = HitEntity;
                PushDirection = MoveDirection;
            }                        
        }
    }
    
#if 0 
    
    v3f PlayerLine[2]
    {
        V3(Player->Position.xy, Player->Position.z + Player->Radius),
        V3(PlayerLine[0].xy, PlayerLine[1].z + Player->Height)
    };    
    
    for(box_entity* FirstEntity = World->Entities.First; FirstEntity; FirstEntity = FirstEntity->Next)
    {
        if(!FirstEntity->Walkable)
        {                        
            aabb3D FirstAABB = GetAABB(FirstEntity);
            for(box_entity* SecondEntity = World->Entities.First; SecondEntity; SecondEntity = SecondEntity->Next)
            {
                if(FirstEntity != SecondEntity)
                {                    
                    aabb3D SecondAABB = GetAABB(SecondEntity);
                    penetration_result Penetration = PenetrationTestAABB3D(FirstAABB, SecondAABB);
                    if(Penetration.Hit)
                    {
                        f32 VelocityMagntiude = Dot(Penetration.Normal, FirstEntity->Velocity);
                        
                        FirstEntity->Velocity -= (VelocityMagntiude*Penetration.Normal);                        
                        FirstEntity->Position -= (Penetration.Normal*Penetration.Distance);                        
                    }
                }
            }                        
        }
    }        
#endif
    f32 PlayerWalkHeight = (Player->Position.z + PlayerHeight*0.25f);
    
    f32 BestPointZ = -FLT_MAX;
    triangle3D BestTriangle = InvalidTriangle3D();          
    for(box_entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->Walkable)
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
    
    Player->State = PlayerState;
    Player->PushingBlock = PushingBlock;
    Player->PushDirection = PushDirection;
    
    ASSERT(BestPointZ != -FLT_MAX);    
    Player->Position.z = BestPointZ;                    
    
}

#include "player.cpp"