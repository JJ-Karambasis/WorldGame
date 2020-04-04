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
    Result.Min = RoundPrecisionMag2f(Min*GRID_DENSITY, 10);
    Result.Poles = PushArray(Result.PoleCount.x*Result.PoleCount.y, walkable_pole, Clear, 0);
    for(i32 YIndex = 0; YIndex < Result.PoleCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Result.PoleCount.x; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Result, XIndex, YIndex);
            Pole->Position2D = Result.Min + V2(XIndex, YIndex)*GRID_DENSITY;     
            Pole->ZIntersection = -FLT_MAX;
        }
    }
    return Result;
}

f32 TestEdgeForZ(edge3D Edge, v2f Position, f32 LargestZ)
{
    if(IsPointOnEdge2D(Edge, Position))
    {
        f32 Z = FindLineZ(Edge, Position);
        if(Z > LargestZ)
            return Z;
    }    
    return LargestZ;
}

void TestSurfaceEdge(surface_edge_test* EdgeTest, edge2D PoleEdge, edge2D TriangleEdge)
{
    v2f Point;
    if(EdgeToEdgeIntersection2D(&Point, TriangleEdge, PoleEdge))
    {
        f32 Distance = SquareMagnitude(PoleEdge.P[0]-Point);
        if(Distance > EdgeTest->Distance)
        {
            EdgeTest->Distance = Distance;
            EdgeTest->Point = Point;
        }
    }    
    else if(IsPointOnEdge2D(TriangleEdge, PoleEdge.P[0]))
    {
        if(0.0f > EdgeTest->Distance)
        {
            EdgeTest->Distance = 0.0f;
            EdgeTest->Point = PoleEdge.P[0];
        }
    }
}

pole_surrounding_index GetSurroundingIndex(pole_cell_index HitIndex, pole_cell_index MissIndex)
{
    if(HitIndex == POLE_CELL_INDEX_BOTTOM_LEFT)
    {
        if(MissIndex == POLE_CELL_INDEX_BOTTOM_RIGHT)
            return POLE_SURROUNDING_INDEX_RIGHT;
        else if(MissIndex == POLE_CELL_INDEX_TOP_LEFT)
            return POLE_SURROUNDING_INDEX_TOP;
        else if(MissIndex == POLE_CELL_INDEX_TOP_RIGHT)
            return POLE_SURROUNDING_INDEX_TOP_RIGHT;
        INVALID_ELSE;
    }
    else if(HitIndex == POLE_CELL_INDEX_BOTTOM_RIGHT)
    {
        if(MissIndex == POLE_CELL_INDEX_BOTTOM_LEFT)
            return POLE_SURROUNDING_INDEX_LEFT;
        else if(MissIndex == POLE_CELL_INDEX_TOP_RIGHT)
            return POLE_SURROUNDING_INDEX_TOP;
        else if(MissIndex == POLE_CELL_INDEX_TOP_LEFT)
            return POLE_SURROUNDING_INDEX_TOP_LEFT;
        INVALID_ELSE;
    }
    else if(HitIndex == POLE_CELL_INDEX_TOP_LEFT)
    {
        if(MissIndex == POLE_CELL_INDEX_BOTTOM_LEFT)
            return POLE_SURROUNDING_INDEX_BOTTOM;
        else if(MissIndex == POLE_CELL_INDEX_TOP_RIGHT)
            return POLE_SURROUNDING_INDEX_RIGHT;
        else if(MissIndex == POLE_CELL_INDEX_BOTTOM_RIGHT)
            return POLE_SURROUNDING_INDEX_BOTTOM_RIGHT;
        INVALID_ELSE;
    }
    else if(HitIndex == POLE_CELL_INDEX_TOP_RIGHT)
    {
        if(MissIndex == POLE_CELL_INDEX_BOTTOM_RIGHT)
            return POLE_SURROUNDING_INDEX_BOTTOM;
        else if(MissIndex == POLE_CELL_INDEX_TOP_LEFT)
            return POLE_SURROUNDING_INDEX_LEFT;
        else if(MissIndex == POLE_CELL_INDEX_BOTTOM_LEFT)
            return POLE_SURROUNDING_INDEX_BOTTOM_LEFT;
    }
    INVALID_ELSE;
    
    return (pole_surrounding_index)-1;
}

void TestSurfaceIntersection(walkable_pole** CellPoles, pole_cell_index HitIndex, pole_cell_index MissIndex)
{
    pole_surrounding_index SurroundIndex = GetSurroundingIndex(HitIndex, MissIndex);
    
    walkable_pole* HitPole = CellPoles[HitIndex];
    walkable_pole* MissPole = CellPoles[MissIndex];
    if(((HitPole->Flags & WALKABLE_POLE_FLAG_WALK_EDGE) == 0) &&
       !HitPole->SurfaceTests[SurroundIndex].HasIntersected)
    {
        static_entity* Entity = HitPole->HitEntity;
        edge2D PoleEdge = CreateEdge2D(HitPole->IntersectionPoint, MissPole->IntersectionPoint);                            
        surface_edge_test SurfaceEdgeTest = CreateDefaultSurfaceEdgeTest();
        
        //TODO(JJ): Can probably use adj information to speed up the searching on the triangle mesh. This might be tricky to implement,
        //so I want to have real data for I optimize it, especially if this doesn't ending up being a bottleneck                                                                                    
        for(u32 TriangleIndex = 0; TriangleIndex < Entity->Mesh->TriangleCount; TriangleIndex++)
        {
            triangle3D Triangle = TransformTriangle3D(Entity->Mesh->Triangles[TriangleIndex], Entity->Transform);
            edge2D Edges[3];
            GetTriangleEdges2D(Edges, Triangle);
            
            TestSurfaceEdge(&SurfaceEdgeTest, PoleEdge, Edges[0]);
            TestSurfaceEdge(&SurfaceEdgeTest, PoleEdge, Edges[1]);
            TestSurfaceEdge(&SurfaceEdgeTest, PoleEdge, Edges[2]);                                
        }
        
        ASSERT(SurfaceEdgeTest.Distance != -FLT_MAX);                                    
        if(SurfaceEdgeTest.IsPoleWalkEdge)
            HitPole->Flags |= WALKABLE_POLE_FLAG_WALK_EDGE;
        else
            HitPole->SurfaceTests[SurroundIndex] = CreateSurfaceTest(SurfaceEdgeTest.Point);
        
        WALKING_SYSTEM_EVENT_DRAW_POINT(V3(SurfaceEdgeTest.Point, HitPole->ZIntersection), Blue());                                
    }
}

grid_method_type IsCellWalkableMethodType(walkable_pole** CellPoles)
{
    u32 Method = 1;
    if(CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]->Flags & WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flags     & WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flags    & WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    
    switch(Method)
    {
        case 1:
        return GRID_METHOD_TYPE_NONE;
        
        case 2:
        return GRID_METHOD_TYPE_CORNER;
        
        case 4:
        return GRID_METHOD_TYPE_WALL;
        
        case 8:
        return GRID_METHOD_TYPE_TRIANGLE;
        
        case 16:
        return GRID_METHOD_TYPE_SURFACE;
        
        INVALID_DEFAULT_CASE;
    }
    
    return GRID_METHOD_TYPE_UNKNOWN;
}

v3f GetWalkPosition(game* Game, walkable_grid* Grid, v3f RequestedPosition, 
                    f32 ZPosition, f32 Height, f32 Radius)
{
    BEGIN_POLE_TESTING();
    for(i32 YIndex = 0; YIndex < Grid->PoleCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Grid->PoleCount.x; XIndex++)
        {
            walkable_pole* Pole = GetPole(Grid, XIndex, YIndex);
            //TODO(JJ): Replace with spatial partioning (quad-tree)
            
            static_entity* HitEntity = NULL;
            for(static_entity* Entity = Game->StaticEntities.Head; Entity; Entity = Entity->Next)
            {
                triangle3D_mesh* Triangles = Entity->Mesh;
                for(u32 TriangleIndex = 0; TriangleIndex < Triangles->TriangleCount; TriangleIndex++)
                {                    
                    triangle3D Triangle = TransformTriangle3D(Triangles->Triangles[TriangleIndex], Entity->Transform);
                    
                    f32 ZIntersection = INFINITY;                                        
                    if(IsPointInTriangle2D(Triangle, Pole->Position2D))
                    {
                        ZIntersection = FindTriangleZ(Triangle, Pole->Position2D);
                    }
                    else
                    {        
                        edge3D Edges[3];
                        GetTriangleEdges3D(Edges, Triangle);
                        
                        f32 LargestZ = -FLT_MAX;
                        LargestZ = TestEdgeForZ(Edges[0], Pole->Position2D, LargestZ);
                        LargestZ = TestEdgeForZ(Edges[1], Pole->Position2D, LargestZ);
                        LargestZ = TestEdgeForZ(Edges[2], Pole->Position2D, LargestZ);
                        
                        if(LargestZ != -FLT_MAX)
                            ZIntersection = LargestZ;
                    }
                    
                    if(ZIntersection != INFINITY)
                    {
                        if(ZIntersection > Pole->ZIntersection)
                        {
                            if(Pole->ZIntersection != -FLT_MAX)
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, ZIntersection), Red());
                            
                            Pole->ZIntersection = ZIntersection;
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
                Pole->Flags |= WALKABLE_POLE_FLAG_WALKABLE;                
                Pole->HitEntity = HitEntity;
            }                        
        }
    }
    
    END_POLE_TESTING();
    
    BEGIN_POLE_EDGE_TESTING();
    for(i32 YIndex = 0; YIndex < Grid->CellCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Grid->CellCount.x; XIndex++)
        {
            walkable_pole** CellPoles = GetCellPoles(Grid, XIndex, YIndex);
            grid_method_type Type = IsCellWalkableMethodType(CellPoles);
            switch(Type)
            {
                case GRID_METHOD_TYPE_CORNER:
                {
                } break;
                
                case GRID_METHOD_TYPE_TRIANGLE:
                {
                    if(!CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]->Flags)
                    {
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_LEFT, POLE_CELL_INDEX_BOTTOM_LEFT);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_RIGHT, POLE_CELL_INDEX_BOTTOM_LEFT);
                    }
                    else if(!CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->Flags)
                    {
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_RIGHT, POLE_CELL_INDEX_BOTTOM_RIGHT);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_LEFT, POLE_CELL_INDEX_BOTTOM_RIGHT);
                    }
                    else if(!CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flags)
                    {
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_RIGHT, POLE_CELL_INDEX_TOP_LEFT);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_LEFT, POLE_CELL_INDEX_TOP_LEFT);
                    }
                    else if(!CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flags)
                    {
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_LEFT, POLE_CELL_INDEX_TOP_RIGHT);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_RIGHT, POLE_CELL_INDEX_TOP_RIGHT);
                    }
                    INVALID_ELSE;
                    
                } break;
                
                case GRID_METHOD_TYPE_WALL:
                {                                        
                    if(CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                    {                        
                        if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                        {   
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_LEFT, POLE_CELL_INDEX_BOTTOM_RIGHT);
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_LEFT, POLE_CELL_INDEX_TOP_RIGHT);                                                        
                        }
                        else if(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_LEFT, POLE_CELL_INDEX_TOP_LEFT);
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_RIGHT, POLE_CELL_INDEX_TOP_RIGHT);
                        }
                        else if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            //TODO(JJ): Do we need to implement this
                            NOT_IMPLEMENTED;
                        }
                        INVALID_ELSE;                           
                    }
                    else if(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                    {
                        if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_BOTTOM_RIGHT, POLE_CELL_INDEX_BOTTOM_LEFT);
                            TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_RIGHT, POLE_CELL_INDEX_TOP_LEFT);
                        }
                        else if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            //TODO(JJ): Same as the todo above
                            NOT_IMPLEMENTED;
                        }
                        INVALID_ELSE;
                    }
                    else if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flags & WALKABLE_POLE_FLAG_WALKABLE)
                    {
                        ASSERT(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flags & WALKABLE_POLE_FLAG_WALKABLE);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_LEFT, POLE_CELL_INDEX_BOTTOM_LEFT);
                        TestSurfaceIntersection(CellPoles, POLE_CELL_INDEX_TOP_RIGHT, POLE_CELL_INDEX_BOTTOM_RIGHT);
                    }
                    INVALID_ELSE;
                    
                } break;                                
            }
        }
    }
    END_POLE_EDGE_TESTING();
    
    return RequestedPosition;
}