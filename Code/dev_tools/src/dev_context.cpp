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

graphics_mesh_id DevContext_AllocateMesh(graphics* Graphics, ak_mesh_result* Mesh)
{
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(ak_vertex_p3), GRAPHICS_VERTEX_FORMAT_P3, 
                                                     Mesh->Indices, Mesh->IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);
    return Result;
}

void DevContext_CreateLineSphereMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateLineSphere(GlobalArena, 1.0f, CircleSampleCount);        
    DevContext->LineSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineSphereMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleCircleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Height)
{
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCircle(DevContext->DevStorage, 1.0f, Height, CircleSampleCount);        
    DevContext->TriangleCircleMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCircleMesh.Indices = MeshGenerationResult.Indices;
    DevContext->TriangleCircleMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TriangleCircleMesh.Vertices = MeshGenerationResult.Vertices;
    DevContext->TriangleCircleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);        
}

void DevContext_CreateTriangleTorusMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Width)
{
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleTorus(DevContext->DevStorage, 1.0f, Width, CircleSampleCount);        
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
    
    ak_mesh_result MeshGenerationResult = AK_GenerateLineBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));    
    DevContext->LineBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->LineBoxMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleBoxMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleBox(GlobalArena, AK_V3(1.0f, 1.0f, 1.0f));       
    DevContext->TriangleBoxMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleBoxMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);    
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleSphereMesh(dev_context* DevContext)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleSphere(GlobalArena, 1.0f, 2);
    DevContext->TriangleSphereMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleSphereMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleCylinderMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCylinder(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleCylinderMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleCylinderMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleConeMesh(dev_context* DevContext, ak_u16 CircleSampleCount)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_temp_arena TempArena = GlobalArena->BeginTemp();
    
    ak_mesh_result MeshGenerationResult = AK_GenerateTriangleCone(GlobalArena, 1.0f, 1.0f, CircleSampleCount);
    DevContext->TriangleConeMesh.IndexCount = MeshGenerationResult.IndexCount;
    DevContext->TriangleConeMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleArrowMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 ArrowRadius, ak_f32 ArrowHeight)
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
    DevContext->TriangleArrowMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateTriangleScaleMesh(dev_context* DevContext, ak_u16 CircleSampleCount, ak_f32 Radius, ak_f32 Height, ak_f32 CubeSize)
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
    DevContext->TriangleScaleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}

void DevContext_CreateLineCapsuleMesh(dev_context* DevContext, ak_f32 Radius, ak_u16 CircleSampleCount)
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
    DevContext->LineCapsuleMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
    
    GlobalArena->EndTemp(&TempArena);
}


void DevContext_CreatePlaneMesh(dev_context* DevContext, ak_f32 Width, ak_f32 Height)
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
    
    DevContext->TrianglePlaneMesh.IndexCount  = MeshGenerationResult.IndexCount;
    DevContext->TrianglePlaneMesh.VertexCount = MeshGenerationResult.VertexCount;
    DevContext->TrianglePlaneMesh.Vertices    = MeshGenerationResult.Vertices;
    DevContext->TrianglePlaneMesh.Indices     = MeshGenerationResult.Indices;
    DevContext->TrianglePlaneMesh.MeshID = DevContext_AllocateMesh(DevContext->Graphics, &MeshGenerationResult);
}

void DevContext_UpdateObjectOrientation(ak_quatf* Orientation, ak_v3f* OldEuler, ak_v3f NewEuler)
{
    *OldEuler = NewEuler;
    *Orientation = AK_EulerToQuat(NewEuler.roll, NewEuler.pitch, NewEuler.yaw);
}

ak_v3f DevContext_GetSelectedObjectPosition(dev_context* Context, dev_selected_object* SelectedObject)
{
    AK_Assert(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE, "There is no selected object. Cannot retrieve position");
    switch(SelectedObject->Type)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:          
        return GetEntityPositionNew(Context->Game, SelectedObject->EntityID);            
        
        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:         
        return Context->Game->GraphicsStates[SelectedObject->PointLightID.WorldIndex].PointLightStorage.Get(SelectedObject->PointLightID.ID)->Position;
        
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

void DevContext_AddToDevTransform(dev_context* DevContext, world_id EntityID)
{    
    ak_array<dev_transform>* DevTransforms = &DevContext->InitialTransforms[EntityID.WorldIndex];
    ak_u32 Index = AK_PoolIndex(EntityID.ID);
    if((Index+1) > DevTransforms->Size)
        DevTransforms->Resize(Index+1);
    
    dev_transform* DevTransform = DevTransforms->Get(Index);
    
    ak_sqtf* Transform = GetEntityTransformNew(DevContext->Game, EntityID);
    
    DevTransform->Translation = Transform->Translation;
    DevTransform->Scale = Transform->Scale;
    DevTransform->Euler = AK_QuatToEuler(Transform->Orientation);    
}

void DevContext_Initialize(game* Game, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui, platform_development_update* PlatformUpdate)
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
    
    capsule PlayerCapsule = CreateCapsule(AK_V3<ak_f32>(), PLAYER_HEIGHT, PLAYER_RADIUS);
    world_id PlayerA = CreatePlayerEntity(DevContext->Game, 0, AK_V3<ak_f32>(), AK_IdentityQuat<ak_f32>(), 65, Global_PlayerMaterial, &PlayerCapsule);
    world_id PlayerB = CreatePlayerEntity(DevContext->Game, 1, AK_V3<ak_f32>(), AK_IdentityQuat<ak_f32>(), 65, Global_PlayerMaterial, &PlayerCapsule);
    
    DevContext_AddToDevTransform(DevContext, PlayerA);
    DevContext_AddToDevTransform(DevContext, PlayerB);
        
    DevContext->Cameras[0].Target = AK_V3<ak_f32>();
    DevContext->Cameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));        
    
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
    
    graphics_state* CurrentGraphicsState = GetGraphicsState(Game, Game->CurrentWorldIndex);
    
    UpdateRenderBuffer(Graphics, &CurrentGraphicsState->RenderBuffer, Game->Resolution);    
    Context->PlatformUpdate(&GetIO(), DevInput, CurrentGraphicsState->RenderBuffer->Resolution, dt);        
    
    camera* DevCamera = &Context->Cameras[Game->CurrentWorldIndex];        
    
    ak_v2i MouseDelta = DevInput->MouseCoordinates - DevInput->LastMouseCoordinates;
    
    ak_v3f* SphericalCoordinates = &DevCamera->SphericalCoordinates;
    
    ak_f32 Roll = 0;
    ak_f32 Pitch = 0;        
    
    ak_v2f PanDelta = AK_V2<ak_f32>();
    ak_f32 Scroll = 0;
    
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
    
    view_settings ViewSettings = {};
    ak_v3f Up = AK_ZAxis();        
    ak_f32 Degree = AK_ToDegree(SphericalCoordinates->inclination);                    
    if(Degree > 0)
        Up = -AK_ZAxis();    
    if(Degree < -180.0f)
        Up = -AK_YAxis();
    if(AK_EqualZeroEps(Degree))
        Up = AK_YAxis(); 
    if(AK_EqualEps(AK_Abs(Degree), 180.0f))
        Up = -AK_YAxis();        
    
    ViewSettings.Position = DevCamera->Target + AK_SphericalToCartesian(*SphericalCoordinates);            
    ViewSettings.Orientation = AK_OrientAt(ViewSettings.Position, DevCamera->Target, Up);       
    
    ViewSettings.ZNear = 0.01f;
    ViewSettings.ZFar = 1000.0f;
    ViewSettings.FieldOfView = AK_PI*0.3f;
    
    DevCamera->Target += (ViewSettings.Orientation.XAxis*PanDelta.x - ViewSettings.Orientation.YAxis*PanDelta.y);
    
    ray RayCast = DevRay_GetRayFromMouse(DevInput->MouseCoordinates, &ViewSettings, CurrentGraphicsState->RenderBuffer->Resolution);                                         
    dev_gizmo_state* GizmoState = &Context->GizmoState;
    if(!IsDown(DevInput->Alt) && !GetIO().WantCaptureMouse)
    {        
        if(IsPressed(DevInput->LMB))
        {            
            gizmo_intersection_result GizmoHit = DevRay_CastToGizmos(Context, GizmoState, RayCast, ViewSettings.ZNear);
            if(!GizmoHit.Hit)
            {
                GizmoState->GizmoHit = {};
                Context->SelectedObject = DevRay_CastToAllSelectables(Context, RayCast, 0, ViewSettings.ZNear);            
            }
            else
            {
                GizmoState->GizmoHit = GizmoHit;
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
        ak_v3f SelectedObjectPosition = DevContext_GetSelectedObjectPosition(Context, SelectedObject);        
        
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
                    entity* Entity = GetEntity(Game, SelectedObject->EntityID);
                    ak_u32 Index = AK_PoolIndex(SelectedObject->EntityID.ID);                                        
                    ak_array<dev_transform>* DevTransforms = &Context->InitialTransforms[SelectedObject->EntityID.WorldIndex];                    
                    if(DevTransforms->Size < (Index+1))
                        DevTransforms->Resize(Index+1);
                    
                    dev_transform* DevTransform = DevTransforms->Get(Index);                    
                    if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE) DevTransform->Translation -= PointDiff;
                    else if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_ROTATE) DevTransform->Euler -= PointDiff;
                    else if(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_SCALE) DevTransform->Scale -= PointDiff;
                    
                    ak_sqtf* Transform = GetEntityTransformNew(Game, SelectedObject->EntityID);
                    Transform->Translation = DevTransform->Translation;
                    Transform->Scale = DevTransform->Scale;
                    Transform->Orientation = AK_Normalize(AK_EulerToQuat(DevTransform->Euler));
                    
                    *GetEntityTransformOld(Game, SelectedObject->EntityID) = *Transform;
                    
                    sim_entity* SimEntity = GetSimEntity(Game, SelectedObject->EntityID);
                    SimEntity->Transform = *Transform;                    
                    
                    graphics_state* GraphicsState = &Game->GraphicsStates[SelectedObject->EntityID.WorldIndex];
                    graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
                    GraphicsEntity->Transform = AK_TransformM4(*Transform);
                } break;
                
                case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                {
                    AK_Assert(GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE, "Only valid transform mode for point lights is translation. This is a programming error");
                    graphics_state* GraphicsState = GetGraphicsState(Context->Game, SelectedObject->EntityID);
                    point_light* PointLight = GraphicsState->PointLightStorage.Get(SelectedObject->EntityID.ID);                            
                    PointLight->Position -= PointDiff;
                } break;                
            }            
        }
    }
    
    if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE)
    {
        if(IsPressed(DevInput->Delete))
        {
            switch(SelectedObject->Type)
            {
                case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                {
                    FreeEntity(Game, SelectedObject->EntityID);                    
                    *SelectedObject = {};
                } break;
                
                case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                {
                    graphics_state* GraphicsState = GetGraphicsState(Game, SelectedObject->PointLightID.WorldIndex);
                    GraphicsState->GraphicsEntityStorage.Free(SelectedObject->PointLightID.ID);                    
                    *SelectedObject = {};
                } break;                                
            }
        }
    }
    
    DevUI_Update(Context, &Context->DevUI);
    
    DevInput->LastMouseCoordinates = DevInput->MouseCoordinates;
    DevInput->MouseCoordinates = {};
    DevInput->Scroll = 0.0f;
    for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(DevInput->Buttons); ButtonIndex++)
        DevInput->Buttons[ButtonIndex].WasDown = DevInput->Buttons[ButtonIndex].IsDown;    
}

void DevContext_Render()
{
    dev_context* Context = Dev_GetDeveloperContext();
    game* Game = Context->Game;
    graphics* Graphics = Context->Graphics;        
    
    graphics_state* CurrentGraphicsState = Game->GraphicsStates + Game->CurrentWorldIndex;
    camera* Camera = Context->Cameras + Game->CurrentWorldIndex;
    
    view_settings ViewSettings = GetViewSettings(Camera);
    
    graphics_light_buffer LightBuffer = GetLightBuffer(CurrentGraphicsState);
    ShadowPass(Graphics, Game->Assets, &LightBuffer, &CurrentGraphicsState->GraphicsEntityStorage);
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, CurrentGraphicsState->RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    EntityPass(Graphics, Game->Assets, &LightBuffer, &CurrentGraphicsState->GraphicsEntityStorage);        
    AK_ForEach(PointLight, &CurrentGraphicsState->PointLightStorage)    
        DevDraw_Sphere(Context, PointLight->Position, DEV_POINT_LIGHT_RADIUS, AK_Yellow3());    
        
    ak_v3f* FrustumCorners = GetFrustumCorners(&ViewSettings, CurrentGraphicsState->RenderBuffer->Resolution);
    
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
            Distance = AK_Min(Distance, ViewSettings.ZFar);
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
    
    PushDepth(Graphics, false);        
    if(Context->SelectedObject.Type != DEV_SELECTED_OBJECT_TYPE_NONE)
    {
        dev_gizmo_state* GizmoState = &Context->GizmoState;
        ak_v3f SelectedObjectPosition = DevContext_GetSelectedObjectPosition(Context, &Context->SelectedObject);
        
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
    
    PushDepth(Graphics, true);    
    
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