inline f32 
TrueHeight(f32 Height, f32 Radius)
{
    f32 Result = Height + (Radius*2.0f);
    return Result;
}

void UpdatePlayer(game* Game, player* Player)
{
    
    Player->Radius = 0.35f;
    Player->Height = 1.0f;
    
    //TODO(JJ): Implement the Walking System
    input* Input = Game->Input;
    
    v2f MoveDirection = {};
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
    
    v2f MoveAcceleration = MoveDirection*MOVE_ACCELERATION;      
    Player->Velocity.xy += (MoveAcceleration*Input->dt);
    Player->Velocity.xy *= (1.0f / (1.0f + Input->dt*MOVE_DAMPING));    
    
    v2f DeltaPosition = Player->Position.xy + Player->Velocity.xy*Input->dt;    
    
    f32 PlayerHeight = TrueHeight(Player->Height, Player->Radius);
    f32 PlayerTop = Player->Position.z + PlayerHeight;
    
    f32 BestPointZ = -FLT_MAX;
    triangle3D BestTriangle = InvalidTriangle3D();          
    for(entity* Entity = Game->Entities.First; Entity; Entity = Entity->Next)
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
                    if(IsPointInTriangle2D(Triangle, DeltaPosition))
                    {
                        f32 TriangleZ = FindTriangleZ(Triangle, DeltaPosition);
                        if((TriangleZ > BestPointZ) && (TriangleZ <= PlayerTop))
                        {
                            BestPointZ = TriangleZ;       
                            BestTriangle = Triangle;
                        }
                    }
                }
                
#if DEVELOPER_BUILD
                ((development_game*)Game)->WalkingTriangleCount++;
#endif
            }
        }
    }
    
    if(BestPointZ != -FLT_MAX)
    {
        DRAW_TRIANGLE(BestTriangle.P[0], BestTriangle.P[1], BestTriangle.P[2], Yellow(), 0.025f);
        Player->Position = V3(DeltaPosition, BestPointZ);        
    }    
    
    v3f PlayerLine[2]
    {
        V3(Player->Position.xy, Player->Position.z + Player->Radius),
        V3(PlayerLine[0].xy, PlayerLine[1].z + Player->Height)
    };
    
    Player->Color = Blue();
    for(entity* Entity = Game->Entities.First; Entity; Entity = Entity->Next)
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
                
                DRAW_POINT(ContactPoint, 0.05f, RGBA(1.0f, 1.0f, 0.0f, 1.0f));
            }                        
        }
    }        
}