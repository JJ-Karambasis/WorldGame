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

grid_method_type IsCellWalkableMethodType(walkable_pole** CellPoles)
{
    u32 Method = 1;
    if(CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]->Flag  == WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->Flag == WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->Flag     == WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->Flag    == WALKABLE_POLE_FLAG_WALKABLE) Method *= 2;
    
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
                Pole->Flag = WALKABLE_POLE_FLAG_WALKABLE;                
                Pole->HitEntity = HitEntity;
            }                        
        }
    }
    
    END_POLE_TESTING();
    
    walk_edges WalkEdges = {};    
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
                } break;
                
                case GRID_METHOD_TYPE_WALL:
                {
                    if(CellPoles[POLE_INDEX_BOTTOM_LEFT]->Flag == WALKABLE_POLE_FLAG_WALKABLE)
                    {
                        if(CellPoles[POLE_INDEX_TOP_LEFT]->Flag == WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            
                            
                        }
                        else if(CellPoles[POLE_INDEX_BOTTOM_RIGHT]->Flag == WALKABLE_POLE_FLAG_WALKABLE)
                        {
                        }
                        else if(CellPoles[POLE_INDEX_TOP_RIGHT]->Flag == WALKABLE_POLE_FLAG_WALKABLE)
                        {
                            //TODO(JJ): Do we need to implement this
                            NOT_IMPLEMENTED;
                        }
                        INVALID_ELSE;                           
                    }
                } break;                                
            }
        }
    }
    
    return RequestedPosition;
}