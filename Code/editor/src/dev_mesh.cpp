graphics_mesh_id DevMesh_GPUAllocateMesh(graphics* Graphics, ak_mesh_result<ak_vertex_p3>* Mesh)
{
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(ak_vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                                     Mesh->Indices, Mesh->IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

dev_mesh DevMesh_CPUAllocateMesh(ak_mesh_result<ak_vertex_p3>* Mesh)
{
    dev_mesh Result;
    Result.IndexCount = Mesh->IndexCount;
    Result.VertexCount = Mesh->VertexCount;
    
    ak_u32 VertexSize = Mesh->VertexCount*sizeof(ak_vertex_p3);
    ak_u32 IndexSize = Mesh->IndexCount*sizeof(ak_u16);
    Result.Vertices = AK_Allocate(VertexSize);
    Result.Indices = AK_Allocate(IndexSize);
    
    AK_MemoryCopy(Result.Vertices, Mesh->Vertices, VertexSize); 
    AK_MemoryCopy(Result.Indices, Mesh->Indices, IndexSize);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateLineSphereMesh(graphics* Graphics, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateLineSphere(GlobalArena, 1.0f, CircleSampleCount);        
    
    dev_slim_mesh Result = {};
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);        
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_mesh DevMesh_CreateTriangleCircleMesh(graphics* Graphics, ak_u16 CircleSampleCount, ak_f32 Height)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCircle(GlobalArena, 1.0f, Height, CircleSampleCount);        
    GlobalArena->EndTemp(&TempArena);
    
    dev_mesh Result = DevMesh_CPUAllocateMesh(&MeshGenerationResult);
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    return Result;
}

dev_mesh DevMesh_CreateTriangleTorusMesh(graphics* Graphics, ak_u16 CircleSampleCount, ak_f32 Width)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleTorus(GlobalArena, 1.0f, Width, CircleSampleCount);        
    GlobalArena->EndTemp(&TempArena);
    
    dev_mesh Result = DevMesh_CPUAllocateMesh(&MeshGenerationResult);
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    return Result;
}

dev_slim_mesh DevMesh_CreateLineBoxMesh(graphics* Graphics)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateLineBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));    
    
    dev_slim_mesh Result;
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateTriangleBoxMesh(graphics* Graphics)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));       
    
    dev_slim_mesh Result;
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateTriangleSphereMesh(graphics* Graphics)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleSphere(GlobalArena, 1.0f, 2);
    
    dev_slim_mesh Result;
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateTriangleCylinderMesh(graphics* Graphics, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCylinder(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    
    dev_slim_mesh Result;
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateTriangleConeMesh(graphics* Graphics, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCone(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    
    dev_slim_mesh Result;
    Result.IndexCount = MeshGenerationResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_mesh DevMesh_CreateTriangleArrowMesh(graphics* Graphics, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 ArrowRadius, ak_f32 ArrowHeight)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result<ak_vertex_p3> ArrowResult = AK_GenerateTriangleCone(GlobalArena, ArrowRadius, ArrowHeight, CircleSampleCount, AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(GlobalArena, BodyResult.VertexCount+ArrowResult.VertexCount, 
                                                                                            BodyResult.IndexCount+ArrowResult.IndexCount);    
    
    ak_uaddr BodyResultVerticesSize = sizeof(ak_vertex_p3)*BodyResult.VertexCount;
    ak_uaddr BodyResultIndicesSize = sizeof(ak_u16)*BodyResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, BodyResult.Vertices, BodyResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, BodyResult.Indices, BodyResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+BodyResultVerticesSize, ArrowResult.Vertices, sizeof(ak_vertex_p3)*ArrowResult.VertexCount);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Indices+BodyResultIndicesSize, ArrowResult.Indices, sizeof(ak_u16)*ArrowResult.IndexCount);
    
    AK_OffsetIndices(MeshGenerationResult.Indices+BodyResult.IndexCount, AK_SafeU16(ArrowResult.IndexCount), AK_SafeU16(BodyResult.VertexCount));
    
    dev_mesh Result = DevMesh_CPUAllocateMesh(&MeshGenerationResult);
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_mesh DevMesh_CreateTriangleScaleMesh(graphics* Graphics, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 CubeSize)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result<ak_vertex_p3> BoxResult = AK_GenerateTriangleBox(GlobalArena, AK_V3f(CubeSize, CubeSize, CubeSize), AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(GlobalArena, BodyResult.VertexCount+BoxResult.VertexCount, 
                                                                                            BodyResult.IndexCount+BoxResult.IndexCount);    
    
    ak_uaddr BodyResultVerticesSize = sizeof(ak_vertex_p3)*BodyResult.VertexCount;
    ak_uaddr BodyResultIndicesSize = sizeof(ak_u16)*BodyResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, BodyResult.Vertices, BodyResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, BodyResult.Indices, BodyResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+BodyResultVerticesSize, BoxResult.Vertices, sizeof(ak_vertex_p3)*BoxResult.VertexCount);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Indices+BodyResultIndicesSize, BoxResult.Indices, sizeof(ak_u16)*BoxResult.IndexCount);
    
    AK_OffsetIndices(MeshGenerationResult.Indices+BodyResult.IndexCount, AK_SafeU16(BoxResult.IndexCount), AK_SafeU16(BodyResult.VertexCount));
    
    dev_mesh Result = DevMesh_CPUAllocateMesh(&MeshGenerationResult);
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_capsule_mesh DevMesh_CreateLineCapsuleMesh(graphics* Graphics, ak_f32 Radius, ak_u16 CircleSampleCount)
{   
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> CapResult = AK_GenerateLineHemisphere(GlobalArena, Radius, CircleSampleCount);    
    ak_mesh_result<ak_vertex_p3> BodyResult = AK_AllocateMeshResult<ak_vertex_p3>(GlobalArena, 8, 8);
    
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
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(GlobalArena, CapResult.VertexCount+BodyResult.VertexCount, 
                                                                                            CapResult.IndexCount+BodyResult.IndexCount);
    
    ak_uaddr CapResultVerticesSize = sizeof(ak_vertex_p3)*CapResult.VertexCount;
    ak_uaddr CapResultIndicesSize = sizeof(ak_u16)*CapResult.IndexCount;
    AK_MemoryCopy(MeshGenerationResult.Vertices, CapResult.Vertices, CapResultVerticesSize);
    AK_MemoryCopy(MeshGenerationResult.Indices, CapResult.Indices, CapResultIndicesSize);
    AK_MemoryCopy((ak_u8*)MeshGenerationResult.Vertices+CapResultVerticesSize, BodyResult.Vertices, sizeof(ak_vertex_p3)*BodyResult.VertexCount);
    CopyMemory((ak_u8*)MeshGenerationResult.Indices+CapResultIndicesSize, BodyResult.Indices, sizeof(ak_u16)*BodyResult.IndexCount);
    
    dev_capsule_mesh Result;
    Result.CapIndexCount = CapResult.IndexCount;
    Result.CapVertexCount = CapResult.VertexCount;
    Result.BodyIndexCount = BodyResult.IndexCount;
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}


dev_mesh DevMesh_CreatePlaneMesh(graphics* Graphics, ak_f32 Width, ak_f32 Height)
{   
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(GlobalArena, 4, 6);
    
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
    
    dev_mesh Result = DevMesh_CPUAllocateMesh(&MeshGenerationResult);
    Result.MeshID = DevMesh_GPUAllocateMesh(Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}

dev_slim_mesh DevMesh_CreateConvexHullMesh(graphics* Graphics, convex_hull* ConvexHull)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    dev_slim_mesh Result = {};
    
    ak_v3f* Vertices = GlobalArena->PushArray<ak_v3f>(ConvexHull->Header.VertexCount);
    Result.IndexCount = ConvexHull->Header.FaceCount*3*2;
    ak_u16* Indices = GlobalArena->PushArray<ak_u16>(Result.IndexCount);
    
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
    
    AK_Assert(Index == Result.IndexCount, "Failed to build convex hull indexes properly. This is a programming error");
    Result.MeshID = Graphics->AllocateMesh(Graphics, Vertices, sizeof(ak_v3f)*ConvexHull->Header.VertexCount, GRAPHICS_VERTEX_FORMAT_P3,
                                           Indices, Result.IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);    
    GlobalArena->EndTemp(&TempArena);        
    return Result;
}