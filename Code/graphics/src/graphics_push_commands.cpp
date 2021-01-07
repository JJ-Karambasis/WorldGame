#define AllocateCommand(type) AK_GetGlobalArena()->Push<type>(AK_ARENA_CLEAR_FLAGS_NOCLEAR)

void PushCommand(graphics* Graphics, push_command* Command)
{    
    Graphics->CommandList.Add(Command);    
}

void PushCommand(graphics* Graphics, push_command_type Type)
{
    push_command* Command = AllocateCommand(push_command);
    Command->Type = Type;
    PushCommand(Graphics, Command);
}

void PushClearColor(graphics* Graphics, ak_f32 R, ak_f32 G, ak_f32 B, ak_f32 A)
{    
    push_command_clear_color* PushCommandClearColor = AllocateCommand(push_command_clear_color);
    PushCommandClearColor->Type = PUSH_COMMAND_CLEAR_COLOR;
    PushCommandClearColor->R = R;
    PushCommandClearColor->G = G;
    PushCommandClearColor->B = B;
    PushCommandClearColor->A = A;
    
    PushCommand(Graphics, PushCommandClearColor);    
}

void PushClearColor(graphics* Graphics, ak_color4f Color)
{   
    PushClearColor(Graphics, Color.r, Color.g, Color.b, Color.a);    
}

void PushClearDepth(graphics* Graphics, ak_f32 Depth)
{
    push_command_clear_depth* PushCommandClearDepth = AllocateCommand(push_command_clear_depth);
    PushCommandClearDepth->Type = PUSH_COMMAND_CLEAR_DEPTH;
    PushCommandClearDepth->Depth = Depth;
    
    PushCommand(Graphics, PushCommandClearDepth);
}

void PushClearColorAndDepth(graphics* Graphics, ak_f32 R, ak_f32 G, ak_f32 B, ak_f32 A, ak_f32 Depth)
{
    push_command_clear_color_and_depth* PushCommandClearColorAndDepth = AllocateCommand(push_command_clear_color_and_depth);
    PushCommandClearColorAndDepth->Type = PUSH_COMMAND_CLEAR_COLOR_AND_DEPTH;
    PushCommandClearColorAndDepth->R = R;
    PushCommandClearColorAndDepth->G = G;
    PushCommandClearColorAndDepth->B = B;
    PushCommandClearColorAndDepth->A = A;
    PushCommandClearColorAndDepth->Depth = Depth;    
    
    PushCommand(Graphics, PushCommandClearColorAndDepth);    
}

void PushClearColorAndDepth(graphics* Graphics, ak_color4f Color, ak_f32 Depth)
{
    PushClearColorAndDepth(Graphics, Color.r, Color.g, Color.b, Color.a, Depth);
}

void PushDepth(graphics* Graphics, ak_bool Enable)
{
    push_command_depth* PushCommandDepth = AllocateCommand(push_command_depth);
    PushCommandDepth->Type = PUSH_COMMAND_DEPTH;
    PushCommandDepth->Enable = Enable;
    
    PushCommand(Graphics, PushCommandDepth);
}

void PushCull(graphics* Graphics, graphics_cull_mode CullMode)
{
    push_command_cull* PushCommandCull = AllocateCommand(push_command_cull);
    PushCommandCull->Type = PUSH_COMMAND_CULL;
    PushCommandCull->CullMode = CullMode;
    
    PushCommand(Graphics, PushCommandCull);
}

void PushWireframe(graphics* Graphics, ak_bool Enable)
{
    push_command_wireframe* PushCommandWireframe = AllocateCommand(push_command_wireframe);
    PushCommandWireframe->Type = PUSH_COMMAND_WIREFRAME;
    PushCommandWireframe->Enable = Enable;
    
    PushCommand(Graphics, PushCommandWireframe);
}

void PushSRGBRenderBufferWrites(graphics* Graphics, ak_bool Enable)
{
    push_command_srgb_render_buffer_writes* PushCommandSRGBRenderBufferWrites = AllocateCommand(push_command_srgb_render_buffer_writes);
    PushCommandSRGBRenderBufferWrites->Type = PUSH_COMMAND_SRGB_RENDER_BUFFER_WRITES;
    PushCommandSRGBRenderBufferWrites->Enable = Enable;
    
    PushCommand(Graphics, PushCommandSRGBRenderBufferWrites);
}

void PushBlend(graphics* Graphics, ak_bool Enable, graphics_blend SrcGraphicsBlend=GRAPHICS_BLEND_UNKNOWN, graphics_blend DstGraphicsBlend=GRAPHICS_BLEND_UNKNOWN)
{
    push_command_blend* PushCommandBlend = AllocateCommand(push_command_blend);
    PushCommandBlend->Type = PUSH_COMMAND_BLEND;
    PushCommandBlend->Enable = Enable;
    PushCommandBlend->SrcGraphicsBlend = SrcGraphicsBlend;
    PushCommandBlend->DstGraphicsBlend = DstGraphicsBlend;
    
    PushCommand(Graphics, PushCommandBlend);
}

void PushRect(graphics* Graphics, push_command_type Type, ak_i32 X, ak_i32 Y, ak_i32 Width, ak_i32 Height)
{
    push_command_rect* PushCommandRect = AllocateCommand(push_command_rect);
    
    PushCommandRect->Type = Type;
    PushCommandRect->X = X;
    PushCommandRect->Y = Y;
    PushCommandRect->Width = Width;
    PushCommandRect->Height = Height;
    
    PushCommand(Graphics, PushCommandRect);   
}

void PushScissor(graphics* Graphics, ak_i32 X, ak_i32 Y, ak_i32 Width, ak_i32 Height)
{
    PushRect(Graphics, PUSH_COMMAND_SCISSOR, X, Y, Width, Height);    
}

void PushViewport(graphics* Graphics, ak_i32 X, ak_i32 Y, ak_i32 Width, ak_i32 Height)
{
    PushRect(Graphics, PUSH_COMMAND_VIEWPORT, X, Y, Width, Height);
}

void PushMatrix(graphics* Graphics, push_command_type Type, ak_m4f Matrix)
{
    push_command_4x4_matrix* PushCommandMatrix = AllocateCommand(push_command_4x4_matrix);
    PushCommandMatrix->Type = Type;
    PushCommandMatrix->Matrix = Matrix;    
    
    PushCommand(Graphics, PushCommandMatrix);    
}

void PushProjection(graphics* Graphics, ak_m4f Matrix)
{
    PushMatrix(Graphics, PUSH_COMMAND_PROJECTION, Matrix);    
}

void PushViewProjection(graphics* Graphics, ak_m4f Matrix)
{
    PushMatrix(Graphics, PUSH_COMMAND_VIEW_PROJECTION, Matrix);
}

void PushViewPosition(graphics* Graphics, ak_v3f Position)
{
    push_command_view_position* PushCommandViewPosition = AllocateCommand(push_command_view_position);
    PushCommandViewPosition->Type = PUSH_COMMAND_VIEW_POSITION;
    PushCommandViewPosition->Position = Position;
    
    PushCommand(Graphics, PushCommandViewPosition);
}

#define DRAW_INFO(Command) Command->DrawInfo = {IndexCount, IndexOffset, VertexOffset}

void PushShadowMap(graphics* Graphics)
{
    PushCommand(Graphics, PUSH_COMMAND_SHADOW_MAP);
}

void PushOmniShadowMap(graphics* Graphics, ak_f32 FarPlaneDistance)
{
    push_command_omni_shadow_map* PushCommandOmniShadowMap = AllocateCommand(push_command_omni_shadow_map);
    PushCommandOmniShadowMap->Type = PUSH_COMMAND_OMNI_SHADOW_MAP;
    PushCommandOmniShadowMap->FarPlaneDistance = FarPlaneDistance;
    PushCommand(Graphics, PushCommandOmniShadowMap);
}

void PushRenderBuffer(graphics* Graphics, graphics_render_buffer* RenderBuffer)
{
    push_command_render_buffer* PushCommandRenderBuffer = AllocateCommand(push_command_render_buffer);
    PushCommandRenderBuffer->Type = PUSH_COMMAND_RENDER_BUFFER;
    PushCommandRenderBuffer->RenderBuffer = RenderBuffer;
    PushCommand(Graphics, PushCommandRenderBuffer);
}

void PushLightBuffer(graphics* Graphics, graphics_light_buffer* LightBuffer)
{
    push_command_light_buffer* PushCommandLightBuffer = AllocateCommand(push_command_light_buffer);
    PushCommandLightBuffer->Type = PUSH_COMMAND_LIGHT_BUFFER;    
    AK_MemoryCopy(&PushCommandLightBuffer->LightBuffer, LightBuffer, sizeof(graphics_light_buffer));
    PushCommand(Graphics, PushCommandLightBuffer);
}

void PushMaterial(graphics* Graphics, graphics_material Material)
{
    push_command_material* PushCommandMaterial = AllocateCommand(push_command_material);
    PushCommandMaterial->Type = PUSH_COMMAND_MATERIAL;
    PushCommandMaterial->Material = Material;
    PushCommand(Graphics, PushCommandMaterial);
}

void PushDrawMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_m4f Transform, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    push_command_draw_mesh* PushCommandDrawMesh = AllocateCommand(push_command_draw_mesh);
    PushCommandDrawMesh->Type = PUSH_COMMAND_DRAW_MESH;
    PushCommandDrawMesh->MeshID = MeshID;
    PushCommandDrawMesh->WorldTransform = Transform;
    
    DRAW_INFO(PushCommandDrawMesh);
    
    PushCommand(Graphics, PushCommandDrawMesh);
}

void PushDrawMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_sqtf Transform, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    PushDrawMesh(Graphics, MeshID, AK_TransformM4(Transform), IndexCount, IndexOffset, VertexOffset);
}

void PushDrawSkeletonMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_sqtf Transform, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset, ak_m4f* Joints, ak_u32 JointCount)
{
    push_command_draw_skeleton_mesh* PushCommandDrawSkeletonMesh = AllocateCommand(push_command_draw_skeleton_mesh);
    PushCommandDrawSkeletonMesh->Type = PUSH_COMMAND_DRAW_SKELETON_MESH;
    PushCommandDrawSkeletonMesh->MeshID = MeshID;
    PushCommandDrawSkeletonMesh->WorldTransform = AK_TransformM4(Transform);
    
    DRAW_INFO(PushCommandDrawSkeletonMesh);    
    AK_MemoryCopy(PushCommandDrawSkeletonMesh->Joints, Joints, sizeof(ak_m4f)*JointCount);
    
    PushCommand(Graphics, PushCommandDrawSkeletonMesh);
}


void PushDrawUnlitMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_m4f Transform, graphics_diffuse_material_slot DiffuseSlot, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    push_command_draw_unlit_mesh* PushCommandDrawUnlitMesh = AllocateCommand(push_command_draw_unlit_mesh);
    PushCommandDrawUnlitMesh->Type = PUSH_COMMAND_DRAW_UNLIT_MESH;
    PushCommandDrawUnlitMesh->MeshID = MeshID;
    PushCommandDrawUnlitMesh->WorldTransform = Transform;
    PushCommandDrawUnlitMesh->DiffuseSlot = DiffuseSlot;    
    
    DRAW_INFO(PushCommandDrawUnlitMesh);    
    PushCommand(Graphics, PushCommandDrawUnlitMesh);
}

void PushDrawUnlitMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_sqtf Transform, graphics_diffuse_material_slot DiffuseSlot, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    PushDrawUnlitMesh(Graphics, MeshID, AK_TransformM4(Transform), DiffuseSlot, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawUnlitSkeletonMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_m4f Transform, graphics_diffuse_material_slot DiffuseSlot, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset, ak_m4f* Joints, ak_u32 JointCount)
{
    push_command_draw_unlit_skeleton_mesh* PushCommandDrawUnlitSkeletonMesh = AllocateCommand(push_command_draw_unlit_skeleton_mesh);
    PushCommandDrawUnlitSkeletonMesh->Type = PUSH_COMMAND_DRAW_UNLIT_MESH;
    PushCommandDrawUnlitSkeletonMesh->MeshID = MeshID;
    PushCommandDrawUnlitSkeletonMesh->WorldTransform = Transform;
    PushCommandDrawUnlitSkeletonMesh->DiffuseSlot = DiffuseSlot;    
    
    DRAW_INFO(PushCommandDrawUnlitSkeletonMesh);
    AK_MemoryCopy(PushCommandDrawUnlitSkeletonMesh->Joints, Joints, sizeof(ak_m4f)*JointCount);
    PushCommand(Graphics, PushCommandDrawUnlitSkeletonMesh);
}

void PushDrawUnlitSkeletonMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_sqtf Transform, graphics_diffuse_material_slot DiffuseSlot, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset, ak_m4f* Joints, ak_u32 JointCount)
{    
    PushDrawUnlitSkeletonMesh(Graphics, MeshID, AK_TransformM4(Transform), DiffuseSlot, IndexCount, IndexOffset, VertexOffset, Joints, JointCount);
}

void PushDrawLineMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_m4f Transform, ak_color3f Color, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    push_command_draw_line_mesh* PushCommandDrawLineMesh = AllocateCommand(push_command_draw_line_mesh);
    PushCommandDrawLineMesh->Type = PUSH_COMMAND_DRAW_LINE_MESH;
    PushCommandDrawLineMesh->MeshID = MeshID;
    PushCommandDrawLineMesh->WorldTransform = Transform;
    PushCommandDrawLineMesh->Color = Color;    
    DRAW_INFO(PushCommandDrawLineMesh);
    
    PushCommand(Graphics, PushCommandDrawLineMesh);
}

void PushDrawLineMesh(graphics* Graphics, graphics_mesh_id MeshID, ak_sqtf Transform, ak_color3f Color, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    PushDrawLineMesh(Graphics, MeshID, AK_TransformM4(Transform), Color, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawImGuiUI(graphics* Graphics, graphics_mesh_id MeshID, graphics_texture_id TextureID, ak_u32 IndexCount, ak_u32 IndexOffset, ak_u32 VertexOffset)
{
    push_command_draw_imgui_ui* PushCommandDrawImGuiUI = AllocateCommand(push_command_draw_imgui_ui);
    PushCommandDrawImGuiUI->Type = PUSH_COMMAND_DRAW_IMGUI_UI;
    PushCommandDrawImGuiUI->MeshID = MeshID;
    PushCommandDrawImGuiUI->TextureID = TextureID;
    
    DRAW_INFO(PushCommandDrawImGuiUI);
    
    PushCommand(Graphics, PushCommandDrawImGuiUI);
}

void PushCopyToOutput(graphics* Graphics, graphics_render_buffer* RenderBuffer, ak_v2i DstOffset, ak_v2i DstResolution)
{
    push_command_copy_to_output* PushCommandCopyToOutput = AllocateCommand(push_command_copy_to_output);
    PushCommandCopyToOutput->Type = PUSH_COMMAND_COPY_TO_OUTPUT;
    PushCommandCopyToOutput->RenderBuffer = RenderBuffer;
    PushCommandCopyToOutput->DstOffset = DstOffset;
    PushCommandCopyToOutput->DstResolution = DstResolution;
    
    PushCommand(Graphics, PushCommandCopyToOutput);
}

void PushCopyToOutput(graphics* Graphics, graphics_render_buffer* RenderBuffer, ak_v2i DstOffset)
{
    PushCopyToOutput(Graphics, RenderBuffer, DstOffset, RenderBuffer->Resolution);
}

void PushCopyToOutput(graphics* Graphics, graphics_render_buffer* RenderBuffer)
{
    PushCopyToOutput(Graphics, RenderBuffer, AK_V2<ak_i32>());
}

void PushCopyToRenderBuffer(graphics* Graphics, graphics_render_buffer* SrcRenderBuffer, ak_v2i DstOffset, ak_v2i DstResolution)
{
    push_command_copy_to_render_buffer* PushCommandCopyToRenderBuffer = AllocateCommand(push_command_copy_to_render_buffer);
    PushCommandCopyToRenderBuffer->Type = PUSH_COMMAND_COPY_TO_RENDER_BUFFER;
    PushCommandCopyToRenderBuffer->SrcRenderBuffer = SrcRenderBuffer;    
    PushCommandCopyToRenderBuffer->DstOffset = DstOffset;
    PushCommandCopyToRenderBuffer->DstResolution = DstResolution;
    
    PushCommand(Graphics, PushCommandCopyToRenderBuffer);
}

void PushViewportAndScissor(graphics* Graphics, ak_i32 X, ak_i32 Y, ak_i32 Width, ak_i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushViewCommands(graphics* Graphics, ak_v2i Resolution, ak_v3f ViewPosition, ak_m3f ViewOrientation, ak_m4f Projection)
{    
    ak_m4f View = AK_InvTransformM4(ViewPosition, ViewOrientation);
    
    PushViewPosition(Graphics, ViewPosition);
    PushViewProjection(Graphics, View*Projection);    
}

void PushViewCommands(graphics* Graphics, ak_v2i Resolution, view_settings* CameraSettings)
{    
    PushViewCommands(Graphics, Resolution, CameraSettings->Transform.Position, CameraSettings->Transform.Orientation, CameraSettings->Projection);    
}

void PushRenderBufferViewportAndScissor(graphics* Graphics, graphics_render_buffer* RenderBuffer)
{
    PushRenderBuffer(Graphics, RenderBuffer);        
    PushViewportAndScissor(Graphics, 0, 0, RenderBuffer->Resolution.w, RenderBuffer->Resolution.h);
}

void PushRenderBufferViewportScissorAndView(graphics* Graphics, graphics_render_buffer* RenderBuffer, view_settings* View)
{
    PushRenderBufferViewportAndScissor(Graphics, RenderBuffer);            
    PushViewCommands(Graphics, RenderBuffer->Resolution, View);
}