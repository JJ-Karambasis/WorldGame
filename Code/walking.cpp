#define GRID_DENSITY 0.1f

inline v2i 
GetCell(v2f Position)
{
    v2i Result = FloorV2(Position/GRID_DENSITY);
    return Result;
}

inline walkable_pole* 
GetPole(walkable_grid* Grid, i32 XIndex, i32 YIndex)
{
    ASSERT((XIndex < Grid->PoleCount.x) && (XIndex >= 0) && (YIndex < Grid->PoleCount.y) && (YIndex >= 0));    
    walkable_pole* Result = Grid->Poles + ((YIndex*Grid->PoleCount.x)+XIndex); 
    return Result;
}

inline walkable_pole* 
GetPole(walkable_grid* Grid, v2i Index)
{
    walkable_pole* Result = GetPole(Grid, Index.x, Index.y);
    return Result;
}

inline walkable_pole**
GetCellPoles(walkable_grid* Grid, i32 CellX, i32 CellY)
{
    ASSERT((CellX < Grid->CellCount.x) && (CellX >= 0));
    ASSERT((CellY < Grid->CellCount.y) && (CellY >= 0));
    walkable_pole** Result = PushArray(4, walkable_pole*, Clear, 0);
    Result[0] = GetPole(Grid, CellX,   CellY);
    Result[1] = GetPole(Grid, CellX+1, CellY);
    Result[2] = GetPole(Grid, CellX,   CellY+1);
    Result[3] = GetPole(Grid, CellX+1, CellY+1);
    return Result;
}

inline walkable_pole**
GetCellPoles(walkable_grid* Grid, v2i Cell)
{
    walkable_pole** Result = GetCellPoles(Grid, Cell.x, Cell.y);
    return Result;
}

walkable_grid BuildWalkableGrid(v2i Min, v2i Max)
{        
    walkable_grid Result = {};
    Result.PoleCount = V2i(Max.x-Min.x, Max.y-Min.y)+2;
    Result.CellCount = Result.PoleCount-1;
    Result.Min = Min*GRID_DENSITY;
    Result.Poles = PushArray(Result.PoleCount.x*Result.PoleCount.y, walkable_pole, Clear, 0);
    for(i32 YIndex = 0; YIndex < Result.PoleCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Result.PoleCount.x; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Result, XIndex, YIndex);
            Pole->Position2D = Result.Min + V2(XIndex, YIndex)*GRID_DENSITY;            
        }
    }
    return Result;
}

void TestPoles(game* Game, walkable_grid* Grid)
{
    player* Player = &Game->Player;
    
    BEGIN_POLE_TESTING();
    for(i32 YIndex = 0; YIndex < Grid->PoleCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Grid->PoleCount.x; XIndex++)
        {   
            walkable_pole* Pole = GetPole(Grid, XIndex, YIndex);            
            
            Pole->ZIntersection = -FLT_MAX;
            
            static_entity* HitEntity = NULL;
            triangle* HitTriangle = NULL;
            for(static_entity* Entity = Game->StaticEntities.Head; Entity; Entity = Entity->Next)
            {                   
                triangle_mesh* Mesh = Entity->Mesh;
                for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
                {
                    triangle* Triangle = Mesh->Triangles + TriangleIndex;
                    
                    v3f P[3] = 
                    {
                        TransformV3(Triangle->P[0], Entity->Transform),
                        TransformV3(Triangle->P[1], Entity->Transform),
                        TransformV3(Triangle->P[2], Entity->Transform)
                    };                    
                    
                    f32 ZIntersection = INFINITY;
                    
                    u32 LineIndices[2];
                    if(IsDegenerateTriangle2D(P[0].xy, P[1].xy, P[2].xy, LineIndices))
                    {                            
#if 0
                        //TODO(JJ): Do we even need to handle this degenerate case? I feel like the rest of the algorithm 
                        // will provide a much more numerically stable result than to figure out the triangles z on the
                        //degenerate cases. Maybe we handle this case just for blockers since we know the players z should
                        //just be exactly where the player z currently is                        
                        if(IsPointOnLine2D(Pole->Position2D, P[LineIndices[0]].xy, P[LineIndices[1]].xy))                                                        
                            ZIntersection = FindTriangleZ(P[0], P[1], P[2], Pole->Position2D);                                                                                                                                                                        
#endif
                        
                    }
                    else if(IsPointInTriangle2D(P[0].xy, P[1].xy, P[2].xy, Pole->Position2D))                            
                        ZIntersection = FindTriangleZ(P[0], P[1], P[2], Pole->Position2D);                                                                                                                
                    
                    if(ZIntersection != INFINITY)
                    {                           ;
                        if((ZIntersection <= (Player->Position.z + Player->Height)) &&
                           (ZIntersection > Pole->ZIntersection))
                        {
                            if(Pole->ZIntersection != -FLT_MAX)                                
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, ZIntersection), Red());                            
                            
                            Pole->ZIntersection = ZIntersection;
                            HitTriangle = Triangle;
                            HitEntity = Entity;
                        }
                        else
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, ZIntersection), Red());                                           
                    }
                }                                                                            
            }
            
            if(Pole->ZIntersection != -FLT_MAX)
            {
                WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, Green());                
                Pole->HitEntity = HitEntity;                                 
            }            
        }
    }        
    END_POLE_TESTING();
}