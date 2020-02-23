#include "world_game.h"
#include "geometry.cpp"

entity* CreateEntity(game* Game, v3f Position, v3f Scale, v3f Euler, c4 Color, b32 IsBlocker, triangle_mesh* Mesh)
{
    entity* Result = NULL;
    if(Game->FreeEntities.Count > 0)    
        Result = RemoveEndOfList<entity_list, entity>(&Game->FreeEntities);    
    else
        Result = PushStruct(&Game->WorldStorage, entity, Clear, 0);
    
    Result->Transform = CreateSQT(Position, Scale, Euler);
    Result->Color = Color;
    Result->Mesh = Mesh;
    AddToList(&Game->AllocatedEntities, Result);
    return Result;
}

#define GRID_DENSITY 0.5f

v2i GetCell(v2f Position)
{
    v2i Result = FloorV2(Position/GRID_DENSITY);
    return Result;
}

v2f GetPolePosition(v2i Index)
{
    v2f Result = Index * GRID_DENSITY;
    return Result;
}

v2f GetPolePosition(i32 XIndex, i32 YIndex)
{
    v2f Result = GetPolePosition(V2i(XIndex, YIndex));
    return Result;
}

extern "C"
EXPORT GAME_TICK(Tick)
{   
    DEVELOPER_GRAPHICS(Graphics);
    
    Global_Platform = Platform;        
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);       
    
    if(!Game->Initialized)
    {
        Game->Initialized = true;
        Game->WorldStorage = CreateArena(KILOBYTE(32));
        Game->BoxMesh = CreateBoxMesh(&Game->WorldStorage);
        
        Game->Player = CreateEntity(Game, V3(0.0f, 0.0f, 1.0f), V3(1.0f), V3(), RGBA(0.0f, 0.0f, 1.0f, 1.0f), false, &Game->BoxMesh);
        CreateEntity(Game, V3(), V3(10.0f, 10.0f, 1.0f), V3(), RGBA(0.25f, 0.25f, 0.25f, 1.0f), false, &Game->BoxMesh);        
    }        
    
    //TODO(JJ): Implement the Walking System
    input* Input = Game->Input;
    const f32 MOVE_ACCELERATION = 20.0f;
    const f32 MOVE_DAMPING = 5.0f;
    
    entity* Player = Game->Player;
    
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
    
    v2i MinimumCell = MinimumV2(StartCell, EndCell);
    v2i MaximumCell = MaximumV2(StartCell, EndCell)+2;
    
    MinimumCell -= 1;
    MaximumCell += 1;
    
    u32 YCount = MaximumCell.y-MinimumCell.y;
    u32 XCount = MaximumCell.x-MinimumCell.x;
    
    v3f* IntersectionPoles = PushArray(XCount*YCount, v3f, Clear, 0);   
    u32 PoleIndex = 0;
    for(i32 YIndex = MinimumCell.y; YIndex < MaximumCell.y; YIndex++)
    {
        for(i32 XIndex = MinimumCell.x; XIndex < MaximumCell.x; XIndex++)
        {                        
            v2f PolePosition = GetPolePosition(XIndex, YIndex);            
            
            IntersectionPoles[PoleIndex].z = -FLT_MAX;
            
            for(entity* Entity = Game->AllocatedEntities.First; Entity; Entity = Entity->Next)
            {
                if(Entity != Player)
                {
                    v2f LocalPolePosition = Rotate(V3(PolePosition, 0.0f) - Entity->Position, Conjugate(Entity->Orientation)).xy;
                    
                    triangle_mesh* Mesh = Entity->Mesh;
                    for(u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; TriangleIndex++)
                    {
                        triangle* Triangle = Mesh->Triangles + TriangleIndex;
                        
                        v3f P[3] = 
                        {
                            Triangle->P[0]*Entity->Scale,
                            Triangle->P[1]*Entity->Scale,
                            Triangle->P[2]*Entity->Scale
                        };
                        
                        f32 ZIntersection = INFINITY;
                        
                        
                        u32 LineIndices[2];
                        if(IsDegenerateTriangle2D(P[0].xy, P[1].xy, P[2].xy, LineIndices))
                        {                            
                            //TODO(JJ): Do we even need to handle this degenerate case? I feel like the rest of the algorithm 
                            // will provide a much more numerically stable result than to figure out the triangles z on the
                            //degenerate cases. Maybe we handle this case just for blockers since we know the players z should
                            //just be exactly where the player z currently is
#if 0                                                     
                            if(IsPointOnLine2D(LocalPolePosition, P[LineIndices[0]].xy, P[LineIndices[1]].xy))                                                        
                                ZIntersection = FindTriangleZ(P[0], P[1], P[2], LocalPolePosition);                                                                                                                                                                        
#endif
                        }
                        else if(IsPointInTriangle2D(LocalPolePosition, P[0].xy, P[1].xy, P[2].xy))                            
                            ZIntersection = FindTriangleZ(P[0], P[1], P[2], LocalPolePosition);                                                                                        
                        
                        if(ZIntersection != INFINITY)
                        {
                            if((ZIntersection <= (Player->Position.z + Player->Scale.z)) &&
                               (ZIntersection > IntersectionPoles[PoleIndex].z))
                            {
                                if(IntersectionPoles[PoleIndex].z != -FLT_MAX)
                                    DRAW_POINT(V3(PolePosition, ZIntersection), 0.05f, RGBA(1.0f, 0.0f, 0.0f, 1.0f));                                                                                                
                                
                                IntersectionPoles[PoleIndex] = V3(PolePosition, ZIntersection);
                            }
                            else
                                DRAW_POINT(V3(PolePosition, ZIntersection), 0.05f, RGBA(1.0f, 0.0f, 0.0f, 1.0f));
                        }
                    }                                                            
                }
            }
            
            if(IntersectionPoles[PoleIndex].z != -FLT_MAX)
            {
                DRAW_POINT(IntersectionPoles[PoleIndex], 0.05f, RGBA(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            PoleIndex++;
        }
    }
    
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