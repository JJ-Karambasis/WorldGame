#include "world_game.h"
#include "geometry.cpp"

#define GRID_DENSITY 0.5f
#define POLE_BOTTOM_LEFT 0
#define POLE_BOTTOM_RIGHT 1
#define POLE_TOP_LEFT 2
#define POLE_TOP_RIGHT 3

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

inline void 
AddTriangleRing(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2)
{
    walkable_triangle_ring* Ring = PushStruct(walkable_triangle_ring, Clear, 0);
    Ring->P[0] = p0; Ring->P[1] = p1; Ring->P[2] = p2;    
    Ring->Next = List->Head;
    List->Head = Ring;
}

inline void 
AddQuadRings(walkable_triangle_ring_list* List, v3f p0, v3f p1, v3f p2, v3f p3)
{
    AddTriangleRing(List, p0, p1, p2);
    AddTriangleRing(List, p2, p3, p0);
}

struct surface_intersection_query
{
    v3f P;
    b32 Hit;
};

surface_intersection_query SurfaceIntersectionQuery(walkable_pole* HitPole, walkable_pole* MissPole)
{ 
    ASSERT(!MissPole->HitEntity);
    ASSERT(HitPole->HitEntity);

    static_entity* Entity = HitPole->HitEntity;
    
    surface_intersection_query Result = {};
    v2f D = Rotate(V3(MissPole->Position2D - HitPole->Position2D, 0.0f), Conjugate(Entity->Orientation)).xy;
    
    return Result;
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
                
        CreateStaticEntity(Game, V3(1.0f, 1.0f, 0.0f), V3(10.0f, 10.0f, 1.0f), V3(0.0f, 0.0f, 0.0f), RGBA(0.25f, 0.25f, 0.25f, 1.0f), false, &Game->BoxMesh);        
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
    
    v2i StartCell = GetCell(Player->Position.xy);
    v2i EndCell   = GetCell(RequestedPosition.xy);
    
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
                    else if(IsPointInTriangle2D(Pole->Position2D, P[0].xy, P[1].xy, P[2].xy))                            
                        ZIntersection = FindTriangleZ(P[0], P[1], P[2], Pole->Position2D);                                                                                                                
                    
                    if(ZIntersection != INFINITY)
                    {                           ;
                        if((ZIntersection <= (Player->Position.z + Player->Height)) &&
                           (ZIntersection > Pole->ZIntersection))
                        {
                            if(Pole->ZIntersection != -FLT_MAX)
                                DRAW_POINT(V3(Pole->Position2D, ZIntersection), 0.05f, RGBA(1.0f, 0.0f, 0.0f, 1.0f));                                                                                                
                            
                            Pole->ZIntersection = ZIntersection;                                
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
                Pole->HitWalkable = true;
                Pole->HitEntity = HitEntity; 
            }                        
        }
    }
    
#if 0 
    walkable_triangle_ring_list RingList = {};
    for(i32 YIndex = 0; YIndex < Grid.CellCount.y; YIndex++)
    {
        for(i32 XIndex = 0; XIndex < Grid.CellCount.x; XIndex++)
        {            
            walkable_pole** CellPoles = GetCellPoles(&Grid, XIndex, YIndex);
            
            u32 Method = 1;
            if(CellPoles[POLE_BOTTOM_LEFT]->HitWalkable) Method *= 2;            
            if(CellPoles[POLE_BOTTOM_RIGHT]->HitWalkable) Method *= 2;
            if(CellPoles[POLE_TOP_LEFT]->HitWalkable) Method *= 2;
            if(CellPoles[POLE_TOP_RIGHT]->HitWalkable) Method *= 2;
            
            switch(Method)
            {
                case 2:
                {
                    walkable_pole* TargetPole = NULL;
                    walkable_pole* HorizontalPole = NULL;
                    walkable_pole* VerticalPole = NULL;
                    walkable_pole* CornerPole = NULL;
                    
                    if(CellPoles[POLE_BOTTOM_LEFT]->HitWalkable)
                    {
                        TargetPole     = CellPoles[POLE_BOTTOM_LEFT];
                        HorizontalPole = CellPoles[POLE_BOTTOM_RIGHT];
                        VerticalPole   = CellPoles[POLE_TOP_LEFT];
                        CornerPole     = CellPoles[POLE_TOP_RIGHT];
                    }                    
                    else if(CellPoles[POLE_BOTTOM_RIGHT]->HitWalkable)
                    {
                        TargetPole     = CellPoles[POLE_BOTTOM_RIGHT];
                        HorizontalPole = CellPoles[POLE_BOTTOM_LEFT];
                        VerticalPole   = CellPoles[POLE_TOP_RIGHT];
                        CornerPole     = CellPoles[POLE_TOP_LEFT];
                    }
                    else if(CellPoles[POLE_TOP_LEFT]->HitWalkable)
                    {
                        TargetPole     = CellPoles[POLE_TOP_LEFT];
                        HorizontalPole = CellPoles[POLE_TOP_RIGHT];
                        VerticalPole   = CellPoles[POLE_BOTTOM_LEFT];
                        CornerPole     = CellPoles[POLE_BOTTOM_RIGHT];
                    }
                    else if(CellPoles[POLE_TOP_RIGHT]->HitWalkable)
                    {
                        TargetPole     = CellPoles[POLE_TOP_RIGHT];
                        HorizontalPole = CellPoles[POLE_TOP_LEFT];
                        VerticalPole   = CellPoles[POLE_BOTTOM_RIGHT];
                        CornerPole     = CellPoles[POLE_BOTTOM_LEFT];
                    }
                    else
                    {
                        INVALID_CODE;
                    }   
                    
                    surface_intersection_query QueryA0 = SurfaceIntersectionQuery(TargetPole, HorizontalPole);
                    surface_intersection_query QueryB0 = SurfaceIntersectionQuery(TargetPole, VerticalPole);
                } break;
                
                case 4:
                {
                } break;
                
                case 8:
                {
                } break;
                
                case 16:
                {
                    AddQuadRings(&RingList, 
                                 CellPoles[POLE_BOTTOM_LEFT]->IntersectionPoint, CellPoles[POLE_BOTTOM_RIGHT]->IntersectionPoint, 
                                 CellPoles[POLE_TOP_RIGHT]->IntersectionPoint,   CellPoles[POLE_TOP_LEFT]->IntersectionPoint);
                } break;
            } 
        }
    }
    
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
        f32 SqrDistance = SquareMagnitude(ClosestPoint-FinalPosition);
        if(SqrDistance < BestSqrDistance)
        {
            FinalPosition = ClosestPoint;
            BestSqrDistance = SqrDistance;
        }
    }
#endif
    Player->Position = RequestedPosition;
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