#include "world_game.h"
#include "geometry.cpp"
#include "audio.cpp"
#include "walking.cpp"

inline static_entity* 
CreateStaticEntity(game* Game, v3f Position, v3f Scale, v3f Euler, c4 Color, b32 IsBlocker, triangle_mesh* Mesh)
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

inline b32 
IsWalkablePole(walkable_pole* Pole)
{
    ASSERT(Pole);
    b32 Result = Pole->HitEntity && !Pole->HitEntity->IsBlocker;
    return Result;
}

inline b32
IsNotCulledPole(walkable_pole* Pole)
{
    b32 Result = IsWalkablePole(Pole) && !Pole->IsCulled;
    return Result;
}

inline b32
IsNotCulledPole(walkable_pole* Pole, pole_index Index)
{
    b32 Result = IsNotCulledPole(Pole) && Pole->SurfaceQueries[Index].HasIntersected;
    return Result;
}

inline void CullPole(walkable_pole* Pole)
{    
    if(IsWalkablePole(Pole))
    {
        Pole->IsCulled = true;
        //DRAW_POINT(Pole->IntersectionPoint, 0.05f, RGBA(1.0f, 1.0f, 0.0f, 1.0f));
    }
}

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

grid_method_type GetGridPolesWalkableMethodtype(walkable_pole* TargetPole, walkable_pole* VerticalPole, walkable_pole* HorizontalPole, walkable_pole* CornerPole)
{    
    u32 Method = 1;
    if(IsWalkablePole(TargetPole)) Method *= 2;
    if(IsWalkablePole(VerticalPole)) Method *= 2;
    if(IsWalkablePole(HorizontalPole)) Method *= 2;
    if(IsWalkablePole(CornerPole)) Method *= 2;
    
    switch(Method)
    {        
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


inline grid_method_type 
GetGridPolesIsNotCulledWalkableMethodType(walkable_pole** GridPoles)
{   
    u32 Method = 1;
    if(IsNotCulledPole(GridPoles[POLE_CELL_INDEX_BOTTOM_LEFT])) Method *= 2;
    if(IsNotCulledPole(GridPoles[POLE_CELL_INDEX_BOTTOM_RIGHT])) Method *= 2;
    if(IsNotCulledPole(GridPoles[POLE_CELL_INDEX_TOP_RIGHT])) Method *= 2;
    if(IsNotCulledPole(GridPoles[POLE_CELL_INDEX_TOP_LEFT])) Method *= 2;
    
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

#define DEBUG_DRAW_RING(ring) \
DRAW_POINT(ring->P[0], 0.00f, RGBA(0.0f, 1.0f, 0.0f, 1.0f)); \
DRAW_POINT(ring->P[1], 0.00f, RGBA(0.0f, 1.0f, 0.0f, 1.0f)); \
DRAW_POINT(ring->P[2], 0.00f, RGBA(0.0f, 1.0f, 0.0f, 1.0f)); \
DRAW_LINE(ring->P[0], ring->P[1], 0.01f, 0.01f, RGBA(1.0f, 1.0f, 1.0f, 1.0f)); \
DRAW_LINE(ring->P[1], ring->P[2], 0.01f, 0.01f, RGBA(1.0f, 1.0f, 1.0f, 1.0f)); \
DRAW_LINE(ring->P[2], ring->P[0], 0.01f, 0.01f, RGBA(1.0f, 1.0f, 1.0f, 1.0f))

inline void 
AddTriangleRing(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2)
{
    walkable_triangle_ring* Ring = PushStruct(walkable_triangle_ring, Clear, 0);
    Ring->P[0] = p0; Ring->P[1] = p1; Ring->P[2] = p2;    
    Ring->Next = List->Head;
    List->Head = Ring;        
    
    //DEBUG_DRAW_RING(Ring);
}

inline void 
AddQuadRings(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2, v3f p3)
{
    AddTriangleRing(List, p0, p1, p2);
    AddTriangleRing(List, p2, p3, p0);
}

surface_edge_intersection_query SurfaceEdgeIntersectionQuery(walkable_pole* HitPole, walkable_pole* MissPole)
{ 
    ASSERT(!MissPole->HitEntity);
    ASSERT(HitPole->HitEntity);
    
    static_entity* Entity = HitPole->HitEntity;    
    triangle_mesh* Triangles = Entity->Mesh;
    
    edge2D D = CreateEdge2D(MissPole->Position2D, HitPole->Position2D);
    
    
    f32 BestDistance = FLT_MAX;    
    
    surface_edge_intersection_query Result = {};
    Result.P = InvalidV2();
    
    edge2D HitEdge = {};
    
    b32 HasIntersected = false;
    for(u32 TriangleIndex = 0; TriangleIndex < Triangles->TriangleCount; TriangleIndex++)
    {           
        triangle* Triangle = Triangles->Triangles + TriangleIndex;
        
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
        
        f32 Distance = FLT_MAX;    
        v2f Points[3];
        u32 BestIndex = (u32)-1;
        if(EdgeToEdgeIntersection2D(&Points[0], D, Edge[0]))
        {   
            f32 Dist = SquareMagnitude(Points[0]-MissPole->Position2D);
            if(Dist < Distance)
            {
                Distance = Dist;            
                BestIndex = 0;
            }        
        }
        
        if(EdgeToEdgeIntersection2D(&Points[1], D, Edge[1]))
        {
            f32 Dist = SquareMagnitude(Points[1]-MissPole->Position2D);
            if(Dist < Distance)
            {
                Distance = Dist;
                BestIndex = 1;
            }
        }
        
        if(EdgeToEdgeIntersection2D(&Points[2], D, Edge[2]))
        {        
            f32 Dist = SquareMagnitude(Points[2]-MissPole->Position2D);
            if(Dist < Distance)
            {
                Distance = Dist;
                BestIndex = 2;
            }
        }    
        
        if(Distance < BestDistance)
        {
            Result.P = Points[BestIndex];                        
            BestDistance = Distance;
            Result.Edge = Edge[BestIndex];            
            HasIntersected = true;
        }        
    }    
    
    if(HasIntersected)
    {
        if(SquareMagnitude(Result.P-HitPole->Position2D) < 1e-6f)
            HasIntersected = false;
    }
    
    Result.HasIntersected = HasIntersected;
    
    return Result;
}

inline surface_edge_intersection_query 
SurfaceEdgeIntersectionQuery(walkable_pole* TargetPole, walkable_pole** SurroundingPoles, pole_index Index)
{    
    surface_edge_intersection_query Result;                        
    if(TargetPole->SurfaceQueries[Index].HasIntersected)                        
        Result = TargetPole->SurfaceQueries[Index];                        
    else                        
        Result = SurfaceEdgeIntersectionQuery(TargetPole, SurroundingPoles[Index]);                        
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
    triangle_mesh* Mesh = HitEntity->Mesh;
    for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
    {
        triangle* Triangle = Mesh->Triangles + TriangleIndex;
        v3f P[3] = 
        {
            TransformV3(Triangle->P[0], HitEntity->Transform),
            TransformV3(Triangle->P[1], HitEntity->Transform),
            TransformV3(Triangle->P[2], HitEntity->Transform)
        };
        
        if(IsPointInTriangle2D(P[0], P[1], P[2], Point))
        {
            Result.z = FindTriangleZ(P[0], P[1], P[2], Point);
            break;
        }
    }                            
    ASSERT(Result.z != INFINITY);  
    return Result;
}

void SetPoleAsEdge(walkable_pole* Pole, walkable_pole* EdgePole, pole_index Index)
{
    if(IsWalkablePole(Pole))
    {
        MakeSurfaceIntersectedQuery(&Pole->SurfaceQueries[Index], EdgePole->Position2D);
        Pole->HasRadiusProcessed = true;
    }
    CullPole(EdgePole);                                   
}

void HandleEdge(walkable_pole* TestPole, walkable_pole* CornerPole, walkable_pole* VerticalPole, walkable_pole* HorizontalPole, 
               pole_index CornerIndex, pole_index HorizontalIndex, pole_index VerticalIndex, v2f P)
{   
    b32 SameX = Abs(P.x-TestPole->Position2D.x) < 1e-6f;
    b32 SameY = Abs(P.y-TestPole->Position2D.y) < 1e-6f;
    
    if(SameX && SameY)
    {
        SetPoleAsEdge(CornerPole, TestPole, CornerIndex);        
    }
    else if(SameX)
    {
        SetPoleAsEdge(HorizontalPole, TestPole, HorizontalIndex);        
        MakeSurfaceIntersectedQuery(&HorizontalPole->SurfaceQueries[CornerIndex], P);
    }
    else if(SameY)
    {
        SetPoleAsEdge(VerticalPole, TestPole, VerticalIndex);        
        MakeSurfaceIntersectedQuery(&VerticalPole->SurfaceQueries[CornerIndex], P);
    }
    else
    {
        MakeSurfaceIntersectedQuery(&TestPole->SurfaceQueries[CornerIndex], P);
    }    
}

inline void 
HandleWallEdge(walkable_pole* TestPole, walkable_pole* WallPole, pole_index Index, v2f P)
{    
    if(SquareMagnitude(P-TestPole->Position2D) < 1e-6f)
        SetPoleAsEdge(WallPole, TestPole, Index);        
    else    
        MakeSurfaceIntersectedQuery(&TestPole->SurfaceQueries[Index], P);    
}

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
        Game->BoxMesh = CreateBoxMesh(&Game->WorldStorage);
        
        Player->Color = RGBA(0.0f, 0.0f, 1.0f, 1.0f);
        Player->Position = V3(0.0f, 0.0f, 1.0f);
        Player->FacingDirection = V2(0.0f, 1.0f);
        
        CreateStaticEntity(Game, V3(0.0f, 0.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), false, &Game->BoxMesh);        
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
                grid_method_type Type = GetGridPolesWalkableMethodtype(Pole, SurroundingPoles[POLE_INDEX_BOTTOM], SurroundingPoles[POLE_INDEX_LEFT], SurroundingPoles[POLE_INDEX_BOTTOM_LEFT]);
                switch(Type)
                {
                    case GRID_METHOD_TYPE_CORNER:
                    {                                    
                        walkable_pole* HorizontalPole = SurroundingPoles[POLE_INDEX_LEFT];
                        walkable_pole* VerticalPole = SurroundingPoles[POLE_INDEX_BOTTOM];                        
                        
                        surface_edge_intersection_query AQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_LEFT);
                        surface_edge_intersection_query BQuery = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);                                                            
                        
                        WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());
                        
                        if(AQuery.HasIntersected && BQuery.HasIntersected)
                        {                                            
                            ray2D RayA = CreateRay2D(AQuery.Edge);
                            ray2D RayB = CreateRay2D(BQuery.Edge);      
                            v2f CommonPoint;                            
                            
                            Pole->SurfaceQueries[POLE_INDEX_LEFT] = AQuery;
                            Pole->SurfaceQueries[POLE_INDEX_BOTTOM] = BQuery;
                            
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(AQuery.P, Pole->ZIntersection), Yellow());
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(BQuery.P, Pole->ZIntersection), Yellow());
                            
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(Pole->IntersectionPoint, V3(AQuery.P, Pole->ZIntersection), White());
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(Pole->IntersectionPoint, V3(BQuery.P, Pole->ZIntersection), White());
                            
                            if(RayToRayIntersection2D(&CommonPoint, RayA, RayB))                                
                            {
                                MakeSurfaceIntersectedQuery(&SurroundingPoles[POLE_INDEX_BOTTOM_LEFT]->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT], CommonPoint);                                                        
                                
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(CommonPoint, Pole->ZIntersection), Yellow());
                                WALKING_SYSTEM_EVENT_DRAW_EDGE(Pole->IntersectionPoint, V3(CommonPoint), White());
                            }
                        }     
                        else if(AQuery.HasIntersected)
                        {   
                            walkable_pole* TargetPole = SurroundingPoles[POLE_INDEX_TOP];
                            
                            WALKING_SYSTEM_EVENT_DRAW_POINT(TargetPole->IntersectionPoint, White());                            
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, TargetPole->ZIntersection), Yellow());
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(AQuery.P, TargetPole->ZIntersection), Yellow());
                            
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(Pole->Position2D, TargetPole->ZIntersection), White());
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(AQuery.P, TargetPole->ZIntersection), White());
                            
                            MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_BOTTOM], Pole->Position2D);
                            MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT], AQuery.P);
                            CullPole(Pole);
                        }
                        else if(BQuery.HasIntersected)
                        {
                            walkable_pole* TargetPole = SurroundingPoles[POLE_INDEX_RIGHT];
                            
                            WALKING_SYSTEM_EVENT_DRAW_POINT(TargetPole->IntersectionPoint, White());                            
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, TargetPole->ZIntersection), Yellow());
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(BQuery.P, TargetPole->ZIntersection), Yellow());
                            
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(Pole->Position2D, TargetPole->ZIntersection), White());
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(BQuery.P, TargetPole->ZIntersection), White());                            
                            
                            MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_LEFT], Pole->Position2D);
                            MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT], BQuery.P);
                            CullPole(Pole);
                        }                        
                        else
                        {
                            walkable_pole* TargetPole = SurroundingPoles[POLE_INDEX_TOP_RIGHT];
                            WALKING_SYSTEM_EVENT_DRAW_POINT(TargetPole->IntersectionPoint, White());
                            WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, TargetPole->ZIntersection), Yellow());
                            WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(Pole->Position2D, TargetPole->ZIntersection), White());
                            
                            MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT], Pole->Position2D);
                            CullPole(Pole);
                        }
                    } break;
                    
                    case GRID_METHOD_TYPE_WALL:
                    {            
                        if(IsWalkablePole(SurroundingPoles[POLE_INDEX_BOTTOM]))
                        {   
                            WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());
                            
                            surface_edge_intersection_query Query = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_LEFT);    
                            if(Query.HasIntersected)                            
                            {   
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Query.P, Pole->ZIntersection), Yellow());
                                WALKING_SYSTEM_EVENT_DRAW_EDGE(Pole->IntersectionPoint, V3(Query.P, Pole->ZIntersection), White());
                                
                                Pole->SurfaceQueries[POLE_INDEX_LEFT] = Query;                                                                        
                            }
                            else
                            {
                                walkable_pole* TargetPole = SurroundingPoles[POLE_INDEX_RIGHT];
                                WALKING_SYSTEM_EVENT_DRAW_POINT(TargetPole->IntersectionPoint, White());
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, TargetPole->ZIntersection), Yellow());
                                WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(Pole->Position2D, TargetPole->ZIntersection), White());
                                
                                MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_LEFT], Pole->Position2D);                                
                                CullPole(Pole);
                            }
                            
                        }
                        else if(IsWalkablePole(SurroundingPoles[POLE_INDEX_LEFT]))
                        {
                            WALKING_SYSTEM_EVENT_DRAW_POINT(Pole->IntersectionPoint, White());                            
                            
                            surface_edge_intersection_query Query = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);                                                                                            
                            if(Query.HasIntersected)                                
                            {
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Query.P, Pole->ZIntersection), Yellow());
                                WALKING_SYSTEM_EVENT_DRAW_EDGE(Pole->IntersectionPoint, V3(Query.P, Pole->ZIntersection), White());                                
                                Pole->SurfaceQueries[POLE_INDEX_BOTTOM] = Query;                                                                                                         
                            }
                            else
                            {
                                walkable_pole* TargetPole = SurroundingPoles[POLE_INDEX_TOP];
                                
                                WALKING_SYSTEM_EVENT_DRAW_POINT(TargetPole->IntersectionPoint, White());
                                WALKING_SYSTEM_EVENT_DRAW_POINT(V3(Pole->Position2D, TargetPole->ZIntersection), Yellow());
                                WALKING_SYSTEM_EVENT_DRAW_EDGE(TargetPole->IntersectionPoint, V3(Pole->Position2D, TargetPole->ZIntersection), White());
                                
                                MakeSurfaceIntersectedQuery(&TargetPole->SurfaceQueries[POLE_INDEX_BOTTOM], Pole->Position2D);                            
                                CullPole(Pole);
                            }
                        }
                        else if(IsWalkablePole(SurroundingPoles[POLE_INDEX_BOTTOM_LEFT]))
                        {
                            //TODO(JJ): Do we need to implement this?
                            NOT_IMPLEMENTED;
                        }
                        else
                        {
                            INVALID_CODE;
                        }
                    } break;                                        
                    
                    case GRID_METHOD_TYPE_TRIANGLE:
                    {      
                        ASSERT(false);
                        if(!IsWalkablePole(SurroundingPoles[POLE_INDEX_LEFT]))                        
                        {
                            surface_edge_intersection_query Query = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_LEFT);                                                                                                            
                            Pole->SurfaceQueries[POLE_INDEX_LEFT] = Query;                                                            
                        }
                        
                        if(!IsWalkablePole(SurroundingPoles[POLE_INDEX_BOTTOM]))
                        {
                            surface_edge_intersection_query Query = SurfaceEdgeIntersectionQuery(Pole, SurroundingPoles, POLE_INDEX_BOTTOM);
                            Pole->SurfaceQueries[POLE_INDEX_BOTTOM] = Query;                
                        }
                    } break;        
                }                
            }            
        }
    }    
    END_POLE_EDGE_TESTING();
    
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
    
    BEGIN_RING_BUILDING_TESTING();
    walkable_triangle_ring_list RingList = {};    
    for(i32 YIndex = 1; YIndex < Grid.CellCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.CellCount.x-1; XIndex++)
        {
            walkable_pole** CellPoles = GetCellPoles(&Grid, XIndex, YIndex);
            grid_method_type Type = GetGridPolesIsNotCulledWalkableMethodType(CellPoles);
            
            switch(Type)
            {
                case GRID_METHOD_TYPE_CORNER:
                {                    
                    if(IsNotCulledPole(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]))
                    {
                        if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].HasIntersected &&
                           CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM].HasIntersected)                           
                        {                            
                            if(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT].HasIntersected)
                            {
                                
                                v3f P[4] = 
                                {
                                    Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM_LEFT].P),
                                    Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM].P),
                                    CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint,
                                    Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].P)
                                };
                                
                                AddQuadRings(&RingList, P[0], P[1], P[2], P[3]);
                            }
                            else
                            {
                                NOT_IMPLEMENTED;
                            }
                        }
                    }
                } break;
                
                case GRID_METHOD_TYPE_WALL:
                {                    
                    b32 WallCase = false;
                    v3f P[4] = {};
                    
                    if(IsNotCulledPole(CellPoles[POLE_CELL_INDEX_TOP_RIGHT]))
                    {
                        if(IsNotCulledPole(CellPoles[POLE_CELL_INDEX_TOP_LEFT]))
                        {                               
                            if(CellPoles[POLE_CELL_INDEX_TOP_LEFT]->SurfaceQueries[POLE_INDEX_BOTTOM].HasIntersected &&
                               CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM].HasIntersected)
                            {
                                
                                WallCase = true;
                                P[0] = Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_LEFT], CellPoles[POLE_CELL_INDEX_TOP_LEFT]->SurfaceQueries[POLE_INDEX_BOTTOM].P);                            
                                P[1] = Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_BOTTOM].P);
                                P[2] = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;
                                P[3] = CellPoles[POLE_CELL_INDEX_TOP_LEFT]->IntersectionPoint;
                            }
                        }
                        else if(IsNotCulledPole(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]))
                        {                                                        
                            if(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].HasIntersected &&
                               CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].HasIntersected)
                            {
                                WallCase = true;
                                P[0] = Get3DPoint(CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT], CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].P);                            
                                P[1] = CellPoles[POLE_CELL_INDEX_BOTTOM_RIGHT]->IntersectionPoint;                            
                                P[2] = CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->IntersectionPoint;    
                                P[3] = Get3DPoint(CellPoles[POLE_CELL_INDEX_TOP_RIGHT], CellPoles[POLE_CELL_INDEX_TOP_RIGHT]->SurfaceQueries[POLE_INDEX_LEFT].P);                            
                            }
                        }
                    }
                    
                    if(WallCase)
                    {
                        AddQuadRings(&RingList, P[0], P[1], P[2], P[3]);
                    }
                    else
                    {
                        NOT_IMPLEMENTED;
                    }
                } break;
                
                case GRID_METHOD_TYPE_TRIANGLE:
                {
                    NOT_IMPLEMENTED;
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
#if 0
    for(i32 YIndex = 1; YIndex < Grid.CellCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.CellCount.x-1; XIndex++)
        {            
            walkable_pole** CellPoles = GetCellPoles(&Grid, XIndex, YIndex);
            
            u32 BottomLeft = 0;
            u32 BottomRight = 1;
            u32 TopLeft = 2;
            u32 TopRight = 3;
            
            u32 Method = 1;
            if(IsValidWalkablePole(CellPoles[BottomLeft])) Method *= 2;            
            if(IsValidWalkablePole(CellPoles[BottomRight])) Method *= 2;
            if(IsValidWalkablePole(CellPoles[TopLeft])) Method *= 2;
            if(IsValidWalkablePole(CellPoles[TopRight])) Method *= 2;
            
            switch(Method)
            {
                case 2:
                {
                    walkable_pole* TargetPole = NULL;
                    
                    surface_intersection_query Queries[3];
                    if(IsValidWalkablePole(CellPoles[BottomLeft]))
                    {
                        TargetPole = CellPoles[BottomLeft];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_RIGHT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP_RIGHT].Query;                        
                    }                    
                    else if(IsValidWalkablePole(CellPoles[BottomRight]))
                    {
                        TargetPole = CellPoles[BottomRight];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_LEFT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP_LEFT].Query;                        
                    }
                    else if(IsValidWalkablePole(CellPoles[TopLeft]))
                    {
                        TargetPole = CellPoles[TopLeft];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_RIGHT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM_RIGHT].Query;                        
                    }
                    else if(IsValidWalkablePole(CellPoles[TopRight]))
                    {
                        TargetPole = CellPoles[TopRight];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_LEFT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM_LEFT].Query;                        
                    }
                    else
                    {
                        INVALID_CODE;
                    }   
                    
                    v3f RingPoints[3]
                    {
                        Get3DPoint(Queries[0]),
                        Get3DPoint(Queries[1]),                                                        
                        V3(Queries[2].P, INFINITY)
                    };
                    
                    RingPoints[2] = Get3DPoint(TargetPole, RingPoints[2].xy);
                    ASSERT(RingPoints[2].z != INFINITY);                    
                    AddQuadRings(&RingList, TargetPole->IntersectionPoint, RingPoints[0], RingPoints[2], RingPoints[1]);                                        
                } break;
                v
                    case 4:
                {
                    b32 WallCase = false;
                    walkable_pole* InPoleA = NULL;
                    walkable_pole* OutPoleA = NULL;                    
                    walkable_pole* InPoleB = NULL;
                    walkable_pole* OutPoleB = NULL;                    
                    v3f P[4] = {};                    
                    
                    if(IsValidWalkablePole(CellPoles[BottomLeft]))
                    {
                        if(IsValidWalkablePole(CellPoles[BottomRight], POLE_INDEX_TOP) && CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_TOP].HasIntersected)
                        {   
                            ASSERT(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_TOP].HasIntersected);
                            ASSERT(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_TOP].HasIntersected);
                            P[0] = CellPoles[BottomLeft]->IntersectionPoint;                                                                                    
                            P[1] = Get3DPoint(CellPoles[BottomLeft], CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_TOP].Query.P);
                            P[2] = CellPoles[BottomRight]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[BottomRight], CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_TOP].Query.P);                            
                            WallCase = true;                                                        
                        }
                        else if(IsValidWalkablePole(CellPoles[TopLeft], POLE_INDEX_RIGHT) && CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].HasIntersected)
                        {                            
                            ASSERT(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].HasIntersected);
                            ASSERT(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].HasIntersected);
                            P[0] = CellPoles[BottomLeft]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[BottomLeft], CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query.P);
                            P[2] = CellPoles[TopLeft]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[TopLeft], CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query.P);                            
                            WallCase = true;
                        }
                        else if(IsValidWalkablePole(CellPoles[TopRight]))
                        {
                            WallCase = false;
                        }                        
                    }
                    else if(IsValidWalkablePole(CellPoles[BottomRight], POLE_INDEX_LEFT))
                    {
                        if(IsValidWalkablePole(CellPoles[TopRight], POLE_INDEX_LEFT))
                        {
                            ASSERT(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_LEFT].HasIntersected);
                            ASSERT(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_LEFT].HasIntersected);                            
                            P[0] = CellPoles[BottomRight]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[BottomRight], CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query.P);
                            P[2] = CellPoles[TopifRight]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[TopRight], CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query.P);                            
                            WallCase = true;
                        }
                        else if(IsValidWalkablePole(CellPoles[TopLeft]))
                        {
                            WallCase = false;                            
                        }
                    }
                    else if(IsValidWalkablePole(CellPoles[TopLeft], POLE_INDEX_BOTTOM))
                    {
                        if(IsValidWalkablePole(CellPoles[TopRight], POLE_INDEX_BOTTOM))
                        {
                            ASSERT(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_BOTTOM].HasIntersected);
                            ASSERT(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_BOTTOM].HasIntersected);                            
                            P[0] = CellPoles[TopLeft]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[TopLeft], CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query.P);                            
                            P[2] = CellPoles[TopRight]->IntersectionPoint;                            
                            P[3] = Get3DPoint(CellPoles[TopRight], CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query.P);                           
                            WallCase = true;
                        }
                    }
                    else
                    {                        
                        continue;
                    }
                    
                    if(WallCase)
                    {                        
                        b32 AddRings = SquareMagnitude(P[1] - P[0]) > 1e-6f && SquareMagnitude(P[3] - P[2]) > 1e-6f;
                        if(AddRings)
                            AddQuadRings(&RingList, P[0], P[1], P[3], P[2]);
                    }
                    else
                    {
                        //TODO(JJ): Do we need to implement this?
                        NOT_IMPLEMENTED;
                    }
                } break;
                
                case 8:
                {
                    walkable_pole* OutPole = NULL;
                    walkable_pole* CornerInPole = NULL;
                    walkable_pole* HorizontalInPole = NULL;
                    walkable_pole* VerticalInPole = NULL;
                    
                    v3f P[5];
                    
                    if(!CellPoles[BottomLeft]->HitWalkable)
                    {   
                        P[0] = Get3DPoint(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query);
                        P[1] = CellPoles[BottomRight]->IntersectionPoint;
                        P[2] = CellPoles[TopRight]->IntersectionPoint;
                        P[3] = Get3DPoint(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query);
                        P[4] = CellPoles[TopLeft]->IntersectionPoint;                        
                    }
                    else if(!CellPoles[BottomRight]->HitWalkable)
                    {
                        P[0] = Get3DPoint(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query);
                        P[1] = CellPoles[BottomLeft]->IntersectionPoint;
                        P[2] = CellPoles[TopLeft]->IntersectionPoint;
                        P[3] = Get3DPoint(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query);
                        P[4] = CellPoles[TopRight]->IntersectionPoint;                        
                    }
                    else if(!CellPoles[TopLeft]->HitWalkable)
                    {
                        P[0] = Get3DPoint(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query);
                        P[1] = CellPoles[TopRight]->IntersectionPoint;
                        P[2] = CellPoles[BottomRight]->IntersectionPoint;
                        P[3] = Get3DPoint(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_TOP].Query);
                        P[4] = CellPoles[BottomLeft]->IntersectionPoint;                        
                    }
                    else if(!CellPoles[TopRight]->HitWalkable)
                    {
                        P[0] = Get3DPoint(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query);
                        P[1] = CellPoles[TopLeft]->IntersectionPoint;
                        P[2] = CellPoles[BottomLeft]->IntersectionPoint;
                        P[3] = Get3DPoint(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_TOP].Query);
                        P[4] = CellPoles[BottomRight]->IntersectionPoint;                       
                    }
                    else
                    {
                        INVALID_CODE;
                    }
                    
                    AddTriangleRing(&RingList, P[0], P[1], P[2]);
                    AddTriangleRing(&RingList, P[0], P[2], P[3]);
                    AddTriangleRing(&RingList, P[3], P[4], P[2]);
                } break;
                
                case 16:
                {
                    AddQuadRings(&RingList, 
                                 CellPoles[BottomLeft]->IntersectionPoint, CellPoles[BottomRight]->IntersectionPoint, 
                                 CellPoles[TopRight]->IntersectionPoint,   CellPoles[TopLeft]->IntersectionPoint);
                } break;
            } 
        }
    }
#endif
    
    BEGIN_RING_TRAVERSAL_TESTING();
    
    f32 BestSqrDistance = FLT_MAX;
    v3f FinalPosition = RequestedPosition;
    for(walkable_triangle_ring* Ring = RingList.Head; Ring; Ring = Ring->Next)
    {
        if(IsPointInTriangle2D(Ring->P[0].xy, Ring->P[1].xy, Ring->P[2].xy, RequestedPosition.xy))
        {
            FinalPosition.xy = RequestedPosition.xy;
            FinalPosition.z = FindTriangleZ(Ring->P[0], Ring->P[1], Ring->P[2], FinalPosition.xy);
            break;
        }
        
        v3f ClosestPoint = PointTriangleClosestPoint(Ring->P[0], Ring->P[1], Ring->P[2], RequestedPosition);
        //DRAW_POINT(ClosestPoint, 0.05f, RGBA(0.99f, 0.99f, 0.0f, 1.0f));
        f32 SqrDistance = SquareMagnitude(ClosestPoint-RequestedPosition);
        if(SqrDistance < BestSqrDistance)
        {
            FinalPosition = ClosestPoint;
            BestSqrDistance = SqrDistance;
        }
    }
    
    END_RING_TRAVERSAL_TESTING();
    
    Player->Position = FinalPosition;
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