/* Original Author: Armand (JJ) Karambasis */
#include "dev_imgui.cpp"

graphics_mesh_id AllocateConvexHullMesh(graphics* Graphics, convex_hull* ConvexHull)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_v3f* Vertices = GlobalArena->PushArray<ak_v3f>(ConvexHull->Header.VertexCount);
    ak_u32 IndexCount = ConvexHullIndexCount(ConvexHull);
    ak_u16* Indices = GlobalArena->PushArray<ak_u16>(IndexCount);
    
    for(ak_u32 VertexIndex = 0; VertexIndex < ConvexHull->Header.VertexCount; VertexIndex++)
        Vertices[VertexIndex] = ConvexHull->Vertices[VertexIndex].V;
    
    ak_u32 Index = 0;
    for(ak_u32 FaceIndex = 0; FaceIndex < ConvexHull->Header.FaceCount; FaceIndex++)
    {
        half_face* Face = ConvexHull->Faces + FaceIndex;
        
        ak_i32 Edge = Face->Edge;
        do
        {
            Indices[Index++] = (ak_u16)ConvexHull->Edges[Edge].Vertex;
            Indices[Index++] = (ak_u16)ConvexHull->Edges[ConvexHull->Edges[Edge].EdgePair].Vertex;
            Edge = ConvexHull->Edges[Edge].NextEdge;
        } while (Edge != Face->Edge);        
    }
    
    AK_Assert(Index == IndexCount, "Failed to build convex hull indexes properly. This is a programming error");
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Vertices, sizeof(ak_v3f)*ConvexHull->Header.VertexCount, GRAPHICS_VERTEX_FORMAT_P3,
                                                     Indices, IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    GlobalArena->EndTemp(&TempArena);
    return Result;
}

graphics_mesh_id AllocateMesh(graphics* Graphics, ak_mesh_result* Mesh)
{
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(ak_vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                                     Mesh->Indices, Mesh->IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

void CreateDevLineSphereMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateLineSphere(GlobalArena, 1.0f, CircleSampleCount);        
    DevContext->LineSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineSphereMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleCircleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Height)
{
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCircle(DevContext->DevStorage, 1.0f, Height, CircleSampleCount);        
    DevContext->TriangleCircleMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCircleMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleCircleMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleCircleMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleCircleMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void CreateDevTriangleTorusMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Width)
{
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleTorus(DevContext->DevStorage, 1.0f, Width, CircleSampleCount);        
    DevContext->TriangleTorusMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleTorusMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleTorusMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleTorusMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleTorusMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void CreateDevLineBoxMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateLineBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));    
    DevContext->LineBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleBoxMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));       
    DevContext->TriangleBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleBoxMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleSphereMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleSphere(GlobalArena, 1.0f, 2);
    DevContext->TriangleSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleSphereMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleCylinderMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCylinder(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleCylinderMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCylinderMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleConeMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCone(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleConeMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleConeMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleArrowMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 ArrowRadius, ak_f32 ArrowHeight)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result ArrowResult = AK_GenerateTriangleCone(GlobalArena, ArrowRadius, ArrowHeight, CircleSampleCount, AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result MeshGenerationResult = AK_AllocateMeshResult(DevContext->DevStorage, BodyResult.VertexCount+ArrowResult.VertexCount, 
                                                                BodyResult.IndexCount+ArrowResult.IndexCount);    
    
    ak_uaddr BodyResultVerticesSize = sizeof(ak_vertex_p3)*BodyResult.VertexCount;
    ak_uaddr BodyResultIndicesSize = sizeof(ak_u16)*BodyResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, BodyResult.Vertices, BodyResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, BodyResult.Indices, BodyResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+BodyResultVerticesSize, ArrowResult.Vertices, sizeof(ak_vertex_p3)*ArrowResult.VertexCount);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Indices+BodyResultIndicesSize, ArrowResult.Indices, sizeof(ak_u16)*ArrowResult.IndexCount);
    
    AK_OffsetIndices(MeshGenerationResult.Indices+BodyResult.IndexCount, AK_SafeU16(ArrowResult.IndexCount), AK_SafeU16(BodyResult.VertexCount));
    
    DevContext->TriangleArrowMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleArrowMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleArrowMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleArrowMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleArrowMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevTriangleScaleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 CubeSize)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result BoxResult = AK_GenerateTriangleBox(GlobalArena, AK_V3f(CubeSize, CubeSize, CubeSize), AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result MeshGenerationResult = AK_AllocateMeshResult(DevContext->DevStorage, BodyResult.VertexCount+BoxResult.VertexCount, 
                                                                BodyResult.IndexCount+BoxResult.IndexCount);    
    
    ak_uaddr BodyResultVerticesSize = sizeof(ak_vertex_p3)*BodyResult.VertexCount;
    ak_uaddr BodyResultIndicesSize = sizeof(ak_u16)*BodyResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, BodyResult.Vertices, BodyResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, BodyResult.Indices, BodyResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+BodyResultVerticesSize, BoxResult.Vertices, sizeof(ak_vertex_p3)*BoxResult.VertexCount);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Indices+BodyResultIndicesSize, BoxResult.Indices, sizeof(ak_u16)*BoxResult.IndexCount);
    
    AK_OffsetIndices(MeshGenerationResult.Indices+BodyResult.IndexCount, AK_SafeU16(BoxResult.IndexCount), AK_SafeU16(BodyResult.VertexCount));
    
    DevContext->TriangleScaleMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleScaleMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleScaleMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleScaleMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleScaleMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void CreateDevLineCapsuleMesh(dev_context* DevContext, ak_f32 Radius, ak_u16 CircleSampleCount)
{   
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result CapResult = AK_GenerateLineHemisphere(GlobalArena, Radius, CircleSampleCount);    
    ak_mesh_result BodyResult = AK_AllocateMeshResult(GlobalArena, 8, 8);
    
    BodyResult.Vertices[0] = {AK_V3( 1.0f*Radius,  0.0f, -0.5f)};
    BodyResult.Vertices[1] = {AK_V3( 1.0f*Radius,  0.0f,  0.5f)};
    BodyResult.Vertices[2] = {AK_V3( 0.0f,  1.0f*Radius, -0.5f)};
    BodyResult.Vertices[3] = {AK_V3( 0.0f,  1.0f*Radius,  0.5f)};
    BodyResult.Vertices[4] = {AK_V3(-1.0f*Radius,  0.0f, -0.5f)};
    BodyResult.Vertices[5] = {AK_V3(-1.0f*Radius,  0.0f,  0.5f)};
    BodyResult.Vertices[6] = {AK_V3( 0.0f, -1.0f*Radius, -0.5f)};
    BodyResult.Vertices[7] = {AK_V3( 0.0f, -1.0f*Radius,  0.5f)};
    
    BodyResult.Indices[0] = 0;
    BodyResult.Indices[1] = 1;
    BodyResult.Indices[2] = 2;
    BodyResult.Indices[3] = 3;
    BodyResult.Indices[4] = 4;
    BodyResult.Indices[5] = 5;
    BodyResult.Indices[6] = 6;
    BodyResult.Indices[7] = 7;
    
    ak_mesh_result MeshGenerationResult = AK_AllocateMeshResult(GlobalArena, CapResult.VertexCount+BodyResult.VertexCount, 
                                                                CapResult.IndexCount+BodyResult.IndexCount);
    
    ak_uaddr CapResultVerticesSize = sizeof(ak_vertex_p3)*CapResult.VertexCount;
    ak_uaddr CapResultIndicesSize = sizeof(ak_u16)*CapResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, CapResult.Vertices, CapResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, CapResult.Indices, CapResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+CapResultVerticesSize, BodyResult.Vertices, sizeof(ak_vertex_p3)*BodyResult.VertexCount);
    CopyMemory((ak_u8*)MeshGenerationResult.Indices+CapResultIndicesSize, BodyResult.Indices, sizeof(ak_u16)*BodyResult.IndexCount);
    
    DevContext->LineCapsuleMesh.CapIndexCount = CapResult.IndexCount;
    DevContext->LineCapsuleMesh.CapVertexCount = CapResult.VertexCount;
    DevContext->LineCapsuleMesh.BodyIndexCount = BodyResult.IndexCount;
    DevContext->LineCapsuleMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}


void CreateDevPlaneMesh(dev_context* DevContext, ak_f32 Width, ak_f32 Height)
{         
    ak_mesh_result MeshGenerationResult = AK_AllocateMeshResult(DevContext->DevStorage, 4, 6);
    
    MeshGenerationResult.Vertices[0] = {AK_V3(-0.5f*Width,  0.5f*Height, 0.0f)}; //TopLeft
    MeshGenerationResult.Vertices[1] = {AK_V3( 0.5f*Width,  0.5f*Height, 0.0f)}; //TopRight
    MeshGenerationResult.Vertices[2] = {AK_V3(-0.5f*Width, -0.5f*Height, 0.0f)}; //BottomLeft
    MeshGenerationResult.Vertices[3] = {AK_V3( 0.5f*Width, -0.5f*Height, 0.0f)}; //BottomRight

    //BottomLeft, BottomRight, TopRight, TopLeft: Unwinding order
    MeshGenerationResult.Indices[0] = 2;
    MeshGenerationResult.Indices[1] = 3;
    MeshGenerationResult.Indices[2] = 1;
    MeshGenerationResult.Indices[3] = 1;
    MeshGenerationResult.Indices[4] = 0;
    MeshGenerationResult.Indices[5] = 2;
    
    DevContext->PlaneMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->PlaneMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->PlaneMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->PlaneMesh.Indices = MeshGenerationResult.Indices;
    DevContext->PlaneMesh.MeshID = AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void DrawQuad(dev_context* DevContext, ak_v3f CenterP, ak_v3f Normal, ak_v2f Dim, ak_color4f Color)
{
    AK_NotImplemented();
}

void DrawOrientedBox(dev_context* DevContext, ak_v3f P, ak_v3f Dim, ak_v3f XAxis, ak_v3f YAxis, ak_v3f ZAxis, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(P, AK_M3(XAxis, YAxis, ZAxis), Dim);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleBoxMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleBoxMesh.IndexCount, 0, 0);
}

void DrawBox(dev_context* DevContext, ak_v3f P, ak_v3f Dim, ak_color3f Color)
{
    DrawOrientedBox(DevContext, P, Dim, AK_XAxis(), AK_YAxis(), AK_ZAxis(), Color);
}

void DrawPoint(dev_context* DevContext, ak_v3f P, ak_color3f Color)
{
    ak_f32 PointSize = 0.025f;
    DrawBox(DevContext, P, AK_V3(PointSize, PointSize, PointSize), Color);
}


void DrawEdge(dev_context* DevContext, ak_v3f P0, ak_v3f P1, ak_color3f Color)
{
    ak_v3f ZAxis = P1-P0;
    ak_f32 ZLength = AK_Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    ak_v3f XAxis, YAxis;
    AK_Basis(ZAxis, &XAxis, &YAxis);
    DrawOrientedBox(DevContext, P0, AK_V3(0.025f, 0.025f, ZLength), XAxis, YAxis, ZAxis, Color);
}

void DrawGrid(dev_context* DevContext, ak_i32 xLeftBound, ak_i32 xRightBound, ak_i32 yTopBound, ak_i32 yBottomBound, ak_color3f Color)
{
    for(ak_f32 x = 0; x <= xRightBound; x+=DevContext->GridDistance)
    {
        if(x != 0)
        {
            DrawEdge(DevContext, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), Color);
        }
    }
    for(ak_f32 x = 0; x >= xLeftBound; x-=DevContext->GridDistance)
    {
        if(x != 0)
        {
            DrawEdge(DevContext, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), Color);
        }
    }

    for(ak_f32 y = 0; y <= yBottomBound; y+=DevContext->GridDistance)
    {
        if(y != 0)
        {
            DrawEdge(DevContext, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), Color);
        }
    }

    for(ak_f32 y = 0; y >= yTopBound; y-=DevContext->GridDistance)
    {
        if(y != 0)
        {
            DrawEdge(DevContext, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), Color);
        }
    }
    
    DrawEdge(DevContext, AK_V3f(0.0f, (ak_f32)yTopBound, 0.0f), AK_V3f(0.0f, (ak_f32)yBottomBound, 0.0f), AK_Green3());
    DrawEdge(DevContext, AK_V3f((ak_f32)xLeftBound, 0.0f, 0.0f), AK_V3f((ak_f32)xRightBound, 0.0f, 0.0f), AK_Red3());
}

void DrawSphere(dev_context* DevContext, ak_v3f CenterP, ak_f32 Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, AK_V3(Radius, Radius, Radius));
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleSphereMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleSphereMesh.IndexCount, 0, 0);    
}

void DrawLineBox(dev_context* DevContext, ak_v3f P, ak_v3f Dim, ak_color3f Color)
{    
    ak_m4f Model = AK_TransformM4(P, Dim);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineBoxMesh.MeshID, Model, Color, DevContext->LineBoxMesh.IndexCount, 0, 0);        
}

void DrawLineBoxMinMax(dev_context* DevContext, ak_v3f Min, ak_v3f Max, ak_color3f Color)
{   
    ak_v3f Dim = Max-Min;
    ak_v3f P = AK_V3(Min.xy + Dim.xy*0.5f, Min.z);    
    DrawLineBox(DevContext, P, Dim, Color);    
}

void DrawLineCapsule(dev_context* DevContext, ak_v3f P0, ak_v3f P1, ak_f32 Radius, ak_color3f Color)
{
    ak_v3f ZAxis = P0-P1;
    ak_f32 ZScale = AK_Magnitude(ZAxis);
    ZAxis /= ZScale;
    
    dev_capsule_mesh* Mesh = &DevContext->LineCapsuleMesh;
    
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, AK_TransformM4(P0, AK_Basis(ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, AK_TransformM4(P1, AK_Basis(-ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(DevContext->Graphics, Mesh->MeshID, AK_TransformM4(P1 + (ZAxis*ZScale*0.5f), AK_Basis(ZAxis), AK_V3(Radius, Radius, ZScale)),
                     Color, Mesh->BodyIndexCount, Mesh->CapIndexCount, Mesh->CapVertexCount);
    
}

void DrawLineEllipsoid(dev_context* DevContext, ak_v3f CenterP, ak_v3f Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, Radius);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineSphereMesh.MeshID, Model, Color, DevContext->LineSphereMesh.IndexCount, 0, 0); 
}

void DrawCylinder(dev_context* DevContext, ak_v3f Position, ak_v3f Axis, ak_f32 Radius, ak_color3f Color)
{
    ak_v3f X, Y;
    AK_Basis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    ak_m4f Model = AK_TransformM4(Position, X, Y, Axis);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleCylinderMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleCylinderMesh.IndexCount, 0, 0);
}

void DrawCone(dev_context* DevContext, ak_v3f Position, ak_v3f Axis, ak_f32 Radius, ak_color3f Color)
{
    ak_v3f X, Y;
    AK_Basis(Axis, &X, &Y);
    X *= Radius;
    Y *= Radius;
    
    ak_m4f Model = AK_TransformM4(Position, X, Y, Axis);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleConeMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleConeMesh.IndexCount, 0, 0);
}

void DrawFrame(dev_context* DevContext, ak_v3f Position, ak_v3f XAxis = AK_XAxis(), ak_v3f YAxis = AK_YAxis(), ak_v3f ZAxis = AK_ZAxis())
{        
    
    {
        ak_v3f X, Y, Z;
        Z = XAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Red3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);    
    }
    
    {
        ak_v3f X, Y, Z;
        Z = YAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Green3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    {
        ak_v3f X, Y, Z;
        Z = ZAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Blue3()), DevContext->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    DrawSphere(DevContext, Position, 0.04f, AK_White3());    
}

ak_v3f GetGizmoColor(gizmo Gizmo)
{
    switch(Gizmo.MovementDirection)
    {
        case GIZMO_MOVEMENT_DIRECTION_X:
        {
            return AK_Red3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_Y:
        {
            return AK_Green3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_Z:
        {
            return AK_Blue3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_XY:
        {
            return AK_Blue3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_XZ:
        {
            return AK_Green3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_YZ:
        {
            return AK_Red3();
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    return AK_White3();
}

void DrawGizmos(dev_context* DevContext, ak_v3f Position)
{        
    if(DevContext->TransformationMode != GIZMO_MOVEMENT_TYPE_ROTATE)
    {   
        for(ak_i32 i = 0; i < 3; i++)
        {
            gizmo CurrentGizmo = DevContext->Gizmo[i];
            ak_v3f Color = GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(DevContext->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(DevContext->Graphics, graphics_cull_mode::GRAPHICS_CULL_MODE_NONE);
        for(ak_i32 i = 3; i < 6; i++)
        {
            gizmo CurrentGizmo = DevContext->Gizmo[i];
            ak_v3f Color = GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(DevContext->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(DevContext->Graphics, graphics_cull_mode::GRAPHICS_CULL_MODE_FRONT);
    }
    else
    {
        PushCull(DevContext->Graphics, graphics_cull_mode::GRAPHICS_CULL_MODE_NONE);
        for(ak_i32 i = 0; i < 3; i++)
        {
            gizmo CurrentGizmo = DevContext->Gizmo[i];
            ak_v3f Color = GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(DevContext->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(DevContext->Graphics, graphics_cull_mode::GRAPHICS_CULL_MODE_FRONT);
    }
    
    
    DrawSphere(DevContext, Position, 0.04f, AK_White3());    
}


inline view_settings
GetViewSettings(dev_context* DevContext, graphics_state* GraphicsState, ak_u32 WorldIndex)
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
            
            if(AK_Abs(Input->Scroll) > 0.0f)            
                Camera->Velocity.z -= Input->Scroll*Game->dt*CAMERA_SCROLL_ACCELERATION;                                            
        }            
        
        Camera->AngularVelocity *= (1.0f / (1.0f+Game->dt*CAMERA_ANGULAR_DAMPING));            
        ak_v3f Eulers = (Camera->AngularVelocity*Game->dt);            
        
        ak_quatf Orientation = AK_Normalize(AK_RotQuat(Camera->Orientation.XAxis, Eulers.roll)*AK_RotQuat(Camera->Orientation.YAxis, Eulers.pitch));
        Camera->Orientation *= AK_QuatToMatrix(Orientation);
        
        Camera->Velocity.xy *= (1.0f /  (1.0f+Game->dt*CAMERA_LINEAR_DAMPING));            
        ak_v2f Vel = Camera->Velocity.xy*Game->dt;
        ak_v3f Delta = Vel.x*Camera->Orientation.XAxis - Vel.y*Camera->Orientation.YAxis;
        
        Camera->FocalPoint += Delta;
        Camera->Position += Delta;
        
        Camera->Velocity.z *= (1.0f/ (1.0f+Game->dt*CAMERA_SCROLL_DAMPING));            
        Camera->Distance += Camera->Velocity.z*Game->dt;            
        
        if(Camera->Distance < CAMERA_MIN_DISTANCE)
            Camera->Distance = CAMERA_MIN_DISTANCE;
        
        Camera->Position = Camera->FocalPoint + (Camera->Orientation.ZAxis*Camera->Distance);
    }    
}

void PopulateTranslateAndScaleGizmos(dev_context* DevContext, ak_v3f Position, dev_mesh* DirectionMesh)
{
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = DirectionMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_X;
        DevContext->Gizmo[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = DirectionMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Y;
        DevContext->Gizmo[1] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = DirectionMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Z;
        DevContext->Gizmo[2] =  Gizmo;
    }

    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(GIZMO_PLANE_DISTANCE, GIZMO_PLANE_DISTANCE, 0.0f), X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->PlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_XY;
        DevContext->Gizmo[3] =  Gizmo;
    }

    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(GIZMO_PLANE_DISTANCE, 0.0f, GIZMO_PLANE_DISTANCE), X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->PlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_XZ;
        DevContext->Gizmo[4] =  Gizmo;
    }

    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(0.0f, GIZMO_PLANE_DISTANCE, GIZMO_PLANE_DISTANCE), X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->PlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(1, 0, 0);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_YZ;
        DevContext->Gizmo[5] =  Gizmo;
    }
}

void PopulateCircleGizmos(dev_context* DevContext, ak_v3f Position)
{
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_X;
        DevContext->Gizmo[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Y;
        DevContext->Gizmo[1] =  Gizmo;
    }

    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = &DevContext->TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Z;
        DevContext->Gizmo[2] =  Gizmo;
    }
    
    {
        gizmo Gizmo;
        Gizmo.Mesh = nullptr;
        DevContext->Gizmo[3] =  Gizmo;
    }

    {
        gizmo Gizmo;
        Gizmo.Mesh = nullptr;
        DevContext->Gizmo[4] =  Gizmo;
    }

    {
        gizmo Gizmo;
        Gizmo.Mesh = nullptr;
        DevContext->Gizmo[5] =  Gizmo;
    }
}

void PopulateGizmo(dev_context* DevContext, ak_v3f Position)
{
    if(DevContext->TransformationMode != GIZMO_MOVEMENT_TYPE_ROTATE)
    {
        dev_mesh* DirectionMesh = &DevContext->TriangleArrowMesh;
        if(DevContext->TransformationMode == GIZMO_MOVEMENT_TYPE_SCALE)
        {
            DirectionMesh = &DevContext->TriangleScaleMesh;
        }
        PopulateTranslateAndScaleGizmos(DevContext, Position, DirectionMesh);
    }
    else
    {
        PopulateCircleGizmos(DevContext, Position);
    }
}

ray CastRayFromCameraToMouse(dev_context* DevContext, graphics_state* GraphicsState)
{
    ray Result;
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    dev_input* Input = &DevContext->Input;
    
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, Game->CurrentWorldIndex);    
    
    //For not just getting the player entity. need to change to cast the ray and get the intersected object
    
    ak_i32 Height = Graphics->RenderDim.h;
    ak_i32 Width = Graphics->RenderDim.w;
    ak_f32 x = (2.0f * Input->MouseCoordinates.x) / Width - 1.0f;
    ak_f32 y = 1.0f - (2.0f * Input->MouseCoordinates.y) / Height;
    ak_f32 z = 1.0f;
    ak_v3f ray_nds = AK_V3(x, y, z);
    ak_v4f ray_clip = AK_V4(ray_nds.xy, -1.0f, 1.0f);
    ak_m4f Perspective = AK_Perspective(ViewSettings.FieldOfView, AK_SafeRatio(Graphics->RenderDim.w, Graphics->RenderDim.h), ViewSettings.ZNear, ViewSettings.ZFar);
    ak_v4f ray_eye =  ray_clip * AK_Inverse(Perspective);
    ray_eye = AK_V4(ray_eye.xy, -1.0f, 0.0f);
    ak_v3f ray_wor =  (ray_eye * AK_TransformM4(ViewSettings.Position, ViewSettings.Orientation)).xyz;
    ray_wor = AK_Normalize(ray_wor);

    Result.Origin = ViewSettings.Position;
    Result.Direction = ray_wor;

    return Result;
}

entity_id GetSelectedObject(dev_context* DevContext, graphics_state* GraphicsState, ray RayCast)
{        
    game* Game = DevContext->Game;
    graphics* Graphics = DevContext->Graphics;
    dev_input* Input = &DevContext->Input;
    
    if(DevContext->IsGizmoHit)    
        return DevContext->SelectedObjectID;    
    
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, Game->CurrentWorldIndex);  
    
    
    
    ak_i32 Height = Graphics->RenderDim.h;
    ak_i32 Width = Graphics->RenderDim.w;
    ak_f32 x = (2.0f * Input->MouseCoordinates.x) / Width - 1.0f;
    ak_f32 y = 1.0f - (2.0f * Input->MouseCoordinates.y) / Height;
    ak_f32 z = 1.0f;
    ak_v3f ray_nds = AK_V3(x, y, z);
    ak_v4f ray_clip = AK_V4(ray_nds.xy, -1.0f, 1.0f);
    ak_m4f Perspective = AK_Perspective(ViewSettings.FieldOfView, AK_SafeRatio(Graphics->RenderDim.w, Graphics->RenderDim.h), ViewSettings.ZNear, ViewSettings.ZFar);
    ak_v4f ray_eye =  ray_clip * AK_Inverse(Perspective);
    ray_eye = AK_V4(ray_eye.xy, -1.0f, 0.0f);
    ak_v3f ray_wor =  (ray_eye * AK_TransformM4(ViewSettings.Position, ViewSettings.Orientation)).xyz;
    ray_wor = AK_Normalize(ray_wor);
    
    ak_f32 tBest = INFINITY;
    entity* Result = NULL;
    
    AK_ForEach(Entity, &Game->EntityStorage[Game->CurrentWorldIndex])        
    {        
        if(Entity->MeshID != INVALID_MESH_ID)
        {   
            mesh_info* MeshInfo = GetMeshInfo(DevContext->Game->Assets, Entity->MeshID);
            mesh* Mesh = GetMesh(DevContext->Game->Assets, Entity->MeshID);
            if(!Mesh)
                Mesh = LoadMesh(DevContext->Game->Assets, Entity->MeshID);
            
            ak_sqtf Transform = *GetEntityTransform(Game, Entity->ID);
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
    
    return Result ? Result->ID : InvalidEntityID();
}

gizmo_hit* GetSelectedGizmo(dev_context* DevContext, graphics_state* GraphicsState, ray RayCast)
{
    ak_f32 tBest = INFINITY;
    gizmo_hit* Result = &DevContext->GizmoHit;
    game* Game = DevContext->Game;
    
    if(!DevContext->SelectedObjectID.IsValid() || !DevContext->EditMode)
    {
        DevContext->IsGizmoHit = false;
        return Result;
    }
    
    if(DevContext->IsGizmoHit)
    {
        return &DevContext->GizmoHit;
    }
    
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, Game->CurrentWorldIndex);  
    
    ak_i32 NumberOfGizmos = 6;
    if(DevContext->TransformationMode == GIZMO_MOVEMENT_TYPE_ROTATE)
    {
        NumberOfGizmos = 3;
    }
    
    for(ak_i32 i = 0; i < 6; i++)
    {
        if(DevContext->Gizmo[i].Mesh != NULL)
        {
            mesh_info MeshInfo = GetMeshInfoFromDevMesh(*DevContext->Gizmo[i].Mesh);
            mesh Mesh = GetMeshFromDevMesh(*DevContext->Gizmo[i].Mesh);
            
            ray_mesh_intersection_result IntersectionResult = RayMeshIntersection(RayCast.Origin, RayCast.Direction, &Mesh, &MeshInfo, DevContext->Gizmo[i].Transform, true, false);
            if(IntersectionResult.FoundCollision)
            {
                if(tBest > IntersectionResult.t && IntersectionResult.t > ViewSettings.ZNear)
                {
                    tBest = IntersectionResult.t;
                    Result->Gizmo = &DevContext->Gizmo[i];
                    Result->HitMousePosition = RayCast.Origin + (IntersectionResult.t * RayCast.Direction);
                    DevContext->IsGizmoHit = true;
                    DebugLog(DevContext, "Hit Gizmo");
                }
            }
        }
    }
    return Result;
}

void DrawWireframeWorld(game* Game, graphics* Graphics, ak_u32 WorldIndex, assets* Assets)
{
    PushWireframe(Graphics, true);
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);        
    AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
    {
        if(Entity->MeshID != INVALID_MESH_ID)
        {
            graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, Entity->MeshID);
            
            ak_sqtf Transform = *GetEntityTransform(Game, Entity->ID);
            PushDrawUnlitMesh(Graphics, MeshHandle, Transform, CreateDiffuseMaterialSlot(AK_Cyan3()), 
                              GetMeshIndexCount(Assets, Entity->MeshID), 0, 0);
        }
    }
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushWireframe(Graphics, false);
}

void DrawWorld(dev_context* DevContext, graphics_render_buffer* RenderBuffer, ak_u32 WorldIndex, graphics_state* GraphicsState, ak_f32 tRenderInterpolate)
{           
    view_settings ViewSettings = GetViewSettings(DevContext, GraphicsState, WorldIndex);    
    
    PushRenderBufferViewportScissorAndView(DevContext->Graphics, RenderBuffer, &ViewSettings);
    PushClearColorAndDepth(DevContext->Graphics, AK_Black4(), 1.0f);                
    
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
            AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
            {
                if(Entity->MeshID != INVALID_MESH_ID)                    
                {
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, DevContext->Graphics, Entity->MeshID);                    
                    graphics_diffuse_material_slot Diffuse = ConvertToGraphicsDiffuse(Assets, DevContext->Graphics, Entity->Material.Diffuse);
                    
                    ak_sqtf Transform = *GetEntityTransform(Game, Entity->ID);
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
        
        AK_INVALID_DEFAULT_CASE;        
    }
    
    PushDepth(DevContext->Graphics, false);
    
    if(DevContext->DrawColliders)
    {
        AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
        {   
            mesh_convex_hull_gdi* ConvexHullGDI = NULL; 
            if(Entity->MeshID != INVALID_MESH_ID)
            {
                ConvexHullGDI = DevContext->MeshConvexHulls + Entity->MeshID;
                
                if(ConvexHullGDI->Count == (ak_u32)-1)
                {
                    mesh_info* MeshInfo = GetMeshInfo(Assets, Entity->MeshID);                
                    ConvexHullGDI->Count = MeshInfo->Header.ConvexHullCount;
                    
                    if(ConvexHullGDI->Count > 0)
                    {
                        ConvexHullGDI->Meshes = DevContext->DevStorage->PushArray<graphics_mesh_id>(ConvexHullGDI->Count);
                        for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < ConvexHullGDI->Count; ConvexHullIndex++)
                        {
                            convex_hull* ConvexHull = MeshInfo->ConvexHulls + ConvexHullIndex;                            
                            ConvexHullGDI->Meshes[ConvexHullIndex] = AllocateConvexHullMesh(DevContext->Graphics, ConvexHull);                            
                        }
                    }
                }                                
            }
            
            ak_sqtf OldState = *GetEntityTransformOld(Game, Entity->ID);
            ak_sqtf NewState = *GetEntityTransform(Game, Entity->ID);        
            
            //CONFIRM(JJ): Should we be doing runtime changes on an entities scale? Definitely not on rigid bodies,
            //but do other entities might want this? Keep this assertion just in case. Not sure if we even need to 
            //interpolate them either
            AK_Assert(OldState.Scale == NewState.Scale, "Not interploating scale yet");
            
            ak_sqtf InterpTransform;
            InterpTransform.Translation = AK_Lerp(OldState.Translation, tRenderInterpolate, NewState.Translation);    
            InterpTransform.Orientation = AK_Lerp(OldState.Orientation, tRenderInterpolate, NewState.Orientation);
            InterpTransform.Scale = NewState.Scale;
            
            ak_u32 ConvexHullIndex = 0;
            
            simulation* Simulation = GetSimulation(Game, Entity->ID);
            sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
            collision_volume* Volume = Simulation->CollisionVolumeStorage.Get(SimEntity->CollisionVolumeID);
            while(Volume)                
            {                
                switch(Volume->Type)
                {
                    case COLLISION_VOLUME_TYPE_SPHERE:
                    {
                        sphere Sphere = TransformSphere(&Volume->Sphere, InterpTransform);                    
                        DrawLineEllipsoid(DevContext, Sphere.CenterP, AK_V3(Sphere.Radius, Sphere.Radius, Sphere.Radius), AK_Blue3());
                    } break;
                    
                    case COLLISION_VOLUME_TYPE_CAPSULE:
                    {
                        capsule Capsule = TransformCapsule(&Volume->Capsule, InterpTransform);
                        DrawLineCapsule(DevContext, Capsule.P0, Capsule.P1, Capsule.Radius, AK_Blue3());                                                
                    } break;
                    
                    case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                    {                           
                        AK_Assert(Entity->MeshID != INVALID_MESH_ID && ConvexHullGDI, "Invalid collision volume mesh for the convex hull");
                        AK_Assert(ConvexHullGDI->Count != 0, "No graphics device interface for the collision volume convex hull");
                        
                        ak_sqtf Transform = Volume->ConvexHull->Header.Transform*InterpTransform;
                        ak_m4f Model = AK_TransformM4(Transform);
                        
                        PushDrawLineMesh(DevContext->Graphics, ConvexHullGDI->Meshes[ConvexHullIndex], Model, AK_Blue3(), 
                                         ConvexHullIndexCount(Volume->ConvexHull), 0, 0);
                        
                        ConvexHullIndex++;
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
                Volume = Simulation->CollisionVolumeStorage.Get(Volume->NextID);
            }                        
        }                        
    }   
    
    PushDepth(DevContext->Graphics, true);    
}

void DevelopmentHandleGizmoTranslate(dev_context* DevContext, graphics_state* GraphicsState, ak_v3f PointDiff)
{        
    game* Game = DevContext->Game;
    gizmo_hit* GizmoHit = &DevContext->GizmoHit;

    entity* Entity = GetEntity(Game, DevContext->SelectedObjectID);
    
    simulation* Simulation = GetSimulation(Game, DevContext->SelectedObjectID);
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    
    ak_sqtf* Transform = GetEntityTransform(Game, DevContext->SelectedObjectID);
    Transform->Translation -= PointDiff;
    
    SimEntity->Transform = *Transform;
}

void DevelopmentHandleGizmoScale(dev_context* DevContext, graphics_state* GraphicsState, ak_v3f PointDiff)
{
    game* Game = DevContext->Game;
    gizmo_hit* GizmoHit = &DevContext->GizmoHit;

    entity* Entity = GetEntity(Game, DevContext->SelectedObjectID);
    
    simulation* Simulation = GetSimulation(Game, DevContext->SelectedObjectID);
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    
    ak_sqtf* Transform = GetEntityTransform(Game, DevContext->SelectedObjectID);
    Transform->Scale -= PointDiff;
    
    SimEntity->Transform = *Transform;
}

void DevelopmentHandleGizmoRotate(dev_context* DevContext, graphics_state* GraphicsState, ak_v3f PointDiff)
{
    game* Game = DevContext->Game;
    gizmo_hit* GizmoHit = &DevContext->GizmoHit;

    entity* Entity = GetEntity(Game, DevContext->SelectedObjectID);
    
    simulation* Simulation = GetSimulation(Game, DevContext->SelectedObjectID);
    
    sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
    
    ak_sqtf* Transform = GetEntityTransform(Game, DevContext->SelectedObjectID);
    
    ak_u32 Index = AK_PoolIndex(DevContext->SelectedObjectID.ID);
    if(DevContext->EntityRotations[DevContext->SelectedObjectID.WorldIndex].Size < (Index+1))
        DevContext->EntityRotations[DevContext->SelectedObjectID.WorldIndex].Resize(Index+1);
    
    ak_v3f* Rotation = DevContext->EntityRotations[DevContext->SelectedObjectID.WorldIndex].Get(Index);
    *Rotation -= PointDiff;
    Transform->Orientation =  AK_Normalize(AK_EulerToQuat(*Rotation));
    
    SimEntity->Transform = *Transform;
}

ak_v3f DevelopmentGetGizmoPointDiff(dev_context* DevContext, graphics_state* GraphicsState, ak_v3f SelectedObjectPosition)
{
    ray RayCast = CastRayFromCameraToMouse(DevContext, GraphicsState);
    ray_mesh_intersection_result IntersectionResult = RayPlaneIntersection(DevContext->GizmoHit.Gizmo->IntersectionPlane, DevContext->GizmoHit.HitMousePosition, RayCast);

    ak_v3f Result = AK_V3<ak_f32>();
    if(!IntersectionResult.FoundCollision)
    {
        DebugLog(DevContext, "There was not intersection with Gizmp diff plane");
        return Result;
    }

    ak_v3f NewPoint = RayCast.Origin + (RayCast.Direction * IntersectionResult.t);

    if(DevContext->TransformationMode != GIZMO_MOVEMENT_TYPE_ROTATE)
    {
        switch(DevContext->GizmoHit.Gizmo->MovementDirection)
        {
            case GIZMO_MOVEMENT_DIRECTION_X:
            {
                Result = AK_V3(DevContext->GizmoHit.HitMousePosition.x - NewPoint.x, 0.0f, 0.0f);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_Y:
            {
                Result = AK_V3(0.0f, DevContext->GizmoHit.HitMousePosition.y - NewPoint.y, 0.0f);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_Z:
            {
                Result = AK_V3(0.0f, 0.0f, DevContext->GizmoHit.HitMousePosition.z - NewPoint.z);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_XY:
            {
                Result = AK_V3(DevContext->GizmoHit.HitMousePosition.x - NewPoint.x, DevContext->GizmoHit.HitMousePosition.y - NewPoint.y, 0.0f);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_XZ:
            {
                Result = AK_V3(DevContext->GizmoHit.HitMousePosition.x - NewPoint.x, 0.0f, DevContext->GizmoHit.HitMousePosition.z - NewPoint.z);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_YZ:
            {
                Result = AK_V3(0.0f, DevContext->GizmoHit.HitMousePosition.y - NewPoint.y, DevContext->GizmoHit.HitMousePosition.z - NewPoint.z);
            } break;

            AK_INVALID_DEFAULT_CASE;
        }
    }
    else
    {
        ak_v3f DirectionToOld = DevContext->GizmoHit.HitMousePosition - SelectedObjectPosition;
        ak_v3f DirectionToNew = NewPoint - SelectedObjectPosition;
        ak_f32 AngleDiff = AK_ATan2(AK_Dot(AK_Cross(DirectionToNew, DirectionToOld), 
                                           DevContext->GizmoHit.Gizmo->IntersectionPlane), 
                                    AK_Dot(DirectionToOld, DirectionToNew));
        
        if(AngleDiff != 0)
            int x = 0;
        
        switch(DevContext->GizmoHit.Gizmo->MovementDirection)
        {
            case GIZMO_MOVEMENT_DIRECTION_X:
            {
                Result = AK_V3(AngleDiff, 0.0f, 0.0f);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_Y:
            {
                Result = AK_V3(0.0f, AngleDiff, 0.0f);
            } break;
            
            case GIZMO_MOVEMENT_DIRECTION_Z:
            {
                Result = AK_V3(0.0f, 0.0f, AngleDiff);
            } break;

            AK_INVALID_DEFAULT_CASE;
        }
    }

    if(DevContext->TransformationMode == GIZMO_MOVEMENT_TYPE_TRANSLATE)
    {
        if(IsDown(DevContext->Input.Ctl))
        {
            for(int i = 0; i < 3; i++)
            {
                if(AK_Abs(Result[i]) < DevContext->GridDistance && Result[i] != 0 )
                {
                    NewPoint[i] = DevContext->GizmoHit.HitMousePosition[i];
                    Result[i] = 0.0f;
                }
                else if(Result[i] < 0)
                {
                    Result[i] = -1 * DevContext->GridDistance;
                }
                else if(Result[i] > 0)
                {
                    Result[i] = DevContext->GridDistance;
                }
            }
        }
    }

    DevContext->GizmoHit.HitMousePosition = NewPoint;
    return Result;
}

void DevelopmentHandleGizmoTransform(dev_context* DevContext, graphics_state* GraphicsState, ak_v3f SelectedObjectPosition)
{
    ak_v3f PointDiff = DevelopmentGetGizmoPointDiff(DevContext, GraphicsState, SelectedObjectPosition);

    switch(DevContext->TransformationMode)
    {
        case GIZMO_MOVEMENT_TYPE_TRANSLATE:
        {
            DevelopmentHandleGizmoTranslate(DevContext, GraphicsState, PointDiff);
        } break;

        case GIZMO_MOVEMENT_TYPE_SCALE:
        {
            DevelopmentHandleGizmoScale(DevContext, GraphicsState, PointDiff);
        } break;

        case GIZMO_MOVEMENT_TYPE_ROTATE:
        {
            DevelopmentHandleGizmoRotate(DevContext, GraphicsState, PointDiff);
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
}

void DevelopmentRender(dev_context* DevContext, graphics_state* GraphicsState, ak_f32 tRenderInterpolate)
{   
    graphics* Graphics = DevContext->Graphics;
    game* Game = DevContext->Game;
    dev_input* Input = &DevContext->Input;
    
    if((Graphics->RenderDim.w <= 0) ||  (Graphics->RenderDim.h <= 0))
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
    
    if(DevContext->EditMode && IsPressed(Input->W))
    {
        DevContext->TransformationMode = GIZMO_MOVEMENT_TYPE_TRANSLATE;
    }
    if(DevContext->EditMode && IsPressed(Input->E))
    {
        DevContext->TransformationMode = GIZMO_MOVEMENT_TYPE_SCALE;
    }
    if(DevContext->EditMode && IsPressed(Input->R))
    {
        DevContext->TransformationMode = GIZMO_MOVEMENT_TYPE_ROTATE;
    }

    PushDepth(Graphics, false); 
    ak_v3f SelectedObjectPosition = {};
    if(DevContext->SelectedObjectID.IsValid())
    {                
        SelectedObjectPosition = AK_Lerp(GetEntityPositionOld(Game, DevContext->SelectedObjectID), 
                                         tRenderInterpolate, 
                                         GetEntityPosition(Game, DevContext->SelectedObjectID));
        if(!DevContext->EditMode)
        {
            DrawFrame(DevContext, SelectedObjectPosition, AK_V3f(1, 0, 0), AK_V3f(0, 1, 0), AK_V3f(0, 0, 1));
        }
        else
        {
            PopulateGizmo(DevContext, SelectedObjectPosition);
            DrawGizmos(DevContext, SelectedObjectPosition);
        }
    }
    
    if(DevContext->IsGizmoHit && DevContext->EditMode)
    {
        DevelopmentHandleGizmoTransform(DevContext, GraphicsState, SelectedObjectPosition);
    }
    
    if(!IsDown(Input->Alt) && !ImGui::GetIO().WantCaptureMouse)
    {
        if(IsPressed(Input->LMB))
        {
            ray RayCast = CastRayFromCameraToMouse(DevContext, GraphicsState);
            DevContext->InspectRay = RayCast.Direction;
            GetSelectedGizmo(DevContext, GraphicsState, RayCast);
            DevContext->SelectedObjectID = GetSelectedObject(DevContext, GraphicsState, RayCast);
        }
        if(IsPressed(Input->MMB))
        {
            DevContext->SelectedObjectID = InvalidEntityID();
        }
        if(IsReleased(Input->LMB))
        {
            DevContext->IsGizmoHit = false;
        }
    }
    
    PushDepth(Graphics, false);    
    for(ak_u32 PrimitiveIndex = 0; PrimitiveIndex < DevContext->DebugPrimitives.Size; PrimitiveIndex++)        
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
            
            AK_INVALID_DEFAULT_CASE;
        }
    }
    DevContext->DebugPrimitives.Size = 0;
    
    
    PushDepth(Graphics, true);
    
    if(DevContext->DrawGrid)
    {
        ak_m4f Perspective = AK_Perspective(ViewSettings.FieldOfView, AK_SafeRatio(Graphics->RenderDim.w, Graphics->RenderDim.h), ViewSettings.ZNear, ViewSettings.ZFar);
        ak_v3f FrustumCorners[8];
        AK_GetFrustumCorners(FrustumCorners, Perspective);
        AK_TransformPoints(FrustumCorners, 8, AK_TransformM4(ViewSettings.Position, ViewSettings.Orientation));

        ak_v3f FrustumPlaneIntersectionPoints[4];
        ak_i8 IntersectedCount = 0;
        for(int i = 0; i < 4; i++)
        {
            ray FrustumRay = {};
            FrustumRay.Origin = FrustumCorners[i];
            FrustumRay.Direction = AK_Normalize(FrustumCorners[i + 4] - FrustumCorners[i]);
            ray_mesh_intersection_result FrustumRayResult = RayPlaneIntersection(AK_V3f(0, 0, 1), AK_V3f(0,0,0), FrustumRay);
            if(FrustumRayResult.FoundCollision)
            {
                IntersectedCount++;
                ak_f32 Distance = FrustumRayResult.t;
                Distance = AK_Min(Distance, ViewSettings.ZFar);
                FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin + (FrustumRay.Direction * Distance);
            }
            else
            {
                ak_f32 Distance = ViewSettings.ZFar;
                FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin + (FrustumRay.Direction * Distance);
            }
        }

        if(IntersectedCount != 0)
        {
            //if not all frustum rays intersected, we want to use the less efficient method for getting grid bounds
            ak_f32 MinX;
            ak_f32 MaxX;
            ak_f32 MinY;
            ak_f32 MaxY;
            if(IntersectedCount == 4)
            {
                MinX = FrustumPlaneIntersectionPoints[0].x;
                MaxX = FrustumPlaneIntersectionPoints[0].x;
                MinY = FrustumPlaneIntersectionPoints[0].y;
                MaxY = FrustumPlaneIntersectionPoints[0].y;
                for(int i = 0; i < 4; i++)
                {
                    MinX = AK_Min(FrustumPlaneIntersectionPoints[i].x, MinX);                                             
                    MaxX = AK_Max(FrustumPlaneIntersectionPoints[i].x, MaxX);
                    MinY = AK_Min(FrustumPlaneIntersectionPoints[i].y, MinY);
                    MaxY = AK_Max(FrustumPlaneIntersectionPoints[i].y, MaxY);
                }
            }
            else
            {
                MinX = FrustumCorners[0].x;
                MaxX = FrustumCorners[0].x;
                MinY = FrustumCorners[0].y;
                MaxY = FrustumCorners[0].y;
                for(int i = 0; i < 8; i++)
                {
                    MinX = AK_Min(FrustumCorners[i].x, MinX);                                             
                    MaxX = AK_Max(FrustumCorners[i].x, MaxX);
                    MinY = AK_Min(FrustumCorners[i].y, MinY);
                    MaxY = AK_Max(FrustumCorners[i].y, MaxY);
                }
            }
            
            DrawGrid(DevContext, AK_Floor(MinX), AK_Ceil(MaxX), AK_Floor(MinY), AK_Ceil(MaxY), AK_RGB(0.1f, 0.1f, 0.1f)); 
        }
    }
    
    DevelopmentImGuiRender(DevContext);  
    
    PushScissor(Graphics, 0, 0, Game->RenderBuffer->Resolution.w, Game->RenderBuffer->Resolution.h);
    PushCopyToOutput(Graphics, Game->RenderBuffer, AK_V2(0, 0), Game->RenderBuffer->Resolution);    
    
    if(DevContext->DrawOtherWorld)
    {
        ak_v2i Offset = Game->RenderBuffer->Resolution - DevContext->RenderBuffer->Resolution;
        PushCopyToOutput(Graphics, DevContext->RenderBuffer, Offset, DevContext->RenderBuffer->Resolution);    
    }
}

void DevelopmentTick(dev_context* DevContext, game* Game, graphics* Graphics, graphics_state* GraphicsState, ak_f32 tRenderInterpolate)
{
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    
    if(!DevContext->Initialized)
    {
        DevContext->DevStorage = AK_CreateArena(AK_Kilobyte(32));                               
        DevContext->LogStorage = AK_CreateArena(AK_Megabyte(1));
        
        DevContext->FramePlayback.MaxRecordingPathLength = 8092;
        DevContext->FramePlayback.RecordingPath = DevContext->DevStorage->PushArray<ak_char>(DevContext->FramePlayback.MaxRecordingPathLength+1);
        
        DevContext->RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim/5);
        //DevContext->DrawGrid = true;
        //DevContext->DrawColliders = true;      
        //DevContext->EditMode = true;  
        DevContext->GridDistance = 1.0f;
        
        for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
            DevContext->MeshConvexHulls[MeshIndex].Count = (ak_u32)-1;
        
        CreateDevLineCapsuleMesh(DevContext, 1.0f, 60);
        CreateDevLineBoxMesh(DevContext);
        CreateDevLineSphereMesh(DevContext, 60);
        CreateDevTriangleBoxMesh(DevContext);
        CreateDevTriangleSphereMesh(DevContext);
        CreateDevTriangleCylinderMesh(DevContext, 60);
        CreateDevTriangleConeMesh(DevContext, 60);
        CreateDevTriangleArrowMesh(DevContext, 60, 0.02f, 0.85f, 0.035f, 0.15f);
        CreateDevPlaneMesh(DevContext, 0.4f, 0.4f);
        CreateDevTriangleCircleMesh(DevContext, 60, 0.05f);
        CreateDevTriangleScaleMesh(DevContext, 60, 0.02f, 0.85f, 0.1f);
        CreateDevTriangleTorusMesh(DevContext, 20, 0.03f);
        
        DevContext->EntityRotations[0] = AK_CreateArray<ak_v3f>(Game->EntityStorage[0].Capacity);
        DevContext->EntityRotations[1] = AK_CreateArray<ak_v3f>(Game->EntityStorage[1].Capacity);
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
            {
                ak_sqtf* Transform = GetEntityTransform(Game, Entity->ID);
                ak_u32 PoolIndex = AK_PoolIndex(Entity->ID.ID);
                
                if(DevContext->EntityRotations[WorldIndex].Size < (PoolIndex+1))
                    DevContext->EntityRotations[WorldIndex].Resize(PoolIndex+1);
                DevContext->EntityRotations[WorldIndex][PoolIndex] = AK_QuatToEuler(Transform->Orientation);
            }
        }
        
        DevelopmentImGuiInit(DevContext);        
        DevContext->Initialized = true;                        
    }
    
    dev_input* Input = &DevContext->Input;    
    
    if(IsInDevelopmentMode(DevContext))
    {        
        Platform_DevUpdate(DevContext->PlatformData[0], Graphics->RenderDim, Game->dt, DevContext);        
        
        if(IsPressed(Input->Delete))
        {
            if(DevContext->SelectedObjectID.IsValid())
            {                
                if(GetEntityType(DevContext->Game, DevContext->SelectedObjectID) != ENTITY_TYPE_PLAYER)
                {
                    FreeEntity(DevContext->Game, DevContext->SelectedObjectID);
                    DevContext->SelectedObjectID = InvalidEntityID();
                }
            }
        }
        
        DevelopmentRender(DevContext, GraphicsState, tRenderInterpolate);        
    }
    
    if(IsPressed(Input->ToggleDevState)) DevContext->InDevelopmentMode = !DevContext->InDevelopmentMode;
    
    Input->MouseDelta = {};
    Input->Scroll = 0.0f;
    for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(Input->Buttons); ButtonIndex++)
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