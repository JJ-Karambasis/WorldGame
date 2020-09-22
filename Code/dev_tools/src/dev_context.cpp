void DevContext_DeleteDevEntity(ak_pool<dev_entity>* EntityStorage, world_id ID)
{
    EntityStorage[ID.WorldIndex].Free(ID.ID);
}

world_id DevContext_CreateDevEntity(ak_pool<dev_entity>* EntityStorage, entity_type Type, ak_u32 WorldIndex, ak_v3f Position, 
                                    ak_v3f Scale, ak_quatf Orientation, mesh_asset_id MeshID, material Material)
{    
    world_id Result = MakeWorldID(EntityStorage[WorldIndex].Allocate(), WorldIndex);
    dev_entity* Entity = EntityStorage->Get(Result.ID);
    
    Entity->Type = Type;
    Entity->ID = Result;
    Entity->Transform = AK_SQT(Position, Orientation, Scale);
    Entity->MeshID = MeshID;
    Entity->Material = Material;
    return Result;    
}

void DevContext_DeletePointLight(ak_pool<dev_point_light>* PointLightStorage, world_id ID)
{
    PointLightStorage[ID.WorldIndex].Free(ID.ID);
}

world_id DevContext_CreatePointLight(ak_pool<dev_point_light>* PointLightStorage, ak_u32 WorldIndex, ak_v3f Position, ak_f32 Radius,
                                     ak_color3f Color, ak_f32 Intensity)
{
    world_id Result = MakeWorldID(PointLightStorage[WorldIndex].Allocate(), WorldIndex);
    dev_point_light* PointLight = PointLightStorage[WorldIndex].Get(Result.ID);
    PointLight->ID = Result;
    PointLight->On = true;
    PointLight->Position = Position;
    PointLight->Radius = Radius;
    PointLight->Color = Color;
    PointLight->Intensity = Intensity;
    return Result;
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

dev_entity* DevContext_GetEntity(dev_context* Context, world_id WorldID)
{
    return Context->InitialEntityStorage[WorldID.WorldIndex].Get(WorldID.ID);
}

dev_point_light* DevContext_GetPointLight(dev_context* Context, world_id WorldID)
{
    return Context->InitialPointLights[WorldID.WorldIndex].Get(WorldID.ID);
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
        case DEV_SELECTED_OBJECT_TYPE_ENTITY: return Context->InitialEntityStorage[SelectedObject->EntityID.WorldIndex].Get(SelectedObject->EntityID.ID)->Transform.Translation;        
        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT: return Context->InitialPointLights[SelectedObject->PointLightID.WorldIndex].Get(SelectedObject->PointLightID.ID)->Position;
        case DEV_SELECTED_OBJECT_TYPE_PLAYER_CAPSULE: return SelectedObject->PlayerCapsule->GetBottom();
        
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
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
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
        Gizmo.IntersectionPlane = AK_V3f(0, 1, 0);
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
        Gizmo.IntersectionPlane = AK_V3f(0, 0, 1);
        Gizmo.MovementDirection = DEV_GIZMO_MOVEMENT_DIRECTION_Z;
        GizmoState->Gizmos[2] =  Gizmo;
    }
    
    GizmoState->Gizmos[3] = {};                                                                
    GizmoState->Gizmos[4] = {};                                                                 
    GizmoState->Gizmos[5] = {};                
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
    
    DevContext->InitialPlayerCapsules[1] = DevContext->InitialPlayerCapsules[0] = CreateCapsule(AK_V3<ak_f32>(), PLAYER_HEIGHT, PLAYER_RADIUS),     
    DevContext->DevCameras[0].Target = DevContext->InitialPlayerCapsules[0].GetBottom();
    DevContext->DevCameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));        
    
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
    
    Context->PlatformUpdate(&GetIO(), DevInput, Graphics->RenderDim, dt);        
    UpdateRenderBuffer(&Context->DevRenderBuffer, Graphics, Graphics->RenderDim);
    
    dev_camera* DevCamera = &Context->DevCameras[0];        
    
    ak_v2i MouseDelta = DevInput->MouseCoordinates - DevInput->LastMouseCoordinates;
    
    ak_v3f* SphericalCoordinates = &DevCamera->SphericalCoordinates;
    
    ak_f32 Roll = 0;
    ak_f32 Pitch = 0;        
    
    if(IsDown(DevInput->Alt))
    {
        if(IsDown(DevInput->LMB))
        {
            SphericalCoordinates->inclination += MouseDelta.y*1e-3f;
            SphericalCoordinates->azimuth -= MouseDelta.x*1e-3f;
            
            ak_f32 InclindationDegree = AK_ToDegree(SphericalCoordinates->inclination);
            if(InclindationDegree < -180.0f)
            {
                ak_f32 Diff = InclindationDegree + 180.0f;
                InclindationDegree = 180.0f - Diff;
                SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
            }
            else if(InclindationDegree > 180.0f)
            {
                ak_f32 Diff = InclindationDegree - 180.0f;
                InclindationDegree = -180.0f + Diff;
                SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
            }            
        }
    }    
    
    view_settings ViewSettings = {};
    ViewSettings.Position = DevCamera->Target + AK_SphericalToCartesian(*SphericalCoordinates);            
    ak_v3f Up = AK_ZAxis();        
    ak_f32 Degree = AK_ToDegree(SphericalCoordinates->inclination);            
    if(Degree == 0)
        Up = AK_YAxis();    
    if(Degree > 0)
        Up = -AK_ZAxis();    
    if(Degree < -180.0f)
        Up = -AK_ZAxis();
    
    
    ViewSettings.Orientation = AK_OrientAt(ViewSettings.Position, DevCamera->Target, Up);       
    ViewSettings.ZNear = 0.01f;
    ViewSettings.ZFar = 1000.0f;
    ViewSettings.FieldOfView = AK_PI*0.3f;
    
    if(!IsDown(DevInput->Alt) && !GetIO().WantCaptureMouse)
    {
        if(IsPressed(DevInput->LMB))
        {
            ray RayCast = DevRay_GetRayFromMouse(DevInput->MouseCoordinates, &ViewSettings, Graphics->RenderDim);
            Context->SelectedObject = DevRay_CastToAllSelectables(Context, RayCast, 0, ViewSettings.ZNear);
        }
    }
    
    if(Context->SelectedObject.Type != DEV_SELECTED_OBJECT_TYPE_NONE)
    {
        if(IsPressed(DevInput->Delete))
        {
            switch(Context->SelectedObject.Type)
            {
                case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                {
                    DevContext_DeleteDevEntity(Context->InitialEntityStorage, Context->SelectedObject.EntityID);
                    Context->SelectedObject = {};
                } break;
                
                case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                {
                    DevContext_DeletePointLight(Context->InitialPointLights, Context->SelectedObject.PointLightID);
                    Context->SelectedObject = {};
                } break;                                
            }
        }
    }
    
    DevUI_Update(Context, &Context->DevUI);
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    
    ak_u32 GraphicsObjectCount = 0;
    graphics_object* GraphicsObjects = GlobalArena->PushArray<graphics_object>(Context->InitialEntityStorage[0].MaxUsed);
    AK_ForEach(DevEntity, &Context->InitialEntityStorage[0])
    {
        graphics_object* GraphicsObject = GraphicsObjects + GraphicsObjectCount++;
        GraphicsObject->WorldTransform = AK_TransformM4(DevEntity->Transform);
        GraphicsObject->MeshID = DevEntity->MeshID;
        GraphicsObject->Material = DevEntity->Material;                                          
    }  
    
    ak_u32 PointLightCount = 0;
    point_light* PointLights = GlobalArena->PushArray<point_light>(Context->InitialPointLights[0].MaxUsed);
    AK_ForEach(PointLight, &Context->InitialPointLights[0])
    {
        PointLights[PointLightCount++] = *PointLight;
    }
    
    ak_fixed_array<point_light> PointLightList = AK_CreateArray<point_light>(PointLights, PointLightCount);
    ak_fixed_array<graphics_object> GraphicsObjectList = AK_CreateArray<graphics_object>(GraphicsObjects, GraphicsObjectCount);
    PushWorldShadingCommands(Graphics, Context->DevRenderBuffer, &ViewSettings, Game->Assets, 
                             &GraphicsObjectList, &PointLightList);
    
    AK_ForEach(PointLight, &PointLightList)
    {
        DevDraw_Sphere(Context, PointLight->Position, DEV_POINT_LIGHT_RADIUS, AK_Yellow3());
    }
    
    PushDepth(Graphics, false);
    if(Context->SelectedObject.Type != DEV_SELECTED_OBJECT_TYPE_NONE)
    {
        ak_v3f Position = DevContext_GetSelectedObjectPosition(Context, &Context->SelectedObject);
        
        dev_gizmo_state* GizmoState = &Context->GizmoState;
        
        switch(GizmoState->TransformMode)
        {
            case DEV_GIZMO_MOVEMENT_TYPE_TRANSLATE:
            case DEV_GIZMO_MOVEMENT_TYPE_SCALE:
            {
                dev_mesh* GizmoMesh = (GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_SCALE) ? &Context->TriangleScaleMesh : &Context->TriangleArrowMesh;
                DevContext_PopulateNonRotationGizmos(GizmoState, GizmoMesh, &Context->TrianglePlaneMesh, Position);                
            } break;
            
            case DEV_GIZMO_MOVEMENT_TYPE_ROTATE:
            {
                DevContext_PopulateRotationGizmos(GizmoState, &Context->TriangleTorusMesh, Position);
            } break;
        }
        
        DevDraw_GizmoState(Context, GizmoState, Position);
    }
    PushDepth(Graphics, true);
    
    DevDraw_LineCapsule(Context, &Context->InitialPlayerCapsules[0], AK_Blue3());            
    DevUI_Render(Graphics, &Context->DevUI, Context->DevRenderBuffer);    
    
    //PushCopyToOutput(Graphics, Context->DevRenderBuffer);
    PushScissor(Graphics, 0, 0, Graphics->RenderDim.w, Graphics->RenderDim.h);
    PushCopyToOutput(Graphics, Context->DevUI.UIRenderBuffer);
    
    DevInput->LastMouseCoordinates = DevInput->MouseCoordinates;
    DevInput->MouseCoordinates = {};
    DevInput->Scroll = 0.0f;
    for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(DevInput->Buttons); ButtonIndex++)
        DevInput->Buttons[ButtonIndex].WasDown = DevInput->Buttons[ButtonIndex].IsDown;    
}

void DevContext_DebugLog(const ak_char* Format, ...)
{
    dev_context* Context = Dev_GetDeveloperContext();
    va_list Args;
    va_start(Args, Format);    
    Context->DevUI.Logs.Add(AK_FormatString(Context->DevUI.LogArena, Format, Args));
    va_end(Args);
}