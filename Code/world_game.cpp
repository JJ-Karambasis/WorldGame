#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "walking.cpp"

inline static_entity* 
CreateStaticEntity(game* Game, v3f Position, v3f Scale, v3f Euler, c4 Color, b32 IsBlocker, triangle3D_mesh* Mesh)
{
    static_entity* Result = PushStruct(&Game->WorldStorage, static_entity, Clear, 0);    
    Result->Transform = CreateSQT(Position, Scale, Euler);
    Result->Color = Color;
    Result->Mesh = Mesh;
    Result->IsBlocker = IsBlocker;
    Result->Next = Game->StaticEntities.Head;
    Game->StaticEntities.Head = Result;
    return Result;
}
#if 0
walkable_pole** GetSurroundingPoles(walkable_grid* Grid, i32 XIndex, i32 YIndex)
{
    walkable_pole** Result = PushArray(8, walkable_pole*, Clear, 0);    
    Result[POLE_INDEX_BOTTOM_LEFT]  = GetPole(Grid, XIndex-1, YIndex-1);
    Result[POLE_INDEX_BOTTOM]       = GetPole(Grid, XIndex,   YIndex-1);
    Result[POLE_INDEX_BOTTOM_RIGHT] = GetPole(Grid, XIndex+1, YIndex-1);
    Result[POLE_INDEX_LEFT]         = GetPole(Grid, XIndex-1, YIndex);
    Result[POLE_INDEX_RIGHT]        = GetPole(Grid, XIndex+1, YIndex);
    Result[POLE_INDEX_TOP_LEFT]     = GetPole(Grid, XIndex-1, YIndex+1);
    Result[POLE_INDEX_TOP]          = GetPole(Grid, XIndex,   YIndex+1);
    Result[POLE_INDEX_TOP_RIGHT]    = GetPole(Grid, XIndex+1, YIndex+1);       
    
    return Result;
}


grid_method_type GetSurfaceQueriesMethodType(surface_edge_intersection_query* SurfaceQueries, pole_index VerticalIndex, pole_index HorizontalIndex, pole_index CornerIndex)
{        
    u32 Method = 1;
    if(SurfaceQueries[VerticalIndex].HasIntersected) Method *= 2;
    if(SurfaceQueries[HorizontalIndex].HasIntersected) Method *= 2;
    if(SurfaceQueries[CornerIndex].HasIntersected) Method *= 2;
    
    switch(Method)
    {
        case 8:
        return GRID_METHOD_TYPE_CORNER;
        
        case 4:
        return GRID_METHOD_TYPE_TRIANGLE;
        
        case 2:
        return GRID_METHOD_TYPE_WALL;
        
        case 1:
        return GRID_METHOD_TYPE_SURFACE;
        
        INVALID_DEFAULT_CASE;
    }
    
    return GRID_METHOD_TYPE_UNKNOWN;
}

inline void 
AddTriangleRing(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2)
{
    walkable_triangle_ring* Ring = PushStruct(walkable_triangle_ring, Clear, 0);
    Ring->P[0] = p0; Ring->P[1] = p1; Ring->P[2] = p2;    
    Ring->Next = List->Head;
    List->Head = Ring;        
    
    WALKING_SYSTEM_EVENT_DRAW_RING(p0, p1, p2);    
}

inline void 
AddQuadRings(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2, v3f p3)
{
    AddTriangleRing(List, p0, p1, p2);
    AddTriangleRing(List, p2, p3, p0);
}

surface_edge_intersection_query SurfaceLineIntersectionQuery(walkable_pole* HitPole, v2f p0, v2f p1)
{
    ASSERT(HitPole->HitEntity);    
    static_entity* Entity = HitPole->HitEntity;    
    triangle3D_mesh* Triangles = Entity->Mesh;        
    
    surface_edge_intersection_query Result = {};
    Result.P = InvalidV2();
    edge2D HitEdge = {};
        
    f32 tBest = -FLT_MAX;
    for(u32 TriangleIndex = 0; TriangleIndex < Triangles->TriangleCount; TriangleIndex++)
    {           
        triangle3D* Triangle = Triangles->Triangles + TriangleIndex;        
        v3f P[3] = 
        {
            TransformV3(Triangle->P[0], Entity->Transform),
            TransformV3(Triangle->P[1], Entity->Transform),
            TransformV3(Triangle->P[2], Entity->Transform)
        };
        
        edge2D Edge[3];
        Edge[0] = CreateEdge2D(P[0], P[1]);
        Edge[1] = CreateEdge2D(P[1], P[2]);
        Edge[2] = CreateEdge2D(P[2], P[0]);                
        
        f32 t, u;
        if(LineIntersections2D(p0, p1, Edge[0].P[0], Edge[0].P[1], &t, &u))
        {
            if(u >= 0.0f && u <= 1.0f && t > tBest)
            {                
                tBest = t;
                Result.Edge = Edge[0];                
            }
        }
                
        if(LineIntersections2D(p0, p1, Edge[1].P[0], Edge[1].P[1], &t, &u))
        {
            if(u >= 0.0f && u <= 1.0f && t > tBest)
            {                
                tBest = t;
                Result.Edge = Edge[1];                
            }
        }
        
        if(LineIntersections2D(p0, p1, Edge[2].P[0], Edge[2].P[1], &t, &u))
        {
            if(u >= 0.0f && u <= 1.0f && t > tBest)
            {                
                tBest = t;
                Result.Edge = Edge[2];                
            }
        }        
    }    
    
    Result.HasIntersected = (tBest != -FLT_MAX);    
    if(Result.HasIntersected)    
        Result.P = p0 + tBest*(p1-p0);                
    
    return Result;
}
 
surface_edge_intersection_query SurfaceEdgeIntersectionQuery(walkable_pole* HitPole, v2f p0, v2f p1)
{     
    ASSERT(HitPole->HitEntity);
    
    static_entity* Entity = HitPole->HitEntity;    
    triangle3D_mesh* Triangles = Entity->Mesh;
    
    edge2D D = CreateEdge2D(p0, p1);
        
    f32 BestDistance = FLT_MAX;    
    
    surface_edge_intersection_query Result = {};
    Result.P = InvalidV2();
        
    for(u32 TriangleIndex = 0; TriangleIndex < Triangles->TriangleCount; TriangleIndex++)
    {           
        triangle3D* Triangle = Triangles->Triangles + TriangleIndex;
        
        v3f P[3] = 
        {
            TransformV3(Triangle->P[0], Entity->Transform),
            TransformV3(Triangle->P[1], Entity->Transform),
            TransformV3(Triangle->P[2], Entity->Transform)
        };
        
        edge2D Edge[3];
        Edge[0] = CreateEdge2D(P[0], P[1]);
        Edge[1] = CreateEdge2D(P[1], P[2]);
        Edge[2] = CreateEdge2D(P[2], P[0]);        
                
        v2f Point0;        
        if(EdgeToEdgeIntersection2D(&Point0, D, Edge[0]))
        {   
            f32 Distance = SquareMagnitude(Point0-p1);
            if(Distance < BestDistance)
            {
                BestDistance = Distance;
                Result.P = Point0;
                Result.Edge = Edge[0];                                
            }        
        }
        
        v2f Point1;
        if(EdgeToEdgeIntersection2D(&Point1, D, Edge[1]))
        {
            f32 Distance = SquareMagnitude(Point1-p1);
            if(Distance < BestDistance)
            {
                BestDistance = Distance;
                Result.P = Point1;
                Result.Edge = Edge[1];                                
            }                    
        }
        
        v2f Point2;
        if(EdgeToEdgeIntersection2D(&Point2, D, Edge[2]))
        {
            f32 Distance = SquareMagnitude(Point2-p1);
            if(Distance < BestDistance)
            {
                BestDistance = Distance;
                Result.P = Point2;
                Result.Edge = Edge[2];                                
            }                    
        }
    }    
    
    Result.HasIntersected = BestDistance != FLT_MAX;    
    if(Result.HasIntersected)    
        Result.HasIntersected = !AreEqual(Result.P, HitPole->Position2D, 1e-6f);            
    
    return Result;
}

inline surface_edge_intersection_query 
SurfaceEdgeIntersectionQuery(walkable_pole* TargetPole, walkable_pole** SurroundingPoles, pole_index Index)
{    
    surface_edge_intersection_query Result;                        
    if(TargetPole->SurfaceQueries[Index].HasIntersected)                        
        Result = TargetPole->SurfaceQueries[Index];                        
    else                        
        Result = SurfaceEdgeIntersectionQuery(TargetPole, TargetPole->Position2D, SurroundingPoles[Index]->Position2D);                        
    return Result;
}

inline void 
MakeSurfaceIntersectedQuery(surface_edge_intersection_query* Query, v2f Point)
{    
    Query->P = Point;
    Query->HasIntersected = true;    
    Query->Edge = CreateEdge2D(InvalidV2(), InvalidV2());
}

inline v3f Get3DPoint(walkable_pole* TargetPole, v2f Point)
{
    v3f Result = V3(Point, INFINITY);    
    static_entity* HitEntity = TargetPole->HitEntity;
    triangle3D_mesh* Mesh = HitEntity->Mesh;
    for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
    {
        triangle3D Triangle = TransformTriangle3D(Mesh->Triangles[TriangleIndex], HitEntity->Transform);                
        if(IsPointInTriangle2D(Triangle, Point))
        {
            Result.z = FindTriangleZ(Triangle, Point);
            break;
        }        
    }                            
    ASSERT(Result.z != INFINITY);  
    return Result;
}

inline b32 
IsWalkablePole(walkable_pole* Pole)
{
    b32 Result = (Pole->Flag == WALKABLE_POLE_FLAG_WALKABLE);
    return Result;
}

inline b32 
IsWalkEdgePole(walkable_pole* Pole)
{
    b32 Result = (Pole->Flag == WALKABLE_POLE_FLAG_WALK_EDGE);
    return Result;
}

inline void
PoleEdgeTest(walkable_pole* Pole, walkable_pole** SurroundingPoles, pole_index Index)
{
    surface_edge_intersection_query Query = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, Index);
    if(Query.HasIntersected)
    {
        Pole->SurfaceQueries[Index] = Query;
        WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, Pole->SurfaceQueries[Index].P, White());
    }
    else
        Pole->Flag = WALKABLE_POLE_FLAG_WALK_EDGE;
}

grid_method_type GetSurroundingPolesIsWalkableMethodType(walkable_pole* Pole0, walkable_pole* Pole1, walkable_pole* Pole2)
{
    u32 Method = 1;
    if(IsWalkablePole(Pole0)) Method *= 2;
    if(IsWalkablePole(Pole1)) Method *= 2;
    if(IsWalkablePole(Pole2)) Method *= 2;
    
    switch(Method)
    {
        case 1:
        return GRID_METHOD_TYPE_CORNER;
        
        case 2:
        return GRID_METHOD_TYPE_WALL;
        
        case 4:
        return GRID_METHOD_TYPE_TRIANGLE;
        
        case 8:
        return GRID_METHOD_TYPE_SURFACE;
        
        INVALID_DEFAULT_CASE;
    }
    
    return GRID_METHOD_TYPE_UNKNOWN;
}

b32 PoleHasEdge(walkable_pole* Pole, walkable_pole* EdgePole, pole_index EdgeIndex)
{    
    b32 Result = (Pole->SurfaceQueries[EdgeIndex].HasIntersected) || (EdgePole->Flag == WALKABLE_POLE_FLAG_WALK_EDGE);
    return Result;    
}
b32 PoleHasEdge(walkable_pole* Pole, walkable_pole** SurroundingPoles, pole_index EdgeIndex)
{    
    b32 Result = PoleHasEdge(Pole, SurroundingPoles[EdgeIndex], EdgeIndex);
    return Result;    
}

grid_method_type GetEdgeMethodType(walkable_pole* Pole, walkable_pole** SurroundingPoles, pole_index VerticalIndex, pole_index HorizontalIndex, pole_index CornerIndex)
{
    u32 Method = 1;
    if(PoleHasEdge(Pole, SurroundingPoles,   VerticalIndex))   Method *= 2;
    if(PoleHasEdge(Pole, SurroundingPoles,   HorizontalIndex)) Method *= 2;
    if(PoleHasEdge(Pole, SurroundingPoles,   CornerIndex))     Method *= 2;
    
    switch(Method)
    {
        case 1:
        return GRID_METHOD_TYPE_SURFACE;
        
        case 2:
        return GRID_METHOD_TYPE_WALL;
        
        case 4:
        return GRID_METHOD_TYPE_TRIANGLE;
        
        case 8:
        return GRID_METHOD_TYPE_CORNER;
        
        INVALID_DEFAULT_CASE;
    }
    
    return GRID_METHOD_TYPE_UNKNOWN;
}

v2f Get2DEdgePoint(walkable_pole* Pole, walkable_pole* EdgePole, pole_index EdgeIndex)
{
    ASSERT(PoleHasEdge(Pole, EdgePole, EdgeIndex));
    
    v2f P = InvalidV2();
    if(Pole->SurfaceQueries[EdgeIndex].HasIntersected)    
    {
        ASSERT(EdgePole->Flag != WALKABLE_POLE_FLAG_WALK_EDGE);
        P = Pole->SurfaceQueries[EdgeIndex].P;
    }
    else if(EdgePole->Flag == WALKABLE_POLE_FLAG_WALK_EDGE)
        P = EdgePole->Position2D;
    else
        INVALID_CODE;
    
    return P;
}

v2f Get2DEdgePoint(walkable_pole* Pole, walkable_pole** SurroundingPoles, pole_index EdgeIndex)
{
    v2f Result = Get2DEdgePoint(Pole, SurroundingPoles[EdgeIndex], EdgeIndex);    
    return Result;
}

v3f Get3DEdgePoint(walkable_pole* Pole, walkable_pole* EdgePole, pole_index EdgeIndex)
{
    v2f P = Get2DEdgePoint(Pole, EdgePole, EdgeIndex);    
    v3f Result = Get3DPoint(Pole, P);
    return Result;
}

b32 GetRaysFromQueries(ray2D* RayA, ray2D* RayB, v2f A, v2f B, surface_edge_intersection_query AQuery, surface_edge_intersection_query BQuery)
{    
    if(AQuery.HasIntersected && BQuery.HasIntersected)
    {
        *RayA = CreateRay2D(A, AQuery.P-A);
        *RayB = CreateRay2D(B, BQuery.P-B);                                
    }
    else if(AQuery.HasIntersected)
    {                                 
        *RayA = CreateRay2D(A, AQuery.P-A);                                
        if(Dot(RayA->Direction, (AQuery.Edge.P[0]-RayA->Origin)) >= 0)                                
            *RayB = CreateRay2D(B, AQuery.Edge.P[0]);                                
        else
            *RayB = CreateRay2D(B, AQuery.Edge.P[1]);
        
    }
    else if(BQuery.HasIntersected)
    {
        *RayB = CreateRay2D(B, BQuery.P-B);
        if(Dot(RayB->Direction, (BQuery.Edge.P[0]-RayB->Origin)) >= 0)
            *RayA = CreateRay2D(A, BQuery.Edge.P[0]);
        else
            *RayA = CreateRay2D(A, BQuery.Edge.P[1]);
    }    
    else
    {
        return false;
    }           
    return true;
}

void GetSurfaceLineQueries(surface_edge_intersection_query* AQuery, surface_edge_intersection_query* BQuery, 
                           walkable_pole* Pole, v2f A, v2f B)
{        
    {
        v2f P0 = V2(Pole->Position2D.x, Pole->Position2D.y + (B.y-Pole->Position2D.y)*0.1f);
        v2f P1 = V2(A.x, P0.y);                                    
        *AQuery = SurfaceLineIntersectionQuery(Pole, P0, P1);                                    
    }
    
    {
        v2f P0 = V2(Pole->Position2D.x + (A.x-Pole->Position2D.x)*0.1f, Pole->Position2D.y);
        v2f P1 = V2(P0.x, B.y);                                    
        *BQuery = SurfaceLineIntersectionQuery(Pole, P0, P1);                                    
    }
}

void HandleCorner(walkable_pole* Pole, v2f A, v2f B, pole_index CornerIndex)
{   
    WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, A, White());
    WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, B, White());        
    
    surface_edge_intersection_query AEdgeQuery;
    surface_edge_intersection_query BEdgeQuery;                            
    GetSurfaceLineQueries(&AEdgeQuery, &BEdgeQuery, Pole, A, B);
    
    ray2D RayA = {};
    ray2D RayB = {};                                
    if(!GetRaysFromQueries(&RayA, &RayB, A, B, AEdgeQuery, BEdgeQuery))
        return;
    
    v2f CommonPoint;
    if(RayToRayIntersection2D(&CommonPoint, RayA, RayB))                                
    {
        MakeSurfaceIntersectedQuery(&Pole->SurfaceQueries[CornerIndex], CommonPoint);                                                                
        WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, CommonPoint, White());
    }
}
#endif
extern "C"
EXPORT GAME_TICK(Tick)
{   
    DEVELOPER_GRAPHICS(Graphics);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    player* Player = &Game->Player;
    
    if(!Game->Initialized)
    {
        Game->Initialized = true;
        Game->WorldStorage = CreateArena(KILOBYTE(32));
        Game->BoxMesh = CreateBoxTriangleMesh(&Game->WorldStorage);
        
        Player->Color = RGBA(0.0f, 0.0f, 1.0f, 1.0f);
        Player->Position = V3(0.0f, 0.0f, 1.0f);
        Player->FacingDirection = V2(0.0f, 1.0f);
        
        CreateStaticEntity(Game, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(PI*0.0f, 0.0f, PI*0.2f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), false, &Game->BoxMesh);        
    }        
    
    Player->Radius = 0.5f;
    Player->Height = 1.0f;
    
    //TODO(JJ): Implement the Walking System
    input* Input = Game->Input;
    const f32 MOVE_ACCELERATION = 20.0f;
    const f32 MOVE_DAMPING = 5.0f;
    
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
    
    v3f MoveAcceleration = V3(MoveDirection*MOVE_ACCELERATION);      
    Player->Velocity += (MoveAcceleration*Input->dt);
    Player->Velocity *= (1.0f / (1.0f + Input->dt*MOVE_DAMPING));    
    
    v3f RequestedPosition = Player->Position + Player->Velocity*Input->dt;
    
    u32 CellPadding = CeilU32(Player->Radius/GRID_DENSITY);
    
    v2i StartCell = GetCell(Player->Position.xy)  - CellPadding;
    v2i EndCell   = GetCell(RequestedPosition.xy) + CellPadding;
    
    v2i MinimumCell = MinimumV2(StartCell, EndCell)-1;
    v2i MaximumCell = MaximumV2(StartCell, EndCell)+1;    
    
    BEGIN_WALKING_SYSTEM(Game);    
    walkable_grid Grid = BuildWalkableGrid(MinimumCell, MaximumCell);                        
    
    Player->Position = GetWalkPosition(Game, &Grid, RequestedPosition, Player->Position.z, Player->Height, Player->Radius);
    
    #if 0 
    TestPoles(Game, &Grid);    
    
    BEGIN_POLE_EDGE_TESTING();
    for(i32 YIndex = 1; YIndex < Grid.PoleCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.PoleCount.x-1; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);
            if(IsWalkablePole(Pole))
            {
                walkable_pole** SurroundingPoles = GetSurroundingPoles(&Grid, XIndex, YIndex);
                grid_method_type Type = GetSurroundingPolesIsWalkableMethodType(SurroundingPoles[POLE_INDEX_BOTTOM], SurroundingPoles[POLE_INDEX_LEFT], SurroundingPoles[POLE_INDEX_BOTTOM_LEFT]);
                switch(Type)
                {
                    case GRID_METHOD_TYPE_CORNER:
                    {                        
                        WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());
                        walkable_pole* HorizontalPole = SurroundingPoles[POLE_INDEX_LEFT];
                        walkable_pole* VerticalPole = SurroundingPoles[POLE_INDEX_BOTTOM];                    
                        walkable_pole* CornerPole = SurroundingPoles[POLE_INDEX_BOTTOM_LEFT];
                        
                        if(IsWalkEdgePole(HorizontalPole) && IsWalkEdgePole(VerticalPole) && IsWalkEdgePole(CornerPole))  
                        {
                            WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, HorizontalPole->Position2D, White());
                            WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, VerticalPole->Position2D, White());
                            WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, CornerPole->Position2D, White());
                            continue;                        
                        }
                        else if(IsWalkEdgePole(VerticalPole) && IsWalkEdgePole(HorizontalPole))                                                                                   
                            HandleCorner(Pole, HorizontalPole->Position2D, VerticalPole->Position2D,  POLE_INDEX_BOTTOM_LEFT);                                                    
                        else if(IsWalkEdgePole(VerticalPole))
                        {       
                            surface_edge_intersection_query AQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_LEFT);
                            if(AQuery.HasIntersected)
                            {
                                Pole->SurfaceQueries[POLE_INDEX_LEFT] = AQuery;                                
                                HandleCorner(Pole, AQuery.P, VerticalPole->Position2D, POLE_INDEX_BOTTOM_LEFT);
                            }                            
                            else                            
                                Pole->Flag = WALKABLE_POLE_FLAG_WALK_EDGE;                            
                        }
                        else if(IsWalkEdgePole(HorizontalPole))
                        {
                            surface_edge_intersection_query BQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);
                            if(BQuery.HasIntersected)
                            {                                
                                Pole->SurfaceQueries[POLE_INDEX_BOTTOM] = BQuery;                                                                
                                HandleCorner(Pole, HorizontalPole->Position2D, BQuery.P, POLE_INDEX_BOTTOM_LEFT);                                
                            }
                            else                            
                                Pole->Flag = WALKABLE_POLE_FLAG_WALK_EDGE;                            
                        }
                        else if(IsWalkEdgePole(CornerPole))
                        {   
                            //TODO(JJ): Is this actually invalid?
                            INVALID_CODE; 
                        }
                        else
                        {                            
                            surface_edge_intersection_query AQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_LEFT);
                            surface_edge_intersection_query BQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);                                                            
                            
                            if(AQuery.HasIntersected && BQuery.HasIntersected)
                            {                                
                                Pole->SurfaceQueries[POLE_INDEX_LEFT] = AQuery;
                                Pole->SurfaceQueries[POLE_INDEX_BOTTOM] = BQuery;                                
                                HandleCorner(Pole, AQuery.P, BQuery.P, POLE_INDEX_BOTTOM_LEFT);                                
                            }                            
                            else                            
                                Pole->Flag = WALKABLE_POLE_FLAG_WALK_EDGE;                            
                        }                        
                    } break;
                    
                    case GRID_METHOD_TYPE_TRIANGLE:
                    {
                        WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());                        
                        walkable_pole* VerticalPole = SurroundingPoles[POLE_INDEX_BOTTOM];
                        walkable_pole* HorizontalPole = SurroundingPoles[POLE_INDEX_LEFT];                        
                        
                        if(!IsWalkablePole(HorizontalPole))
                        {
                            if(!IsWalkEdgePole(HorizontalPole))                            
                                PoleEdgeTest(Pole, SurroundingPoles, POLE_INDEX_LEFT);                            
                            else
                                WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, HorizontalPole->Position2D, White());                            
                        }                        
                        else if(!IsWalkablePole(VerticalPole))
                        {
                             if(!IsWalkEdgePole(VerticalPole))
                                PoleEdgeTest(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);
                            else
                                WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, VerticalPole->Position2D, White());                            
                        }                        
                    } break;
                    
                    case GRID_METHOD_TYPE_WALL:
                    {
                        WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());
                        walkable_pole* VerticalPole = SurroundingPoles[POLE_INDEX_BOTTOM];
                        walkable_pole* HorizontalPole = SurroundingPoles[POLE_INDEX_LEFT];
                        walkable_pole* CornerPole = SurroundingPoles[POLE_INDEX_BOTTOM_LEFT];
                        
                        if(IsWalkablePole(CornerPole))
                        {
                            //TODO(JJ): Do we want to handle this case? How can we test and make sure it's robust                            
                            NOT_IMPLEMENTED;
                        }
                        
                        if(IsWalkablePole(HorizontalPole))
                        {
                            if(!IsWalkEdgePole(VerticalPole))                            
                                PoleEdgeTest(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);                                                                                        
                            else                            
                                WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, VerticalPole->Position2D, White());                            
                        }                                                
                        
                        else if(IsWalkablePole(VerticalPole))
                        {
                            if(!IsWalkEdgePole(HorizontalPole))                                
                                PoleEdgeTest(Pole, SurroundingPoles, POLE_INDEX_LEFT);                                                                                        
                            else
                                WALKING_SYSTEM_EVENT_DRAW_WALK_EDGE(Pole, HorizontalPole->Position2D, White());
                        }
                        
                    } break;
                }
            }
        }
    }    
    END_POLE_EDGE_TESTING();
    
    BEGIN_RADIUS_TESTING();
    
    f32 Radius = Player->Radius;
    for(i32 YIndex = 1; YIndex < Grid.PoleCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.PoleCount.x-1; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);
            if(IsWalkablePole(Pole) && !Pole->RadiusCheck)
            {
                walkable_pole** SurroundingPoles = GetSurroundingPoles(&Grid, XIndex, YIndex);
                grid_method_type Type = GetEdgeMethodType(Pole, SurroundingPoles, POLE_INDEX_BOTTOM, POLE_INDEX_LEFT, POLE_INDEX_BOTTOM_LEFT);
                switch(Type)
                {
                    case GRID_METHOD_TYPE_CORNER:
                    {                        
                    } break;
                    
                    case GRID_METHOD_TYPE_WALL:
                    {
                        if(PoleHasEdge(Pole, SurroundingPoles, POLE_INDEX_BOTTOM))
                        {                            
                            v2f P = Get2DEdgePoint(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);     
                            
                            i32 X = XIndex;
                            i32 Y;
                            for(Y = YIndex; Y < Grid.PoleCount.y; Y++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                if(IsPointInCircle2D(P, Radius, TestPole->Position2D))
                                    TestPole->Flag = WALKABLE_POLE_FLAG_NONE;
                                else
                                    break;
                            }
                            
                            if(Y < Grid.PoleCount.y)
                            {
                                walkable_pole* NewPole = GetPole(&Grid, X, Y);                                
                                if(IsWalkablePole(NewPole))
                                {
                                    v2f NewP = V2(P.x, P.y+Radius);                                    
                                    if(Abs(NewPole->Position2D.y-NewP.y) < 1e-6f)
                                    {                                                                        
                                        Y++;                                    
                                        walkable_pole* NextPole = (Y < Grid.PoleCount.y) ? GetPole(&Grid, X, Y) : NULL;
                                        if(NextPole && IsWalkablePole(NextPole))                                        
                                            NewPole->Flag = WALKABLE_POLE_FLAG_WALK_EDGE;                                        
                                        else                                    
                                            NewPole->Flag = WALKABLE_POLE_FLAG_NONE;   
                                        NextPole->RadiusCheck = true;
                                    }
                                    else
                                    {                                    
                                        ASSERT(NewPole->Position2D.y > NewP.y);
                                        MakeSurfaceIntersectedQuery(&NewPole->SurfaceQueries[POLE_INDEX_BOTTOM], NewP);
                                        NewPole->RadiusCheck = true;
                                    }
                                }
                            }
                            
                            if((SurroundingPoles[POLE_INDEX_BOTTOM]->Flag == WALKABLE_POLE_FLAG_WALK_EDGE) && AreEqual(P, SurroundingPoles[POLE_INDEX_BOTTOM]->Position2D, 1e-6f))
                                SurroundingPoles[POLE_INDEX_BOTTOM]->Flag = WALKABLE_POLE_FLAG_NONE;                            
                            
                            Pole->RadiusCheck = true;
                            
                        }
                        else if(PoleHasEdge(Pole, SurroundingPoles, POLE_INDEX_LEFT))
                        {
                            v2f P = Get2DEdgePoint(Pole, SurroundingPoles, POLE_INDEX_LEFT);
                            
                            i32 X;
                            i32 Y = YIndex;
                            for(X = XIndex; X < Grid.PoleCount.x; X++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                if(IsPointInCircle2D(P, Radius, TestPole->Position2D))
                                    TestPole->Flag = WALKABLE_POLE_FLAG_NONE;
                                else
                                    break;
                            }
                            
                            if(X < Grid.PoleCount.x)
                            {
                                walkable_pole* NewPole = GetPole(&Grid, X, Y);
                                if(IsWalkablePole(NewPole))
                                {
                                    v2f NewP = V2(P.x+Radius, P.y);
                                    if(Abs(NewPole->Position2D.x-NewP.x) < 1e-6f)
                                    {
                                    }
                                    else
                                    {
                                        ASSERT(NewPole->Position2D.x > NewP.x);
                                        MakeSurfaceIntersectedQuery(&NewPole->SurfaceQueries[POLE_INDEX_LEFT], NewP);
                                        NewPole->RadiusCheck = true;
                                    }
                                }
                            }
                            
                            if((SurroundingPoles[POLE_INDEX_LEFT]->Flag == WALKABLE_POLE_FLAG_WALK_EDGE) && AreEqual(P, SurroundingPoles[POLE_INDEX_LEFT]->Position2D, 1e-6f))
                                SurroundingPoles[POLE_INDEX_LEFT]->Flag = WALKABLE_POLE_FLAG_NONE;
                            
                            Pole->RadiusCheck = true;
                        }
                        else if(PoleHasEdge(Pole, SurroundingPoles, POLE_INDEX_BOTTOM_LEFT))
                        {
                            //TODO(JJ): How can we test this case and make it robust
                            NOT_IMPLEMENTED;
                        }
                        else
                            INVALID_CODE;
                    } break;
                    
                    case GRID_METHOD_TYPE_TRIANGLE:
                    {                        
                    } break;
                }
            }
        }
    }
    
    END_RADIUS_TESTING();
    
#if 0 
    f32 Radius = Player->Radius;   
    
    BEGIN_RADIUS_TESTING();
    for(i32 YIndex = 1; YIndex < Grid.PoleCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.PoleCount.x-1; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);
            if(IsWalkablePole(Pole) && !Pole->HasRadiusProcessed)
            {
                grid_method_type Type = GetSurfaceQueriesMethodType(Pole->SurfaceQueries, POLE_INDEX_BOTTOM, POLE_INDEX_LEFT, POLE_INDEX_BOTTOM_LEFT);
                switch(Type)
                {
                    case GRID_METHOD_TYPE_CORNER:
                    {                           
                        v2f P[3] = 
                        {                            
                            Pole->SurfaceQueries[POLE_INDEX_BOTTOM].P,
                            Pole->SurfaceQueries[POLE_INDEX_LEFT].P,
                            Pole->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT].P
                        }; 
                        
                        i32 XMax;
                        i32 YMax;                        
                        for(YMax = YIndex; YMax < Grid.PoleCount.y; YMax++)
                        {
                            walkable_pole* TestPole = GetPole(&Grid, XIndex, YMax);
                            if(!PointInCircle(Pole->SurfaceQueries[POLE_INDEX_BOTTOM].P, Player->Radius, TestPole->Position2D))
                                break;                                                                                                
                            CullPole(TestPole);                                
                        }                                                        
                        
                        for(XMax = XIndex; XMax < Grid.PoleCount.x; XMax++)
                        {
                            walkable_pole* TestPole = GetPole(&Grid, XMax, YIndex);
                            if(!PointInCircle(Pole->SurfaceQueries[POLE_INDEX_LEFT].P, Player->Radius, TestPole->Position2D))
                                break;
                            CullPole(TestPole);                                
                        }                                                                        
                        
                        for(i32 Y = YIndex+1; Y < YMax; Y++)
                        {
                            for(i32 X = XIndex+1; X < XMax; X++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                CullPole(TestPole);
                            }
                        }
                        
                        walkable_pole* TestPole = GetPole(&Grid, V2i(XMax, YMax));
                        if(IsWalkablePole(TestPole))
                        {
                            v2f Vertical = P[0] + Radius;
                            v2f Horizontal = P[1] + Radius;
                            v2f Corner = P[2] + Radius;                                                        
                            
                            walkable_pole* CornerPole = GetPole(&Grid, XMax+1, YMax+1);
                            walkable_pole* RightPole = GetPole(&Grid, XMax+1, YMax);
                            walkable_pole* TopPole = GetPole(&Grid, XMax, YMax+1);
                            
                            HandleEdge(TestPole, CornerPole, TopPole, RightPole, POLE_INDEX_BOTTOM_LEFT, POLE_INDEX_LEFT, POLE_INDEX_BOTTOM, Corner);                                                                 
                        }                                
                        
                        TestPole->HasRadiusProcessed = true;
                        Pole->HasRadiusProcessed = true;
                    } break;
                    
                    case GRID_METHOD_TYPE_WALL:
                     {                        
                        if(Pole->SurfaceQueries[POLE_INDEX_BOTTOM].HasIntersected)
                        {                                                        
                            i32 X = XIndex;
                            i32 Y;
                            for(Y = YIndex; Y < Grid.PoleCount.y; Y++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                if(!PointInCircle(Pole->SurfaceQueries[POLE_INDEX_BOTTOM].P, Player->Radius, TestPole->Position2D))
                                    break;                                                                                                
                                CullPole(TestPole);                                
                            }                                                        
                            
                            walkable_pole* TestPole = GetPole(&Grid, X, Y);
                            if(IsWalkablePole(TestPole))
                            {   
                                v2f P = V2(Pole->SurfaceQueries[POLE_INDEX_BOTTOM].P.x, Pole->SurfaceQueries[POLE_INDEX_BOTTOM].P.y + Radius);                                                                                                        
                                HandleWallEdge(TestPole, GetPole(&Grid, X, Y+1), POLE_INDEX_BOTTOM, P);                                
                            }
                            
                            TestPole->HasRadiusProcessed = true;
                            Pole->HasRadiusProcessed = true;
                        }
                        else if(Pole->SurfaceQueries[POLE_INDEX_LEFT].HasIntersected)
                        {
                            i32 Y = YIndex;
                            i32 X;
                            for(X = XIndex; X < Grid.PoleCount.x; X++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                if(!PointInCircle(Pole->SurfaceQueries[POLE_INDEX_LEFT].P, Player->Radius, TestPole->Position2D))
                                    break;
                                CullPole(TestPole);                                
                            }
                            
                            walkable_pole* TestPole = GetPole(&Grid, X, Y);
                            if(IsWalkablePole(TestPole))
                            {
                                v2f P = V2(Pole->SurfaceQueries[POLE_INDEX_LEFT].P.x + Radius, Pole->SurfaceQueries[POLE_INDEX_LEFT].P.y);
                                HandleWallEdge(TestPole, GetPole(&Grid, X+1, Y), POLE_INDEX_LEFT, P);                                
                            }
                            
                            TestPole->HasRadiusProcessed = true;
                            Pole->HasRadiusProcessed = true;
                        }
                        else if(Pole->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT].HasIntersected)
                        {
                            //TODO(JJ): Do we need to implement this case?
                            NOT_IMPLEMENTED;
                        }
                        else
                        {
                            INVALID_CODE;
                        }                                                
                    } break;
                    
                    case GRID_METHOD_TYPE_TRIANGLE:
                    {
                        NOT_IMPLEMENTED;
                        Pole->IsCulled = true;
                    } break;                    
                } 
            }
        }
    }    
    END_RADIUS_TESTING();
#endif
    
    BEGIN_RING_BUILDING_TESTING();     
    walkable_triangle_ring_list RingList = {};
    for(i32 YIndex = 1; YIndex < Grid.CellCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.CellCount.x-1; XIndex++)
        {
            walkable_pole** CellPoles = GetCellPoles(&Grid, XIndex, YIndex);
            grid_method_type GridType = GetIsWalkableGridPolesMethodType(CellPoles);
            switch(GridType)
            {
                case GRID_METHOD_TYPE_CORNER:
                {
                    if(IsWalkablePole(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]))
                    {
                        ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_LEFT], POLE_INDEX_LEFT));
                        ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], POLE_INDEX_BOTTOM));
                        
                        if(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM_LEFT))
                        {
                            v3f p0 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM_LEFT);
                            v3f p1 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], POLE_INDEX_BOTTOM);
                            v3f p2 = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                            v3f p3 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_LEFT], POLE_INDEX_LEFT);                            
                            AddQuadRings(&RingList, p0, p1, p2, p3);
                        }
                        else
                        {
                            v3f p0 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], POLE_INDEX_BOTTOM);
                            v3f p1 = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                            v3f p2 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_LEFT], POLE_INDEX_LEFT);
                            AddTriangleRing(&RingList, p0, p1, p2);
                        }                        
                    }
                } break;
                
                case GRID_METHOD_TYPE_WALL:
                {
                    if(IsWalkablePole(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]))
                    {
                        if(IsWalkablePole(CellPoles[POLE_CELL_INDEX_TOP_LEFT]))
                        {
                            ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_LEFT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM));
                            ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], POLE_INDEX_BOTTOM));                            
                            v3f p0 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_LEFT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM);
                            v3f p1 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], POLE_INDEX_BOTTOM);
                            v3f p2 = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                            v3f p3 = CellPoles[POLE_CELL_INDEX_TOP_LEFT]->IntersectionPoint;
                            AddQuadRings(&RingList, p0, p1, p2, p3);
                        }
                        else if(IsWalkablePole(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]))
                        {
                            ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_LEFT], POLE_INDEX_LEFT));
                            ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_LEFT));
                            v3f p0 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_LEFT);
                            v3f p1 = CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->IntersectionPoint;
                            v3f p2 = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                            v3f p3 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_LEFT], POLE_INDEX_LEFT);\
                            AddQuadRings(&RingList, p0, p1, p2, p3);
                            
                        }
                        else if(IsWalkablePole(CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]))
                        {
                            //TODO(JJ): Probably need to implement and test this case just to make sure the algorithm is robust
                            NOT_IMPLEMENTED;
                        }
                        else
                        {
                            INVALID_CODE;
                        }
                    }
                } break;
                
                case GRID_METHOD_TYPE_TRIANGLE:
                {
                    v3f p0 = InvalidV3();
                    v3f p1 = InvalidV3();
                    v3f p2 = InvalidV3();
                    v3f p3 = InvalidV3();
                    v3f p4 = InvalidV3();
                    
                    if(!IsWalkablePole(CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]))
                    {
                        ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_LEFT));
                        ASSERT(PoleHasEdge(CellPoles[POLE_CELL_INDEX_TOP_LEFT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM));
                        p0 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_LEFT);
                        p1 = CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->IntersectionPoint;
                        p2 = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                        p3 = CellPoles[POLE_CELL_INDEX_TOP_LEFT]->IntersectionPoint;
                        p4 = Get3DEdgePoint(CellPoles[POLE_CELL_INDEX_TOP_LEFT], CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT], POLE_INDEX_BOTTOM);
                    }
                    else if(!IsWalkablePole(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]))
                    {
                    }
                    else if(!IsWalkablePole(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]))
                    {
                    }
                    else if(!IsWalkablePole(CellPoles[POLE_CELL_INDEX_TOP_LEFT]))
                    {
                    }
                    
                    AddTriangleRing(&RingList, p0, p1, p2);
                    AddTriangleRing(&RingList, p0, p2, p3);
                    AddTriangleRing(&RingList, p3, p4, p2);
                } break;
                
                case GRID_METHOD_TYPE_SURFACE:
                {
                    AddQuadRings(&RingList, 
                                 CellPoles[POLE_CELL_INDEX_BOTTOM_LEFT]->IntersectionPoint, 
                                 CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->IntersectionPoint, 
                                 CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint, 
                                 CellPoles[POLE_CELL_INDEX_TOP_LEFT]->IntersectionPoint);
                } break;
            }
        }
    }    
    
    END_RING_BUILDING_TESTING();
    
    BEGIN_RING_TRAVERSAL_TESTING();
    
    f32 BestSqrDistance = FLT_MAX;
    v3f FinalPosition = RequestedPosition;
    for(walkable_triangle_ring* Ring = RingList.Head; Ring; Ring = Ring->Next)
    {        
        if(IsPointInTriangle2D(Ring->P[0].xy, Ring->P[1].xy, Ring->P[2].xy, RequestedPosition.xy))
        {
            WALKING_SYSTEM_EVENT_DRAW_RING_TEST(Ring->P[0], Ring->P[1], Ring->P[2], RequestedPosition, Yellow(), White());
            FinalPosition.xy = RequestedPosition.xy;
            FinalPosition.z = FindTriangleZ(Ring->P[0], Ring->P[1], Ring->P[2], FinalPosition.xy);
            break;
        }
        else
        {
            WALKING_SYSTEM_EVENT_DRAW_RING_TEST(Ring->P[0], Ring->P[1], Ring->P[2], RequestedPosition, Red(), White());
        }
        
        v3f ClosestPoint = PointTriangleClosestPoint3D(Ring->P[0], Ring->P[1], Ring->P[2], RequestedPosition);
        WALKING_SYSTEM_EVENT_DRAW_POINT(ClosestPoint, Yellow());        
        f32 SqrDistance = SquareMagnitude(ClosestPoint-RequestedPosition);
        if(SqrDistance < BestSqrDistance)
        {
            FinalPosition = ClosestPoint;
            BestSqrDistance = SqrDistance;
        }
    }
    
    END_RING_TRAVERSAL_TESTING();
    Player->Position = FinalPosition;
    #endif    
    END_WALKING_SYSTEM();
    DRAW_POINT(Player->Position, 0.05f, RGBA(0.0f, 0.0f, 0.0f, 1.0f));
    
#if DEVELOPER_BUILD
    development_game* DevGame = (development_game*)Game;
    if(DevGame->InDevelopmentMode)
    {
        development_input* DevInput = (development_input*)Input;
        
        camera* DevCamera = &DevGame->DevCamera;
        
        const f32 ANGULAR_DAMPING = 10.0f;
        const f32 ANGULAR_ACCELERATION = 5.0f;
        const f32 LINEAR_DAMPING = 10.0f;     
        const f32 LINEAR_ACCELERATION = 7.5f;
        const f32 SCROLL_ACCELERATION = 300.0f*5;
        const f32 SCROLL_DAMPING = 7.5f;  
        const f32 MIN_DISTANCE = 0.1f;
        
        
        if(IsDown(DevInput->Alt))
        {                                    
            if(IsDown(DevInput->LMB))
            {
                DevCamera->AngularVelocity.x += (DevInput->MouseDelta.y*DevInput->dt*ANGULAR_ACCELERATION);
                DevCamera->AngularVelocity.y += (DevInput->MouseDelta.x*DevInput->dt*ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(DevInput->MMB))
            {
                DevCamera->Velocity.x += (DevInput->MouseDelta.x*DevInput->dt*LINEAR_ACCELERATION);
                DevCamera->Velocity.y += (DevInput->MouseDelta.y*DevInput->dt*LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(DevInput->Scroll) > 0.0f)            
                DevCamera->Velocity.z -= DevInput->Scroll*DevInput->dt*SCROLL_ACCELERATION;                                            
        }                
        
        DevCamera->AngularVelocity *= (1.0f / (1.0f+DevInput->dt*ANGULAR_DAMPING));            
        v3f Eulers = (DevCamera->AngularVelocity*DevInput->dt);            
        
        quaternion Orientation = Normalize(RotQuat(DevCamera->Orientation.XAxis, Eulers.pitch)*RotQuat(DevCamera->Orientation.YAxis, Eulers.yaw));
        DevCamera->Orientation *= ToMatrix3(Orientation);
        
        DevCamera->Velocity.xy *= (1.0f /  (1.0f+DevInput->dt*LINEAR_DAMPING));            
        v2f Vel = DevCamera->Velocity.xy*DevInput->dt;
        v3f Delta = Vel.x*DevCamera->Orientation.XAxis - Vel.y*DevCamera->Orientation.YAxis;
        
        DevCamera->FocalPoint += Delta;
        DevCamera->Position += Delta;
        
        DevCamera->Velocity.z *= (1.0f/ (1.0f+DevInput->dt*SCROLL_DAMPING));            
        DevCamera->Distance += DevCamera->Velocity.z*DevInput->dt;            
        
        if(DevCamera->Distance < MIN_DISTANCE)
            DevCamera->Distance = MIN_DISTANCE;
        
        DevCamera->Position = DevCamera->FocalPoint + (DevCamera->Orientation.ZAxis*DevCamera->Distance);
    }
    else    
    #endif
    {
        camera* Camera = &Game->Camera;
        Camera->Position = Player->Position;
        Camera->FocalPoint = Player->Position;
        Camera->Position.z += 6.0f;
        Camera->Orientation = IdentityM3();
    }        
}