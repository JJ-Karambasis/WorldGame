#include "world_game.h"
#include "geometry.cpp"

#define GRID_DENSITY 0.2f

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

inline v2i 
GetCell(v2f Position)
{
    v2i Result = FloorV2(Position/GRID_DENSITY);
    return Result;
}

inline walkable_pole* 
GetPole(walkable_grid* Grid, i32 XIndex, i32 YIndex)
{
    ASSERT((XIndex < Grid->PoleCount.x) && (XIndex >= 0));
    ASSERT((YIndex < Grid->PoleCount.y) && (YIndex >= 0));
    walkable_pole* Result = Grid->Poles + ((YIndex*Grid->PoleCount.x)+XIndex);
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

method_type GetMethodType(walkable_pole* TargetPole, walkable_pole* VerticalPole, walkable_pole* HorizontalPole, walkable_pole* CornerPole)
{
    ASSERT(TargetPole->HitWalkable);
    u32 Method = 2;
    if(VerticalPole->HitWalkable) Method *= 2;
    if(HorizontalPole->HitWalkable) Method *= 2;
    if(CornerPole->HitWalkable) Method *= 2;
    
    switch(Method)
    {
        case 2:
        return METHOD_TYPE_CORNER;
        
        case 4:
        return METHOD_TYPE_WALL;
        
        case 8:
        return METHOD_TYPE_TRIANGLE;
        
        case 16:
        return METHOD_TYPE_SURFACE;
        
        INVALID_DEFAULT_CASE;
    }
    
    return METHOD_TYPE_UNKNOWN;
}

method_type GetMethodType(walkable_surface_intersection* SurfaceIntersections, pole_index VerticalIndex, pole_index HorizontalIndex, pole_index CornerIndex)
{        
    u32 Method = 1;
    if(SurfaceIntersections[VerticalIndex].HasIntersected) Method *= 2;
    if(SurfaceIntersections[HorizontalIndex].HasIntersected) Method *= 2;
    if(SurfaceIntersections[CornerIndex].HasIntersected) Method *= 2;
    
    switch(Method)
    {
        case 8:
        return METHOD_TYPE_CORNER;
        
        case 4:
        return METHOD_TYPE_TRIANGLE;
        
        case 2:
        return METHOD_TYPE_WALL;
        
        case 1:
        return METHOD_TYPE_SURFACE;
                        
        INVALID_DEFAULT_CASE;
    }
    
    return METHOD_TYPE_UNKNOWN;
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
    
    DEBUG_DRAW_RING(Ring);
}

inline void 
AddQuadRings(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2, v3f p3)
{
    AddTriangleRing(List, p0, p1, p2);
    AddTriangleRing(List, p2, p3, p0);
}

inline void AddSurfaceIntersection(walkable_pole* Pole, pole_index Index, surface_intersection_query Query)
{
    Pole->SurfaceIntersections[Index].HasIntersected = true;
    Pole->SurfaceIntersections[Index].Query = Query;    
}

inline void AddSurfaceIntersection(walkable_pole* Pole, pole_index Index, v2f P)
{
    surface_intersection_query Result = {P};
    AddSurfaceIntersection(Pole, Index, Result);
}

surface_intersection_query SurfaceIntersectionQuery(walkable_pole* HitPole, walkable_pole* MissPole)
{ 
    ASSERT(!MissPole->HitEntity);
    ASSERT(HitPole->HitEntity);
    
    static_entity* Entity = HitPole->HitEntity;    
    triangle_mesh* Triangles = Entity->Mesh;
    
    edge2D D = CreateEdge2D(MissPole->Position2D, HitPole->Position2D);
    
    
    f32 BestDistance = FLT_MAX;    
    
    surface_intersection_query Result = {};
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
            CopyArray(Result.TriangleP, P, 3, v3f);            
            Result.Edge = Edge[BestIndex];            
            BestDistance = Distance;
        }        
    }        
    
    ASSERT(BestDistance != FLT_MAX);
        
    return Result;
}

surface_intersection_query SurfaceIntersectionQuery(walkable_pole* TargetPole, walkable_pole** SurroundingPoles, pole_index Index)
{    
    surface_intersection_query Result;                        
    if(TargetPole->SurfaceIntersections[Index].HasIntersected)                        
        Result = TargetPole->SurfaceIntersections[Index].Query;                        
    else                        
        Result = SurfaceIntersectionQuery(TargetPole, SurroundingPoles[Index]);                        
    return Result;
}

inline v3f Get3DPoint(surface_intersection_query Query)
{
    v3f Result = V3(Query.P, FindTriangleZ(Query.TriangleP[0], Query.TriangleP[1], Query.TriangleP[2], Query.P));
    return Result;
}

inline void PoleSectionTest(walkable_pole* TargetPole, walkable_pole** SurroundingPoles, 
                            pole_index VerticalIndex, pole_index HorizontalIndex, pole_index CornerIndex)
{    
    method_type Type = GetMethodType(TargetPole, SurroundingPoles[VerticalIndex], SurroundingPoles[HorizontalIndex], SurroundingPoles[CornerIndex]);
    switch(Type)
    {
        case METHOD_TYPE_CORNER:
        {            
            walkable_pole* HorizontalPole = SurroundingPoles[HorizontalIndex];
            walkable_pole* VerticalPole = SurroundingPoles[VerticalIndex];
            walkable_pole* CornerPole = SurroundingPoles[CornerIndex];
            
            surface_intersection_query AQuery = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, HorizontalIndex);
            surface_intersection_query BQuery = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, VerticalIndex);                        
            
            edge2D A, B;
            A.P[0] = AQuery.P;                    
            B.P[0] = BQuery.P;
            
            f32 SignDistA0 = TargetPole->IntersectionPoint.x - A.P[0].x;
            f32 SignDistB0 = TargetPole->IntersectionPoint.y - B.P[0].y;
            if(Abs(SignDistA0) > 1e-8f && Abs(SignDistB0) > 1e-8f)
            {                        
                {
                    v2f P0 = V2(TargetPole->Position2D.x, TargetPole->Position2D.y - (SignDistB0*0.5f));
                    v2f P1 = V2(HorizontalPole->Position2D.x, HorizontalPole->Position2D.y - (SignDistB0*0.5f));
                    
                    b32 Check = EdgeToEdgeIntersection2D(&A.P[1], CreateEdge2D(P0, P1), AQuery.Edge);
                    ASSERT(Check);
                }
                
                {
                    v2f P0 = V2(TargetPole->Position2D.x - (SignDistA0*0.5f),   TargetPole->Position2D.y);
                    v2f P1 = V2(VerticalPole->Position2D.x - (SignDistA0*0.5f), VerticalPole->Position2D.y);                                                        
                    b32 Check = EdgeToEdgeIntersection2D(&B.P[1], CreateEdge2D(P0, P1), BQuery.Edge);
                    ASSERT(Check);
                }
                
                ray2D RayA = CreateRay2D(A);
                ray2D RayB = CreateRay2D(B);
                
                v2f CommonPoint = InvalidV2();                    
                if(RayToRayIntersection2D(&CommonPoint, RayA, RayB))
                {   
                    AddSurfaceIntersection(TargetPole, HorizontalIndex, AQuery);
                    AddSurfaceIntersection(TargetPole, VerticalIndex, BQuery);
                    AddSurfaceIntersection(TargetPole, CornerIndex, CommonPoint);                                
                }
                else
                {
                    NOT_IMPLEMENTED;                                
                }
            }     
            else
            {
                NOT_IMPLEMENTED;
            }                        
        } break;
        
        case METHOD_TYPE_WALL:
        {            
            if(SurroundingPoles[VerticalIndex]->HitWalkable)
            {   
                surface_intersection_query Query = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, HorizontalIndex);
                AddSurfaceIntersection(TargetPole, HorizontalIndex, Query);
            }
            else if(SurroundingPoles[HorizontalIndex]->HitWalkable)
            {
                surface_intersection_query Query = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, VerticalIndex);
                AddSurfaceIntersection(TargetPole, VerticalIndex, Query);
            }
            else if(SurroundingPoles[CornerIndex]->HitWalkable)
            {
                //TODO(JJ): Do we need to implement this?
                NOT_IMPLEMENTED;
            }
            else
            {
                INVALID_CODE;
            }
        } break;                                        
                
        case METHOD_TYPE_TRIANGLE:
        {                                                
            if(!SurroundingPoles[HorizontalIndex]->HitWalkable)                        
            {
                surface_intersection_query Query = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, HorizontalIndex);                                                                                
                AddSurfaceIntersection(TargetPole, HorizontalIndex, Query);
            }
            
            if(!SurroundingPoles[VerticalIndex]->HitWalkable)
            {
                surface_intersection_query Query = SurfaceIntersectionQuery(TargetPole, SurroundingPoles, VerticalIndex);
                AddSurfaceIntersection(TargetPole, VerticalIndex, Query);
            }
        } break;        
    }    
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
        
        CreateStaticEntity(Game, V3(1.1f, 1.1f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), false, &Game->BoxMesh);        
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
    
    walkable_grid Grid = BuildWalkableGrid(MinimumCell, MaximumCell);                
    for(i32 YIndex = 0; YIndex < Grid.PoleCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Grid.PoleCount.x; XIndex++)
        {   
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);            
            
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
                                DRAW_POINT(V3(Pole->Position2D, ZIntersection), 0.05f, RGBA(1.0f, 0.0f, 0.0f, 1.0f));                                                                                                
                            
                            Pole->ZIntersection = ZIntersection;
                            HitTriangle = Triangle;
                            HitEntity = Entity;
                        }
                        else
                            DRAW_POINT(V3(Pole->Position2D, ZIntersection), 0.05f, RGBA(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                }                                                                            
            }
            
            if(Pole->ZIntersection != -FLT_MAX)
            {
                DRAW_POINT(Pole->IntersectionPoint, 0.05f, RGBA(1.0f, 1.0f, 1.0f, 1.0f));                
                Pole->HitEntity = HitEntity; 
                Pole->HitTriangle = HitTriangle;
                Pole->HitWalkable = true;
            }                        
        }
    }
    
    for(i32 YIndex = 1; YIndex < Grid.PoleCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.PoleCount.x-1; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);
            if(Pole->HitWalkable)
            {
                walkable_pole** SurroundingPoles = GetSurroundingPoles(&Grid, XIndex, YIndex);                
                PoleSectionTest(Pole, SurroundingPoles, POLE_INDEX_BOTTOM, POLE_INDEX_LEFT,  POLE_INDEX_BOTTOM_LEFT);
                PoleSectionTest(Pole, SurroundingPoles, POLE_INDEX_TOP,    POLE_INDEX_LEFT,  POLE_INDEX_TOP_LEFT);
                PoleSectionTest(Pole, SurroundingPoles, POLE_INDEX_TOP,    POLE_INDEX_RIGHT, POLE_INDEX_TOP_RIGHT);
                PoleSectionTest(Pole, SurroundingPoles, POLE_INDEX_BOTTOM, POLE_INDEX_RIGHT, POLE_INDEX_BOTTOM_RIGHT);                
            }            
        }
    }
    
    f32 Radius = Player->Radius;         
    for(i32 YIndex = 1; YIndex < Grid.PoleCount.y-1; YIndex++)
    {
        for(i32 XIndex = 1; XIndex < Grid.PoleCount.x-1; XIndex++)
        {
            walkable_pole* Pole = GetPole(&Grid, XIndex, YIndex);
            if(Pole->HitWalkable)
            {
                method_type Type = GetMethodType(Pole->SurfaceIntersections, POLE_INDEX_BOTTOM, POLE_INDEX_LEFT, POLE_INDEX_BOTTOM_LEFT);
                switch(Type)
                {
                    case METHOD_TYPE_CORNER:
                    {   
                        v2f P[3] = 
                        {
                            Pole->SurfaceIntersections[POLE_INDEX_BOTTOM].P,
                            Pole->SurfaceIntersections[POLE_INDEX_LEFT].P,
                            Pole->SurfaceIntersections[POLE_INDEX_BOTTOM_LEFT].P
                        }; 
                        
                        v2i Min = V2i(XIndex, YIndex);                        
                        v2i Max = MaximumV2(Min+(Padding-1), Grid.PoleCount);                                                
                        
                        for(u32 Y = Min.y; Y < Max.y; Y++)
                        {
                            for (u32 X = Min.x; X < Max.x; X++)
                            {
                                walkable_pole* TestPole = GetPole(&Grid, X, Y);
                                if(TestPole->HitWalkable)
                                    TestPole->IsCulled = true;                                
                            }
                        }
                        
                        walkable_pole* TestPole = GetPole(&Grid, Min+Padding);
                        if(TestPole->HitWalkable)
                        {
                            v2f Corner = P[2] + Radius;
                            v2f Vertical = V2(TestPole->Position2D.x, P[0].y + Radius);
                            v2f Horizontal = V2(P[1].x + Radius, TestPole->Position2D.y);
                            
                            TestPole->SurfaceIntersections[POLE_INDEX_BOTTOM_LEFT].P = Corner;
                            TestPole->SurfaceIntersections[POLE_INDEX_BOTTOM].P = Vertical;
                            TestPole->SurfaceIntersections[POLE_INDEX_LEFT].P = Horizontal;
                        }                                
                    } break;
                    
                    case METHOD_TYPE_WALL:
                    {
                        Pole->IsCulled = true;
                    } break;
                    
                    case METHOD_TYPE_TRIANGLE:
                    {
                        Pole->IsCulled = true;
                    } break;                    
                } 
            }
        }
    }
    
    walkable_triangle_ring_list RingList = {};
    
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
            if(CellPoles[BottomLeft]->HitWalkable) Method *= 2;            
            if(CellPoles[BottomRight]->HitWalkable) Method *= 2;
            if(CellPoles[TopLeft]->HitWalkable) Method *= 2;
            if(CellPoles[TopRight]->HitWalkable) Method *= 2;
            
            switch(Method)
            {
                case 2:
                {
                    walkable_pole* TargetPole = NULL;
                    
                    surface_intersection_query Queries[3];
                    if(CellPoles[BottomLeft]->HitWalkable)
                    {
                        TargetPole = CellPoles[BottomLeft];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_RIGHT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP_RIGHT].Query;                        
                    }                    
                    else if(CellPoles[BottomRight]->HitWalkable)
                    {
                        TargetPole = CellPoles[BottomRight];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_LEFT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_TOP_LEFT].Query;                        
                    }
                    else if(CellPoles[TopLeft]->HitWalkable)
                    {
                        TargetPole = CellPoles[TopLeft];
                        Queries[0] = TargetPole->SurfaceIntersections[POLE_INDEX_RIGHT].Query;
                        Queries[1] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM].Query;
                        Queries[2] = TargetPole->SurfaceIntersections[POLE_INDEX_BOTTOM_RIGHT].Query;                        
                    }
                    else if(CellPoles[TopRight]->HitWalkable)
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
                        
                        if(IsPointInTriangle2D(P[0], P[1], P[2], RingPoints[2].xy))
                        {
                            RingPoints[2].z = FindTriangleZ(P[0], P[1], P[2], RingPoints[2].xy);
                            break;
                        }
                    }                            
                    ASSERT(RingPoints[2].z != INFINITY);                    
                    AddQuadRings(&RingList, TargetPole->IntersectionPoint, RingPoints[0], RingPoints[2], RingPoints[1]);                                        
                } break;
                
                case 4:
                {
                    b32 WallCase = false;
                    walkable_pole* InPoleA = NULL;
                    walkable_pole* OutPoleA = NULL;                    
                    walkable_pole* InPoleB = NULL;
                    walkable_pole* OutPoleB = NULL;
                    
                    v3f P[4] = {};                    
                    
                    if(CellPoles[BottomLeft]->HitWalkable)
                    {
                        if(CellPoles[BottomRight]->HitWalkable)
                        {   
                            P[0] = CellPoles[BottomLeft]->IntersectionPoint;                                                                                    
                            P[1] = Get3DPoint(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_TOP].Query);
                            P[2] = CellPoles[BottomRight]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_TOP].Query);                            
                            WallCase = true;                                                        
                        }
                        else if(CellPoles[TopLeft]->HitWalkable)
                        {
                            P[0] = CellPoles[BottomLeft]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[BottomLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query);
                            P[2] = CellPoles[TopLeft]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_RIGHT].Query);                            
                            WallCase = true;
                        }
                        else if(CellPoles[TopRight]->HitWalkable)
                        {
                            WallCase = false;
                        }                        
                    }
                    else if(CellPoles[BottomRight]->HitWalkable)
                    {
                        if(CellPoles[TopRight]->HitWalkable)
                        {
                            P[0] = CellPoles[BottomRight]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[BottomRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query);
                            P[2] = CellPoles[TopRight]->IntersectionPoint;
                            P[3] = Get3DPoint(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_LEFT].Query);                            
                            WallCase = true;
                        }
                        else if(CellPoles[TopLeft]->HitWalkable)
                        {
                            WallCase = false;                            
                        }
                    }
                    else if(CellPoles[TopLeft]->HitWalkable)
                    {
                        if(CellPoles[TopRight]->HitWalkable)
                        {
                            P[0] = CellPoles[TopLeft]->IntersectionPoint;
                            P[1] = Get3DPoint(CellPoles[TopLeft]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query);                            
                            P[2] = CellPoles[TopRight]->IntersectionPoint;                            
                            P[3] = Get3DPoint(CellPoles[TopRight]->SurfaceIntersections[POLE_INDEX_BOTTOM].Query);                           
                            WallCase = true;
                        }
                    }
                    else
                    {
                        INVALID_CODE; //I think
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
    
    Player->Position = FinalPosition;
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