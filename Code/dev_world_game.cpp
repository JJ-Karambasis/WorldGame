/* Original Author: Armand (JJ) Karambasis */
#include "dev_imgui.cpp"

graphics_mesh_id AllocateConvexHullMesh(graphics* Graphics, convex_hull* ConvexHull)
{
    v3f* Vertices = PushArray(ConvexHull->Header.VertexCount, v3f, Clear, 0);    
    u32 IndexCount = ConvexHullIndexCount(ConvexHull);
    u16* Indices = PushArray(IndexCount, u16, Clear, 0);
    
    for(u32 VertexIndex = 0; VertexIndex < ConvexHull->Header.VertexCount; VertexIndex++)
        Vertices[VertexIndex] = ConvexHull->Vertices[VertexIndex].V;
    
    u32 Index = 0;
    for(u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
    {
        half_face* Face = ConvexHull->Faces + FaceIndex;
        
        i32 Edge = Face->Edge;
        do
        {
            Indices[Index++] = (u16)ConvexHull->Edges[Edge].Vertex;
            Indices[Index++] = (u16)ConvexHull->Edges[ConvexHull->Edges[Edge].EdgePair].Vertex;
            Edge = ConvexHull->Edges[Edge].NextEdge;
        } while (Edge != Face->Edge);        
    }
    
    ASSERT(Index == IndexCount);
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Vertices, sizeof(v3f)*ConvexHull->Header.VertexCount, GRAPHICS_VERTEX_FORMAT_P3,
                                                     Indices, IndexCount*sizeof(u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

graphics_mesh_id AllocateMesh(graphics* Graphics, mesh_generation_result* Mesh)
{
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
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

void DrawGrid(dev_context* DevContext, int xLeftBound, int xRightBound, int yTopBound, int yBottomBound, c3 Color)
{
    for(int x = xLeftBound; x <= xRightBound; x++)
    {
        if(x != 0)
        {
            DrawEdge(DevContext, V3((float)x, (float)yTopBound, 0.0f), V3((float)x, (float)yBottomBound, 0.0f), Color);
        }
        else
        {
            DrawEdge(DevContext, V3((float)x, (float)yTopBound, 0.0f), V3((float)x, (float)yBottomBound, 0.0f), Green3());
        }
    }
    
    for(int y = yTopBound; y <= yBottomBound; y++)
    {
        if(y != 0)
        {
            DrawEdge(DevContext, V3((float)xLeftBound, (float)y, 0.0f), V3((float)xRightBound, (float)y, 0.0f), Color);
        }
        else
        {            
            DrawEdge(DevContext, V3((float)xLeftBound, (float)y, 0.0f), V3((float)xRightBound, (float)y, 0.0f), Red3());             
        }
    }
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
GetViewSettings(dev_context* DevContext, graphics_state* GraphicsState, u32 WorldIndex)
{    
    game* Game = DevContext->Game;
    if(DevContext->UseDevCamera)
    {   
        camera* Camera = DevContext->Cameras + WorldIndex;
        
        view_settings Result = {};    
        Result.Position = Camera->Position;
        Result.Orientation = Camera->Orientation;
        Result.FieldOfView = Game->CurrentCameras[WorldIndex].FieldOfView;
        Result.ZNear = Game->CurrentCameras[WorldIndex].ZNear;
        Result.ZFar = Game->CurrentCameras[WorldIndex].ZFar;
        return Result;
    }
    else
    {
        view_settings Result = GetViewSettings(&GraphicsState->Camera);        
        return Result;        
    }        
}

void DevelopmentUpdateCamera(dev_context* DevContext)
{    
    dev_input* Input = &DevContext->Input;
    game* Game = DevContext->Game;
    graphics* Graphics = DevContext->Graphics;
    
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
        
        quaternion Orientation = Normalize(RotQuat(Camera->Orientation.XAxis, Eulers.roll)*RotQuat(Camera->Orientation.YAxis, Eulers.pitch));
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

entity* GetSelectedObject(dev_context* DevContext, graphics_state* GraphicsState)
{
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    dev_input* Input = &DevContext->Input;
    
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, Game->CurrentWorldIndex);    
    
    //For not just getting the player entity. need to change to cast the ray and get the intersected object
    i32 Height = Graphics->RenderDim.height;
    i32 Width = Graphics->RenderDim.width;
    f32 x = (2.0f * Input->MouseCoordinates.x) / Width - 1.0f;
    f32 y = 1.0f - (2.0f * Input->MouseCoordinates.y) / Height;
    f32 z = 1.0f;
    v3f ray_nds = V3(x, y, z);
    v4f ray_clip = V4(ray_nds.xy, -1.0f, 1.0f);
    m4 Perspective = PerspectiveM4(ViewSettings.FieldOfView, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), ViewSettings.ZNear, ViewSettings.ZFar);
    v4f ray_eye =  ray_clip * Inverse(Perspective);
    ray_eye = V4(ray_eye.xy, -1.0, 0.0);
    v3f ray_wor =  (ray_eye * TransformM4(ViewSettings.Position, ViewSettings.Orientation)).xyz;
    ray_wor = Normalize(ray_wor);
    
    f32 tBest = INFINITY;
    entity* Result = NULL;
    
    FOR_EACH(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])
    {        
        if(Entity->MeshID != INVALID_MESH_ID)
        {   
            mesh_info* MeshInfo = GetMeshInfo(DevContext->Game->Assets, Entity->MeshID);
            mesh* Mesh = GetMesh(DevContext->Game->Assets, Entity->MeshID);
            if(!Mesh)
                Mesh = LoadMesh(DevContext->Game->Assets, Entity->MeshID);
            
            sqt Transform = *GetEntityTransform(Game, Entity->ID);
            ray_mesh_intersection_result IntersectionResult = RayMeshIntersection(ViewSettings.Position, ray_wor, Mesh, MeshInfo, Transform);
            if(IntersectionResult.FoundCollision)
            {
                if(tBest > IntersectionResult.t && IntersectionResult.t > ViewSettings.ZNear)
                {
                    tBest = IntersectionResult.t;
                    Result = Entity;
                }
            }            
        }
    }    
    DevContext->InspectRay = ray_wor;
    
    return Result;
}

void DrawWireframeWorld(game* Game, graphics* Graphics, u32 WorldIndex, assets* Assets)
{
    PushWireframe(Graphics, true);
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);        
    FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
    {
        if(Entity->MeshID != INVALID_MESH_ID)
        {
            graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, Entity->MeshID);
            
            sqt Transform = *GetEntityTransform(Game, Entity->ID);
            PushDrawUnlitMesh(Graphics, MeshHandle, Transform, CreateDiffuseMaterialSlot(Cyan3()), 
                              GetMeshIndexCount(Assets, Entity->MeshID), 0, 0);
        }
    }
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushWireframe(Graphics, false);
}

void DrawWorld(dev_context* DevContext, graphics_render_buffer* RenderBuffer, u32 WorldIndex, graphics_state* GraphicsState, f32 tRenderInterpolate)
{           
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, WorldIndex);    
    
    PushRenderBufferViewportScissorAndView(DevContext->Graphics, RenderBuffer, &ViewSettings);
    PushClearColorAndDepth(DevContext->Graphics, Black4(), 1.0f);                
    
    game* Game = DevContext->Game;
    assets* Assets = Game->Assets;
    
    switch(DevContext->ViewModeType)
    {
        case VIEW_MODE_TYPE_LIT:
        {                        
            PushWorldShadingCommands(DevContext->Graphics, RenderBuffer, &ViewSettings, Assets, GraphicsState->GraphicsObjects);                                                         
        } break;
        
        case VIEW_MODE_TYPE_UNLIT:        
        {                                    
            FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
            {
                if(Entity->MeshID != INVALID_MESH_ID)                    
                {
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, DevContext->Graphics, Entity->MeshID);                    
                    graphics_diffuse_material_slot Diffuse = ConvertToGraphicsDiffuse(Assets, DevContext->Graphics, Entity->Material.Diffuse);
                    
                    sqt Transform = *GetEntityTransform(Game, Entity->ID);
                    PushDrawUnlitMesh(DevContext->Graphics, MeshHandle, Transform, Diffuse, GetMeshIndexCount(Assets, Entity->MeshID), 0, 0);                                     
                }
            }
        } break;                
        
        case VIEW_MODE_TYPE_WIREFRAME:
        {                                    
            DrawWireframeWorld(Game, DevContext->Graphics, WorldIndex, Assets);            
        } break;
        
        case VIEW_MODE_TYPE_WIREFRAME_ON_LIT:
        {
            PushWorldShadingCommands(DevContext->Graphics, RenderBuffer, &ViewSettings, Assets, GraphicsState->GraphicsObjects);            
            DrawWireframeWorld(Game, DevContext->Graphics, WorldIndex, Assets);                        
        } break;
        
        INVALID_DEFAULT_CASE;        
    }
    
    PushDepth(DevContext->Graphics, false);
    
    if(DevContext->DrawColliders)
    {
        FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
        {   
            mesh_convex_hull_gdi* ConvexHullGDI = NULL; 
            if(Entity->MeshID != INVALID_MESH_ID)
            {
                ConvexHullGDI = DevContext->MeshConvexHulls + Entity->MeshID;
                
                if(ConvexHullGDI->Count == (u32)-1)
                {
                    mesh_info* MeshInfo = GetMeshInfo(Assets, Entity->MeshID);                
                    ConvexHullGDI->Count = MeshInfo->Header.ConvexHullCount;
                    
                    if(ConvexHullGDI->Count > 0)
                    {
                        ConvexHullGDI->Meshes = PushArray(&DevContext->DevStorage, ConvexHullGDI->Count, graphics_mesh_id, Clear, 0);
                        for(u32 ConvexHullIndex = 0; ConvexHullIndex < ConvexHullGDI->Count; ConvexHullIndex++)
                        {
                            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;                            
                            ConvexHullGDI->Meshes[ConvexHullIndex] = AllocateConvexHullMesh(DevContext->Graphics, ConvexHull);                            
                        }
                    }
                }                                
            }
            
            sqt OldState = *GetEntityTransformOld(Game, Entity->ID);
            sqt NewState = *GetEntityTransform(Game, Entity->ID);        
            
            //CONFIRM(JJ): Should we be doing runtime changes on an entities scale? Definitely not on rigid bodies,
            //but do other entities might want this? Keep this assertion just in case. Not sure if we even need to 
            //interpolate them either
            ASSERT(OldState.Scale == NewState.Scale);
            
            sqt InterpTransform;
            InterpTransform.Translation = Lerp(OldState.Translation, tRenderInterpolate, NewState.Translation);    
            InterpTransform.Orientation = Lerp(OldState.Orientation, tRenderInterpolate, NewState.Orientation);
            InterpTransform.Scale = NewState.Scale;
            
            u32 ConvexHullIndex = 0;
            
            simulation* Simulation = GetSimulation(Game, Entity->ID);
            sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
            FOR_EACH(Volume, SimEntity->CollisionVolumes)
            {                
                switch(Volume->Type)
                {
                    case COLLISION_VOLUME_TYPE_SPHERE:
                    {
                        sphere Sphere = TransformSphere(&Volume->Sphere, InterpTransform);                    
                        DrawLineEllipsoid(DevContext, Sphere.CenterP, V3(Sphere.Radius, Sphere.Radius, Sphere.Radius), Blue3());
                    } break;
                    
                    case COLLISION_VOLUME_TYPE_CAPSULE:
                    {
                        capsule Capsule = TransformCapsule(&Volume->Capsule, InterpTransform);
                        DrawLineCapsule(DevContext, Capsule.P0, Capsule.P1, Capsule.Radius, Blue3());                                                
                    } break;
                    
                    case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                    {                           
                        ASSERT(Entity->MeshID != INVALID_MESH_ID && ConvexHullGDI);
                        ASSERT(ConvexHullGDI->Count != 0);
                        
                        sqt Transform = ToParentCoordinates(Volume->ConvexHull->Header.Transform, InterpTransform);
                        m4 Model = TransformM4(Transform);
                        
                        PushDrawLineMesh(DevContext->Graphics, ConvexHullGDI->Meshes[ConvexHullIndex], Model, Blue3(), 
                                         ConvexHullIndexCount(Volume->ConvexHull), 0, 0);
                        
                        ConvexHullIndex++;
                    } break;
                    
                    INVALID_DEFAULT_CASE;
                }
            }                        
        }                        
    }   
    
    PushDepth(DevContext->Graphics, true);    
}

void DevelopmentRender(dev_context* DevContext, graphics_state* GraphicsState, f32 tRenderInterpolate)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    dev_input* Input = &DevContext->Input;
    
    if((Graphics->RenderDim.width <= 0) ||  (Graphics->RenderDim.height <= 0))
        return;
    
    DevelopmentImGuiUpdate(DevContext);    
    DevelopmentUpdateCamera(DevContext);
    
    UpdateRenderBuffer(&Game->RenderBuffer, Graphics, Graphics->RenderDim);    
    
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, Game->CurrentWorldIndex);    
    
    DrawWorld(DevContext, Game->RenderBuffer, Game->CurrentWorldIndex, GraphicsState, tRenderInterpolate);    
    if(DevContext->DrawOtherWorld)
    {
        graphics_state OtherGraphicsState = GetGraphicsState(Game, !Game->CurrentWorldIndex, tRenderInterpolate);        
        UpdateRenderBuffer(&DevContext->RenderBuffer, Graphics, Graphics->RenderDim/5);                           
        DrawWorld(DevContext, DevContext->RenderBuffer, !Game->CurrentWorldIndex, &OtherGraphicsState, tRenderInterpolate);
        PushRenderBufferViewportScissorAndView(Graphics, Game->RenderBuffer, &ViewSettings);        
    }    
    
    if(!IsDown(Input->Alt) && !ImGui::GetIO().WantCaptureMouse)
    {
        if(IsPressed(Input->LMB))
        {
            DevContext->SelectedObject = GetSelectedObject(DevContext, GraphicsState);                
        }
        if(IsPressed(Input->MMB))
        {
            DevContext->SelectedObject = nullptr;
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
    
    if(DevContext->SelectedObject != nullptr)
    {                
        v3f Position = Lerp(GetEntityPositionOld(Game, DevContext->SelectedObject->ID), 
                            tRenderInterpolate, 
                            GetEntityPosition(Game, DevContext->SelectedObject->ID));
        DrawFrame(DevContext, Position, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1));
    }
    
    PushDepth(Graphics, true);
    
    if(DevContext->DrawGrid)
    {
        m4 Perspective = PerspectiveM4(ViewSettings.FieldOfView, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), ViewSettings.ZNear, ViewSettings.ZFar);
        v3f FrustumCorners[8];
        GetFrustumCorners(FrustumCorners, Perspective);
        TransformPoints(FrustumCorners, 8, TransformM4(ViewSettings.Position, ViewSettings.Orientation));
        
        f32 MinX = FrustumCorners[0].x;
        f32 MaxX = FrustumCorners[0].x;
        f32 MinY = FrustumCorners[0].y;
        f32 MaxY = FrustumCorners[0].y;
        for(int i = 0; i < 8; i++)
        {
            MinX = MinimumF32(FrustumCorners[i].x, MinX);                                             
            MaxX = MaximumF32(FrustumCorners[i].x, MaxX);
            MinY = MinimumF32(FrustumCorners[i].y, MinY);
            MaxY = MaximumF32(FrustumCorners[i].y, MaxY);            
        }
        
        DrawGrid(DevContext, Floor(MinX), Ceil(MaxX), Floor(MinY), Ceil(MaxY), RGB(0.1f, 0.1f, 0.1f));        
    }
    
    DevelopmentImGuiRender(DevContext);  
    
    PushScissor(Graphics, 0, 0, Game->RenderBuffer->Resolution.width, Game->RenderBuffer->Resolution.height);
    PushCopyToOutput(Graphics, Game->RenderBuffer, V2i(0, 0), Game->RenderBuffer->Resolution);    
    
    if(DevContext->DrawOtherWorld)
    {
        v2i Offset = Game->RenderBuffer->Resolution - DevContext->RenderBuffer->Resolution;
        PushCopyToOutput(Graphics, DevContext->RenderBuffer, Offset, DevContext->RenderBuffer->Resolution);    
    }
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics, graphics_state* GraphicsState, f32 tRenderInterpolate)
{
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = CreateArena(KILOBYTE(32));                               
        DevContext->LogStorage = CreateArena(MEGABYTE(1));
        
        DevContext->FramePlayback.MaxRecordingPathLength = 8092;
        DevContext->FramePlayback.RecordingPath = (char*)PushSize(&DevContext->DevStorage, (DevContext->FramePlayback.MaxRecordingPathLength+1)*sizeof(char), Clear, 0); 
        
        DevContext->RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim/5);
        //DevContext->DrawGrid = true;
        //DevContext->DrawColliders = true;      
        //DevContext->EditMode = true;  
        
        for(u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
            DevContext->MeshConvexHulls[MeshIndex].Count = (u32)-1;
        
        CreateDevLineCapsuleMesh(DevContext, 1.0f, 60);
        CreateDevLineBoxMesh(DevContext);
        CreateDevLineSphereMesh(DevContext, 60);
        CreateDevTriangleBoxMesh(DevContext);
        CreateDevTriangleSphereMesh(DevContext);
        CreateDevTriangleCylinderMesh(DevContext, 60);
        CreateDevTriangleConeMesh(DevContext, 60);
        CreateDevTriangleArrowMesh(DevContext, 60, 0.02f, 0.85f, 0.035f, 0.15f);
        
        DevContext->EntityRotations[0] = PushArray(&DevContext->DevStorage, Game->EntityStorage[0].Capacity, v3f, Clear, 0);
        DevContext->EntityRotations[1] = PushArray(&DevContext->DevStorage, Game->EntityStorage[1].Capacity, v3f, Clear, 0);
        
        for(u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            FOR_EACH(Entity, &Game->EntityStorage[WorldIndex])
            {
                sqt* Transform = GetEntityTransform(Game, Entity->ID);
                u32 PoolIndex = Game->EntityStorage[Entity->ID.WorldIndex].GetIndex(Entity->ID.ID);
                DevContext->EntityRotations[WorldIndex][PoolIndex] = QuaternionEuler(Transform->Orientation);
            }
        }
        
        DevelopmentImGuiInit(DevContext);        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData[0], Graphics->RenderDim, Game->dt, DevContext);        
        DevelopmentRender(DevContext, GraphicsState, tRenderInterpolate);        
    }
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(u32 ButtonIndex = 0; ButtonIndex < ARRAYCOUNT(Input->Buttons); ButtonIndex++)
        Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;    
}

void DevelopmentRecord(dev_context* DevContext)
{
    frame_playback* Playback = &DevContext->FramePlayback;
    
    if((Playback->PlaybackState == FRAME_PLAYBACK_STATE_PLAYING) ||
       (Playback->PlaybackState == FRAME_PLAYBACK_STATE_INSPECT_FRAMES))
    {
        Playback->Recording.PlayFrame(DevContext->Game, Playback->CurrentFrameIndex);        
        if(Playback->PlaybackState == FRAME_PLAYBACK_STATE_PLAYING)
        {
            Playback->CurrentFrameIndex++;
            if(Playback->CurrentFrameIndex == Playback->Recording.FrameCount)
                Playback->CurrentFrameIndex = 0;                        
        }
    }
    else if(Playback->PlaybackState == FRAME_PLAYBACK_STATE_RECORDING)
    {        
        Playback->Recording.RecordFrame(DevContext->Game);
    }        
}

#include "dev_frame_recording.cpp"