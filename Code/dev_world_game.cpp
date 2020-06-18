/* Original Author: Armand (JJ) Karambasis */
#include "dev_imgui.cpp"

i64 AllocateMesh(graphics* Graphics, mesh_generation_result* Mesh)
{
    i64 Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                        Mesh->Indices, Mesh->IndexCount*sizeof(u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
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
    NOT_IMPLEMENTED;    
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


void DrawEdge(dev_context* DevContext, v3f P0, v3f P1, c4 Color)
{
    v3f ZAxis = P1-P0;
    f32 ZLength = Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    v3f XAxis, YAxis;
    CreateBasis(ZAxis, &XAxis, &YAxis);
    DrawOrientedBox(DevContext, P0, V3(0.025f, 0.025f, ZLength), XAxis, YAxis, ZAxis, Color);
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

void DevelopmentUpdateCamera(dev_context* DevContext)
{    
    dev_input* Input = &DevContext->Input;
    game* Game = DevContext->Game;
    
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
}

void DrawWireframeWorld(graphics* Graphics, world* World)
{
    PushWireframe(Graphics, true);
    PushCull(Graphics, false);    
    FOR_EACH(Entity, &World->EntityPool)
    {
        if(Entity->Mesh)
            PushDrawFilledMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Cyan(), Entity->Mesh->IndexCount, 0, 0);
    }
    PushCull(Graphics, true);
    PushWireframe(Graphics, false);
}

void DevelopmentRender(dev_context* DevContext)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGuiUpdate(DevContext);    
    DevelopmentUpdateCamera(DevContext);
    
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);    
    PushClearColorAndDepth(Graphics, Black(), 1.0f);        
    PushDepth(Graphics, true);
    
    world* World = GetCurrentWorld(Game);
    camera* Camera = GetProperCamera(DevContext, World);
    
    PushCameraCommands(Graphics, Camera);
    switch(DevContext->ViewModeType)
    {
        case VIEW_MODE_TYPE_LIT:
        {                        
            PushWorldShadingCommands(Graphics, World);                                                         
        } break;
        
        case VIEW_MODE_TYPE_UNLIT:        
        {            
            FOR_EACH(Entity, &World->EntityPool)
            {
                if(Entity->Mesh)                    
                    PushDrawFilledMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, Entity->Mesh->IndexCount, 0, 0);                                     
            }
        } break;                
        
        case VIEW_MODE_TYPE_WIREFRAME:
        {
            DrawWireframeWorld(Graphics, World);            
        } break;
        
        case VIEW_MODE_TYPE_WIREFRAME_ON_LIT:
        {
            PushWorldShadingCommands(Graphics, World);
            DrawWireframeWorld(Graphics, World);            
        } break;
        
        INVALID_DEFAULT_CASE;        
    }
    
    if(DevContext->DrawPlayerCollisionVolume)
    {
        world_entity* PlayerEntity = GetPlayerEntity(World);
        DrawLineEllipsoid(DevContext, GetPlayerEllipsoid(Game, (player*)PlayerEntity->UserData), Magenta());
    }
    
    PushDepth(Graphics, false);    
    for(u32 PrimitiveIndex = 0; PrimitiveIndex < DevContext->DebugPrimitives.Size; PrimitiveIndex++)
    {
        debug_primitive* Primitive = DevContext->DebugPrimitives + PrimitiveIndex;
        switch(Primitive->Type)
        {
            case DEBUG_PRIMITIVE_TYPE_POINT:
            {
                DrawPoint(DevContext, Primitive->Point.P, Primitive->Point.Color);
            } break;
            
            case DEBUG_PRIMITIVE_TYPE_EDGE:
            {
                DrawEdge(DevContext, Primitive->Edge.P0, Primitive->Edge.P1, Primitive->Edge.Color);
            } break;
            
            case DEBUG_PRIMITIVE_TYPE_QUAD:
            {
                //DrawQuad(DevContext, Primitive->Quad.CenterP, Primitive->Quad.Normal, Primitive->Quad.Dim, Primitive->Quad.Color);                
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    DevContext->DebugPrimitives.Size = 0;
    
    if(DevContext->DrawFrames)
    {        
        FOR_EACH(Entity, &World->EntityPool)
        {
            m3 Orientation = ToMatrix3(Entity->Transform.Orientation);
            DrawFrame(DevContext, Entity->Transform.Position, Orientation.XAxis, Orientation.YAxis, Orientation.ZAxis);
        }        
    }
    PushDepth(Graphics, true);
    
    DevelopmentImGuiRender(DevContext);
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics)
{
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = CreateArena(KILOBYTE(32));                               
        DevContext->LogStorage = CreateArena(MEGABYTE(1));
        DevContext->FrameRecording.RecordingPath = AllocateStringStorage(&DevContext->DevStorage, 8092);
        DevContext->FrameRecording.RecordedFrames = CreateDynamicArray<frame>(1024);        
        
        CreateDevLineBoxMesh(DevContext);
        CreateDevLineSphereMesh(DevContext, 60);
        CreateDevTriangleBoxMesh(DevContext);
        CreateDevTriangleSphereMesh(DevContext);
        CreateDevTriangleCylinderMesh(DevContext, 60);
        CreateDevTriangleConeMesh(DevContext, 60);
        CreateDevTriangleArrowMesh(DevContext, 60, 0.02f, 0.85f, 0.035f, 0.15f);
        
        DevelopmentImGuiInit(DevContext);
        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData, Graphics->RenderDim, Game->dt);        
        DevelopmentRender(DevContext);        
    }
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}

#include "dev_frame_recording.cpp"