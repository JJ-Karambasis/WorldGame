void PushCommand(graphics* Graphics, push_command* Command)
{
    ASSERT(Graphics->CommandList.Count < MAX_COMMAND_COUNT);    
    Graphics->CommandList.Ptr[Graphics->CommandList.Count++] = Command;
}

void PushCommand(graphics* Graphics, push_command_type Type)
{
    push_command* Command = PushStruct(push_command, NoClear, 0);
    Command->Type = Type;
    PushCommand(Graphics, Command);
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

void PushClearDepth(graphics* Graphics, f32 Depth)
{
    push_command_clear_depth* PushCommandClearDepth = PushStruct(push_command_clear_depth, NoClear, 0);
    PushCommandClearDepth->Type = PUSH_COMMAND_CLEAR_DEPTH;
    PushCommandClearDepth->Depth = Depth;
    
    PushCommand(Graphics, PushCommandClearDepth);
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

void PushCull(graphics* Graphics, graphics_cull_mode CullMode)
{
    push_command_cull* PushCommandCull = PushStruct(push_command_cull, NoClear, 0);
    PushCommandCull->Type = PUSH_COMMAND_CULL;
    PushCommandCull->CullMode = CullMode;
    
    PushCommand(Graphics, PushCommandCull);
}

void PushWireframe(graphics* Graphics, b32 Enable)
{
    push_command_wireframe* PushCommandWireframe = PushStruct(push_command_wireframe, NoClear, 0);
    PushCommandWireframe->Type = PUSH_COMMAND_WIREFRAME;
    PushCommandWireframe->Enable = Enable;
    
    PushCommand(Graphics, PushCommandWireframe);
}

void PushSRGBRenderBufferWrites(graphics* Graphics, b32 Enable)
{
    push_command_srgb_render_buffer_writes* PushCommandSRGBRenderBufferWrites = PushStruct(push_command_srgb_render_buffer_writes, NoClear, 0);
    PushCommandSRGBRenderBufferWrites->Type = PUSH_COMMAND_SRGB_RENDER_BUFFER_WRITES;
    PushCommandSRGBRenderBufferWrites->Enable = Enable;
    
    PushCommand(Graphics, PushCommandSRGBRenderBufferWrites);
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

void PushViewProjection(graphics* Graphics, m4 Matrix)
{
    PushMatrix(Graphics, PUSH_COMMAND_VIEW_PROJECTION, Matrix);
}

void PushViewPosition(graphics* Graphics, v3f Position)
{
    push_command_view_position* PushCommandViewPosition = PushStruct(push_command_view_position, NoClear, 0);
    PushCommandViewPosition->Type = PUSH_COMMAND_VIEW_POSITION;
    PushCommandViewPosition->Position = Position;
    
    PushCommand(Graphics, PushCommandViewPosition);
}

#define DRAW_INFO(Command) Command->DrawInfo = {IndexCount, IndexOffset, VertexOffset}

void PushShadowMap(graphics* Graphics)
{
    PushCommand(Graphics, PUSH_COMMAND_SHADOW_MAP);
}

void PushOmniShadowMap(graphics* Graphics, f32 FarPlaneDistance)
{
    push_command_omni_shadow_map* PushCommandOmniShadowMap = PushStruct(push_command_omni_shadow_map, NoClear, 0);
    PushCommandOmniShadowMap->Type = PUSH_COMMAND_OMNI_SHADOW_MAP;
    PushCommandOmniShadowMap->FarPlaneDistance = FarPlaneDistance;
    PushCommand(Graphics, PushCommandOmniShadowMap);
}

void PushRenderBuffer(graphics* Graphics, graphics_render_buffer* RenderBuffer)
{
    push_command_render_buffer* PushCommandRenderBuffer = PushStruct(push_command_render_buffer, NoClear, 0);
    PushCommandRenderBuffer->Type = PUSH_COMMAND_RENDER_BUFFER;
    PushCommandRenderBuffer->RenderBuffer = RenderBuffer;
    PushCommand(Graphics, PushCommandRenderBuffer);
}

void PushLightBuffer(graphics* Graphics, graphics_light_buffer* LightBuffer)
{
    push_command_light_buffer* PushCommandLightBuffer = PushStruct(push_command_light_buffer, NoClear, 0);
    PushCommandLightBuffer->Type = PUSH_COMMAND_LIGHT_BUFFER;    
    CopyMemory(&PushCommandLightBuffer->LightBuffer, LightBuffer, sizeof(graphics_light_buffer));
    PushCommand(Graphics, PushCommandLightBuffer);
}

void PushMaterial(graphics* Graphics, graphics_material* Material)
{
    push_command_material* PushCommandMaterial = PushStruct(push_command_material, NoClear, 0);
    PushCommandMaterial->Type = PUSH_COMMAND_MATERIAL;
    PushCommandMaterial->Material = Material;
    PushCommand(Graphics, PushCommandMaterial);
}

void PushDrawMesh(graphics* Graphics, i64 MeshID, sqt Transform, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_mesh* PushCommandDrawMesh = PushStruct(push_command_draw_mesh, NoClear, 0);
    PushCommandDrawMesh->Type = PUSH_COMMAND_DRAW_MESH;
    PushCommandDrawMesh->MeshID = MeshID;
    PushCommandDrawMesh->WorldTransform = TransformM4(Transform);
    
    DRAW_INFO(PushCommandDrawMesh);
    
    PushCommand(Graphics, PushCommandDrawMesh);
}

void PushDrawSkeletonMesh(graphics* Graphics, i64 MeshID, sqt Transform, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_skeleton_mesh* PushCommandDrawSkeletonMesh = PushStruct(push_command_draw_skeleton_mesh, NoClear, 0);
    PushCommandDrawSkeletonMesh->Type = PUSH_COMMAND_DRAW_SKELETON_MESH;
    PushCommandDrawSkeletonMesh->MeshID = MeshID;
    PushCommandDrawSkeletonMesh->WorldTransform = TransformM4(Transform);
    
    DRAW_INFO(PushCommandDrawSkeletonMesh);    
    CopyMemory(PushCommandDrawSkeletonMesh->Joints, Joints, sizeof(m4)*JointCount);
    
    PushCommand(Graphics, PushCommandDrawSkeletonMesh);
}


void PushDrawUnlitMesh(graphics* Graphics, i64 MeshID, m4 Transform, graphics_diffuse_material_slot DiffuseSlot, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_unlit_mesh* PushCommandDrawUnlitMesh = PushStruct(push_command_draw_unlit_mesh, NoClear, 0);
    PushCommandDrawUnlitMesh->Type = PUSH_COMMAND_DRAW_UNLIT_MESH;
    PushCommandDrawUnlitMesh->MeshID = MeshID;
    PushCommandDrawUnlitMesh->WorldTransform = Transform;
    PushCommandDrawUnlitMesh->DiffuseSlot = DiffuseSlot;    
    
    DRAW_INFO(PushCommandDrawUnlitMesh);    
    PushCommand(Graphics, PushCommandDrawUnlitMesh);
}

void PushDrawUnlitMesh(graphics* Graphics, i64 MeshID, sqt Transform, graphics_diffuse_material_slot DiffuseSlot, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawUnlitMesh(Graphics, MeshID, TransformM4(Transform), DiffuseSlot, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawUnlitSkeletonMesh(graphics* Graphics, i64 MeshID, sqt Transform, graphics_diffuse_material_slot DiffuseSlot, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_unlit_skeleton_mesh* PushCommandDrawUnlitSkeletonMesh = PushStruct(push_command_draw_unlit_skeleton_mesh, NoClear, 0);
    PushCommandDrawUnlitSkeletonMesh->Type = PUSH_COMMAND_DRAW_UNLIT_MESH;
    PushCommandDrawUnlitSkeletonMesh->MeshID = MeshID;
    PushCommandDrawUnlitSkeletonMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawUnlitSkeletonMesh->DiffuseSlot = DiffuseSlot;    
    
    DRAW_INFO(PushCommandDrawUnlitSkeletonMesh);
    CopyMemory(PushCommandDrawUnlitSkeletonMesh->Joints, Joints, sizeof(m4)*JointCount);
    PushCommand(Graphics, PushCommandDrawUnlitSkeletonMesh);
}

void PushDrawLineMesh(graphics* Graphics, i64 MeshID, m4 Transform, c3 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_line_mesh* PushCommandDrawLineMesh = PushStruct(push_command_draw_line_mesh, NoClear, 0);
    PushCommandDrawLineMesh->Type = PUSH_COMMAND_DRAW_LINE_MESH;
    PushCommandDrawLineMesh->MeshID = MeshID;
    PushCommandDrawLineMesh->WorldTransform = Transform;
    PushCommandDrawLineMesh->Color = Color;    
    DRAW_INFO(PushCommandDrawLineMesh);
    
    PushCommand(Graphics, PushCommandDrawLineMesh);
}

void PushDrawLineMesh(graphics* Graphics, i64 MeshID, sqt Transform, c3 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawLineMesh(Graphics, MeshID, TransformM4(Transform), Color, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawImGuiUI(graphics* Graphics, i64 MeshID, i64 TextureID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_imgui_ui* PushCommandDrawImGuiUI = PushStruct(push_command_draw_imgui_ui, NoClear, 0);
    PushCommandDrawImGuiUI->Type = PUSH_COMMAND_DRAW_IMGUI_UI;
    PushCommandDrawImGuiUI->MeshID = MeshID;
    PushCommandDrawImGuiUI->TextureID = TextureID;
    
    DRAW_INFO(PushCommandDrawImGuiUI);
    
    PushCommand(Graphics, PushCommandDrawImGuiUI);
}

void PushCopyToOutput(graphics* Graphics, graphics_render_buffer* RenderBuffer, v2i DstOffset, v2i DstResolution)
{
    push_command_copy_to_output* PushCommandCopyToOutput = PushStruct(push_command_copy_to_output, NoClear, 0);
    PushCommandCopyToOutput->Type = PUSH_COMMAND_COPY_TO_OUTPUT;
    PushCommandCopyToOutput->RenderBuffer = RenderBuffer;
    PushCommandCopyToOutput->DstOffset = DstOffset;
    PushCommandCopyToOutput->DstResolution = DstResolution;
    
    PushCommand(Graphics, PushCommandCopyToOutput);
}

void PushViewportAndScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushCameraCommands(graphics* Graphics, camera* Camera, v2i RenderDim)
{    
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(RenderDim.width, RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    m4 View = InverseTransformM4(Camera->Position, Camera->Orientation);
    PushViewPosition(Graphics, Camera->Position);
    PushViewProjection(Graphics, View*Perspective);    
}

void PushRenderBufferViewportAndScissor(graphics* Graphics, graphics_render_buffer* RenderBuffer)
{
    PushRenderBuffer(Graphics, RenderBuffer);        
    PushViewportAndScissor(Graphics, 0, 0, RenderBuffer->Resolution.width, RenderBuffer->Resolution.height);
}

void PushRenderBufferCameraViewportAndScissor(graphics* Graphics, graphics_render_buffer* RenderBuffer, camera* Camera)
{
    PushRenderBufferViewportAndScissor(Graphics, RenderBuffer);            
    PushCameraCommands(Graphics, Camera, RenderBuffer->Resolution);
}

void PushWorldShadingCommands(graphics* Graphics, graphics_render_buffer* RenderBuffer, world* World, camera* Camera, assets* Assets)
{    
    graphics_light_buffer LightBuffer = {};
    LightBuffer.DirectionalLightCount = 0;        
    LightBuffer.DirectionalLights[0] = CreateDirectionalLight(V3(0.0f, 0.0f, 4.0f), White3(), 1.0f, Normalize(V3(0.0f, 0.3f, -0.6f)), 
                                                              -5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 7.5f);
    
    LightBuffer.PointLightCount = 9;
    LightBuffer.PointLights[0] = CreatePointLight(White3(),  5.0f, V3(-1.0f,  1.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[1] = CreatePointLight(Red3(),   2.0f, V3(-4.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[2] = CreatePointLight(Green3(), 2.0f, V3(-4.0f, -4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[3] = CreatePointLight(Blue3(),  2.0f, V3( 0.0f,  0.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[4] = CreatePointLight(Red3(),   2.0f, V3( 0.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[5] = CreatePointLight(Green3(), 2.0f, V3( 0.0f, -4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[6] = CreatePointLight(Blue3(),  2.0f, V3( 4.0f,  0.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[7] = CreatePointLight(Red3(),   2.0f, V3( 4.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[8] = CreatePointLight(Green3(), 2.0f, V3( 4.0f, -4.0f, 3.0f), 10.0f);    
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, false);
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    PushViewportAndScissor(Graphics, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);        
    for(u32 DirectionalLightIndex = 0; DirectionalLightIndex < LightBuffer.DirectionalLightCount; DirectionalLightIndex++)
    {                   
        graphics_directional_light* DirectionalLight = LightBuffer.DirectionalLights + DirectionalLightIndex;                
        
        PushViewProjection(Graphics, DirectionalLight->ViewProjection);
        PushShadowMap(Graphics);
        PushClearDepth(Graphics, 1.0f);        
        FOR_EACH(Entity, &World->EntityPool)
        {
            PushDrawMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Mesh->IndexCount, 0, 0);            
        }
    }
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    for(u32 PointLightIndex = 0; PointLightIndex < LightBuffer.PointLightCount; PointLightIndex++)
    {           
        graphics_point_light* PointLight = LightBuffer.PointLights + PointLightIndex;              
        m4 LightPerspective = PerspectiveM4(PI*0.5f, SHADOW_MAP_WIDTH/SHADOW_MAP_HEIGHT, 0.01f, PointLight->Radius);
        
        m4 LightViewProjections[6] = 
        {
            LookAt(PointLight->Position, PointLight->Position + Global_WorldXAxis)*LightPerspective,
            LookAt(PointLight->Position, PointLight->Position - Global_WorldXAxis)*LightPerspective,
            LookAt(PointLight->Position, PointLight->Position + Global_WorldYAxis)*LightPerspective,
            LookAt(PointLight->Position, PointLight->Position - Global_WorldYAxis)*LightPerspective,
            LookAt(PointLight->Position, PointLight->Position + Global_WorldZAxis)*LightPerspective,
            LookAt(PointLight->Position, PointLight->Position - Global_WorldZAxis)*LightPerspective
        };
        
        PushViewPosition(Graphics, PointLight->Position);
        for(u32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
        {
            PushViewProjection(Graphics, LightViewProjections[FaceIndex]);
            PushOmniShadowMap(Graphics, PointLight->Radius);
            PushClearDepth(Graphics, 1.0f);
            FOR_EACH(Entity, &World->EntityPool)
            {
                PushDrawMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Mesh->IndexCount, 0, 0);
            }
        }
    }    
    
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferCameraViewportAndScissor(Graphics, RenderBuffer, Camera);    
    PushClearColorAndDepth(Graphics, Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    PushLightBuffer(Graphics, &LightBuffer);            
    FOR_EACH(Entity, &World->EntityPool)        
    {                
        b32 Flip = false;
        if(Entity->Mesh)            
        {            
            graphics_material* Material = Entity->Material;
            PushMaterial(Graphics, Material);
            PushDrawMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Mesh->IndexCount, 0, 0);
        }
    }        
}
