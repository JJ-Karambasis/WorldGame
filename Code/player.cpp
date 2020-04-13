inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

void UpdatePlayer(game* Game, u32 WorldIndex)
{   
    world* World = GetWorld(Game, WorldIndex);
    
    player* Player = &World->Player;
    
    Player->Radius = 0.35f;
    Player->Height = 1.0f;
    
    v2f MoveDirection = {};
    
    f32 dt = Game->dt;
    input* Input = Game->Input;
    
    f32 PlayerHeight = TrueHeight(Player->Height, Player->Radius);
    f32 PlayerTop = Player->Position.z + PlayerHeight;
    
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
    
    v2f OldPlayerP = Player->Position.xy;
    
    v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;      
    Player->Velocity.xy += (MoveAcceleration*dt);
    Player->Velocity.xy *= (1.0f / (1.0f + dt*MOVE_DAMPING));        
    v2f Delta = Player->Velocity.xy*dt;
    
    f32 tRemaining = 1.0f;    
    for(u32 Iterations = 0; (Iterations < 4) && (tRemaining > 0.0f); Iterations++)
    {
        f32 tMin = 1.0f;
        v2f BlockerNormal = {};
        
        b32 Hit = false;
        for(blocker* Blocker = World->Blockers.First; Blocker; Blocker = Blocker->Next)
        {
            v2f Normal = {};
            time_result_2D TimeResult = EdgeCapsuleIntersectionTime2D(Player->Position.xy, Player->Position.xy+Delta, Blocker->P0.xy, Blocker->P1.xy, Player->Radius, &BlockerNormal);        
            if(!IsInvalidTimeResult2D(TimeResult))
            {
                if(tMin > TimeResult.Time)                                
                    tMin = MaximumF32(1e-5f, TimeResult.Time);                                                                    
            }
        }
        
        Player->Position.xy = OldPlayerP + tMin*Delta;        
        Player->Velocity.xy -= Dot(Player->Velocity.xy, BlockerNormal)*BlockerNormal;
        Delta -= Dot(Delta, BlockerNormal)*BlockerNormal;
        tRemaining -= tMin*tRemaining;                
    }
    
    f32 BestPointZ = -FLT_MAX;
    triangle3D BestTriangle = InvalidTriangle3D();          
    for(entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->WalkableMesh)
        {            
            ASSERT(!Entity->Simulate); //CONFIRM(JJ): Do we ever want to simulate walkables (like moving platforms for example)            
            triangle3D_mesh* Mesh = Entity->WalkableMesh;                        
            
            for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
            {
                triangle3D Triangle = TransformTriangle3D(Mesh->Triangles[TriangleIndex], Entity->Transform);                        
                
                if(!IsDegenerateTriangle2D(Triangle))
                {                                                               
                    if(IsPointInTriangle2D(Triangle, Player->Position.xy))
                    {
                        f32 TriangleZ = FindTriangleZ(Triangle, Player->Position.xy);
                        if((TriangleZ > BestPointZ) && (TriangleZ <= PlayerTop))
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
    
    v3f PlayerLine[2]
    {
        V3(Player->Position.xy, Player->Position.z + Player->Radius),
        V3(PlayerLine[0].xy, PlayerLine[1].z + Player->Height)
    };
    
    for(entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
    {
        if(Entity->Simulate)
        {                  
            aabb3D AABB = TransformAABB3D(Entity->AABB, Entity->Transform);
            gjk_result GJKResult = GJK(Line3DSupport, PlayerLine, AABB3DSupport, &AABB);        
            ASSERT(!GJKResult.Intersected);
            
            v3f Normal = GJKResult.ClosestPoints[1]-GJKResult.ClosestPoints[0];                                    
            if(SquareMagnitude(Normal) <= Square(Player->Radius))            
            {                                
                Normal = Normalize(Normal);                                
                v3f ContactPoint = GJKResult.ClosestPoints[0] + Normal*Player->Radius;
                f32 PenetrationDepth = Magnitude(ContactPoint-GJKResult.ClosestPoints[1]);
                
                Entity->Position += Normal*PenetrationDepth;
                
                if(Entity->Link)                
                    Entity->Link->Position += Normal*PenetrationDepth;                                                
            }                        
        }
    }        
}