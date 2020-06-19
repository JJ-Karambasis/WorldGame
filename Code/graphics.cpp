void PushCommand(graphics* Graphics, push_command* Command)
{
    ASSERT(Graphics->CommandList.Count < MAX_COMMAND_COUNT);    
    Graphics->CommandList.Ptr[Graphics->CommandList.Count++] = Command;
}

void PushClearColor(graphics* Graphics, f32 R, f32 G, f32 B, f32 A)
{    
    push_command_clear_color* PushCommandClearColor = PushStruct(push_command_clear_color, NoClear, 0);
    PushCommandClearColor->Type = PUSH_COMMAND_CLEAR_COLOR;
    PushCommandClearColor->R = R;
    PushCommandClearColor->G = G;
    PushCommandClearColor->B = B;
    PushCommandClearColor->A = A;
    
    PushCommand(Graphics, PushCommandClearColor);    
}

void PushClearColor(graphics* Graphics, c4 Color)
{   
    PushClearColor(Graphics, Color.r, Color.g, Color.b, Color.a);    
}

void PushClearColorAndDepth(graphics* Graphics, f32 R, f32 G, f32 B, f32 A, f32 Depth)
{
    push_command_clear_color_and_depth* PushCommandClearColorAndDepth = PushStruct(push_command_clear_color_and_depth, NoClear, 0);
    PushCommandClearColorAndDepth->Type = PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH;
    PushCommandClearColorAndDepth->R = R;
    PushCommandClearColorAndDepth->G = G;
    PushCommandClearColorAndDepth->B = B;
    PushCommandClearColorAndDepth->A = A;
    PushCommandClearColorAndDepth->Depth = Depth;    
    
    PushCommand(Graphics, PushCommandClearColorAndDepth);    
}

void PushClearColorAndDepth(graphics* Graphics, c4 Color, f32 Depth)
{
    PushClearColorAndDepth(Graphics, Color.r, Color.g, Color.b, Color.a, Depth);
}

void PushDepth(graphics* Graphics, b32 Enable)
{
    push_command_depth* PushCommandDepth = PushStruct(push_command_depth, NoClear, 0);
    PushCommandDepth->Type = PUSH_COMMAND_DEPTH;
    PushCommandDepth->Enable = Enable;
    
    PushCommand(Graphics, PushCommandDepth);
}

void PushCull(graphics* Graphics, b32 Enable)
{
    push_command_cull* PushCommandCull = PushStruct(push_command_cull, NoClear, 0);
    PushCommandCull->Type = PUSH_COMMAND_CULL;
    PushCommandCull->Enable = Enable;
    
    PushCommand(Graphics, PushCommandCull);
}

void PushWireframe(graphics* Graphics, b32 Enable)
{
    push_command_wireframe* PushCommandWireframe = PushStruct(push_command_wireframe, NoClear, 0);
    PushCommandWireframe->Type = PUSH_COMMAND_WIREFRAME;
    PushCommandWireframe->Enable = Enable;
    
    PushCommand(Graphics, PushCommandWireframe);
}

void PushBlend(graphics* Graphics, b32 Enable, graphics_blend SrcGraphicsBlend=GRAPHICS_BLEND_UNKNOWN, graphics_blend DstGraphicsBlend=GRAPHICS_BLEND_UNKNOWN)
{
    push_command_blend* PushCommandBlend = PushStruct(push_command_blend, NoClear, 0);
    PushCommandBlend->Type = PUSH_COMMAND_BLEND;
    PushCommandBlend->Enable = Enable;
    PushCommandBlend->SrcGraphicsBlend = SrcGraphicsBlend;
    PushCommandBlend->DstGraphicsBlend = DstGraphicsBlend;
    
    PushCommand(Graphics, PushCommandBlend);
}

void PushRect(graphics* Graphics, push_command_type Type, i32 X, i32 Y, i32 Width, i32 Height)
{
    push_command_rect* PushCommandRect = PushStruct(push_command_rect, NoClear, 0);
    
    PushCommandRect->Type = Type;
    PushCommandRect->X = X;
    PushCommandRect->Y = Y;
    PushCommandRect->Width = Width;
    PushCommandRect->Height = Height;
    
    PushCommand(Graphics, PushCommandRect);   
}

void PushScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushRect(Graphics, PUSH_COMMAND_SCISSOR, X, Y, Width, Height);    
}

void PushViewport(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushRect(Graphics, PUSH_COMMAND_VIEWPORT, X, Y, Width, Height);
}

void PushMatrix(graphics* Graphics, push_command_type Type, m4 Matrix)
{
    push_command_4x4_matrix* PushCommandMatrix = PushStruct(push_command_4x4_matrix, NoClear, 0);
    PushCommandMatrix->Type = Type;
    PushCommandMatrix->Matrix = Matrix;    
    
    PushCommand(Graphics, PushCommandMatrix);    
}

void PushProjection(graphics* Graphics, m4 Matrix)
{
    PushMatrix(Graphics, PUSH_COMMAND_PROJECTION, Matrix);    
}

void PushCameraView(graphics* Graphics, m4 Matrix)
{
    PushMatrix(Graphics, PUSH_COMMAND_CAMERA_VIEW, Matrix);
}

void PushSubmitLightBuffer(graphics* Graphics, graphics_light_buffer* LightBuffer)
{
    push_command_submit_light_buffer* PushCommandSubmitLightBuffer = PushStruct(push_command_submit_light_buffer, NoClear, 0);
    PushCommandSubmitLightBuffer->Type = PUSH_COMMAND_SUBMIT_LIGHT_BUFFER;
    CopyMemory(&PushCommandSubmitLightBuffer->LightBuffer, LightBuffer, sizeof(graphics_light_buffer));
    
    PushCommand(Graphics, PushCommandSubmitLightBuffer);
}

void PushDrawPhongColoredMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 SurfaceColor, c4 SpecularColor, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_phong_colored_mesh* PushCommandDrawPhongColoredMesh = PushStruct(push_command_draw_phong_colored_mesh, NoClear, 0);
    PushCommandDrawPhongColoredMesh->Type = PUSH_COMMAND_DRAW_PHONG_COLORED_MESH;
    PushCommandDrawPhongColoredMesh->MeshID = MeshID;
    PushCommandDrawPhongColoredMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawPhongColoredMesh->SurfaceColor = SurfaceColor;
    PushCommandDrawPhongColoredMesh->SpecularColor = SpecularColor;
    PushCommandDrawPhongColoredMesh->Shininess = Shininess;
    
    PushCommandDrawPhongColoredMesh->IndexCount = IndexCount;
    PushCommandDrawPhongColoredMesh->IndexOffset = IndexOffset;
    PushCommandDrawPhongColoredMesh->VertexOffset = VertexOffset;
    
    PushCommand(Graphics, PushCommandDrawPhongColoredMesh);    
}

//NOTE(EVERYONE): Joints does not get copied over, make sure the pointer is alive until you have executed the push commands (like storing in a frame allocator)
void PushDrawShadedColoredSkinningMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 SurfaceColor, c4 SpecularColor, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_phong_colored_skinning_mesh* PushCommandDrawPhongColoredSkinningMesh = PushStruct(push_command_draw_phong_colored_skinning_mesh, NoClear, 0);
    PushCommandDrawPhongColoredSkinningMesh->Type = PUSH_COMMAND_DRAW_PHONG_COLORED_SKINNING_MESH;
    PushCommandDrawPhongColoredSkinningMesh->MeshID = MeshID;
    PushCommandDrawPhongColoredSkinningMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawPhongColoredSkinningMesh->SurfaceColor = SurfaceColor;
    PushCommandDrawPhongColoredSkinningMesh->SpecularColor = SpecularColor;
    PushCommandDrawPhongColoredSkinningMesh->Shininess = Shininess;
    
    PushCommandDrawPhongColoredSkinningMesh->IndexCount = IndexCount;
    PushCommandDrawPhongColoredSkinningMesh->IndexOffset = IndexOffset;
    PushCommandDrawPhongColoredSkinningMesh->VertexOffset = VertexOffset;
    
    PushCommandDrawPhongColoredSkinningMesh->Joints = Joints;
    PushCommandDrawPhongColoredSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawPhongColoredSkinningMesh);
}

void PushDrawLineMesh(graphics* Graphics, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_line_mesh* PushCommandDrawLineMesh = PushStruct(push_command_draw_line_mesh, NoClear, 0);
    PushCommandDrawLineMesh->Type = PUSH_COMMAND_DRAW_LINE_MESH;
    PushCommandDrawLineMesh->MeshID = MeshID;
    PushCommandDrawLineMesh->WorldTransform = Transform;
    PushCommandDrawLineMesh->R = Color.r;
    PushCommandDrawLineMesh->G = Color.g;
    PushCommandDrawLineMesh->B = Color.b;
    PushCommandDrawLineMesh->A = Color.a;
    
    PushCommandDrawLineMesh->IndexCount = IndexCount;
    PushCommandDrawLineMesh->IndexOffset = IndexOffset;
    PushCommandDrawLineMesh->VertexOffset = VertexOffset;
    
    PushCommand(Graphics, PushCommandDrawLineMesh);
}

void PushDrawLineMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawLineMesh(Graphics, MeshID, TransformM4(Transform), Color, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawFilledMesh(graphics* Graphics, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_filled_mesh* PushCommandDrawFilledMesh = PushStruct(push_command_draw_filled_mesh, NoClear, 0);
    PushCommandDrawFilledMesh->Type = PUSH_COMMAND_DRAW_FILLED_MESH;
    PushCommandDrawFilledMesh->MeshID = MeshID;
    PushCommandDrawFilledMesh->WorldTransform = Transform;
    PushCommandDrawFilledMesh->R = Color.r;
    PushCommandDrawFilledMesh->G = Color.g;
    PushCommandDrawFilledMesh->B = Color.b;
    PushCommandDrawFilledMesh->A = Color.a;
    
    PushCommandDrawFilledMesh->IndexCount   = IndexCount;
    PushCommandDrawFilledMesh->IndexOffset  = IndexOffset;
    PushCommandDrawFilledMesh->VertexOffset = VertexOffset;
    
    PushCommand(Graphics, PushCommandDrawFilledMesh);
}

void PushDrawFilledMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawFilledMesh(Graphics, MeshID, TransformM4(Transform), Color, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawImGuiUI(graphics* Graphics, i64 MeshID, i64 TextureID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_imgui_ui* PushCommandDrawImGuiUI = PushStruct(push_command_draw_imgui_ui, NoClear, 0);
    PushCommandDrawImGuiUI->Type = PUSH_COMMAND_DRAW_IMGUI_UI;
    PushCommandDrawImGuiUI->MeshID = MeshID;
    PushCommandDrawImGuiUI->TextureID = TextureID;
    PushCommandDrawImGuiUI->IndexCount = IndexCount;
    PushCommandDrawImGuiUI->IndexOffset = IndexOffset;
    PushCommandDrawImGuiUI->VertexOffset = VertexOffset;
    
    PushCommand(Graphics, PushCommandDrawImGuiUI);
}

void PushViewportAndScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushCameraCommands(graphics* Graphics, camera* Camera)
{
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);            
    PushProjection(Graphics, Perspective); 
    PushCameraView(Graphics, CameraView);        
}

void PushWorldShadingCommands(graphics* Graphics, world* World)
{
    graphics_light_buffer LightBuffer = {};
    LightBuffer.DirectionalLightCount = 1;
    LightBuffer.DirectionalLights[0] = CreateDirectionalLight(White3(), 1.0f, -Global_WorldZAxis); 
    
    LightBuffer.PointLightCount = 2;
    LightBuffer.PointLights[0] = CreatePointLight(White3(), 1.0f, V3(0.0f, 0.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[1] = CreatePointLight(White3(), 1.0f, V3(-5.0f, 0.0f, 3.0f), 10.0f);
    
    PushSubmitLightBuffer(Graphics, &LightBuffer);
    
    FOR_EACH(Entity, &World->EntityPool)        
    {
        if(Entity->Mesh)            
            PushDrawPhongColoredMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, RGBA(0.5f, 0.5f, 0.5f, 1.0f), 8, Entity->Mesh->IndexCount, 0, 0);                     
    }
}

void PushGameCommands(graphics* Graphics, game* Game)
{
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);        
    
    PushClearColorAndDepth(Graphics, Black4(), 1.0f);
    PushDepth(Graphics, true);
    
    world* World = GetCurrentWorld(Game);
    
    PushCameraCommands(Graphics, &World->Camera);    
    PushWorldShadingCommands(Graphics, World);        
}