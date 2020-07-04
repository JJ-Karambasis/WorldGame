/* Original Author: Armand (JJ) Karambasis */
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

i64 AllocateMesh(graphics* Graphics, mesh_generation_result* Mesh)
{
    i64 Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                        Mesh->Indices, Mesh->IndexCount*sizeof(u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

i64 AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &ImGui::GetIO();
    void* ImGuiFontData;
    v2i ImGuiFontDimensions;
    
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &ImGuiFontDimensions.width, &ImGuiFontDimensions.height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    i64 FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, ImGuiFontDimensions, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void CreateDevLineSphereMesh(dev_context* DevContext, u16 CircleSampleCount)
{
    mesh_generation_result MeshGenerationResult = GenerateLineSphere(GetDefaultArena(), 1.0f, CircleSampleCount);        
    DevContext->LineSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineSphereMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void CreateDevLineBoxMesh(dev_context* DevContext)
{
    mesh_generation_result MeshGenerationResult = GenerateLineBox(GetDefaultArena(), V3(1.0f, 1.0f, 1.0f));    
    DevContext->LineBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
}

void CreateDevTriangleBoxMesh(dev_context* DevContext)
{
    mesh_generation_result MeshGenerationResult = GenerateTriangleBox(GetDefaultArena(), V3(1.0f, 1.0f, 1.0f));       
    DevContext->TriangleBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
}

void CreateDevTriangleSphereMesh(dev_context* DevContext)
{
    mesh_generation_result MeshGenerationResult = GenerateIcosahedronSphere(GetDefaultArena(), 2, 1.0f);
    DevContext->TriangleSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleSphereMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void CreateDevTriangleCylinderMesh(dev_context* DevContext, u16 CircleSampleCount)
{
    mesh_generation_result MeshGenerationResult = GenerateTriangleCylinder(GetDefaultArena(), 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleCylinderMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCylinderMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void CreateDevTriangleConeMesh(dev_context* DevContext, u16 CircleSampleCount)
{
    mesh_generation_result MeshGenerationResult = GenerateTriangleCone(GetDefaultArena(), 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleConeMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleConeMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void CreateDevTriangleArrowMesh(dev_context* DevContext, u16 CircleSampleCount, f32 Radius, f32 Height, f32 ArrowRadius, f32 ArrowHeight)
{
    mesh_generation_result BodyResult = GenerateTriangleCylinder(GetDefaultArena(), Radius, Height, CircleSampleCount);
    mesh_generation_result ArrowResult = GenerateTriangleCone(GetDefaultArena(), ArrowRadius, ArrowHeight, CircleSampleCount, V3(0.0f, 0.0f, Height));
    
    mesh_generation_result MeshGenerationResult = AllocateMeshGenerationResult(GetDefaultArena(), BodyResult.VertexCount+ArrowResult.VertexCount, 
                                                                               BodyResult.IndexCount+ArrowResult.IndexCount);
    
    ptr BodyResultVerticesSize = sizeof(vertex_p3)*BodyResult.VertexCount;
    ptr BodyResultIndicesSize = sizeof(u16)*BodyResult.IndexCount;
    CopyMemory(MeshGenerationResult.Vertices, BodyResult.Vertices, BodyResultVerticesSize);
    CopyMemory(MeshGenerationResult.Indices, BodyResult.Indices, BodyResultIndicesSize);
    CopyMemory((u8*)MeshGenerationResult.Vertices+BodyResultVerticesSize, ArrowResult.Vertices, sizeof(vertex_p3)*ArrowResult.VertexCount);
    CopyMemory((u8*)MeshGenerationResult.Indices+BodyResultIndicesSize, ArrowResult.Indices, sizeof(u16)*ArrowResult.IndexCount);
    
    OffsetIndices(MeshGenerationResult.Indices+BodyResult.IndexCount, SafeU16(ArrowResult.IndexCount), SafeU16(BodyResult.VertexCount));
    
    DevContext->TriangleArrowMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleArrowMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}


void DrawQuad(dev_context* DevContext, v3f CenterP, v3f Normal, v2f Dim, c4 Color)
{
    v3f X, Y;
    CreateBasis(Normal, &X, &Y);
    
    v2f HalfDim = Dim*0.5f;
    
    v3f P[4] = 
    {
        CenterP - X*HalfDim.x - Y*HalfDim.y,
        CenterP + X*HalfDim.x - Y*HalfDim.y,
        CenterP + X*HalfDim.x + Y*HalfDim.y,
        CenterP - X*HalfDim.x + Y*HalfDim.y
    };
    
    PushDrawQuad(DevContext->Graphics, P[0], P[1], P[2], P[3], Color);    
}

void DrawOrientedBox(dev_context* DevContext, v3f P, v3f Dim, v3f XAxis, v3f YAxis, v3f ZAxis, c4 Color)
{
    m4 Model = TransformM4(P, XAxis, YAxis, ZAxis, Dim);
    PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleBoxMesh.MeshID, Model, Color, DevContext->TriangleBoxMesh.IndexCount, 0, 0);
}

void DrawBox(dev_context* DevContext, v3f P, v3f Dim, c4 Color)
{
    DrawOrientedBox(DevContext, P, Dim, Global_WorldXAxis, Global_WorldYAxis, Global_WorldZAxis, Color);
}

void DrawPoint(dev_context* DevContext, v3f P, c4 Color)
{
    DrawBox(DevContext, P, V3(0.025f), Color);
}

void DrawSphere(dev_context* DevContext, v3f CenterP, f32 Radius, c4 Color)
{
    m4 Model = TransformM4(CenterP, V3(Radius, Radius, Radius));
    PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleSphereMesh.MeshID, Model, Color, DevContext->TriangleSphereMesh.IndexCount, 0, 0);    
}

void DrawLineBox(dev_context* DevContext, v3f P, v3f Dim, c4 Color)
{    
    m4 Model = TransformM4(P, Dim);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineBoxMesh.MeshID, Model, Color, DevContext->LineBoxMesh.IndexCount, 0, 0);        
}

void DrawLineBoxMinMax(dev_context* DevContext, v3f Min, v3f Max, c4 Color)
{   
    v3f Dim = Max-Min;
    v3f P = V3(Min.xy + Dim.xy*0.5f, Min.z);
    DrawLineBox(DevContext, P, Dim, Color);    
}

void DrawLineEllipsoid(dev_context* DevContext, v3f CenterP, v3f Radius, c4 Color)
{
    m4 Model = TransformM4(CenterP, Radius);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineSphereMesh.MeshID, Model, Color, DevContext->LineSphereMesh.IndexCount, 0, 0); 
}

void DrawLineEllipsoid(dev_context* DevContext, ellipsoid3D Ellipsoid, c4 Color)
{
    DrawLineEllipsoid(DevContext, Ellipsoid.CenterP, Ellipsoid.Radius, Color);
}

void DrawCylinder(dev_context* DevContext, v3f Position, v3f Axis, f32 Radius, c4 Color)
{
    v3f X, Y;
    CreateBasis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    m4 Model = TransformM4(Position, X, Y, Axis);
    PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleCylinderMesh.MeshID, Model, Color, DevContext->TriangleCylinderMesh.IndexCount, 0, 0);
}

void DrawCone(dev_context* DevContext, v3f Position, v3f Axis, f32 Radius, c4 Color)
{
    v3f X, Y;
    CreateBasis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    m4 Model = TransformM4(Position, X, Y, Axis);
    PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleConeMesh.MeshID, Model, Color, DevContext->TriangleConeMesh.IndexCount, 0, 0);
}

void DrawFrame(dev_context* DevContext, v3f Position, v3f XAxis = Global_WorldXAxis, v3f YAxis = Global_WorldYAxis, v3f ZAxis = Global_WorldZAxis)
{        
    
    {
        v3f X, Y, Z;
        Z = XAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, Red(), DevContext->TriangleArrowMesh.IndexCount, 0, 0);    
    }
    
    {
        v3f X, Y, Z;
        Z = YAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, Green(), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    {
        v3f X, Y, Z;
        Z = ZAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawFilledMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, Blue(), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    DrawSphere(DevContext, Position, 0.04f, White());    
}

inline camera* 
GetProperCamera(dev_context* DevContext, world* World)
{
    camera* Camera = &World->Camera;
    if(DevContext->UseDevCamera)
        Camera = DevContext->Cameras + World->WorldIndex;
    return Camera;
}

void DevelopmentImGui(dev_context* DevContext)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    ImGui::NewFrame();
    
    //IMPORTANT(EVERYONE): If you need help figuring out how to use IMGUI you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start
#if 0 
    local bool demo_window;
    ImGui::ShowDemoWindow(&demo_window);
#endif
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((f32)Graphics->RenderDim.x/3.0f, (f32)Graphics->RenderDim.y));    
    
    local bool open = true;
    ImGui::Begin("Developer Tools", &open, ImGuiWindowFlags_NoCollapse);    
    
    ImGui::Text("FPS: %f", 1.0f/Game->dt);   
    ImGui::Text("DebugMessage: %s", DevContext->DebugMessage);       
    
    if(ImGui::Checkbox("Debug Camera", (bool*)&DevContext->UseDevCamera))
    {
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {                    
            camera* Camera = &Game->Worlds[WorldIndex].Camera;
            camera* DevCamera = &DevContext->Cameras[WorldIndex];
            
            DevCamera->Position = Camera->Position;
            DevCamera->AngularVelocity = {};
            DevCamera->Orientation = IdentityM3();
            DevCamera->FocalPoint = Camera->FocalPoint;
            DevCamera->Distance = Magnitude(DevCamera->FocalPoint-DevCamera->Position);
        }
    }    
    
    //const char* ShadingTypes[] = {"Normal Shading", "Wireframe Shading", "Wireframe on Normal Shading"};
    
    const char* ShadingTypes[] = {"Normal Shading", "Wireframe Shading"};
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Shading Style");
    ImGui::SameLine();
    ImGui::Combo("", (int*)&DevContext->ShadingType, ShadingTypes, ARRAYCOUNT(ShadingTypes));
   
    DevelopmentFrameRecording(DevContext);
    
    ImGui::Checkbox("Mute", (bool*)&Game->AudioOutput->Mute);    
    ImGui::Checkbox("Draw Other World", (bool*)&DevContext->DrawOtherWorld);        
    ImGui::Checkbox("Draw Frames", (bool*)&DevContext->DrawFrames);
    ImGui::Checkbox("Inspect Objects", (bool*)&DevContext->SelectObjects);  
    
    if(ImGui::CollapsingHeader("Game Information"))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        
        world_entity* PlayerEntity0 = GetPlayerEntity(&Game->Worlds[0]);
        world_entity* PlayerEntity1 = GetPlayerEntity(&Game->Worlds[1]);
        
        v3f PlayerPosition0 = PlayerEntity0->Position;
        v3f PlayerPosition1 = PlayerEntity1->Position;
        
        v3f PlayerVelocity0 = PlayerEntity0->Velocity;
        v3f PlayerVelocity1 = PlayerEntity1->Velocity;
        
        v3f CollisionNormal = PlayerEntity0->CollidedNormal;
        
        ImGui::Text("Player 0 Position: (%.2f, %.2f, %.2f)", PlayerPosition0.x, PlayerPosition0.y, PlayerPosition0.z);
        ImGui::Text("Player 0 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity0.x, PlayerVelocity0.y, PlayerVelocity0.z);
        ImGui::Text("Collided Normal: (%.2f, %.2f, %.2f)", CollisionNormal.x, CollisionNormal.y, CollisionNormal.z);
        
        ImGui::Text("Player 1 Position: (%.2f, %.2f, %.2f)", PlayerPosition1.x, PlayerPosition1.y, PlayerPosition1.z);
        ImGui::Text("Player 1 Velocity: (%.2f, %.2f, %.2f)", PlayerVelocity1.x, PlayerVelocity1.y, PlayerVelocity1.z);
        
        ImGui::Text("Movement Time Max Iterations: %I64u", GameInformation->MaxTimeIterations);
        ImGui::Text("GJK Max Iterations: %I64u", GameInformation->MaxGJKIterations);
    }

    if(ImGui::CollapsingHeader("SelectedObject"))
    {
        game_information* GameInformation = &DevContext->GameInformation;
        if(DevContext->SelectedObject != nullptr)
        {
            v3f ObjectPosition = DevContext->SelectedObject->Position;
            v3f ObjectVelocity = DevContext->SelectedObject->Velocity;
            
            ImGui::Text("Selected Object Position: (%.2f, %.2f, %.2f)", ObjectPosition.x, ObjectPosition.y, ObjectPosition.z);
            ImGui::Text("Selected Object Velocity: (%.2f, %.2f, %.2f)", ObjectVelocity.x, ObjectVelocity.y, ObjectVelocity.z);
            ImGui::Text("Selected Object Type: (%d)", DevContext->SelectedObject->Type);
            ImGui::Text("Selected Object ID: (%d)", DevContext->SelectedObject->ID.ID);
            ImGui::Text("Selected Object WorldIndex: (%d)", DevContext->SelectedObject->ID.WorldIndex);
            if(IsInvalidEntityID(DevContext->SelectedObject->LinkID))
            {
                ImGui::Text("Selected Object Link ID: (%d)", DevContext->SelectedObject->LinkID.ID);
                ImGui::Text("Selected Object Link WorldIndex: (%d)", DevContext->SelectedObject->LinkID.WorldIndex);
            }
            else
            {
                ImGui::Text("Selected Object Link ID: (%s)", "No Linked Entity");
                ImGui::Text("Selected Object Link WorldIndex: (%s)", "No Linked Entity");
            }
            ImGui::Text("RayCast Direction: (%.2f, %.2f, %.2f)", DevContext->InspectRay.x, DevContext->InspectRay.y, DevContext->InspectRay.z);
            
        }
        else
        {
            ImGui::Text("No object selected");
            ImGui::Text("RayCast Direction: (%.2f, %.2f, %.2f)", DevContext->InspectRay.x, DevContext->InspectRay.y, DevContext->InspectRay.z);
        }
    }
    
    ImGui::End();    
    
    ImGui::Render();        
}

void DevelopmentRenderWorld(dev_context* DevContext, world* World, camera* Camera)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    PushDepth(Graphics, true);    
    
    PushWorldCommands(Graphics, World, Camera);                    
    PushDepth(Graphics, false);    
    world_entity* PlayerEntity = GetPlayerEntity(World);
    player* Player = (player*)PlayerEntity->UserData;    
    
    c4 PlayerColor = Blue();    
    if(World == &Game->Worlds[1])
        PlayerColor = Red();    
    
    ellipsoid3D Ellipsoid = GetPlayerEllipsoid(Game, Player);
    DrawLineEllipsoid(DevContext, Ellipsoid, PlayerColor);
    
    for(u32 PointIndex = 0; PointIndex < DevContext->DebugPoints.Size; PointIndex++)
    {
        debug_point* Point = DevContext->DebugPoints + PointIndex;
        DrawPoint(DevContext, Point->P, Point->Color);
    }
    DevContext->DebugPoints.Size = 0;    
    
    for(u32 EdgeIndex = 0; EdgeIndex < DevContext->DebugEdges.Size; EdgeIndex++)
    {
        debug_edges* DebugEdge = DevContext->DebugEdges + EdgeIndex++;
        
        v3f Edge = DebugEdge->P1 - DebugEdge->P0;
        f32 EdgeLength = Magnitude(Edge);
        
        v3f Z = Edge/EdgeLength;
        v3f X, Y;
        CreateBasis(Z, &X, &Y);
        
        DrawOrientedBox(DevContext, DebugEdge->P0, V3(0.025f, 0.025f, EdgeLength), X, Y, Z, DebugEdge->Color);        
    }    
    DevContext->DebugEdges.Size = 0;
    
    for(u32 DirectionVectorIndex = 0; DirectionVectorIndex < DevContext->DebugDirectionVectors.Size; DirectionVectorIndex++)
    {
        debug_direction_vector* DirectionVector = DevContext->DebugDirectionVectors + DirectionVectorIndex;
        v3f X, Y;
        CreateBasis(DirectionVector->Direction, &X, &Y);        
        DrawOrientedBox(DevContext, DirectionVector->Origin, V3(0.025f, 0.025f, 1.0f), X, Y, DirectionVector->Direction, DirectionVector->Color);         
    }
    DevContext->DebugDirectionVectors.Size = 0;
    
    PushCull(Graphics, false);
    for(u32 QuadIndex = 0; QuadIndex < DevContext->DebugQuads.Size; QuadIndex++)
    {
        debug_quad* Quad = DevContext->DebugQuads + QuadIndex;
        DrawQuad(DevContext, Quad->CenterP, Quad->Normal, Quad->Dim, Quad->Color);
    }

    v3f alexX, alexY;
    CreateBasis(DevContext->InspectRay, &alexX, &alexY);       
    DrawOrientedBox(DevContext, Camera->Position + 0.2f * DevContext->InspectRay, V3(0.01f, 0.01f, 10.0f), alexX, alexY, DevContext->InspectRay, White()); 

    DevContext->DebugQuads.Size = 0;
    
    
    if(DevContext->DrawFrames)
    {
        FOR_EACH(Entity, &World->EntityPool)
        {
            m3 Orientation = ToMatrix3(Entity->Transform.Orientation);            
            DrawFrame(DevContext, Entity->Transform.Position, Orientation.XAxis, Orientation.YAxis, Orientation.ZAxis);
        }
    }
    
    PushCull(Graphics, true);
}

void DevelopmentUpdateCamera(dev_context* DevContext)
{    
    dev_input* Input = &DevContext->Input;
    game* Game = DevContext->Game;
    graphics* Graphics = DevContext->Graphics;
    
    world* World = GetCurrentWorld(Game);
    camera* Camera = &World->Camera;
    if(DevContext->UseDevCamera)
    {
        Camera = &DevContext->Cameras[Game->CurrentWorldIndex];
        
        if(IsDown(Input->Alt))
        {                                    
            if(IsDown(Input->LMB))
            {
                Camera->AngularVelocity.x += (Input->MouseDelta.y*Game->dt*CAMERA_ANGULAR_ACCELERATION);
                Camera->AngularVelocity.y += (Input->MouseDelta.x*Game->dt*CAMERA_ANGULAR_ACCELERATION);                                        
            }
            
            if(IsDown(Input->MMB))
            {
                Camera->Velocity.x += (Input->MouseDelta.x*Game->dt*CAMERA_LINEAR_ACCELERATION);
                Camera->Velocity.y += (Input->MouseDelta.y*Game->dt*CAMERA_LINEAR_ACCELERATION);                                        
            }
            
            if(Abs(Input->Scroll) > 0.0f)            
                Camera->Velocity.z -= Input->Scroll*Game->dt*CAMERA_SCROLL_ACCELERATION;                                            
        }            
        
        Camera->AngularVelocity *= (1.0f / (1.0f+Game->dt*CAMERA_ANGULAR_DAMPING));            
        v3f Eulers = (Camera->AngularVelocity*Game->dt);            
        
        quaternion Orientation = Normalize(RotQuat(Camera->Orientation.XAxis, Eulers.pitch)*RotQuat(Camera->Orientation.YAxis, Eulers.yaw));
        Camera->Orientation *= ToMatrix3(Orientation);
        
        Camera->Velocity.xy *= (1.0f /  (1.0f+Game->dt*CAMERA_LINEAR_DAMPING));            
        v2f Vel = Camera->Velocity.xy*Game->dt;
        v3f Delta = Vel.x*Camera->Orientation.XAxis - Vel.y*Camera->Orientation.YAxis;
        
        Camera->FocalPoint += Delta;
        Camera->Position += Delta;
        
        Camera->Velocity.z *= (1.0f/ (1.0f+Game->dt*CAMERA_SCROLL_DAMPING));            
        Camera->Distance += Camera->Velocity.z*Game->dt;            
        
        if(Camera->Distance < CAMERA_MIN_DISTANCE)
            Camera->Distance = CAMERA_MIN_DISTANCE;
        
        Camera->Position = Camera->FocalPoint + (Camera->Orientation.ZAxis*Camera->Distance);
    }
    
    if(DevContext->SelectObjects)
    {
        if(!IsDown(Input->Alt))
        {
            if(IsPressed(Input->LMB))
            {
                //For not just getting the player entity. need to change to cast the ray and get the intersected object
                i32 Height = Graphics->RenderDim.height;
                i32 Width = Graphics->RenderDim.width;
                f32 x = (2.0f * Input->MouseCoordinates.x) / Width - 1.0f;
                f32 y = 1.0f - (2.0f * Input->MouseCoordinates.y) / Height;
                f32 z = 1.0f;
                v3f ray_nds = V3(x, y, z);
                v4f ray_clip = V4(ray_nds.xy, -1.0f, 1.0f);
                m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
                v4f ray_eye =  ray_clip * Inverse(Perspective);
                ray_eye = V4(ray_eye.xy, -1.0, 0.0);
                v3f ray_wor =  (ray_eye * TransformM4(Camera->Position, Camera->Orientation)).xyz;
                ray_wor = Normalize(ray_wor);
                

                f32 t = INFINITY;
                pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);
                for(world_entity* Entity = GetFirst(&Iter); Entity; Entity = GetNext(&Iter))
                {
                    f32 tempt = INFINITY, tempu = INFINITY, tempv = INFINITY;
                    if(Entity->Mesh)
                    {
                        if(IsRayIntersectingEntity(Camera->Position, ray_wor, Entity, &tempt, &tempu, &tempv))
                        {
                            if(tempt < t)
                            {
                                t = tempt;
                                DevContext->SelectedObject = Entity;
                            }
                        }
                    }
                }
                DevContext->InspectRay = ray_wor;
            }
            if(IsPressed(Input->MMB))
            {
                DevContext->SelectedObject = nullptr;
            }
        }    
    }    
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    PushClearColorAndDepth(Graphics, Black(), 1.0f);    
    ///////////////////
    
    switch(DevContext->ShadingType)
    {
        case SHADING_TYPE_NORMAL:
        {
            PushWireframe(Graphics, false);
            DevelopmentRenderWorld(DevContext,World, Camera);
        } break;
        
        case SHADING_TYPE_WIREFRAME:
        {
            PushWireframe(Graphics, true);
            DevelopmentRenderWorld(DevContext, World, Camera);
            PushWireframe(Graphics, false);
        } break;                
        
        //TODO(JJ): Support this case
        INVALID_CASE(SHADING_TYPE_WIREFRAME_ON_NORMAL);
        INVALID_DEFAULT_CASE;        
    }
    
    i32 OtherWidth = Graphics->RenderDim.width/4;
    i32 OtherHeight = Graphics->RenderDim.height/4;
    
    block_puzzle* Puzzle = &Game->TestPuzzle;
    
    for(u32 GoalIndex = 0; GoalIndex < Puzzle->GoalRectCount; GoalIndex++)
    {
        goal_rect* GoalRect = Puzzle->GoalRects + GoalIndex;
        
        c4 Color = Blue();
        if(GoalRect->GoalIsMet)
            Color = Red();
        
        DrawLineBoxMinMax(DevContext, GoalRect->Rect.Min, GoalRect->Rect.Max, Color);
    }     

    if(DevContext->DrawOtherWorld)
    {        
        PushViewportAndScissor(Graphics, Graphics->RenderDim.width-OtherWidth, Graphics->RenderDim.height-OtherHeight, 
                               OtherWidth, OtherHeight);
        
        world* OtherWorld = GetNotCurrentWorld(Game);
        camera* OtherCamera = &OtherWorld->Camera;
        if(DevContext->UseDevCamera)
            OtherCamera = &DevContext->Cameras[!Game->CurrentWorldIndex];
        DevelopmentRenderWorld(DevContext, OtherWorld, OtherCamera);
    }   
}

void DevelopmentRenderImGui(dev_context* DevContext)
{
    graphics* Graphics = DevContext->Graphics;
    
    PushCull(Graphics, false);
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    m4 Orthographic = OrthographicM4(0, (f32)Graphics->RenderDim.width, 0, (f32)Graphics->RenderDim.height, -1.0f, 1.0f);
    PushProjection(Graphics, Orthographic);
    
    ptr IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = ImGui::GetDrawData();        
    
    u32 LastImGuiMeshCount = DevContext->ImGuiMeshCount;
    DevContext->ImGuiMeshCount = DrawData->CmdListsCount;
    ASSERT(DevContext->ImGuiMeshCount <= MAX_IMGUI_MESHES);        
    
    for(i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(DevContext->ImGuiMeshes[CmdListIndex] == -1)
            DevContext->ImGuiMeshes[CmdListIndex] = Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT);
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(Graphics, DevContext->ImGuiMeshes[CmdListIndex], 
                                 CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);                
        
        for(i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            ASSERT(!Cmd->UserCallback);
            
            ImVec4 ClipRect = Cmd->ClipRect;
            if((ClipRect.x < Graphics->RenderDim.width) && (ClipRect.y < Graphics->RenderDim.height) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                i32 X = (i32)ClipRect.x;
                i32 Y = (i32)(Graphics->RenderDim.height-ClipRect.w);
                i32 Width = (i32)(ClipRect.z - ClipRect.x);
                i32 Height = (i32)(ClipRect.w - ClipRect.y);
                
                PushScissor(Graphics, X, Y, Width, Height);
                
                i64 TextureID = (i64)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, DevContext->ImGuiMeshes[CmdListIndex], TextureID, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
            }
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, true);
    PushDepth(Graphics, true);            
}

void DevelopmentRender(dev_context* DevContext)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGui(DevContext);    
    DevelopmentUpdateCamera(DevContext);
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    
    PushClearColorAndDepth(Graphics, Black(), 1.0f);    
    ///////////////////
    
    world* World = GetCurrentWorld(Game);
    camera* Camera = GetProperCamera(DevContext, World);
    switch(DevContext->ShadingType)
    {
        case SHADING_TYPE_NORMAL:
        {
            PushWireframe(Graphics, false);
            DevelopmentRenderWorld(DevContext, World, Camera);
        } break;
        
        case SHADING_TYPE_WIREFRAME:
        {
            PushWireframe(Graphics, true);
            DevelopmentRenderWorld(DevContext, World, Camera);
            PushWireframe(Graphics, false);
        } break;                
        
        //TODO(JJ): Support this case
        INVALID_CASE(SHADING_TYPE_WIREFRAME_ON_NORMAL);
        INVALID_DEFAULT_CASE;        
    }
    
    i32 OtherWidth = Graphics->RenderDim.width/4;
    i32 OtherHeight = Graphics->RenderDim.height/4;
    
    if(DevContext->DrawOtherWorld)
    {        
        PushViewportAndScissor(Graphics, Graphics->RenderDim.width-OtherWidth, Graphics->RenderDim.height-OtherHeight, 
                               OtherWidth, OtherHeight);
        
        world* OtherWorld = GetNotCurrentWorld(Game);
        camera* OtherCamera = GetProperCamera(DevContext, OtherWorld);        
        DevelopmentRenderWorld(DevContext, OtherWorld, OtherCamera);
    }
    
    DevelopmentRenderImGui(DevContext);
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics)
{
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = CreateArena(KILOBYTE(32));                               
        DevContext->FrameRecording.RecordingPath = AllocateStringStorage(&DevContext->DevStorage, 8092);
        DevContext->FrameRecording.RecordedFrames = CreateDynamicArray<frame>(1024);        
        
        Platform_InitImGui(DevContext->PlatformData);                
        AllocateImGuiFont(Graphics);                
        
        CreateDevLineBoxMesh(DevContext);
        CreateDevLineSphereMesh(DevContext, 60);
        CreateDevTriangleBoxMesh(DevContext);
        CreateDevTriangleSphereMesh(DevContext);
        CreateDevTriangleCylinderMesh(DevContext, 60);
        CreateDevTriangleConeMesh(DevContext, 60);
        CreateDevTriangleArrowMesh(DevContext, 60, 0.02f, 0.85f, 0.035f, 0.15f);
        
        SetMemoryI64(DevContext->ImGuiMeshes, -1, sizeof(DevContext->ImGuiMeshes));
        
        ImGuiIO* IO = &ImGui::GetIO();
        IO->BackendRendererName = "OpenGL";
        IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData, Graphics->RenderDim, Game->dt, DevContext);        
        DevelopmentRender(DevContext);        
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}

#include "dev_frame_recording.cpp"