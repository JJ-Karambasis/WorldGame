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

void CreateDevLineCapsuleMesh(dev_context* DevContext, f32 Radius, u16 CircleSampleCount)
{    
    mesh_generation_result CapResult = GenerateLineHemisphere(GetDefaultArena(), Radius, CircleSampleCount);    
    mesh_generation_result BodyResult = AllocateMeshGenerationResult(GetDefaultArena(), 8, 8);
    
    BodyResult.Vertices[0] = {V3( 1.0f*Radius,  0.0f, -0.5f)};
    BodyResult.Vertices[1] = {V3( 1.0f*Radius,  0.0f,  0.5f)};
    BodyResult.Vertices[2] = {V3( 0.0f,  1.0f*Radius, -0.5f)};
    BodyResult.Vertices[3] = {V3( 0.0f,  1.0f*Radius,  0.5f)};
    BodyResult.Vertices[4] = {V3(-1.0f*Radius,  0.0f, -0.5f)};
    BodyResult.Vertices[5] = {V3(-1.0f*Radius,  0.0f,  0.5f)};
    BodyResult.Vertices[6] = {V3( 0.0f, -1.0f*Radius, -0.5f)};
    BodyResult.Vertices[7] = {V3( 0.0f, -1.0f*Radius,  0.5f)};
    
    BodyResult.Indices[0] = 0;
    BodyResult.Indices[1] = 1;
    BodyResult.Indices[2] = 2;
    BodyResult.Indices[3] = 3;
    BodyResult.Indices[4] = 4;
    BodyResult.Indices[5] = 5;
    BodyResult.Indices[6] = 6;
    BodyResult.Indices[7] = 7;
    
    mesh_generation_result MeshGenerationResult = AllocateMeshGenerationResult(GetDefaultArena(), CapResult.VertexCount+BodyResult.VertexCount, 
                                                                               CapResult.IndexCount+BodyResult.IndexCount);
    
    ptr CapResultVerticesSize = sizeof(vertex_p3)*CapResult.VertexCount;
    ptr CapResultIndicesSize = sizeof(u16)*CapResult.IndexCount;
    CopyMemory(MeshGenerationResult.Vertices, CapResult.Vertices, CapResultVerticesSize);
    CopyMemory(MeshGenerationResult.Indices, CapResult.Indices, CapResultIndicesSize);
    CopyMemory((u8*)MeshGenerationResult.Vertices+CapResultVerticesSize, BodyResult.Vertices, sizeof(vertex_p3)*BodyResult.VertexCount);
    CopyMemory((u8*)MeshGenerationResult.Indices+CapResultIndicesSize, BodyResult.Indices, sizeof(u16)*BodyResult.IndexCount);
    
    DevContext->LineCapsuleMesh.CapIndexCount = CapResult.IndexCount;
    DevContext->LineCapsuleMesh.CapVertexCount = CapResult.VertexCount;
    DevContext->LineCapsuleMesh.BodyIndexCount = BodyResult.IndexCount;
    DevContext->LineCapsuleMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void DrawQuad(dev_context* DevContext, v3f CenterP, v3f Normal, v2f Dim, c4 Color)
{
    NOT_IMPLEMENTED;    
}

void DrawOrientedBox(dev_context* DevContext, v3f P, v3f Dim, v3f XAxis, v3f YAxis, v3f ZAxis, c3 Color)
{
    m4 Model = TransformM4(P, XAxis, YAxis, ZAxis, Dim);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleBoxMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleBoxMesh.IndexCount, 0, 0);
}

void DrawBox(dev_context* DevContext, v3f P, v3f Dim, c3 Color)
{
    DrawOrientedBox(DevContext, P, Dim, Global_WorldXAxis, Global_WorldYAxis, Global_WorldZAxis, Color);
}

void DrawPoint(dev_context* DevContext, v3f P, c3 Color)
{
    DrawBox(DevContext, P, V3(0.025f), Color);
}


void DrawEdge(dev_context* DevContext, v3f P0, v3f P1, c3 Color)
{
    v3f ZAxis = P1-P0;
    f32 ZLength = Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    v3f XAxis, YAxis;
    CreateBasis(ZAxis, &XAxis, &YAxis);
    DrawOrientedBox(DevContext, P0, V3(0.025f, 0.025f, ZLength), XAxis, YAxis, ZAxis, Color);
}

void DrawSphere(dev_context* DevContext, v3f CenterP, f32 Radius, c3 Color)
{
    m4 Model = TransformM4(CenterP, V3(Radius, Radius, Radius));
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleSphereMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleSphereMesh.IndexCount, 0, 0);    
}

void DrawLineBox(dev_context* DevContext, v3f P, v3f Dim, c3 Color)
{    
    m4 Model = TransformM4(P, Dim);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineBoxMesh.MeshID, Model, Color, DevContext->LineBoxMesh.IndexCount, 0, 0);        
}

void DrawLineBoxMinMax(dev_context* DevContext, v3f Min, v3f Max, c3 Color)
{   
    v3f Dim = Max-Min;
    v3f P = V3(Min.xy + Dim.xy*0.5f, Min.z);    
    DrawLineBox(DevContext, P, Dim, Color);    
}

void DrawLineCapsule(dev_context* DevContext, v3f P0, v3f P1, f32 Radius, c3 Color)
{
    v3f ZAxis = P0-P1;
    f32 ZScale = Magnitude(ZAxis);
    ZAxis /= ZScale;
    
    dev_capsule_mesh* Mesh = &DevContext->LineCapsuleMesh;
    
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, TransformM4(P0, CreateBasis(ZAxis), V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, TransformM4(P1, CreateBasis(-ZAxis), V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, TransformM4(P1 + (ZAxis*ZScale*0.5f), CreateBasis(ZAxis), V3(Radius, Radius, ZScale)),
                     Color, Mesh->BodyIndexCount, Mesh->CapIndexCount, Mesh->CapVertexCount);
    
}

void DrawLineEllipsoid(dev_context* DevContext, v3f CenterP, v3f Radius, c3 Color)
{
    m4 Model = TransformM4(CenterP, Radius);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineSphereMesh.MeshID, Model, Color, DevContext->LineSphereMesh.IndexCount, 0, 0); 
}

void DrawLineEllipsoid(dev_context* DevContext, ellipsoid3D Ellipsoid, c3 Color)
{
    DrawLineEllipsoid(DevContext, Ellipsoid.CenterP, Ellipsoid.Radius, Color);
}

void DrawCylinder(dev_context* DevContext, v3f Position, v3f Axis, f32 Radius, c3 Color)
{
    v3f X, Y;
    CreateBasis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    m4 Model = TransformM4(Position, X, Y, Axis);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleCylinderMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleCylinderMesh.IndexCount, 0, 0);
}

void DrawCone(dev_context* DevContext, v3f Position, v3f Axis, f32 Radius, c3 Color)
{
    v3f X, Y;
    CreateBasis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    m4 Model = TransformM4(Position, X, Y, Axis);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleConeMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleConeMesh.IndexCount, 0, 0);
}

void DrawFrame(dev_context* DevContext, v3f Position, v3f XAxis = Global_WorldXAxis, v3f YAxis = Global_WorldYAxis, v3f ZAxis = Global_WorldZAxis)
{        
    
    {
        v3f X, Y, Z;
        Z = XAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(Red3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);    
    }
    
    {
        v3f X, Y, Z;
        Z = YAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(Green3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    {
        v3f X, Y, Z;
        Z = ZAxis;
        CreateBasis(Z, &X, &Y);
        
        m4 Transform = TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(Blue3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    DrawSphere(DevContext, Position, 0.04f, White3());    
}

inline view_settings
GetViewSettings(dev_context* DevContext, world* World)
{    
    if(DevContext->UseDevCamera)
    {   
        camera* Camera = DevContext->Cameras + World->WorldIndex;
        
        view_settings Result = {};    
        Result.Position = Camera->Position;
        Result.Orientation = Camera->Orientation;
        Result.FieldOfView = CAMERA_FIELD_OF_VIEW;
        Result.ZNear = CAMERA_ZNEAR;
        Result.ZFar = CAMERA_ZFAR;
        return Result;
    }
    else
    {
        view_settings Result = GetViewSettings(&World->Camera);        
        return Result;        
    }        
}

void DevelopmentUpdateCamera(dev_context* DevContext)
{    
    dev_input* Input = &DevContext->Input;
    game* Game = DevContext->Game;
    graphics* Graphics = DevContext->Graphics;
    
    world* World = GetCurrentWorld(Game);    
    if(DevContext->UseDevCamera)
    {
        camera* Camera = DevContext->Cameras + Game->CurrentWorldIndex;
        
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
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);        
    FOR_EACH(Entity, &World->EntityPool)
    {
        if(Entity->Mesh)
            PushDrawUnlitMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, CreateDiffuseMaterialSlot(Cyan3()), Entity->Mesh->IndexCount, 0, 0);
    }
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushWireframe(Graphics, false);
}

void DrawWorld(dev_context* DevContext, graphics_render_buffer* RenderBuffer, world* World)
{            
    view_settings ViewSettings = GetViewSettings(DevContext, World);    
    
    PushRenderBufferViewportScissorAndView(DevContext->Graphics, RenderBuffer, &ViewSettings);
    PushClearColorAndDepth(DevContext->Graphics, Black4(), 1.0f);                
    
    switch(DevContext->ViewModeType)
    {
        case VIEW_MODE_TYPE_LIT:
        {                        
            PushWorldShadingCommands(DevContext->Graphics, RenderBuffer, World, &ViewSettings, DevContext->Game->Assets);                                                         
        } break;
        
        case VIEW_MODE_TYPE_UNLIT:        
        {                                    
            FOR_EACH(Entity, &World->EntityPool)
            {
                if(Entity->Mesh)                    
                {
                    graphics_material* Material = Entity->Material;                    
                    PushDrawUnlitMesh(DevContext->Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Material->Diffuse, Entity->Mesh->IndexCount, 0, 0);                                     
                }
            }
        } break;                
        
        case VIEW_MODE_TYPE_WIREFRAME:
        {                                    
            DrawWireframeWorld(DevContext->Graphics, World);            
        } break;
        
        case VIEW_MODE_TYPE_WIREFRAME_ON_LIT:
        {
            PushWorldShadingCommands(DevContext->Graphics, RenderBuffer, World, &ViewSettings, DevContext->Game->Assets);            
            DrawWireframeWorld(DevContext->Graphics, World);                        
        } break;
        
        INVALID_DEFAULT_CASE;        
    }
    
    PushDepth(DevContext->Graphics, false);
    
#if 1 
    if(DevContext->DrawColliders)
    {
        FOR_EACH(Entity, &World->EntityPool)
        {            
            dev_mesh Mesh = {};                        
            switch(Entity->CollisionVolume.Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere Sphere = TransformSphere(&Entity->CollisionVolume.Sphere, Entity->Transform);                    
                    DrawLineEllipsoid(DevContext, Sphere.CenterP, V3(Sphere.Radius, Sphere.Radius, Sphere.Radius), Blue3());
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule Capsule = TransformCapsule(&Entity->CollisionVolume.Capsule, Entity->Transform);
                    DrawLineCapsule(DevContext, Capsule.P0, Capsule.P1, Capsule.Radius, Blue3());
                    
                    
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    rigid_transform Transform = RigidTransform(Entity->CollisionVolume.Transform, Entity->Transform);
                    m4 Model = TransformM4(Transform);
                    PushDrawLineMesh(DevContext->Graphics, Entity->CollisionVolume.ConvexHull->GDIHandle, Model, Blue3(), 
                                     ConvexHullIndexCount(Entity->CollisionVolume.ConvexHull), 0, 0);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }                        
    }   
#endif
    PushDepth(DevContext->Graphics, true);
#if 0 
    if(DevContext->DrawPlayerCollisionVolume)
    {
        world_entity* PlayerEntity = GetPlayerEntity(World);
        DrawLineEllipsoid(DevContext, GetPlayerEllipsoid(DevContext->Game, (player*)PlayerEntity->UserData), Magenta3());
    }    
#endif
}

void DevelopmentRender(dev_context* DevContext)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    dev_input* Input = &DevContext->Input;
    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGuiUpdate(DevContext);    
    DevelopmentUpdateCamera(DevContext);
    
    world* World = GetCurrentWorld(Game);    
    
    view_settings ViewSettings = GetViewSettings(DevContext, World);
    
    DrawWorld(DevContext, Game->RenderBuffer, World);    
    if(DevContext->DrawOtherWorld)
    {
        DrawWorld(DevContext, DevContext->RenderBuffer, GetNotCurrentWorld(Game));                      
        PushRenderBufferViewportScissorAndView(Graphics, Game->RenderBuffer, &ViewSettings);        
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
                v3f ray_wor =  (ray_eye * TransformM4(ViewSettings.Position, ViewSettings.Orientation)).xyz;
                ray_wor = Normalize(ray_wor);
                
                
                f32 t = INFINITY;
                FOR_EACH(Entity, &World->EntityPool)
                {
                    f32 tempt = INFINITY, tempu = INFINITY, tempv = INFINITY;
                    if(Entity->Mesh)
                    {
                        if(IsRayIntersectingEntity(ViewSettings.Position, ray_wor, Entity, &tempt, &tempu, &tempv))
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
            DrawFrame(DevContext, Entity->Transform.Translation, Orientation.XAxis, Orientation.YAxis, Orientation.ZAxis);
        }        
    }
    PushDepth(Graphics, true);
    
    DevelopmentImGuiRender(DevContext);  
    
    PushScissor(Graphics, 0, 0, Game->RenderBuffer->Resolution.width, Game->RenderBuffer->Resolution.height);
    PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Game->RenderBuffer->Resolution);    
    
    if(DevContext->DrawOtherWorld)
    {
        v2i Offset = Game->RenderBuffer->Resolution - DevContext->RenderBuffer->Resolution;
        PushCopyToOutput(Graphics, DevContext->RenderBuffer, Offset, DevContext->RenderBuffer->Resolution);    
    }
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
        
        DevContext->RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim/5);
        
        CreateDevLineCapsuleMesh(DevContext, 1.0f, 60);
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
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData[0], Graphics->RenderDim, Game->dt, DevContext);        
        DevelopmentRender(DevContext);        
    }
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}

#include "dev_frame_recording.cpp"