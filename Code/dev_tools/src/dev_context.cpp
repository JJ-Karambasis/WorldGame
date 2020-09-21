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
    
    __Internal_Dev_Context__ = DevContext;
}

void DevContext_Tick()
{
    dev_context* Context = Dev_GetDeveloperContext();    
    graphics* Graphics = Context->Graphics;
    game* Game = Context->Game;
    
    Context->PlatformUpdate(&GetIO(), &Context->DevInput, Graphics->RenderDim, Game->dt);
    
    DevUI_Update(&Context->DevUI, Game);
    DevUI_Render(Graphics, Game, &Context->DevUI);    
    
    PushScissor(Graphics, 0, 0, Graphics->RenderDim.w, Graphics->RenderDim.h);
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