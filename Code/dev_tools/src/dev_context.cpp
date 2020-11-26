DEV_RENDER_GRID_CALLBACK(DevContext_RenderGrid);

mesh_info DevContext_GetMeshInfoFromDevMesh(dev_mesh* DevMesh)
{
    mesh_info MeshInfo;
    MeshInfo.Header.VertexCount = DevMesh->VertexCount;
    MeshInfo.Header.IndexCount = DevMesh->IndexCount;
    MeshInfo.Header.IsIndexFormat32 = false;
    MeshInfo.Header.IsSkeletalMesh = false;    
    return MeshInfo;
}

mesh DevContext_GetMeshFromDevMesh(dev_mesh* DevMesh)
{
    mesh Mesh;
    Mesh.Vertices = DevMesh->Vertices;
    Mesh.Indices = DevMesh->Indices;    
    return Mesh;
}

graphics_mesh_id DevContext_AllocateMesh(graphics* Graphics, ak_mesh_result<ak_vertex_p3>* Mesh)
{
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(ak_vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                                     Mesh->Indices, Mesh->IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

void DevContext_CreateLineSphereMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateLineSphere(GlobalArena, 1.0f, CircleSampleCount);        
    DevContext->LineSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineSphereMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleCircleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Height)
{
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCircle(DevContext->DevStorage, 1.0f, Height, CircleSampleCount);        
    DevContext->TriangleCircleMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCircleMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleCircleMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleCircleMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleCircleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void DevContext_CreateTriangleTorusMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Width)
{
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleTorus(DevContext->DevStorage, 1.0f, Width, CircleSampleCount);        
    DevContext->TriangleTorusMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleTorusMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleTorusMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleTorusMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleTorusMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void DevContext_CreateLineBoxMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateLineBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));    
    DevContext->LineBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineBoxMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleBoxMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));       
    DevContext->TriangleBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleBoxMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleSphereMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleSphere(GlobalArena, 1.0f, 2);
    DevContext->TriangleSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleSphereMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleCylinderMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCylinder(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleCylinderMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCylinderMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleConeMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_GenerateTriangleCone(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleConeMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleConeMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleArrowMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 ArrowRadius, ak_f32 ArrowHeight)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result<ak_vertex_p3> ArrowResult = AK_GenerateTriangleCone(GlobalArena, ArrowRadius, ArrowHeight, CircleSampleCount, AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(DevContext->DevStorage, BodyResult.VertexCount+ArrowResult.VertexCount, 
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
    DevContext->TriangleArrowMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleScaleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 CubeSize)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result<ak_vertex_p3> BodyResult = AK_GenerateTriangleCylinder(GlobalArena, Radius, Height, CircleSampleCount);
    ak_mesh_result<ak_vertex_p3> BoxResult = AK_GenerateTriangleBox(GlobalArena, AK_V3f(CubeSize, CubeSize, CubeSize), AK_V3(0.0f, 0.0f, Height));
    
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(DevContext->DevStorage, BodyResult.VertexCount+BoxResult.VertexCount, 
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
    DevContext->TriangleScaleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateLineCapsuleMesh(dev_context* DevContext, ak_f32 Radius, ak_u16 CircleSampleCount)
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
    
    DevContext->LineCapsuleMesh.CapIndexCount = CapResult.IndexCount;
    DevContext->LineCapsuleMesh.CapVertexCount = CapResult.VertexCount;
    DevContext->LineCapsuleMesh.BodyIndexCount = BodyResult.IndexCount;
    DevContext->LineCapsuleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}


void DevContext_CreatePlaneMesh(dev_context* DevContext, ak_f32 Width, ak_f32 Height)
{         
    ak_mesh_result<ak_vertex_p3> MeshGenerationResult = AK_AllocateMeshResult<ak_vertex_p3>(DevContext->DevStorage, 4, 6);
    
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
    
    DevContext->TrianglePlaneMesh.IndexCount  = MeshGenerationResult.IndexCount;
    DevContext->TrianglePlaneMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TrianglePlaneMesh.Vertices    = MeshGenerationResult.Vertices;
    DevContext->TrianglePlaneMesh.Indices     = MeshGenerationResult.Indices;
    DevContext->TrianglePlaneMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

dev_slim_mesh DevContext_CreateConvexHullMesh(dev_context* DevContext, convex_hull* ConvexHull)
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
    Result.MeshID = DevContext->Graphics->AllocateMesh(DevContext->Graphics, Vertices, sizeof(ak_v3f)*ConvexHull->Header.VertexCount, GRAPHICS_VERTEX_FORMAT_P3,
                                                       Indices, Result.IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);    
    GlobalArena->EndTemp(&TempArena);        
    return Result;
}

void DevContext_SetEntityAsSelectedObject(dev_selected_object* SelectedObject, world_id ID, material* Material)
{
    SelectedObject->Type = DEV_SELECTED_OBJECT_TYPE_ENTITY;
    SelectedObject->EntityID = ID;
    SelectedObject->MaterialContext = DevUI_ContextFromMaterial(Material);    
}

void DevContext_UpdateObjectOrientation(ak_quatf* Orientation, ak_v3f* OldEuler, ak_v3f NewEuler)
{
    *OldEuler = NewEuler;
    *Orientation = AK_EulerToQuat(NewEuler.roll, NewEuler.pitch, NewEuler.yaw);
}

ak_u32 DevContext_GetSelectedObjectWorldIndex(dev_selected_object* SelectedObject)
{
    AK_Assert(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE, "There is no selected object. Cannot retrieve world index");
    switch(SelectedObject->Type)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:
        return SelectedObject->EntityID.WorldIndex;
        
        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
        return SelectedObject->PointLightID.WorldIndex;
        
        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
        return SelectedObject->JumpingQuadID.WorldIndex;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (ak_u32)-1;
}

ak_v3f DevContext_GetSelectedObjectPosition(world* World, dev_selected_object* SelectedObject)
{    
    AK_Assert(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE, "There is no selected object. Cannot retrieve position");
    switch(SelectedObject->Type)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:          
        return World->NewTransforms[SelectedObject->EntityID.WorldIndex][AK_PoolIndex(SelectedObject->EntityID.ID)].Translation;            
        
        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:         
        return World->GraphicsStates[SelectedObject->PointLightID.WorldIndex].PointLightStorage.Get(SelectedObject->PointLightID.ID)->Position;
        
        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
        return World->JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(SelectedObject->JumpingQuadID.ID)->CenterP;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return {};
}

void DevContext_PopulateNonRotationGizmos(dev_gizmo_state* GizmoState, dev_mesh* GizmoMesh, dev_mesh* TrianglePlaneMesh, ak_v3f Position)
{    
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_X;
        GizmoState->Gizmos[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_Y;
        GizmoState->Gizmos[1] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_Z;
        GizmoState->Gizmos[2] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(DEV_GIZMO_PLANE_DISTANCE, DEV_GIZMO_PLANE_DISTANCE, 0.0f), X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_XY;
        GizmoState->Gizmos[3] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(DEV_GIZMO_PLANE_DISTANCE, 0.0f, DEV_GIZMO_PLANE_DISTANCE), X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_XZ;
        GizmoState->Gizmos[4] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + AK_V3(0.0f, DEV_GIZMO_PLANE_DISTANCE, DEV_GIZMO_PLANE_DISTANCE), X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_V3f(1, 0, 0);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_YZ;
        GizmoState->Gizmos[5] =  Gizmo;
    }    
}

void DevContext_PopulateRotationGizmos(dev_gizmo_state* GizmoState, dev_mesh* TriangleTorusMesh, ak_v3f Position)
{
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_XAxis();
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_X;
        GizmoState->Gizmos[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_YAxis();
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_Y;
        GizmoState->Gizmos[1] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        dev_gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_ZAxis();
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_Z;
        GizmoState->Gizmos[2] =  Gizmo;
    }
    
    GizmoState->Gizmos[3] = {};                                                                
    GizmoState->Gizmos[4] = {};                                                                 
    GizmoState->Gizmos[5] = {};                
}

void DevContext_AddToDevTransform(ak_array<dev_transform>* DevTransformsArray, world* World, world_id EntityID)
{    
    ak_array<dev_transform>* DevTransforms = &DevTransformsArray[EntityID.WorldIndex];
    ak_u32 Index = AK_PoolIndex(EntityID.ID);
    if((Index+1) > DevTransforms->Size)
        DevTransforms->Resize(Index+1);
    
    dev_transform* DevTransform = DevTransforms->Get(Index);    
    ak_sqtf Transform = World->NewTransforms[EntityID.WorldIndex][Index];    
    DevTransform->Translation = Transform.Translation;
    DevTransform->Scale = Transform.Scale;
    DevTransform->Euler = AK_QuatToEuler(Transform.Orientation);    
}

void DevContext_WriteName(ak_file_handle* FileHandle, ak_char* Name, ak_u32 NameLength)
{
    AK_WriteFile(FileHandle, &NameLength, sizeof(NameLength));
    AK_WriteFile(FileHandle, Name, sizeof(ak_char)*NameLength);                            
}

void DevContext_WriteMaterial(assets* Assets, ak_file_handle* FileHandle, material* Material)
{        
    AK_WriteFile(FileHandle, &Material->Diffuse.IsTexture, sizeof(Material->Diffuse.IsTexture));
    if(Material->Diffuse.IsTexture)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Diffuse.DiffuseID);            
        DevContext_WriteName(FileHandle, TextureInfo->Name, TextureInfo->Header.NameLength);        
    }
    else    
        AK_WriteFile(FileHandle, &Material->Diffuse.Diffuse, sizeof(Material->Diffuse.Diffuse));    
    
    AK_WriteFile(FileHandle, &Material->Specular.InUse, sizeof(Material->Specular.InUse));    
    if(Material->Specular.InUse)
    {
        AK_WriteFile(FileHandle, &Material->Specular.IsTexture, sizeof(Material->Specular.IsTexture));
        if(Material->Specular.IsTexture)
        {
            texture_info* TextureInfo = GetTextureInfo(Assets, Material->Specular.SpecularID); 
            DevContext_WriteName(FileHandle, TextureInfo->Name, TextureInfo->Header.NameLength);            
        }
        else        
            AK_WriteFile(FileHandle, &Material->Specular.Specular, sizeof(Material->Specular.Specular));        
        AK_WriteFile(FileHandle, &Material->Specular.Shininess, sizeof(Material->Specular.Shininess));
    }
    
    AK_WriteFile(FileHandle, &Material->Normal.InUse, sizeof(Material->Normal.InUse));    
    if(Material->Normal.InUse)
    {
        texture_info* TextureInfo = GetTextureInfo(Assets, Material->Normal.NormalID); 
        DevContext_WriteName(FileHandle, TextureInfo->Name, TextureInfo->Header.NameLength);        
    }                        
}

ak_string DevContext_ReadAssetName(ak_stream* Stream, ak_arena* Arena)
{    
    ak_string Result;
    Result.Length = *Stream->Read<ak_u32>();
    Result.Data = Arena->PushArray<char>(Result.Length+1);
    Result.Data[Result.Length] = 0;
    AK_MemoryCopy(Result.Data, Stream->ReadArray<char>(Result.Length), Result.Length);
    return Result;
}

ak_bool DevContext_ReadMaterial(ak_stream* Stream, assets* Assets, material* Material)
{
    Material->Diffuse.IsTexture = *Stream->Read<ak_bool>();
    if(Material->Diffuse.IsTexture)
    {
        texture_asset_id* ID = Assets->TextureNameMap.Find(DevContext_ReadAssetName(Stream, AK_GetGlobalArena()).Data);
        if(!ID)
            return false;        
        Material->Diffuse.DiffuseID = *ID;
    }
    else
        Material->Diffuse.Diffuse = *Stream->Read<ak_color3f>();
    
    Material->Specular.InUse = *Stream->Read<ak_bool>();
    if(Material->Specular.InUse)
    {
        Material->Specular.IsTexture = *Stream->Read<ak_bool>();
        if(Material->Specular.IsTexture)
        {
            texture_asset_id* ID = Assets->TextureNameMap.Find(DevContext_ReadAssetName(Stream, AK_GetGlobalArena()).Data);
            if(!ID)
                return false;
            Material->Specular.SpecularID = *ID;
        }        
        else
            Material->Specular.Specular = *Stream->Read<ak_f32>();        
        Material->Specular.Shininess = *Stream->Read<ak_i32>();
    }
    
    Material->Normal.InUse = *Stream->Read<ak_bool>();
    if(Material->Normal.InUse)
    {
        texture_asset_id* ID = Assets->TextureNameMap.Find(DevContext_ReadAssetName(Stream, AK_GetGlobalArena()).Data);
        if(!ID)
            return false;
        Material->Normal.NormalID = *ID;
    }
    
    return true;
}

#define LoadWorld_Error(message, ...) do \
{ \
    AK_Free(LoadedWorldFile.Data); DeleteWorld(&World, DevContext->Graphics); AK_DeleteArray(&DevTransforms[0]); AK_DeleteArray(&DevTransforms[1]); AK_MessageBoxOk("Load World Error", AK_FormatString(GlobalArena, message, __VA_ARGS__)); \
    GlobalArena->EndTemp(&TempArena); \
    return false; \
} while(0)

ak_bool DevContext_LoadWorld(dev_context* DevContext, dev_loaded_world* LoadedWorld, ak_string LoadedWorldFile)
{
    game* Game = DevContext->Game;
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_buffer WorldBuffer = AK_ReadEntireFile(LoadedWorldFile, GlobalArena);        
    
    ak_array<dev_transform> DevTransforms[2] = {};
    world World = {};        
    
    if(!WorldBuffer.IsValid())    
        LoadWorld_Error("Error loading world file %s, message: %s", LoadedWorldFile.Data, AK_PlatformGetErrorMessage().Data);                
    
    ak_stream Stream = AK_CreateStream(WorldBuffer.Data, WorldBuffer.Size);
    world_file_header* Header = Stream.Read<world_file_header>();
    if(!AK_StringEquals(Header->Signature, WORLD_FILE_SIGNATURE))    
        LoadWorld_Error("Error validating world file signature. Is the file %s corrupted?", LoadedWorldFile.Data);            
    
    if((Header->MajorVersion != WORLD_FILE_MAJOR_VERSION) || (Header->MinorVersion != WORLD_FILE_MINOR_VERSION))    
        LoadWorld_Error("Error validating world file version. Found version %d.%d, must be %d.%d", Header->MajorVersion, Header->MinorVersion, WORLD_FILE_MAJOR_VERSION, WORLD_FILE_MINOR_VERSION);            
    
    world_id* Entities[2] = {};
    Entities[0] = GlobalArena->PushArray<world_id>(Header->EntityCounts[0]);
    Entities[1] = GlobalArena->PushArray<world_id>(Header->EntityCounts[1]);
    
    world_id* JumpingQuads[2] = {};
    JumpingQuads[0] = GlobalArena->PushArray<world_id>(Header->JumpingQuadCounts[0]);
    JumpingQuads[1] = GlobalArena->PushArray<world_id>(Header->JumpingQuadCounts[1]);
    
    ak_u32* EntityLinkIndexes[2] = {};
    EntityLinkIndexes[0] = GlobalArena->PushArray<ak_u32>(Header->EntityCounts[0]);
    EntityLinkIndexes[1] = GlobalArena->PushArray<ak_u32>(Header->EntityCounts[1]);
    
    ak_u32* QuadLinkIndexes[2] = {};
    QuadLinkIndexes[0] = GlobalArena->PushArray<ak_u32>(Header->JumpingQuadCounts[0]);
    QuadLinkIndexes[1] = GlobalArena->PushArray<ak_u32>(Header->JumpingQuadCounts[1]);
    
    Game->Players[0] = {};
    Game->Players[1] = {};
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        for(ak_u32 EntityIndex = 0; EntityIndex < Header->EntityCounts[WorldIndex]; EntityIndex++)
        {                
            entity_type Type = *Stream.Read<entity_type>();    
            
            const ak_char* MaterialNotLoadedError = "Could not load the material. Could not find asset that the material uses";
            
            world_id EntityID = InvalidWorldID();
            switch(Type)
            {
                case ENTITY_TYPE_PLAYER:
                {
                    EntityLinkIndexes[WorldIndex][EntityIndex] = (ak_u32)-1;
                    
                    ak_v3f Position = *Stream.Read<ak_v3f>();                                                
                    material Material = {};
                    if(!DevContext_ReadMaterial(&Stream, DevContext->Game->Assets, &Material)) LoadWorld_Error(MaterialNotLoadedError);                                            
                    
                    EntityID = CreatePlayerEntity(&World, Game->Assets, WorldIndex, Position, Material, &Game->Players[WorldIndex]);                                                
                } break;
                
                case ENTITY_TYPE_STATIC:
                {
                    EntityLinkIndexes[WorldIndex][EntityIndex] = (ak_u32)-1;
                    ak_sqtf Transform = *Stream.Read<ak_sqtf>();
                    
                    material Material = {};
                    if(!DevContext_ReadMaterial(&Stream, DevContext->Game->Assets, &Material)) LoadWorld_Error(MaterialNotLoadedError);                    
                    
                    ak_string MeshName = DevContext_ReadAssetName(&Stream, GlobalArena);
                    mesh_asset_id* MeshID = DevContext->Game->Assets->MeshNameMap.Find(MeshName.Data);
                    if(!MeshID) LoadWorld_Error("Could not load mesh with name %s", MeshName.Data);                    
                    
                    EntityID = CreateStaticEntity(&World, Game->Assets, WorldIndex, Transform.Translation, Transform.Scale, Transform.Orientation, 
                                                  *MeshID, Material);                        
                } break;
                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    EntityLinkIndexes[WorldIndex][EntityIndex] = (ak_u32)-1;
                    ak_sqtf Transform = *Stream.Read<ak_sqtf>();
                    
                    material Material = {};
                    if(!DevContext_ReadMaterial(&Stream, DevContext->Game->Assets, &Material)) LoadWorld_Error(MaterialNotLoadedError);                    
                    
                    ak_f32 Mass = *Stream.Read<ak_f32>();
                    ak_f32 Restitution = *Stream.Read<ak_f32>();
                    ak_f32 Radius = *Stream.Read<ak_f32>();
                    
                    EntityID = CreateSphereRigidBody(&World, Game->Assets, WorldIndex, Transform.Translation, Transform.Scale, Transform.Orientation, 
                                                     Radius, Mass, Restitution, Material);                    
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {
                    EntityLinkIndexes[WorldIndex][EntityIndex] = *Stream.Read<ak_u32>();
                    ak_sqtf Transform = *Stream.Read<ak_sqtf>();
                    
                    material Material = {};
                    if(!DevContext_ReadMaterial(&Stream, DevContext->Game->Assets, &Material)) LoadWorld_Error(MaterialNotLoadedError);                    
                    
                    ak_f32 Mass = *Stream.Read<ak_f32>();
                    EntityID = CreatePushableBox(&World, Game->Assets, WorldIndex, Transform.Translation, Transform.Scale, Transform.Orientation, Mass, Material, true);                    
                } break;
                
                AK_INVALID_DEFAULT_CASE;
            }
            
            Entities[WorldIndex][EntityIndex] = EntityID;
            DevContext_AddToDevTransform(DevTransforms, &World, EntityID);                        
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        for(ak_u32 EntityIndex = 0; EntityIndex < Header->JumpingQuadCounts[WorldIndex]; EntityIndex++)
        { 
            ak_v3f CenterP = *Stream.Read<ak_v3f>();
            ak_v2f Dimensions = *Stream.Read<ak_v2f>();
            QuadLinkIndexes[WorldIndex][EntityIndex] = *Stream.Read<ak_u32>();            
            world_id JumpingQuadID = CreateJumpingQuad(&World, WorldIndex, CenterP, Dimensions);            
            JumpingQuads[WorldIndex][EntityIndex] = JumpingQuadID;
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_u32 EntityIndex = 0;
        AK_ForEach(Entity, &World.EntityStorage[WorldIndex])
        {
            ak_u32 LinkIndex = EntityLinkIndexes[WorldIndex][EntityIndex];
            if(LinkIndex != (ak_u32)-1)
            {
                world_id LinkEntityID = Entities[!WorldIndex][LinkIndex];
                Entity->LinkID = LinkEntityID;
            }
            EntityIndex++;
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        ak_u32 JumpingQuadIndex = 0;
        AK_ForEach(JumpingQuad, &World.JumpingQuadStorage[WorldIndex])
        {
            ak_u32 LinkIndex = QuadLinkIndexes[WorldIndex][JumpingQuadIndex];
            world_id LinkID = JumpingQuads[WorldIndex][LinkIndex];            
            JumpingQuad->OtherQuad = LinkID;
            JumpingQuadIndex++;
        }
    }
    
    for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
    {
        for(ak_u32 PointLightIndex = 0; PointLightIndex < Header->PointLightCounts[WorldIndex]; PointLightIndex++)
        {
            ak_v3f Position = *Stream.Read<ak_v3f>();
            ak_f32 Radius = *Stream.Read<ak_f32>();
            ak_color3f Color = *Stream.Read<ak_color3f>();
            ak_f32 Intensity = *Stream.Read<ak_f32>();
            ak_bool On = *Stream.Read<ak_bool>();                
            CreatePointLight(&World.GraphicsStates[WorldIndex], Position, Radius, Color, Intensity, On);
        }
    }
    
    DeleteWorld(&Game->World, DevContext->Graphics);
    AK_DeleteArray(&DevContext->InitialTransforms[0]);
    AK_DeleteArray(&DevContext->InitialTransforms[1]);
    
    Game->World = World;
    DevContext->InitialTransforms[0] = DevTransforms[0];
    DevContext->InitialTransforms[1] = DevTransforms[1];
    
    Game->World.NewCameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));    
    Game->World.NewCameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    AK_Free(LoadedWorld->LoadedWorldFile.Data);            
    LoadedWorld->LoadedWorldFile = LoadedWorldFile;
    
    DevContext->SelectedObject = {};
    
    GlobalArena->EndTemp(&TempArena);
    
    return true;
}

ak_bool DevContext_LoadWorld(dev_context* DevContext, dev_loaded_world* LoadedWorld)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_bool Result = false;
    
    DevContext->DevInput = {};
    ak_string LoadedWorldFile = AK_OpenFileDialog("world");    
    if(!AK_StringIsNullOrEmpty(LoadedWorldFile))
    {        
        Result = DevContext_LoadWorld(DevContext, LoadedWorld, LoadedWorldFile);
    }
    GlobalArena->EndTemp(&TempArena);
    
    return Result;
}
#undef LoadWorld_Error

ak_bool DevContext_SaveWorld(dev_context* DevContext, dev_loaded_world* LoadedWorld, ak_bool SaveNewWorld)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    if(!SaveNewWorld)
        SaveNewWorld = AK_StringIsNullOrEmpty(LoadedWorld->LoadedWorldFile);
    
    ak_string LoadedWorldFile = AK_CreateEmptyString();
    if(SaveNewWorld)        
    {        
        DevContext->DevInput = {};
        LoadedWorldFile = AK_SaveFileDialog("world");        
    }
    else
    {
        
        LoadedWorldFile.Data = (ak_char*)AK_Allocate(sizeof(ak_char)*LoadedWorld->LoadedWorldFile.Length+1);
        LoadedWorldFile.Length = LoadedWorld->LoadedWorldFile.Length;
        LoadedWorldFile.Data[LoadedWorldFile.Length] = 0;
        AK_MemoryCopy(LoadedWorldFile.Data, LoadedWorld->LoadedWorldFile.Data, LoadedWorldFile.Length*sizeof(ak_char));
    }
    
    if(AK_StringIsNullOrEmpty(LoadedWorldFile))
    {
        GlobalArena->EndTemp(&TempArena);
        return false;
    }
    else
    {
        ak_file_handle* FileHandle = AK_OpenFile(LoadedWorldFile, AK_FILE_ATTRIBUTES_WRITE);
        if(!FileHandle)
        {              
            AK_Free(LoadedWorldFile.Data);
            GlobalArena->EndTemp(&TempArena);
            AK_MessageBoxOk("FileIO Error", AK_FormatString(GlobalArena, "Failed to open world file at location %s, message: %s\n", 
                                                            LoadedWorldFile.Data, AK_PlatformGetErrorMessage().Data));
            return false;
        }
        
        world* World = &DevContext->Game->World;
        ak_hash_map<ak_u64, ak_u32> EntityMap[2] = {};
        ak_hash_map<ak_u64, ak_u32> JumpingQuadMap[2] = {};
        
        world_file_header FileHeader = {};
        AK_MemoryCopy(FileHeader.Signature, WORLD_FILE_SIGNATURE, sizeof(WORLD_FILE_SIGNATURE));
        FileHeader.MajorVersion = WORLD_FILE_MAJOR_VERSION;
        FileHeader.MinorVersion = WORLD_FILE_MINOR_VERSION;
        FileHeader.EntityCounts[0] = World->EntityStorage[0].Size;
        FileHeader.EntityCounts[1] = World->EntityStorage[1].Size;
        FileHeader.JumpingQuadCounts[0] = World->JumpingQuadStorage[0].Size;
        FileHeader.JumpingQuadCounts[1] = World->JumpingQuadStorage[1].Size;
        FileHeader.PointLightCounts[0] = World->GraphicsStates[0].PointLightStorage.Size;
        FileHeader.PointLightCounts[1] = World->GraphicsStates[1].PointLightStorage.Size;
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {       
            ak_u32 EntityIndex = 0;                
            AK_ForEach(Entity, &World->EntityStorage[WorldIndex])
            {                    
                EntityMap[WorldIndex].Insert(Entity->ID.ID, EntityIndex++);   
            }
            
            ak_u32 JumpingQuadIndex = 0;
            AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[WorldIndex])
            {
                JumpingQuadMap[WorldIndex].Insert(JumpingQuad->OtherQuad.ID, JumpingQuadIndex++);
            }
        }
        
        AK_WriteFile(FileHandle, &FileHeader, sizeof(FileHeader));            
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {       
            ak_u32 EntityIndex = 0;                
            AK_ForEach(Entity, &World->EntityStorage[WorldIndex])
            {
                AK_WriteFile(FileHandle, &Entity->Type, sizeof(Entity->Type));
                
                world_id ID = Entity->ID;
                graphics_entity* GraphicsEntity = World->GraphicsStates[WorldIndex].GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
                switch(Entity->Type)
                {                        
                    case ENTITY_TYPE_PLAYER:
                    {   
                        ak_v3f Position = World->NewTransforms[ID.WorldIndex][AK_PoolIndex(ID.ID)].Translation;
                        AK_WriteFile(FileHandle, &Position, sizeof(Position));                                                        
                        DevContext_WriteMaterial(DevContext->Game->Assets, FileHandle, &GraphicsEntity->Material);
                    } break;
                    
                    case ENTITY_TYPE_STATIC:
                    {                        
                        ak_sqtf Transform = World->NewTransforms[ID.WorldIndex][AK_PoolIndex(ID.ID)];
                        AK_WriteFile(FileHandle, &Transform, sizeof(Transform));
                        DevContext_WriteMaterial(DevContext->Game->Assets, FileHandle, &GraphicsEntity->Material);
                        
                        mesh_info* MeshInfo = GetMeshInfo(DevContext->Game->Assets, GraphicsEntity->MeshID);
                        DevContext_WriteName(FileHandle, MeshInfo->Name, MeshInfo->Header.NameLength);                            
                    } break;
                    
                    case ENTITY_TYPE_RIGID_BODY:
                    {
                        ak_sqtf Transform = World->NewTransforms[ID.WorldIndex][AK_PoolIndex(ID.ID)];
                        AK_WriteFile(FileHandle, &Transform, sizeof(Transform));
                        DevContext_WriteMaterial(DevContext->Game->Assets, FileHandle, &GraphicsEntity->Material);
                        
                        rigid_body* RigidBody = World->Simulations[WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                        collision_volume* Volume = World->Simulations[WorldIndex].CollisionVolumeStorage.Get(RigidBody->CollisionVolumeID);
                        
                        AK_Assert(Volume->Type == COLLISION_VOLUME_TYPE_SPHERE, "We only support sphere collision volumes on rigid bodies for now");
                        
                        ak_f32 Mass = 1.0f/RigidBody->InvMass;
                        AK_WriteFile(FileHandle, &Mass, sizeof(Mass));
                        AK_WriteFile(FileHandle, &RigidBody->Restitution, sizeof(RigidBody->Restitution));       
                        AK_WriteFile(FileHandle, &Volume->Sphere.Radius, sizeof(Volume->Sphere.Radius));                        
                    } break;
                    
                    case ENTITY_TYPE_PUSHABLE:
                    {
                        ak_u32 LinkIndex = (ak_u32)-1;
                        if(Entity->LinkID.IsValid())
                            LinkIndex = *EntityMap[!WorldIndex].Find(Entity->LinkID.ID);
                        
                        AK_WriteFile(FileHandle, &LinkIndex, sizeof(LinkIndex));
                        ak_sqtf Transform = World->NewTransforms[ID.WorldIndex][AK_PoolIndex(ID.ID)];
                        AK_WriteFile(FileHandle, &Transform, sizeof(Transform));
                        DevContext_WriteMaterial(DevContext->Game->Assets, FileHandle, &GraphicsEntity->Material);
                        
                        rigid_body* RigidBody = World->Simulations[WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                        ak_f32 Mass = 1.0f/RigidBody->InvMass;
                        AK_WriteFile(FileHandle, &Mass, sizeof(Mass));
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
            }                                                
        }
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[WorldIndex])
            {
                ak_u32 OtherQuadIndex = *JumpingQuadMap[WorldIndex].Find(JumpingQuad->ID);                
                AK_WriteFile(FileHandle, &JumpingQuad->CenterP, sizeof(JumpingQuad->CenterP));
                AK_WriteFile(FileHandle, &JumpingQuad->Dimensions, sizeof(JumpingQuad->Dimensions));
                AK_WriteFile(FileHandle, &OtherQuadIndex, sizeof(OtherQuadIndex));
            }
        }
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            graphics_state* GraphicsState = World->GraphicsStates + WorldIndex;
            AK_ForEach(PointLight, &GraphicsState->PointLightStorage)
            {
                AK_WriteFile(FileHandle, &PointLight->Position, sizeof(PointLight->Position));
                AK_WriteFile(FileHandle, &PointLight->Radius, sizeof(PointLight->Radius));
                AK_WriteFile(FileHandle, &PointLight->Color, sizeof(PointLight->Color));
                AK_WriteFile(FileHandle, &PointLight->Intensity, sizeof(PointLight->Intensity));
                AK_WriteFile(FileHandle, &PointLight->On, sizeof(PointLight->On));
            }
        }
        
        AK_CloseFile(FileHandle);
        
        AK_Free(LoadedWorld->LoadedWorldFile.Data);            
        LoadedWorld->LoadedWorldFile = LoadedWorldFile;            
        
        AK_DeleteHashMap(&EntityMap[0]);
        AK_DeleteHashMap(&EntityMap[1]);
    }
    
    
    GlobalArena->EndTemp(&TempArena);
    
    return true;
}

void DevContext_SetDefaultWorld(dev_context* DevContext, dev_loaded_world* LoadedWorld)
{
    if(!AK_StringIsNullOrEmpty(LoadedWorld->LoadedWorldFile))
    {
        AK_WriteEntireFile(DevContext->DefaultWorldFilePathName, LoadedWorld->LoadedWorldFile.Data, LoadedWorld->LoadedWorldFile.Length);
        
    }
    else
    {
        if(DevContext_SaveWorld(DevContext, LoadedWorld, true))
            AK_WriteEntireFile(DevContext->DefaultWorldFilePathName, LoadedWorld->LoadedWorldFile.Data, LoadedWorld->LoadedWorldFile.Length);
    }
}

void DevContext_UpdateEntityIdsInStack(ak_array<dev_object_edit> Stack, world_id OldEntityID, world_id NewEntityId)
{
    AK_ForEach(EditObject, &Stack)
    {
        if(AreEqualIDs(EditObject->Entity[0].ID, OldEntityID))
        {
            EditObject->Entity[0].ID = NewEntityId;
        }

        if(AreEqualIDs(EditObject->Entity[1].ID, OldEntityID))
        {
            EditObject->Entity[1].ID = NewEntityId;
        }
    }
}

void DevContext_UpdateJumpEntityIdsInStack(ak_array<dev_object_edit> Stack, world_id OldEntityID, world_id NewEntityId)
{
    AK_ForEach(EditObject, &Stack)
    {
        if(AreEqualIDs(EditObject->JumpIds[0].A, OldEntityID))
        {
            EditObject->JumpIds[0].A = NewEntityId;
        }

        if(AreEqualIDs(EditObject->JumpIds[1].A, OldEntityID))
        {
            EditObject->JumpIds[1].A = NewEntityId;
        }

        if(AreEqualIDs(EditObject->JumpIds[0].B, OldEntityID))
        {
            EditObject->JumpIds[0].B = NewEntityId;
        }

        if(AreEqualIDs(EditObject->JumpIds[1].B, OldEntityID))
        {
            EditObject->JumpIds[1].B = NewEntityId;
        }
    }
}

void DevContext_UpdateLightIdsInStack(ak_array<dev_object_edit> Stack, world_id OldEntityID, world_id NewEntityId)
{
    AK_ForEach(EditObject, &Stack)
    {
        if(AreEqualIDs(EditObject->LightIds[0], OldEntityID))
        {
            EditObject->LightIds[0] = NewEntityId;
        }

        if(AreEqualIDs(EditObject->LightIds[1], OldEntityID))
        {
            EditObject->LightIds[1] = NewEntityId;
        }
    }
}

void DevContext_UndoEntityEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    world_id EntityID = LastEdit.Entity[0].ID;
    world_id CreatedEntity = {};
    
    dev_object_edit Redo;
    Redo.Entity[0] = LastEdit.Entity[0];
    Redo.Entity[1] = LastEdit.Entity[1];
    Redo.ObjectEditType = LastEdit.ObjectEditType;
    Redo.ObjectType = LastEdit.ObjectType;
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        entity* Entity = World->EntityStorage[EntityID.WorldIndex].Get(EntityID.ID);
        ak_u32 Index = AK_PoolIndex(EntityID.ID);                                        
        ak_array<dev_transform>* DevTransforms = &DevContext->InitialTransforms[EntityID.WorldIndex];                    
        if(DevTransforms->Size < (Index+1))
            DevTransforms->Resize(Index+1);
        
        dev_transform* DevTransform = DevTransforms->Get(Index);                    
        Redo.Transform = *DevTransform;
        *DevTransform = LastEdit.Transform;
        
        ak_sqtf* Transform = &World->NewTransforms[EntityID.WorldIndex][Index];
        Transform->Translation = DevTransform->Translation;
        Transform->Scale = DevTransform->Scale;
        Transform->Orientation = AK_Normalize(AK_EulerToQuat(DevTransform->Euler));
        
        World->OldTransforms[EntityID.WorldIndex][Index] = *Transform;                        
        
        sim_entity* SimEntity = GetSimEntity(Game, EntityID);
        SimEntity->Transform = *Transform;                    
        
        graphics_state* GraphicsState = &World->GraphicsStates[EntityID.WorldIndex];
        graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
        GraphicsEntity->Transform = AK_TransformM4(*Transform);
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        world_id LinkedEntity = LastEdit.Entity[0].LinkID;
        Redo = LastEdit;
        switch(LastEdit.Entity[0].Type)
        {
            case ENTITY_TYPE_STATIC:
            {
                CreatedEntity = CreateStaticEntity(World, Game->Assets, EntityID.WorldIndex, 
                                                   LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                   LastEdit.MeshID, LastEdit.Material);
            } break;
            case ENTITY_TYPE_RIGID_BODY:
            {
                CreatedEntity = CreateSphereRigidBody(World, Game->Assets, EntityID.WorldIndex, 
                                                      LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                      1.0f, LastEdit.Mass, LastEdit.Restitution, LastEdit.Material);
            } break;
            case ENTITY_TYPE_PUSHABLE:
            {
                CreatedEntity = CreatePushableBox(World, Game->Assets, EntityID.WorldIndex, 
                                                  LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                  LastEdit.Mass, LastEdit.Material, LastEdit.Interactable);
            } break;
            AK_INVALID_DEFAULT_CASE;
        }
        if(LinkedEntity.IsValid())
        {
            entity* EntityA = Game->World.EntityStorage[CreatedEntity.WorldIndex].Get(CreatedEntity.ID);
            entity* EntityB = Game->World.EntityStorage[LinkedEntity.WorldIndex].Get(LinkedEntity.ID);
            
            EntityA->LinkID = LinkedEntity;
            EntityB->LinkID = CreatedEntity;
        }
        DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, CreatedEntity);
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        entity* Entity = GetEntity(Game, EntityID);
        ak_array<dev_transform>* DevTransforms = &DevContext->InitialTransforms[Entity->ID.WorldIndex];
        ak_u32 Index = AK_PoolIndex(Entity->ID.ID);  
        if(DevTransforms->Size < (Index+1))
            DevTransforms->Resize(Index+1);
        dev_transform* DevTransform = DevTransforms->Get(Index);   
        graphics_state* GraphicsState = &World->GraphicsStates[Entity->ID.WorldIndex];
        graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
        Redo.Material = GraphicsEntity->Material;
        Redo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_CREATE;
        Redo.Transform = *DevTransform;
        switch(Entity->Type)
        {
            case ENTITY_TYPE_STATIC:
            {
                Redo.MeshID = GraphicsEntity->MeshID;
            } break;
            case ENTITY_TYPE_RIGID_BODY:
            {
                rigid_body* RigidBody = GetSimEntity(Game, Entity->ID)->ToRigidBody();
                Redo.Mass = 1.0f / RigidBody->InvMass;
                Redo.Restitution = RigidBody->Restitution;
            } break;
            case ENTITY_TYPE_PUSHABLE:
            {
                rigid_body* RigidBody = GetSimEntity(Game, Entity->ID)->ToRigidBody();
                Redo.Mass = 1.0f / RigidBody->InvMass;
            } break;
        }
        FreeEntity(Game, EntityID);
        if(LastEdit.Entity[1].ID.IsValid())
        {
            FreeEntity(Game, LastEdit.Entity[1].ID);
        }
        if(AreEqualIDs(DevContext->SelectedObject.EntityID, EntityID))
        {
            DevContext->SelectedObject = {};
        }
    }
    DevContext->RedoStack.Add(Redo);
    if(CreatedEntity.IsValid())
    {
        DevContext_UpdateEntityIdsInStack(DevContext->UndoStack, EntityID, CreatedEntity);
        DevContext_UpdateEntityIdsInStack(DevContext->RedoStack, EntityID, CreatedEntity);
    }
}

void DevContext_UndoJumpEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    dev_object_edit Redo;
    dual_world_id AIDs;
    AIDs.A = InvalidWorldID();
    AIDs.B = InvalidWorldID();
    Redo = LastEdit;
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        if( AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[0].A) || 
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[0].B) ||
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[1].A) ||
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[1].B))
        {
            DevContext->SelectedObject = {};
        }
        FreeJumpingQuad(World, LastEdit.JumpIds[0].A);
        if(LastEdit.JumpIds[1].A.IsValid())
        {
            FreeJumpingQuad(World, LastEdit.JumpIds[1].A);
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        ak_v3f Translations[2];
        Translations[0] = Redo.JumpProp[0].CenterP;
        Translations[1] = Redo.JumpProp[1].CenterP;
        AIDs = CreateJumpingQuads(World, Redo.JumpIds[0].A.WorldIndex, Translations, Redo.JumpProp->Dimensions);
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        jumping_quad* JumpingQuad = World->JumpingQuadStorage[LastEdit.JumpIds[0].A.WorldIndex].Get(LastEdit.JumpIds[0].A.ID);  
        Redo.JumpProp[0].CenterP = JumpingQuad->CenterP;
        JumpingQuad->CenterP = LastEdit.JumpProp->CenterP;
    }

    DevContext->RedoStack.Add(Redo);
    if(AIDs.A.IsValid())
    {
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[0].A, AIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[0].A, AIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[0].B, AIDs.B);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[0].B, AIDs.B);
    }
}

void DevContext_UndoLightEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    dev_object_edit Redo;
    world_id AID = InvalidWorldID();
    Redo = LastEdit;
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        if( AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.LightIds[0]) || 
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.LightIds[1]))
        {
            DevContext->SelectedObject = {};
        }
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        GraphicsState->PointLightStorage.Free(LastEdit.LightIds[0].ID);
        if(LastEdit.LightIds[1].IsValid())
        {
            GraphicsState = &World->GraphicsStates[LastEdit.LightIds[1].WorldIndex];
            GraphicsState->PointLightStorage.Free(LastEdit.LightIds[1].ID);
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        AID.ID = CreatePointLight(GraphicsState, LastEdit.LightProp.Position, LastEdit.LightProp.Radius, LastEdit.LightProp.Color, LastEdit.LightProp.Intensity, LastEdit.LightProp.On);
        AID.WorldIndex = LastEdit.LightIds[0].WorldIndex;
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        point_light* LightProp = GraphicsState->PointLightStorage.Get(LastEdit.LightIds[0].ID);
        Redo.LightProp = *LightProp;
        LightProp->Color = LastEdit.LightProp.Color;
        LightProp->Intensity = LastEdit.LightProp.Intensity;
        LightProp->On = LastEdit.LightProp.On;
        LightProp->Position = LastEdit.LightProp.Position;
        LightProp->Radius = LastEdit.LightProp.Radius;
    }

    DevContext->RedoStack.Add(Redo);
    if(AID.IsValid())
    {
        DevContext_UpdateLightIdsInStack(DevContext->UndoStack, LastEdit.LightIds[0], AID);
        DevContext_UpdateLightIdsInStack(DevContext->RedoStack, LastEdit.LightIds[0], AID);
    }
}

void DevContext_UndoLastEdit(dev_context* DevContext)
{  
    dev_object_edit* LastEdit = DevContext->UndoStack.Pop();
    
    if(LastEdit == NULL)
    {
        return;
    }
    
    switch(LastEdit->ObjectType)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:
        {
            DevContext_UndoEntityEdit(DevContext, *LastEdit);
        } break;

        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
        {
            DevContext_UndoJumpEdit(DevContext, *LastEdit);
        } break;

        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
        {
            DevContext_UndoLightEdit(DevContext, *LastEdit);
        } break;

        AK_INVALID_DEFAULT_CASE;
    }
}

void DevContext_RedoEntityEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    world_id EntityID = LastEdit.Entity[0].ID;
    world_id CreatedEntities[2];
    
    dev_object_edit Undo;
    Undo.Entity[0] = LastEdit.Entity[0];
    Undo.Entity[1] = LastEdit.Entity[1];
    Undo.ObjectEditType = LastEdit.ObjectEditType;
    Undo.ObjectType = LastEdit.ObjectType;
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        entity* Entity = World->EntityStorage[LastEdit.Entity[0].ID.WorldIndex].Get(LastEdit.Entity[0].ID.ID);
        ak_u32 Index = AK_PoolIndex(LastEdit.Entity[0].ID.ID);                                        
        ak_array<dev_transform>* DevTransforms = &DevContext->InitialTransforms[LastEdit.Entity[0].ID.WorldIndex];                    
        if(DevTransforms->Size < (Index+1))
            DevTransforms->Resize(Index+1);
        
        dev_transform* DevTransform = DevTransforms->Get(Index);                    
        Undo.Transform = *DevTransform;
        *DevTransform = LastEdit.Transform;
        
        ak_sqtf* Transform = &World->NewTransforms[LastEdit.Entity[0].ID.WorldIndex][Index];
        Transform->Translation = DevTransform->Translation;
        Transform->Scale = DevTransform->Scale;
        Transform->Orientation = AK_Normalize(AK_EulerToQuat(DevTransform->Euler));
        
        World->OldTransforms[LastEdit.Entity[0].ID.WorldIndex][Index] = *Transform;                        
        
        sim_entity* SimEntity = GetSimEntity(Game, LastEdit.Entity[0].ID);
        SimEntity->Transform = *Transform;                    
        
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.Entity[0].ID.WorldIndex];
        graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
        GraphicsEntity->Transform = AK_TransformM4(*Transform);
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        Undo = LastEdit;
        FreeEntity(Game, LastEdit.Entity[0].ID);
        if(LastEdit.Entity[1].ID.IsValid())
        {
            FreeEntity(Game, LastEdit.Entity[1].ID);
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        world_id LinkedEntity = LastEdit.Entity[0].LinkID;
        
        Undo = LastEdit;
        for(int i = 0; i < 2; i++)
        {
            if(!LastEdit.Entity[i].ID.IsValid())
            {
                CreatedEntities[i] = InvalidWorldID();
                continue;
            }
            world_id NewEntityID = LastEdit.Entity[i].ID;
            switch(LastEdit.Entity[i].Type)
            {
                case ENTITY_TYPE_STATIC:
                {
                    CreatedEntities[i] = CreateStaticEntity(World, Game->Assets, NewEntityID.WorldIndex, 
                                                    LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                    LastEdit.MeshID, LastEdit.Material);
                } break;
                case ENTITY_TYPE_RIGID_BODY:
                {
                    CreatedEntities[i] = CreateSphereRigidBody(World, Game->Assets, NewEntityID.WorldIndex, 
                                                        LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                        1.0f, LastEdit.Mass, LastEdit.Restitution, LastEdit.Material);
                } break;
                case ENTITY_TYPE_PUSHABLE:
                {
                    CreatedEntities[i] = CreatePushableBox(World, Game->Assets, NewEntityID.WorldIndex, 
                                                    LastEdit.Transform.Translation, LastEdit.Transform.Scale, AK_EulerToQuat(LastEdit.Transform.Euler), 
                                                    LastEdit.Mass, LastEdit.Material, LastEdit.Interactable);
                } break;
                AK_INVALID_DEFAULT_CASE;
            }
        }
        
        for(int i = 0; i < 2; i++)
        {
            if(CreatedEntities[i].IsValid())
            {
                if(LinkedEntity.IsValid())
                {
                    entity* EntityA = Game->World.EntityStorage[CreatedEntities[i].WorldIndex].Get(CreatedEntities[i].ID);
                    entity* EntityB = Game->World.EntityStorage[LinkedEntity.WorldIndex].Get(LinkedEntity.ID);
                    
                    EntityA->LinkID = LinkedEntity;
                    EntityB->LinkID = CreatedEntities[i];
                }
                DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, CreatedEntities[i]);
                DevContext_UpdateEntityIdsInStack(DevContext->UndoStack, LastEdit.Entity[i].ID, CreatedEntities[i]);
                DevContext_UpdateEntityIdsInStack(DevContext->RedoStack, LastEdit.Entity[i].ID, CreatedEntities[i]);
            }
        }
    }
    
    DevContext->UndoStack.Add(Undo);
    for(int i = 0; i < 2; i++)
    {
        if(CreatedEntities[i].IsValid())
        {
            DevContext_UpdateEntityIdsInStack(DevContext->UndoStack, LastEdit.Entity[i].ID, CreatedEntities[i]);
            DevContext_UpdateEntityIdsInStack(DevContext->RedoStack, LastEdit.Entity[i].ID, CreatedEntities[i]);
        }
    }
}

void DevContext_RedoJumpEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    dev_object_edit Undo;
    Undo = LastEdit;
    dual_world_id AIDs;
    AIDs.A = InvalidWorldID();
    AIDs.B = InvalidWorldID();
    dual_world_id BIDs;
    BIDs.A = InvalidWorldID();
    BIDs.B = InvalidWorldID();
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        ak_v3f Translations[2];
        Translations[0] = Undo.JumpProp[0].CenterP;
        Translations[1] = Undo.JumpProp[1].CenterP;
        AIDs = CreateJumpingQuads(World, Undo.JumpIds[0].A.WorldIndex, Translations, Undo.JumpProp->Dimensions);
        if(Undo.JumpIds[1].A.IsValid())
        {
            BIDs = CreateJumpingQuads(World, Undo.JumpIds[1].A.WorldIndex, Translations, Undo.JumpProp->Dimensions);
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        if( AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[0].A) || 
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[0].B) ||
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[1].A) ||
            AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.JumpIds[1].B))
        {
            DevContext->SelectedObject = {};
        }
        FreeJumpingQuad(World, LastEdit.JumpIds[0].A);
        if(LastEdit.JumpIds[1].A.IsValid())
        {
            FreeJumpingQuad(World, LastEdit.JumpIds[1].A);
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        jumping_quad* JumpingQuad = World->JumpingQuadStorage[LastEdit.JumpIds[0].A.WorldIndex].Get(LastEdit.JumpIds[0].A.ID);  
        Undo.JumpProp[0].CenterP = JumpingQuad->CenterP;
        JumpingQuad->CenterP = LastEdit.JumpProp->CenterP;
    }

    DevContext->UndoStack.Add(Undo);
    if(AIDs.A.IsValid())
    {
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[0].A, AIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[0].A, AIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[0].B, AIDs.B);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[0].B, AIDs.B);
    }
    if(BIDs.A.IsValid())
    {
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[1].A, BIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[1].A, BIDs.A);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->UndoStack, LastEdit.JumpIds[1].B, BIDs.B);
        DevContext_UpdateJumpEntityIdsInStack(DevContext->RedoStack, LastEdit.JumpIds[1].B, BIDs.B);
    }
}

void DevContext_RedoLightEdit(dev_context* DevContext, dev_object_edit LastEdit)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;

    dev_object_edit Undo;
    world_id AID = InvalidWorldID();
    world_id BID = InvalidWorldID();
    Undo = LastEdit;
    if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_CREATE)
    {
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        AID.ID = CreatePointLight(GraphicsState, LastEdit.LightProp.Position, LastEdit.LightProp.Radius, LastEdit.LightProp.Color, LastEdit.LightProp.Intensity, LastEdit.LightProp.On);
        AID.WorldIndex = LastEdit.LightIds[0].WorldIndex;
        if(LastEdit.LightIds[1].IsValid())
        {
            GraphicsState = &World->GraphicsStates[LastEdit.LightIds[1].WorldIndex];
            BID.ID = CreatePointLight(GraphicsState, LastEdit.LightProp.Position, LastEdit.LightProp.Radius, LastEdit.LightProp.Color, LastEdit.LightProp.Intensity, LastEdit.LightProp.On);
            BID.WorldIndex = LastEdit.LightIds[1].WorldIndex;
        }
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_DELETE)
    {
        if(AreEqualIDs(DevContext->SelectedObject.EntityID, LastEdit.LightIds[0]))
        {
            DevContext->SelectedObject = {};
        }
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        GraphicsState->PointLightStorage.Free(LastEdit.LightIds[0].ID);
    }
    else if(LastEdit.ObjectEditType == DEV_OBJECT_EDIT_TYPE_TRANSFORM)
    {
        graphics_state* GraphicsState = &World->GraphicsStates[LastEdit.LightIds[0].WorldIndex];
        point_light* LightProp = GraphicsState->PointLightStorage.Get(LastEdit.LightIds[0].ID);
        Undo.LightProp = *LightProp;
        LightProp->Color = LastEdit.LightProp.Color;
        LightProp->Intensity = LastEdit.LightProp.Intensity;
        LightProp->On = LastEdit.LightProp.On;
        LightProp->Position = LastEdit.LightProp.Position;
        LightProp->Radius = LastEdit.LightProp.Radius;
    }

    DevContext->UndoStack.Add(Undo);
    if(AID.IsValid())
    {
        DevContext_UpdateLightIdsInStack(DevContext->UndoStack, LastEdit.LightIds[0], AID);
        DevContext_UpdateLightIdsInStack(DevContext->RedoStack, LastEdit.LightIds[0], AID);
    }
    if(BID.IsValid())
    {
        DevContext_UpdateLightIdsInStack(DevContext->UndoStack, LastEdit.LightIds[1], BID);
        DevContext_UpdateLightIdsInStack(DevContext->RedoStack, LastEdit.LightIds[1], BID);
    }
}

void DevContext_RedoLastEdit(dev_context* DevContext)
{  
    dev_object_edit* LastEdit = DevContext->RedoStack.Pop();
    
    if(LastEdit == NULL)
    {
        return;
    }
    
    switch(LastEdit->ObjectType)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:
        {
            DevContext_RedoEntityEdit(DevContext, *LastEdit);
        } break;

        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
        {
            DevContext_RedoJumpEdit(DevContext, *LastEdit);
        } break;

        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
        {
            DevContext_RedoLightEdit(DevContext, *LastEdit);
        } break;

        AK_INVALID_DEFAULT_CASE;
    }
}

void DevContext_Initialize(game* Game, graphics* Graphics, ak_string ProgramFilePath, void* PlatformWindow, platform_init_imgui* InitImGui, platform_development_update* PlatformUpdate)
{
    ak_arena* DevStorage = AK_CreateArena(AK_Megabyte(1));
    dev_context* DevContext = DevStorage->Push<dev_context>();
    
    DevContext->DevStorage = DevStorage;    
    DevContext->Game = Game;
    DevContext->Graphics = Graphics;
    DevContext->PlatformWindow = PlatformWindow;
    DevContext->PlatformUpdate = PlatformUpdate;
    
    DevContext_CreateLineCapsuleMesh(DevContext, 1.0f, 60);
    DevContext_CreateLineBoxMesh(DevContext);
    DevContext_CreateLineSphereMesh(DevContext, 60);
    DevContext_CreateTriangleBoxMesh(DevContext);
    DevContext_CreateTriangleSphereMesh(DevContext);
    DevContext_CreateTriangleCylinderMesh(DevContext, 60);
    DevContext_CreateTriangleConeMesh(DevContext, 60);
    DevContext_CreateTriangleArrowMesh(DevContext, 60, 0.02f, 0.85f, 0.035f, 0.15f);
    DevContext_CreatePlaneMesh(DevContext, 0.4f, 0.4f);
    DevContext_CreateTriangleCircleMesh(DevContext, 60, 0.05f);
    DevContext_CreateTriangleScaleMesh(DevContext, 60, 0.02f, 0.85f, 0.1f);
    DevContext_CreateTriangleTorusMesh(DevContext, 20, 0.03f);
    
    DevUI_Initialize(&DevContext->DevUI, Graphics, PlatformWindow, InitImGui);        
    
    DevContext->DefaultWorldFilePathName = AK_StringConcat(ProgramFilePath, "DefaultWorld.txt", DevStorage);
    DevContext->RenderGrid = DevContext_RenderGrid;
    
    ak_arena* GlobalArena = AK_GetGlobalArena();    
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_buffer DefaultWorldBuffer = AK_ReadEntireFile(DevContext->DefaultWorldFilePathName, GlobalArena);
    ak_bool LoadDefaultWorld = DefaultWorldBuffer.IsValid();
    if(LoadDefaultWorld)
    {
        ak_string LoadedWorldFile;
        LoadedWorldFile.Length = AK_SafeU32(DefaultWorldBuffer.Size);
        LoadedWorldFile.Data = (ak_char*)AK_Allocate(sizeof(ak_char)*(LoadedWorldFile.Length+1));
        LoadedWorldFile.Data[LoadedWorldFile.Length] = 0;
        AK_MemoryCopy(LoadedWorldFile.Data, DefaultWorldBuffer.Data, sizeof(ak_char)*(LoadedWorldFile.Length));
        LoadDefaultWorld = DevContext_LoadWorld(DevContext, &DevContext->LoadedWorld, LoadedWorldFile);
        if(!LoadDefaultWorld)        
            AK_FileRemove(DevContext->DefaultWorldFilePathName);
    }
    
    if(!LoadDefaultWorld)
    {                
        material PlayerMaterial = {CreateDiffuse(AK_Blue3()), InvalidNormal(), CreateSpecular(0.5f, 8)};
        
#define DEV_ADD(function) DevContext_AddToDevTransform(DevContext->InitialTransforms, &DevContext->Game->World, function)
#define DEV_ADD_2(function) do { dual_world_id Dual = function; DevContext_AddToDevTransform(DevContext->InitialTransforms, &DevContext->Game->World, Dual.A); DevContext_AddToDevTransform(DevContext->InitialTransforms, &DevContext->Game->World, Dual.B); } while(0)
        
        DEV_ADD(CreatePlayerEntity(&Game->World, Game->Assets, 0, AK_V3<ak_f32>(), PlayerMaterial, &Game->Players[0]));        
        DEV_ADD(CreatePlayerEntity(&Game->World, Game->Assets, 1, AK_V3<ak_f32>(), PlayerMaterial, &Game->Players[1]));
        
        
#if 1
        material FloorMaterial = { CreateDiffuse(AK_White3()) };
        
        DEV_ADD_2(CreateFloorInBothWorlds(&Game->World, Game->Assets, AK_V3(0.0f, 0.0f, -0.1f), AK_V3(20.0f, 20.0f, 1.0f), FloorMaterial));
        
        material ButtonMaterial = { CreateDiffuse(AK_Red3()) };
        DEV_ADD(CreateButton(&Game->World, Game->Assets, 0, AK_V3( 0.0f, 5.0f, 0.0f), ButtonMaterial, true));
        DEV_ADD(CreateButton(&Game->World, Game->Assets, 0, AK_V3(-2.5f, 5.0f, 0.0f), ButtonMaterial, true));
        DEV_ADD(CreateButton(&Game->World, Game->Assets, 0, AK_V3( 2.5f, 5.0f, 0.0f), ButtonMaterial, true));
        
        DEV_ADD_2(CreateFloorInBothWorlds(&Game->World, Game->Assets, AK_V3(-4.0f, -3.5f, 0.0f), AK_V3(1.0f, 1.0f, 10.0f), FloorMaterial));        
        DEV_ADD_2(CreateRampInBothWorlds(&Game->World, Game->Assets, AK_V3(-4.0f, -6.0f, 0.0f), AK_V3(1.0f, 2.0f, 1.0f), AK_IdentityQuat<ak_f32>(), FloorMaterial));
        DEV_ADD_2(CreateFloorInBothWorlds(&Game->World, Game->Assets, AK_V3(-4.0f, 0.05f, 0.0f), AK_V3(1.0f, 4.0f, 10.0f), FloorMaterial));
        
        material PushableBoxMaterial = { CreateDiffuse(AK_White3()*0.4f) };
        DEV_ADD_2(CreateDualPushableBox(&Game->World, Game->Assets, AK_V3(0.0f, -2.0f, 0.0f), 1.0f, 40.0f, PushableBoxMaterial, true, false));
        
        CreatePointLights(Game->World.GraphicsStates, AK_V3(0.0f, 0.0f, 10.0f), 100.0f, AK_White3(), 5.0f, true);
        
        DevContext->DevUI.PlayGameSettings.PlayGame = true;
#else
        material DefaultFloorMaterial = { CreateDiffuse(AK_White3()) };    
        DEV_ADD(CreateStaticEntity(&Game->World, Game->Assets, 0, AK_V3(0.0f, 0.0f, -0.1f), AK_V3(15.0f, 15.0f, 0.1f), 
                                   AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_BOX, DefaultFloorMaterial));
        DEV_ADD(CreateStaticEntity(&Game->World, Game->Assets, 1, AK_V3(0.0f, 0.0f, -0.1f), AK_V3(15.0f, 15.0f, 0.1f), 
                                   AK_IdentityQuat<ak_f32>(), MESH_ASSET_ID_BOX, DefaultFloorMaterial));
        
        material DefaultButtonMaterial = { CreateDiffuse(AK_White3()*0.4f) };
        DEV_ADD(CreateButton(&Game->World, Game->Assets, 0, AK_V3(1.0f, -1.0f, 0.0f), AK_V3(1.0f, 1.0f, 1.0f), AK_IdentityQuat<ak_f32>(), 
                             DefaultButtonMaterial, true));        
        
        material PushingBlockMaterial = { CreateDiffuse(AK_RGB(0.5f, 0.7f, 0.3f)) };
        DEV_ADD(CreatePushableBox(&Game->World, Game->Assets, 0, AK_V3(-2.0f, -2.0f, 0.0f), 1.0f, 50.0f, PushingBlockMaterial));
        
        material RigidBodyMaterial = { CreateDiffuse(AK_Red3()) };
        DEV_ADD(CreateSphereRigidBody(&Game->World, Game->Assets, 0, AK_V3(2.0f, 2.0f, 0.5f), 0.5f, 30.0f, 0.3f, RigidBodyMaterial));        
        
        CreatePointLight(&Game->World.GraphicsStates[0], AK_V3(0.0f, 0.0f, 2.9f), 5.0f, AK_White3(), 1.0f, true);
        CreatePointLight(&Game->World.GraphicsStates[1], AK_V3(0.0f, 0.0f, 2.9f), 5.0f, AK_White3(), 1.0f, true);        
#endif
    }
    
    GlobalArena->EndTemp(&TempArena);
    
    DevContext->Cameras[0].Target = AK_V3<ak_f32>();
    DevContext->Cameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));            
    DevContext->Cameras[1].Target = AK_V3<ak_f32>();
    DevContext->Cameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));        
    
    
    dev_gizmo_state* GizmoState = &DevContext->GizmoState;
    GizmoState->GridDistance = 1.0f;
    GizmoState->ScaleSnap = 1.0f;
    GizmoState->RotationAngleSnap = 5.0f;
    
    __Internal_Dev_Context__ = DevContext;
}

void DevContext_Tick()
{
    dev_context* Context = Dev_GetDeveloperContext();    
    graphics* Graphics = Context->Graphics;
    game* Game = Context->Game;
    dev_input* DevInput = &Context->DevInput;
    ak_f32 dt = Game->dt;
    world* World = &Game->World;           
    
    ak_v2i Resolution = Game->Resolution;    
    Context->PlatformUpdate(&GetIO(), DevInput, Resolution, dt);        
    
    view_settings ViewSettings = {};
    if(!Context->DevUI.PlayGameSettings.PlayGame || Context->DevUI.PlayGameSettings.UseDevCamera)
    {
        
        camera* DevCamera = &Context->Cameras[Game->CurrentWorldIndex];            
        ak_v2i MouseDelta = DevInput->MouseCoordinates - DevInput->LastMouseCoordinates;    
        ak_v3f* SphericalCoordinates = &DevCamera->SphericalCoordinates;                
        
        ak_f32 Roll = 0;
        ak_f32 Pitch = 0;        
        
        ak_v2f PanDelta = AK_V2<ak_f32>();
        ak_f32 Scroll = 0;
        
        if(IsDown(DevInput->Ctrl))
        {            
            if(IsDown(DevInput->S)) DevContext_SaveWorld(Context, &Context->LoadedWorld, false);            
        }
        
        if(IsDown(DevInput->Alt))
        {
            if(IsDown(DevInput->L)) DevContext_LoadWorld(Context, &Context->LoadedWorld);
            if(IsDown(DevInput->S)) DevContext_SaveWorld(Context, &Context->LoadedWorld, true);
        }
        
        if(IsDown(DevInput->Alt))
        {
            if(IsDown(DevInput->LMB))
            {
                SphericalCoordinates->inclination += MouseDelta.y*1e-3f;
                SphericalCoordinates->azimuth += MouseDelta.x*1e-3f;
                
                ak_f32 InclindationDegree = AK_ToDegree(SphericalCoordinates->inclination);
                if(InclindationDegree < -180.0f)
                {
                    ak_f32 Diff = InclindationDegree + 180.0f;
                    InclindationDegree = 180.0f - Diff;
                    InclindationDegree = AK_Min(180.0f, InclindationDegree);
                    SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
                }
                else if(InclindationDegree > 180.0f)
                {
                    ak_f32 Diff = InclindationDegree - 180.0f;
                    InclindationDegree = -180.0f + Diff;
                    InclindationDegree = AK_Min(-180.0f, InclindationDegree);
                    SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
                }            
            }
            
            if(IsDown(DevInput->MMB))        
                PanDelta += AK_V2f(MouseDelta)*1e-3f;        
            
            if(AK_Abs(DevInput->Scroll) > 0)        
            {
                SphericalCoordinates->radius -= DevInput->Scroll*0.5f;                    
                if(SphericalCoordinates->radius < DEV_CAMERA_MIN_DISTANCE)
                    SphericalCoordinates->radius = DEV_CAMERA_MIN_DISTANCE;
            }
        }    
        
        ViewSettings = GetViewSettings(DevCamera);    
        DevCamera->Target += (ViewSettings.Orientation.XAxis*PanDelta.x - ViewSettings.Orientation.YAxis*PanDelta.y);        
    }
    
    if(!Context->DevUI.PlayGameSettings.PlayGame)
    {        
        if(IsPressed(Game->Input->SwitchWorld))
        {
            Context->SelectedObject = {};
            Game->CurrentWorldIndex = !Game->CurrentWorldIndex;                
        }
        
        ray RayCast = DevRay_GetRayFromMouse(DevInput->MouseCoordinates, &ViewSettings, Resolution);                                         
        dev_gizmo_state* GizmoState = &Context->GizmoState;
        if(!IsDown(DevInput->Alt) && !GetIO().WantCaptureMouse)
        {        
            if(IsPressed(DevInput->LMB))
            {            
                gizmo_intersection_result GizmoHit = DevRay_CastToGizmos(Context, GizmoState, RayCast, ViewSettings.ZNear);
                if(!GizmoHit.Hit)
                {
                    GizmoState->GizmoHit = {};
                    Context->SelectedObject = DevRay_CastToAllSelectables(World, Context->Game->Assets, RayCast, Game->CurrentWorldIndex, ViewSettings.ZNear);            
                }
                else
                {
                    GizmoState->GizmoHit = GizmoHit;
                    dev_object_edit Edit;
                    dev_selected_object* SelectedObject = &Context->SelectedObject;  
                    ak_u32 Index = AK_PoolIndex(SelectedObject->EntityID.ID);                                        
                    ak_array<dev_transform>* DevTransforms = &Context->InitialTransforms[SelectedObject->EntityID.WorldIndex];                    
                    if(DevTransforms->Size < (Index+1))
                        DevTransforms->Resize(Index+1);
                    
                    GizmoState->OriginalRotation = AK_IdentityM3<ak_f32>();
                    switch(SelectedObject->Type)
                    {
                        case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                        {      
                            GizmoState->OriginalRotation = AK_Transpose(AK_QuatToMatrix(Game->World.NewTransforms[SelectedObject->EntityID.WorldIndex][AK_PoolIndex(SelectedObject->EntityID.ID)].Orientation));
                            entity* Entity = World->EntityStorage[SelectedObject->EntityID.WorldIndex].Get(SelectedObject->EntityID.ID);
                            Edit.Entity[0] = *World->EntityStorage[SelectedObject->EntityID.WorldIndex].Get(SelectedObject->EntityID.ID);
                            Edit.Entity[1].ID = InvalidWorldID();
                        } break;
                        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
                        {
                            Edit.JumpIds[0].A = SelectedObject->EntityID;
                            Edit.JumpIds[0].B = InvalidWorldID();
                            Edit.JumpIds[1].A = InvalidWorldID();
                            Edit.JumpIds[1].B = InvalidWorldID();
                            jumping_quad* JumpingQuad = World->JumpingQuadStorage[SelectedObject->EntityID.WorldIndex].Get(SelectedObject->EntityID.ID); 
                            Edit.JumpProp[0].CenterP = JumpingQuad->CenterP;
                        } break;
                        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                        {
                            Edit.LightIds[0] = SelectedObject->PointLightID;
                            Edit.LightIds[1] = InvalidWorldID();
                            graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->PointLightID.WorldIndex];
                            point_light* LightProp = GraphicsState->PointLightStorage.Get(SelectedObject->PointLightID.ID);
                            Edit.LightProp = *LightProp;
                        } break;
                    }
                    
                    dev_transform* DevTransform = DevTransforms->Get(Index); 
                    Edit.ObjectEditType = DEV_OBJECT_EDIT_TYPE_TRANSFORM;
                    Edit.Transform = *DevTransform;
                    Edit.ObjectType = SelectedObject->Type;
                    Context->UndoStack.Add(Edit);
                    Context->RedoStack.Clear();
                }
            }
            
            if(IsReleased(DevInput->LMB))
            {            
                GizmoState->GizmoHit = {};
            }
        }
        
        dev_selected_object* SelectedObject = &Context->SelectedObject;            
        if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE)
        {
            if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_ENTITY)
            {   
                GizmoState->TransformMode = DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE;
            }
            else
            {
                if(IsPressed(DevInput->W)) GizmoState->TransformMode = DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE;                                    
                if(IsPressed(DevInput->E)) GizmoState->TransformMode = DEV_GIZMO_MOVEMENT_TYPE_SCALE;                                    
                if(IsPressed(DevInput->R)) GizmoState->TransformMode = DEV_GIZMO_MOVEMENT_TYPE_ROTATE;                         
            }
        }
        
        if(GizmoState->GizmoHit.Hit)
        {
            ak_v3f SelectedObjectPosition = DevContext_GetSelectedObjectPosition(World, SelectedObject);        
            
            gizmo_intersection_result* GizmoHit = &GizmoState->GizmoHit;
            ak_f32 t = RayPlaneIntersection(GizmoHit->Gizmo->IntersectionPlane, GizmoHit->HitMousePosition, RayCast.Origin, RayCast.Direction);        
            if(t != INFINITY)
            {            
                ak_v3f PointDiff = AK_V3<ak_f32>();
                ak_v3f NewPoint = RayCast.Origin + (RayCast.Direction*t);
                if(GizmoState->TransformMode != DEV_GIZMO_MOVEMENT_TYPE_ROTATE)
                {
                    switch(GizmoHit->Gizmo->MovementDirection)
                    {
                        case DEV_GIZMO_MOVEMENT_DIRECTION_X:
                        {
                            PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, 0.0f, 0.0f);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_Y:
                        {
                            PointDiff = AK_V3(0.0f, GizmoHit->HitMousePosition.y - NewPoint.y, 0.0f);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_Z:
                        {
                            PointDiff = AK_V3(0.0f, 0.0f, GizmoHit->HitMousePosition.z - NewPoint.z);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_XY:
                        {
                            PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, GizmoHit->HitMousePosition.y - NewPoint.y, 0.0f);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_XZ:
                        {
                            PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, 0.0f, GizmoHit->HitMousePosition.z - NewPoint.z);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_YZ:
                        {
                            PointDiff = AK_V3(0.0f, GizmoHit->HitMousePosition.y - NewPoint.y, GizmoHit->HitMousePosition.z - NewPoint.z);
                        } break;                    
                    }
                }
                else
                {
                    ak_v3f DirectionToOld = GizmoHit->HitMousePosition - SelectedObjectPosition;
                    ak_v3f DirectionToNew = NewPoint - SelectedObjectPosition;
                    ak_f32 AngleDiff = AK_ATan2(AK_Dot(AK_Cross(DirectionToNew, DirectionToOld), 
                                                       GizmoHit->Gizmo->IntersectionPlane), 
                                                AK_Dot(DirectionToOld, DirectionToNew));
                    
                    
                    switch(GizmoHit->Gizmo->MovementDirection)
                    {
                        case DEV_GIZMO_MOVEMENT_DIRECTION_X:
                        {
                            PointDiff = AK_V3(AngleDiff, 0.0f, 0.0f);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_Y:
                        {
                            PointDiff = AK_V3(0.0f, AngleDiff, 0.0f);
                        } break;
                        
                        case DEV_GIZMO_MOVEMENT_DIRECTION_Z:
                        {
                            PointDiff = AK_V3(0.0f, 0.0f, AngleDiff);
                        } break;
                        
                        AK_INVALID_DEFAULT_CASE;
                    }                
                }
                
                if(GizmoState->ShouldSnap)
                {    
                    ak_f32 Snap;
                    if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_SCALE) Snap = GizmoState->ScaleSnap;                
                    else if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_ROTATE) Snap = AK_ToRadians(GizmoState->RotationAngleSnap);                                    
                    else Snap = GizmoState->GridDistance;
                    
                    for(ak_u32 PlaneIndex = 0; PlaneIndex < 3; PlaneIndex++)
                    {           
                        if((AK_Abs(PointDiff[PlaneIndex]) < Snap) && (PointDiff[PlaneIndex] != 0))
                        {
                            if(GizmoState->TransformMode != DEV_GIZMO_MOVEMENT_TYPE_ROTATE)
                            {
                                NewPoint[PlaneIndex] = GizmoHit->HitMousePosition[PlaneIndex];
                                PointDiff[PlaneIndex] = 0.0f;
                            }
                            else
                            {
                                NewPoint = GizmoHit->HitMousePosition;
                                PointDiff = AK_V3<ak_f32>();
                            }
                        }
                        else if(PointDiff[PlaneIndex] < 0) PointDiff[PlaneIndex] = -Snap;
                        else if(PointDiff[PlaneIndex] > 0) PointDiff[PlaneIndex] = Snap;
                    }
                }
                
                GizmoHit->HitMousePosition = NewPoint;
                
                switch(SelectedObject->Type)
                {
                    case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                    {                           
                        entity* Entity = World->EntityStorage[SelectedObject->EntityID.WorldIndex].Get(SelectedObject->EntityID.ID);
                        ak_u32 Index = AK_PoolIndex(SelectedObject->EntityID.ID);                                        
                        ak_array<dev_transform>* DevTransforms = &Context->InitialTransforms[SelectedObject->EntityID.WorldIndex];                    
                        if(DevTransforms->Size < (Index+1))
                            DevTransforms->Resize(Index+1);
                        
                        dev_transform* DevTransform = DevTransforms->Get(Index);                    
                        if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE) DevTransform->Translation -= PointDiff;
                        else if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_ROTATE) DevTransform->Euler -= PointDiff;
                        else if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_SCALE) DevTransform->Scale -= PointDiff;
                        
                        ak_sqtf* Transform = &World->NewTransforms[SelectedObject->EntityID.WorldIndex][Index];
                        Transform->Translation = DevTransform->Translation;
                        Transform->Scale = DevTransform->Scale;
                        
                        if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_ROTATE)
                        {                        
                            ak_quatf XOrientation = AK_RotQuat(GizmoState->OriginalRotation.XAxis, -PointDiff.x);
                            ak_quatf YOrientation = AK_RotQuat(GizmoState->OriginalRotation.YAxis, -PointDiff.y);
                            ak_quatf ZOrientation = AK_RotQuat(GizmoState->OriginalRotation.ZAxis, -PointDiff.z);
                            
                            Transform->Orientation *= AK_Normalize(XOrientation*YOrientation*ZOrientation);
                        }
                        
                        World->OldTransforms[SelectedObject->EntityID.WorldIndex][Index] = *Transform;                        
                        
                        sim_entity* SimEntity = GetSimEntity(Game, SelectedObject->EntityID);
                        SimEntity->Transform = *Transform;                    
                        
                        graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->EntityID.WorldIndex];
                        graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
                        GraphicsEntity->Transform = AK_TransformM4(*Transform);                        
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
                    {
                        AK_Assert(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE, "Only valid transform mode for point lights is translation. This is a programming error");
                        jumping_quad* JumpingQuad = World->JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(SelectedObject->JumpingQuadID.ID);                        
                        JumpingQuad->CenterP -= PointDiff;                        
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                    {
                        AK_Assert(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE, "Only valid transform mode for point lights is translation. This is a programming error");
                        graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->EntityID.WorldIndex];
                        point_light* PointLight = GraphicsState->PointLightStorage.Get(SelectedObject->EntityID.ID);                            
                        PointLight->Position -= PointDiff;
                    } break;                
                }            
            }
        }
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            AK_ForEach(JumpingQuad, &World->JumpingQuadStorage[WorldIndex])        
                JumpingQuad->Color = AK_Yellow3();                
        }
        
        if(SelectedObject->Type == DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD)
        {
            jumping_quad* JumpingQuad = World->JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(SelectedObject->JumpingQuadID.ID);
            jumping_quad* OtherJumpingQuad = World->JumpingQuadStorage[JumpingQuad->OtherQuad.WorldIndex].Get(JumpingQuad->OtherQuad.ID);
            
            JumpingQuad->Color = AK_Green3();
            OtherJumpingQuad->Color = AK_Red3();
        }
        
        if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE)
        {
            if(IsPressed(DevInput->Delete))
            {
                switch(SelectedObject->Type)
                {
                    case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                    {
                        entity* Entity = GetEntity(Game, SelectedObject->EntityID);
                        if(Entity->Type != ENTITY_TYPE_PLAYER)
                        {
                            dev_object_edit Undo;
                            ak_array<dev_transform>* DevTransforms = &Context->InitialTransforms[Entity->ID.WorldIndex];
                            ak_u32 Index = AK_PoolIndex(Entity->ID.ID);  
                            if(DevTransforms->Size < (Index+1))
                                DevTransforms->Resize(Index+1);
                            dev_transform* DevTransform = DevTransforms->Get(Index);   
                            graphics_state* GraphicsState = &World->GraphicsStates[Entity->ID.WorldIndex];
                            graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
                            Undo.Entity[0] = *Entity;
                            Undo.Entity[1].ID = InvalidWorldID();
                            Undo.Material = GraphicsEntity->Material;
                            Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_DELETE;
                            Undo.Transform = *DevTransform;
                            Undo.ObjectType = DEV_SELECTED_OBJECT_TYPE_ENTITY;
                            switch(Entity->Type)
                            {
                                case ENTITY_TYPE_STATIC:
                                {
                                    Undo.MeshID = GraphicsEntity->MeshID;
                                } break;
                                case ENTITY_TYPE_RIGID_BODY:
                                {
                                    rigid_body* RigidBody = GetSimEntity(Game, Entity->ID)->ToRigidBody();
                                    Undo.Mass = 1.0f / RigidBody->InvMass;
                                    Undo.Restitution = RigidBody->Restitution;
                                } break;
                                
                                case ENTITY_TYPE_PUSHABLE:
                                {
                                    rigid_body* RigidBody = GetSimEntity(Game, Entity->ID)->ToRigidBody();
                                    Undo.Mass = 1.0f / RigidBody->InvMass;
                                    Undo.Interactable = GetPushingObject(&Game->World, Entity)->Interactable;
                                } break;
                            }
                            Context->UndoStack.Add(Undo);
                            Context->RedoStack.Clear();
                            FreeEntity(Game, SelectedObject->EntityID);                    
                            *SelectedObject = {};
                        }
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
                    {
                        dev_object_edit Undo;
                        Undo.ObjectType = DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD;
                        Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_DELETE;
                        jumping_quad* QuadA = World->JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(SelectedObject->JumpingQuadID.ID);
                        jumping_quad* QuadB = World->JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(QuadA->OtherQuad.ID);
                        Undo.JumpIds[0].A = SelectedObject->JumpingQuadID;
                        Undo.JumpIds[0].B = QuadA->OtherQuad;
                        Undo.JumpProp[0] = *QuadA;
                        Undo.JumpProp[1] = *QuadB;
                        Context->UndoStack.Add(Undo);
                        Context->RedoStack.Clear();
                        FreeJumpingQuad(World, SelectedObject->JumpingQuadID);
                        *SelectedObject = {};                        
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                    {
                        graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->PointLightID.WorldIndex];
                        point_light* LightProp = GraphicsState->PointLightStorage.Get(SelectedObject->PointLightID.ID);
                        dev_object_edit Undo;
                        Undo.ObjectType = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
                        Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_DELETE;
                        Undo.LightIds[0] = SelectedObject->PointLightID;
                        Undo.LightIds[1] = InvalidWorldID();
                        Undo.LightProp = *LightProp;
                        GraphicsState->PointLightStorage.Free(SelectedObject->PointLightID.ID);                    
                        *SelectedObject = {};
                        Context->UndoStack.Add(Undo);
                        Context->RedoStack.Clear();
                    } break;                                
                }
            }
        }
        
        if(IsDown(DevInput->Ctrl) && IsPressed(DevInput->Z))
        {
            DevContext_UndoLastEdit(Context);
        }
        
        if(IsDown(DevInput->Ctrl) && IsPressed(DevInput->Y))
        {
            DevContext_RedoLastEdit(Context);
        }
    }
    
    DevUI_Update(Context, &Context->DevUI);
    
    DevInput->LastMouseCoordinates = DevInput->MouseCoordinates;
    DevInput->MouseCoordinates = {};
    DevInput->Scroll = 0.0f;
    for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(DevInput->Buttons); ButtonIndex++)
        DevInput->Buttons[ButtonIndex].WasDown = DevInput->Buttons[ButtonIndex].IsDown;    
}

void DevContext_RenderPrimitives(dev_context* Context, ak_u32 WorldIndex, view_settings* ViewSettings)
{
    graphics_state* GraphicsState = &Context->Game->World.GraphicsStates[WorldIndex];
    
    PushRenderBufferViewportScissorAndView(Context->Graphics, GraphicsState->RenderBuffer, ViewSettings);
    PushDepth(Context->Graphics, false);
    
    AK_ForEach(RenderPrimitive, &Context->RenderPrimitives)
    {
        switch(RenderPrimitive->Type)
        {
            case DEV_RENDER_PRIMITIVE_TYPE_POINT:
            {
                DevDraw_Point(Context, RenderPrimitive->Point.P, RenderPrimitive->Point.Size, RenderPrimitive->Point.Color);
            } break;
            
            case DEV_RENDER_PRIMITIVE_TYPE_SEGMENT:
            {
                DevDraw_Edge(Context, RenderPrimitive->Segment.P0, RenderPrimitive->Segment.P1, RenderPrimitive->Segment.Size, RenderPrimitive->Segment.Color);
            } break;
            
            AK_INVALID_DEFAULT_CASE;
        }
    }        
    
    Context->RenderPrimitives.Clear();
    PushDepth(Context->Graphics, true);
}

void DevContext_RenderConvexHulls(dev_context* Context, ak_u32 WorldIndex, view_settings* ViewSettings)
{
    graphics* Graphics = Context->Graphics;
    game* Game = Context->Game;
    world* World = &Game->World;
    graphics_state* GraphicsState = &World->GraphicsStates[WorldIndex];
    simulation* Simulation = &World->Simulations[WorldIndex];
    
    PushRenderBufferViewportScissorAndView(Graphics, GraphicsState->RenderBuffer, ViewSettings);
    PushDepth(Graphics, false);    
    
    AK_ForEach(GraphicsEntity, &GraphicsState->GraphicsEntityStorage)
    {
        ak_u32 EntityIndex = UserDataToIndex(GraphicsEntity->UserData);                
        dev_slim_mesh* ConvexHulls = Context->ConvexHullMeshes[GraphicsEntity->MeshID];        
        if(!ConvexHulls)
        {
            mesh_info* MeshInfo = GetMeshInfo(Game->Assets, GraphicsEntity->MeshID);
            if(MeshInfo->Header.ConvexHullCount > 0)
            {
                ConvexHulls = Context->DevStorage->PushArray<dev_slim_mesh>(MeshInfo->Header.ConvexHullCount);
                for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
                {
                    ConvexHulls[MeshInfo->Header.ConvexHullCount-ConvexHullIndex-1] = DevContext_CreateConvexHullMesh(Context, &MeshInfo->ConvexHulls[ConvexHullIndex]);
                }                
                Context->ConvexHullMeshes[GraphicsEntity->MeshID] = ConvexHulls;
            }
        }
        
        entity* Entity = World->EntityStorage[WorldIndex].GetByIndex(EntityIndex);                        
        sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
        collision_volume* Volume = Simulation->CollisionVolumeStorage.Get(SimEntity->CollisionVolumeID);
        
        ak_u32 ConvexHullIndex = 0;
        ak_sqtf Transform = AK_SQT(GraphicsEntity->Transform);
        while(Volume)
        {            
            switch(Volume->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {                    
                    sphere Sphere = TransformSphere(&Volume->Sphere, Transform);                    
                    DevDraw_LineEllipsoid(Context, Sphere.CenterP, AK_V3(Sphere.Radius, Sphere.Radius, Sphere.Radius), AK_Blue3());
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule Capsule = TransformCapsule(&Volume->Capsule, Transform);
                    DevDraw_LineCapsule(Context, Capsule.P0, Capsule.P1, Capsule.Radius, AK_Blue3());                                                
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    ak_sqtf NewTransform = Volume->ConvexHull.Header.Transform*Transform;
                    ak_m4f Model = AK_TransformM4(NewTransform);
                    
                    PushDrawLineMesh(Context->Graphics, ConvexHulls[ConvexHullIndex].MeshID, Model, AK_Blue3(),
                                     ConvexHulls[ConvexHullIndex].IndexCount, 0, 0);                    
                    ConvexHullIndex++;
                } break;
            }
            
            Volume = Simulation->CollisionVolumeStorage.Get(Volume->NextID);
        }
    }
    PushDepth(Graphics, true);
}

DEV_RENDER_GRID_CALLBACK(DevContext_RenderGrid)
{    
    ak_v3f* FrustumCorners = GetFrustumCorners(ViewSettings, GraphicsState->RenderBuffer->Resolution);
    
    ak_v3f FrustumPlaneIntersectionPoints[4];
    ak_i8 IntersectedCount = 0;
    for(int i = 0; i < 4; i++)
    {
        ray FrustumRay = {};
        FrustumRay.Origin = FrustumCorners[i];
        FrustumRay.Direction = AK_Normalize(FrustumCorners[i + 4] - FrustumCorners[i]);
        ak_f32 Distance = RayPlaneIntersection(AK_V3f(0, 0, 1), AK_V3f(0,0,0), FrustumRay.Origin, FrustumRay.Direction);
        if(Distance != INFINITY)
        {
            IntersectedCount++;            
            Distance = AK_Min(Distance, ViewSettings->ZFar);
            FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin + (FrustumRay.Direction * Distance);
        }
        else
        {            
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
        
        DevDraw_Grid(Context, AK_Floor(MinX), AK_Ceil(MaxX), AK_Floor(MinY), AK_Ceil(MaxY), AK_RGB(0.1f, 0.1f, 0.1f)); 
    }    
}

void DevContext_RenderWorld(dev_context* Context, ak_u32 WorldIndex)
{    
    game* Game = Context->Game;
    graphics* Graphics = Context->Graphics;        
    world* World = &Game->World;
    
    graphics_state* GraphicsState = &World->GraphicsStates[WorldIndex];        
    
    UpdateRenderBuffer(Graphics, &GraphicsState->RenderBuffer, Game->Resolution);
    
    camera* Camera = Context->Cameras + WorldIndex;    
    view_settings ViewSettings = GetViewSettings(Camera);
    
    switch(Context->DevUI.ViewModeType)
    {
        case VIEW_MODE_TYPE_LIT:
        {                
            NormalEntityPass(Graphics, Game, GraphicsState, &ViewSettings);            
        } break;
        
        case VIEW_MODE_TYPE_UNLIT:
        {                
            StandardEntityCommands(Graphics, GraphicsState, &ViewSettings);
            EntityUnlitPass(Graphics, Game->Assets, &GraphicsState->GraphicsEntityStorage);
        } break;
        
        case VIEW_MODE_TYPE_WIREFRAME:
        {
            StandardEntityCommands(Graphics, GraphicsState, &ViewSettings);
            EntityWireframePass(Graphics, Game->Assets, &GraphicsState->GraphicsEntityStorage);
        } break;
        
        case VIEW_MODE_TYPE_WIREFRAME_ON_LIT:
        {
            graphics_light_buffer LightBuffer = GetLightBuffer(GraphicsState);
            ShadowPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);
            
            StandardEntityCommands(Graphics, GraphicsState, &ViewSettings);                
            EntityLitPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);
            EntityWireframePass(Graphics, Game->Assets, &GraphicsState->GraphicsEntityStorage);
            
        } break;
        
    }
    JumpingQuadPass(Graphics, &World->JumpingQuadStorage[WorldIndex], &Game->QuadMesh);
    AK_ForEach(PointLight, &GraphicsState->PointLightStorage)    
        DevDraw_Sphere(Context, PointLight->Position, DEV_POINT_LIGHT_RADIUS, AK_Yellow3());    
    
    if(Context->DevUI.PlayGameSettings.DrawGrid)
        DevContext_RenderGrid(Context, GraphicsState, &ViewSettings);
        
    PushDepth(Graphics, false);        
    if(Context->SelectedObject.Type != DEV_SELECTED_OBJECT_TYPE_NONE)
    {           
        if(DevContext_GetSelectedObjectWorldIndex(&Context->SelectedObject) == WorldIndex)
        {                        
            dev_gizmo_state* GizmoState = &Context->GizmoState;
            ak_v3f SelectedObjectPosition = DevContext_GetSelectedObjectPosition(World, &Context->SelectedObject);
            
            switch(GizmoState->TransformMode)
            {
                case DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE:
                case DEV_GIZMO_MOVEMENT_TYPE_SCALE:
                {                                        
                    dev_mesh* GizmoMesh = (GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_SCALE) ? &Context->TriangleScaleMesh : &Context->TriangleArrowMesh;
                    DevContext_PopulateNonRotationGizmos(GizmoState, GizmoMesh, &Context->TrianglePlaneMesh, SelectedObjectPosition);                
                    
                } break;
                
                case DEV_GIZMO_MOVEMENT_TYPE_ROTATE:
                {
                    DevContext_PopulateRotationGizmos(GizmoState, &Context->TriangleTorusMesh, SelectedObjectPosition);
                } break;
            }
            
            DevDraw_GizmoState(Context, GizmoState, SelectedObjectPosition);
        }
    }       
    PushDepth(Graphics, true);    
    
    if(Context->DevUI.DrawCollisionVolumes)
        DevContext_RenderConvexHulls(Context, WorldIndex, &ViewSettings);    
    
}

camera* DevContext_GetCameraForRender(dev_context* Context, ak_u32 WorldIndex)
{
    return Context->DevUI.PlayGameSettings.UseDevCamera ? &Context->Cameras[WorldIndex] : &Context->Game->World.GraphicsStates[WorldIndex].Camera;
}

void DevContext_Render()
{
    dev_context* Context = Dev_GetDeveloperContext();
    game* Game = Context->Game;
    graphics* Graphics = Context->Graphics;        
    world* World = &Game->World;        
    
    if(!Context->DevUI.PlayGameSettings.PlayGame)
    {                         
        DevContext_RenderWorld(Context, Game->CurrentWorldIndex);                
        camera* Camera = Context->Cameras + Game->CurrentWorldIndex;    
        view_settings ViewSettings = GetViewSettings(Camera);        
        DevContext_RenderPrimitives(Context, Game->CurrentWorldIndex, &ViewSettings);                                        
        
        if(Context->DevUI.DrawOtherWorld)
            DevContext_RenderWorld(Context, !Game->CurrentWorldIndex);       
    }
    else
    {
        camera* Camera = DevContext_GetCameraForRender(Context, Game->CurrentWorldIndex);
        view_settings ViewSettings = GetViewSettings(Camera);        
        
        if(Context->DevUI.DrawCollisionVolumes)
        {            
            DevContext_RenderConvexHulls(Context, Game->CurrentWorldIndex, &ViewSettings);
            if(Context->DevUI.DrawOtherWorld)
            {
                Camera = DevContext_GetCameraForRender(Context, !Game->CurrentWorldIndex);
                ViewSettings = GetViewSettings(Camera);
                DevContext_RenderConvexHulls(Context, !Game->CurrentWorldIndex, &ViewSettings);
            }                
        }                                                
        
        DevContext_RenderPrimitives(Context, Game->CurrentWorldIndex, &ViewSettings);                                                
    }
        
    if(Context->DevUI.DrawOtherWorld)
    {
        PushRenderBuffer(Graphics, World->GraphicsStates[Game->CurrentWorldIndex].RenderBuffer);
        
        ak_v2i CopyResolution = World->GraphicsStates[Game->CurrentWorldIndex].RenderBuffer->Resolution / 5;
        ak_v2i CopyOffset = World->GraphicsStates[Game->CurrentWorldIndex].RenderBuffer->Resolution - CopyResolution; 
        
        PushCopyToRenderBuffer(Graphics, World->GraphicsStates[!Game->CurrentWorldIndex].RenderBuffer, CopyOffset, CopyResolution);
    }
    
    graphics_state* CurrentGraphicsState = &World->GraphicsStates[Game->CurrentWorldIndex];
    DevUI_Render(Graphics, &Context->DevUI, CurrentGraphicsState->RenderBuffer);    
    
    //PushCopyToOutput(Graphics, Context->DevRenderBuffer);
    PushScissor(Graphics, 0, 0, CurrentGraphicsState->RenderBuffer->Resolution.w, CurrentGraphicsState->RenderBuffer->Resolution.h);
    PushCopyToOutput(Graphics, Context->DevUI.UIRenderBuffer);    
}

void DevContext_DebugLog(const ak_char* Format, ...)
{
    dev_context* Context = Dev_GetDeveloperContext();
    va_list Args;
    va_start(Args, Format);    
    Context->DevUI.Logs.Add(AK_FormatString(Context->DevUI.LogArena, Format, Args));
    va_end(Args);
}