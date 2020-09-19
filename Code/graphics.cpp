#define AllocateCommand(type) AK_GetGlobalArena()->Push<type>(AK_ARENA_CLEAR_FLAGS_NOCLEAR)

void PushCommand(graphics* Graphics, push_command* Command)
{
    AK_Assert(Graphics->CommandList.Count < MAX_COMMAND_COUNT, "Ran out of commands. Increase the MAX_COMMAND_COUNT");    
    Graphics->CommandList.Ptr[Graphics->CommandList.Count++] = Command;
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

void PushViewportAndScissor(graphics* Graphics, ak_i32 X, ak_i32 Y, ak_i32 Width, ak_i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushViewCommands(graphics* Graphics, ak_v2i Resolution, ak_v3f ViewPosition, ak_m3f ViewOrientation, ak_f32 FieldOfView, ak_f32 ZNear, ak_f32 ZFar)
{    
    ak_m4f Perspective = AK_Perspective(FieldOfView, AK_SafeRatio(Resolution.w, Resolution.h), ZNear, ZFar);
    ak_m4f View = AK_InvTransformM4(ViewPosition, ViewOrientation);
    
    PushViewPosition(Graphics, ViewPosition);
    PushViewProjection(Graphics, View*Perspective);    
}

void PushViewCommands(graphics* Graphics, ak_v2i Resolution, view_settings* CameraSettings)
{    
    PushViewCommands(Graphics, Resolution, CameraSettings->Position, CameraSettings->Orientation, CameraSettings->FieldOfView, CameraSettings->ZNear, CameraSettings->ZFar);    
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

graphics_mesh_id LoadGraphicsMesh(assets* Assets, graphics* Graphics, mesh_asset_id ID)
{
    mesh_info* MeshInfo = GetMeshInfo(Assets, ID);
    mesh* Mesh = GetMesh(Assets, ID);
    if(!Mesh)    
        Mesh = LoadMesh(Assets, ID);                    
    
    graphics_vertex_format VertexFormat = MeshInfo->Header.IsSkeletalMesh ? GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS : GRAPHICS_VERTEX_FORMAT_P3_N3_UV;
    graphics_index_format IndexFormat = MeshInfo->Header.IsIndexFormat32 ? GRAPHICS_INDEX_FORMAT_32_BIT : GRAPHICS_INDEX_FORMAT_16_BIT;
    
    ak_u32 VerticesSize = GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount;                
    ak_u32 IndicesSize = GetIndexStride(MeshInfo)*MeshInfo->Header.IndexCount;        
    
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, VerticesSize, VertexFormat, Mesh->Indices, IndicesSize, IndexFormat); 
    Assets->GraphicsMeshes[ID] = Result;
    return Result;
}

graphics_mesh_id GetOrLoadGraphicsMesh(assets* Assets, graphics* Graphics, mesh_asset_id ID)
{    
    graphics_mesh_id MeshHandle = GetGraphicsMeshID(Assets, ID);
    if(MeshHandle == INVALID_GRAPHICS_MESH_ID)
        MeshHandle = LoadGraphicsMesh(Assets, Graphics, ID);
    return MeshHandle;
}

graphics_texture_format GetTextureFormat(ak_u32 ComponentCount, ak_bool IsSRGB)
{
    switch(ComponentCount)
    {
        case 1:
        {
            return GRAPHICS_TEXTURE_FORMAT_R8;
        } break;
        
        case 3:
        {
            return IsSRGB ? GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB : GRAPHICS_TEXTURE_FORMAT_R8G8B8;
        } break;
        
        case 4:
        {
            return IsSRGB ? GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8 : GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (graphics_texture_format)-1;
}

graphics_texture_format GetTextureFormat(texture_info* Info)
{
    graphics_texture_format Result = GetTextureFormat(Info->Header.ComponentCount, Info->Header.IsSRGB);
    return Result;    
}

graphics_texture_id LoadGraphicsTexture(assets* Assets, graphics* Graphics, texture_asset_id ID)
{   
    texture_info* TextureInfo = GetTextureInfo(Assets, ID);
    texture* Texture = GetTexture(Assets, ID);
    if(!Texture)
        Texture = LoadTexture(Assets, ID);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture_format Format = GetTextureFormat(TextureInfo);
    
    graphics_texture_id Result = Graphics->AllocateTexture(Graphics, Texture->Texels, TextureInfo->Header.Width, TextureInfo->Header.Height, Format, &SamplerInfo);
    Assets->GraphicsTextures[ID] = Result;
    return Result;
}

graphics_texture_id GetOrLoadGraphicsTexture(assets* Assets, graphics* Graphics, texture_asset_id ID)
{
    graphics_texture_id TextureHandle = GetGraphicsTextureID(Assets, ID);
    if(TextureHandle == INVALID_GRAPHICS_TEXTURE_ID)
        TextureHandle = LoadGraphicsTexture(Assets, Graphics, ID);
    return TextureHandle;
}

graphics_diffuse_material_slot ConvertToGraphicsDiffuse(assets* Assets, graphics* Graphics, material_diffuse Diffuse)
{
    graphics_diffuse_material_slot DiffuseSlot;
    
    DiffuseSlot.IsTexture = Diffuse.IsTexture;
    if(DiffuseSlot.IsTexture)            
        DiffuseSlot.DiffuseID = GetOrLoadGraphicsTexture(Assets, Graphics, Diffuse.DiffuseID);    
    else            
        DiffuseSlot.Diffuse = Diffuse.Diffuse;     
    
    return DiffuseSlot;
}

graphics_normal_material_slot ConvertToGraphicsNormal(assets* Assets, graphics* Graphics, material_normal Normal)
{
    graphics_normal_material_slot NormalSlot;
    NormalSlot.InUse = Normal.InUse;
    if(!NormalSlot.InUse)
        return NormalSlot;
    
    NormalSlot.NormalID = GetOrLoadGraphicsTexture(Assets, Graphics, Normal.NormalID);
    return NormalSlot;
}

graphics_specular_material_slot ConvertToGraphicsSpecular(assets* Assets, graphics* Graphics, material_specular Specular)
{
    graphics_specular_material_slot SpecularSlot;
    SpecularSlot.InUse = Specular.InUse;
    if(!SpecularSlot.InUse)
        return SpecularSlot;
    
    SpecularSlot.IsTexture = Specular.IsTexture;
    if(SpecularSlot.IsTexture)
        SpecularSlot.SpecularID = GetOrLoadGraphicsTexture(Assets, Graphics, Specular.SpecularID);    
    else    
        SpecularSlot.Specular = Specular.Specular;    
    
    SpecularSlot.Shininess = Specular.Shininess;
    return SpecularSlot;
}

graphics_material ConvertToGraphicsMaterial(assets* Assets, graphics* Graphics, material* Material)
{
    graphics_material GraphicsMaterial;
    GraphicsMaterial.Diffuse = ConvertToGraphicsDiffuse(Assets, Graphics, Material->Diffuse);
    GraphicsMaterial.Normal = ConvertToGraphicsNormal(Assets, Graphics, Material->Normal);
    GraphicsMaterial.Specular = ConvertToGraphicsSpecular(Assets, Graphics, Material->Specular);
    return GraphicsMaterial;
}

void PushWorldShadingCommands(graphics* Graphics, graphics_render_buffer* RenderBuffer, view_settings* Camera, assets* Assets, graphics_object_list GraphicsObjects)
{    
    graphics_light_buffer LightBuffer = {};
    LightBuffer.DirectionalLightCount = 0;        
    LightBuffer.DirectionalLights[0] = CreateDirectionalLight(AK_V3(0.0f, 0.0f, 4.0f), AK_White3(), 1.0f, AK_Normalize(AK_V3(0.0f, 0.3f, -0.6f)), 
                                                              -5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 7.5f);
    
    LightBuffer.PointLightCount = 9;
    LightBuffer.PointLights[0] = CreatePointLight(AK_White3(), 5.0f, AK_V3(-1.0f,  1.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[1] = CreatePointLight(AK_Red3(),   2.0f, AK_V3(-4.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[2] = CreatePointLight(AK_Green3(), 2.0f, AK_V3(-4.0f, -4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[3] = CreatePointLight(AK_Blue3(),  2.0f, AK_V3( 0.0f,  0.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[4] = CreatePointLight(AK_Red3(),   2.0f, AK_V3( 0.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[5] = CreatePointLight(AK_Green3(), 2.0f, AK_V3( 0.0f, -4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[6] = CreatePointLight(AK_Blue3(),  2.0f, AK_V3( 4.0f,  0.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[7] = CreatePointLight(AK_Red3(),   2.0f, AK_V3( 4.0f,  4.0f, 3.0f), 10.0f);
    LightBuffer.PointLights[8] = CreatePointLight(AK_Green3(), 2.0f, AK_V3( 4.0f, -4.0f, 3.0f), 10.0f);    
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, false);
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    PushViewportAndScissor(Graphics, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);        
    for(ak_u32 DirectionalLightIndex = 0; DirectionalLightIndex < LightBuffer.DirectionalLightCount; DirectionalLightIndex++)
    {                   
        graphics_directional_light* DirectionalLight = LightBuffer.DirectionalLights + DirectionalLightIndex;                
        
        PushViewProjection(Graphics, DirectionalLight->ViewProjection);
        PushShadowMap(Graphics);
        PushClearDepth(Graphics, 1.0f);
        
        AK_ForEach(Object, &GraphicsObjects)            
        {
            AK_Assert(Object->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
            
            graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, Object->MeshID);            
            PushDrawMesh(Graphics, MeshHandle, Object->WorldTransform, GetMeshIndexCount(Assets, Object->MeshID), 0, 0);                        
        }
    }
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    for(ak_u32 PointLightIndex = 0; PointLightIndex < LightBuffer.PointLightCount; PointLightIndex++)
    {           
        graphics_point_light* PointLight = LightBuffer.PointLights + PointLightIndex;              
        ak_m4f LightPerspective = AK_Perspective(AK_PI*0.5f, AK_SafeRatio(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT), 0.01f, PointLight->Radius);
        
        ak_m4f LightViewProjections[6] = 
        {
            AK_LookAt(PointLight->Position, PointLight->Position + AK_XAxis(), AK_YAxis())*LightPerspective,
            AK_LookAt(PointLight->Position, PointLight->Position - AK_XAxis(), AK_YAxis())*LightPerspective,
            AK_LookAt(PointLight->Position, PointLight->Position + AK_YAxis(), AK_ZAxis())*LightPerspective,
            AK_LookAt(PointLight->Position, PointLight->Position - AK_YAxis(), AK_ZAxis())*LightPerspective,
            AK_LookAt(PointLight->Position, PointLight->Position + AK_ZAxis(), AK_YAxis())*LightPerspective,
            AK_LookAt(PointLight->Position, PointLight->Position - AK_ZAxis(), AK_YAxis())*LightPerspective
        };
        
        PushViewPosition(Graphics, PointLight->Position);
        for(ak_u32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
        {
            PushViewProjection(Graphics, LightViewProjections[FaceIndex]);
            PushOmniShadowMap(Graphics, PointLight->Radius);
            PushClearDepth(Graphics, 1.0f);
            
            AK_ForEach(Object, &GraphicsObjects)            
            {
                AK_Assert(Object->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
                
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, Object->MeshID);            
                PushDrawMesh(Graphics, MeshHandle, Object->WorldTransform, GetMeshIndexCount(Assets, Object->MeshID), 0, 0);                        
            }            
        }
    }    
    
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, RenderBuffer, Camera);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    PushLightBuffer(Graphics, &LightBuffer);            
        
    AK_ForEach(Object, &GraphicsObjects)            
    {
        AK_Assert(Object->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
        
        graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &Object->Material);            
        PushMaterial(Graphics, Material);
        
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, Object->MeshID);            
        PushDrawMesh(Graphics, MeshHandle, Object->WorldTransform, GetMeshIndexCount(Assets, Object->MeshID), 0, 0);                        
    }    
}

void UpdateRenderBuffer(graphics_render_buffer** RenderBuffer, graphics* Graphics, ak_v2i RenderDim)
{
    if(!(*RenderBuffer))
    {        
        *RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, RenderDim);
        return;
    }
    
    if((*RenderBuffer)->Resolution != RenderDim)
    {
        Graphics->FreeRenderBuffer(Graphics, *RenderBuffer);
        *RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, Graphics->RenderDim);
    }
}

graphics_object InterpolateEntity(game* Game, entity* Entity, ak_f32 t)
{
    graphics_object Result;    
    ak_sqtf OldState = *GetEntityTransformOld(Game, Entity->ID);
    ak_sqtf NewState = *GetEntityTransform(Game, Entity->ID);                
    
    ak_sqtf InterpState;
    InterpState.Translation = AK_Lerp(OldState.Translation, t, NewState.Translation);    
    InterpState.Orientation = AK_Lerp(OldState.Orientation, t, NewState.Orientation);
    InterpState.Scale = NewState.Scale;
    
    Result.WorldTransform = AK_TransformM4(InterpState);
    Result.MeshID = Entity->MeshID;
    Result.Material = Entity->Material;
    
    Result.JointCount = 0;
    return Result;
}

game_camera InterpolateCamera(game* Game, ak_u32 WorldIndex, ak_f32 t)
{    
    game_camera* OldCamera = Game->PrevCameras    + WorldIndex;
    game_camera* NewCamera = Game->CurrentCameras + WorldIndex;
    
    game_camera Result;
    
    Result.Target = AK_Lerp(OldCamera->Target, t, NewCamera->Target);
    
    //TODO(JJ): When we do some more game camera logic, we will need to interpolate the spherical coordinates as well
    //ASSERT(NewCamera->Coordinates == OldCamera->Coordinates);
    Result.SphericalCoordinates = NewCamera->SphericalCoordinates;    
    
    Result.FieldOfView = NewCamera->FieldOfView;
    Result.ZNear = NewCamera->ZNear;
    Result.ZFar = NewCamera->ZFar;
    
    return Result;
}

graphics_state GetGraphicsState(game* Game, ak_u32 WorldIndex, ak_f32 t)
{
    graphics_state Result = {};
    graphics_object_list* GraphicsObjects = &Result.GraphicsObjects;
    
    ak_arena* GlobalArena = AK_GetGlobalArena();
    
    GraphicsObjects->Objects = GlobalArena->PushArray<graphics_object>(Game->EntityStorage[WorldIndex].MaxUsed);
    AK_ForEach(Entity, &Game->EntityStorage[WorldIndex])
    {
        if(Entity->MeshID != INVALID_GRAPHICS_MESH_ID)
        {
            GraphicsObjects->Objects[GraphicsObjects->Count++] = InterpolateEntity(Game, Entity, t);            
        }
    }
    
    Result.Camera = InterpolateCamera(Game, WorldIndex, t);    
    return Result;    
}