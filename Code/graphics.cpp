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

void PushViewTransform(graphics* Graphics, v3f Position, m3 Orientation)
{
    push_command_view_transform* PushCommandViewTransform = PushStruct(push_command_view_transform, NoClear, 0);
    PushCommandViewTransform->Type = PUSH_COMMAND_VIEW_TRANSFORM;
    PushCommandViewTransform->Position = Position;
    PushCommandViewTransform->Orientation = Orientation;
    
    PushCommand(Graphics, PushCommandViewTransform);
}

void PushViewTransform(graphics* Graphics, v3f Position, quaternion Orientation)
{
    PushViewTransform(Graphics, Position, ToMatrix3(Orientation));
}

void PushSubmitLightBuffer(graphics* Graphics, graphics_light_buffer* LightBuffer)
{
    push_command_submit_light_buffer* PushCommandSubmitLightBuffer = PushStruct(push_command_submit_light_buffer, NoClear, 0);
    PushCommandSubmitLightBuffer->Type = PUSH_COMMAND_SUBMIT_LIGHT_BUFFER;
    CopyMemory(&PushCommandSubmitLightBuffer->LightBuffer, LightBuffer, sizeof(graphics_light_buffer));
    
    PushCommand(Graphics, PushCommandSubmitLightBuffer);
}

#define DRAW_INFO(Command) Command->DrawInfo = {IndexCount, IndexOffset, VertexOffset}
void PushDrawColoredMesh(graphics* Graphics, push_command_type Type, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_colored_mesh* PushCommandDrawColoredMesh = PushStruct(push_command_draw_colored_mesh, NoClear, 0);
    PushCommandDrawColoredMesh->Type = Type;
    PushCommandDrawColoredMesh->MeshID = MeshID;
    PushCommandDrawColoredMesh->WorldTransform = Transform;
    PushCommandDrawColoredMesh->Color = Color;
    
    DRAW_INFO(PushCommandDrawColoredMesh);
    
    PushCommand(Graphics, PushCommandDrawColoredMesh);
}

void PushDrawColoredLineMesh(graphics* Graphics, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{    
    PushDrawColoredMesh(Graphics, PUSH_COMMAND_DRAW_COLORED_LINE_MESH, MeshID, Transform, Color, IndexCount, IndexOffset, VertexOffset);    
}

void PushDrawColoredMesh(graphics* Graphics, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawColoredMesh(Graphics, PUSH_COMMAND_DRAW_COLORED_MESH, MeshID, Transform, Color, IndexCount, IndexOffset, VertexOffset);    
}

void PushDrawTexturedMesh(graphics* Graphics, i64 MeshID, m4 Transform, i64 TextureID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_textured_mesh* PushCommandDrawTexturedMesh = PushStruct(push_command_draw_textured_mesh, NoClear, 0);
    PushCommandDrawTexturedMesh->Type = PUSH_COMMAND_DRAW_TEXTURED_MESH;
    PushCommandDrawTexturedMesh->WorldTransform = Transform;
    PushCommandDrawTexturedMesh->MeshID = MeshID;
    PushCommandDrawTexturedMesh->TextureID = TextureID;
    
    DRAW_INFO(PushCommandDrawTexturedMesh);
    
    PushCommand(Graphics, PushCommandDrawTexturedMesh);
}

void PushDrawTexturedMesh(graphics* Graphics, i64 MeshID, sqt Transform, i64 TextureID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawTexturedMesh(Graphics, MeshID, TransformM4(Transform), TextureID, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawColoredSkinningMesh(graphics* Graphics, i64 MeshID, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, u32 JointCount, m4* Joints)
{
    push_command_draw_colored_skinning_mesh* PushCommandDrawColoredSkinningMesh = PushStruct(push_command_draw_colored_skinning_mesh, NoClear, 0);
    PushCommandDrawColoredSkinningMesh->Type = PUSH_COMMAND_DRAW_COLORED_SKINNING_MESH;
    PushCommandDrawColoredSkinningMesh->MeshID = MeshID;
    PushCommandDrawColoredSkinningMesh->WorldTransform = Transform;
    
    DRAW_INFO(PushCommandDrawColoredSkinningMesh);
    
    PushCommandDrawColoredSkinningMesh->Joints = Joints;
    PushCommandDrawColoredSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawColoredSkinningMesh);
}

void PushDrawTexturedSkinningMesh(graphics* Graphics, i64 MeshID, m4 Transform, i64 TextureID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, u32 JointCount, m4* Joints)
{
    push_command_draw_textured_skinning_mesh* PushCommandDrawTexturedSkinningMesh = PushStruct(push_command_draw_textured_skinning_mesh, NoClear, 0);
    PushCommandDrawTexturedSkinningMesh->Type = PUSH_COMMAND_DRAW_TEXTURED_SKINNING_MESH;
    PushCommandDrawTexturedSkinningMesh->MeshID = MeshID;
    PushCommandDrawTexturedSkinningMesh->TextureID = TextureID;
    PushCommandDrawTexturedSkinningMesh->WorldTransform = Transform;
    
    DRAW_INFO(PushCommandDrawTexturedSkinningMesh);
    
    PushCommandDrawTexturedSkinningMesh->Joints = Joints;
    PushCommandDrawTexturedSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawTexturedSkinningMesh);
}

void PushDrawLambertianColoredMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 DiffuseColor, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_lambertian_colored_mesh* PushCommandDrawLambertianColoredMesh = PushStruct(push_command_draw_lambertian_colored_mesh, NoClear, 0);
    PushCommandDrawLambertianColoredMesh->Type = PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_MESH;
    PushCommandDrawLambertianColoredMesh->MeshID = MeshID;
    PushCommandDrawLambertianColoredMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawLambertianColoredMesh->DiffuseColor = DiffuseColor;
    
    DRAW_INFO(PushCommandDrawLambertianColoredMesh);
    
    PushCommand(Graphics, PushCommandDrawLambertianColoredMesh);
}

void PushDrawLambertianTexturedMesh(graphics* Graphics, i64 MeshID, m4 Transform, i64 DiffuseID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_lambertian_textured_mesh* PushCommandDrawLambertianTexturedMesh = PushStruct(push_command_draw_lambertian_textured_mesh, NoClear, 0);
    PushCommandDrawLambertianTexturedMesh->Type = PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_MESH;
    PushCommandDrawLambertianTexturedMesh->MeshID = MeshID;
    PushCommandDrawLambertianTexturedMesh->DiffuseID = DiffuseID;
    PushCommandDrawLambertianTexturedMesh->WorldTransform = Transform;
    
    DRAW_INFO(PushCommandDrawLambertianTexturedMesh);
    
    PushCommand(Graphics, PushCommandDrawLambertianTexturedMesh);
}

void PushDrawLambertianTexturedMesh(graphics* Graphics, i64 MeshID, sqt Transform, i64 DiffuseID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawLambertianTexturedMesh(Graphics, MeshID, TransformM4(Transform), DiffuseID, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawLambertianColoredSkinningMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 DiffuseColor, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, u32 JointCount, m4* Joints)
{
    push_command_draw_lambertian_colored_skinning_mesh* PushCommandDrawLambertianColoredSkinningMesh = PushStruct(push_command_draw_lambertian_colored_skinning_mesh, NoClear, 0);
    PushCommandDrawLambertianColoredSkinningMesh->Type = PUSH_COMMAND_DRAW_LAMBERTIAN_COLORED_SKINNING_MESH;
    PushCommandDrawLambertianColoredSkinningMesh->MeshID = MeshID;
    PushCommandDrawLambertianColoredSkinningMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawLambertianColoredSkinningMesh->DiffuseColor = DiffuseColor;
    
    DRAW_INFO(PushCommandDrawLambertianColoredSkinningMesh);
    
    PushCommandDrawLambertianColoredSkinningMesh->Joints = Joints;
    PushCommandDrawLambertianColoredSkinningMesh->JointCount = JointCount;   
    
    PushCommand(Graphics, PushCommandDrawLambertianColoredSkinningMesh);
}

void PushDrawLambertianTexturedSkinningMesh(graphics* Graphics, i64 MeshID, m4 Transform, i64 DiffuseID, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, u32 JointCount, m4* Joints)
{
    push_command_draw_lambertian_textured_skinning_mesh* PushCommandDrawLambertianTexturedSkinningMesh = PushStruct(push_command_draw_lambertian_textured_skinning_mesh, NoClear, 0);
    PushCommandDrawLambertianTexturedSkinningMesh->Type = PUSH_COMMAND_DRAW_LAMBERTIAN_TEXTURED_SKINNING_MESH;
    PushCommandDrawLambertianTexturedSkinningMesh->MeshID = MeshID;
    PushCommandDrawLambertianTexturedSkinningMesh->DiffuseID = DiffuseID;
    PushCommandDrawLambertianTexturedSkinningMesh->WorldTransform = Transform;
    
    DRAW_INFO(PushCommandDrawLambertianTexturedSkinningMesh);
    
    PushCommandDrawLambertianTexturedSkinningMesh->Joints = Joints;
    PushCommandDrawLambertianTexturedSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawLambertianTexturedSkinningMesh);
}

void PushDrawPhongColoredMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 DiffuseColor, c4 SpecularColor, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_phong_colored_mesh* PushCommandDrawPhongColoredMesh = PushStruct(push_command_draw_phong_colored_mesh, NoClear, 0);
    PushCommandDrawPhongColoredMesh->Type = PUSH_COMMAND_DRAW_PHONG_COLORED_MESH;
    PushCommandDrawPhongColoredMesh->MeshID = MeshID;
    PushCommandDrawPhongColoredMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawPhongColoredMesh->DiffuseColor = DiffuseColor;
    PushCommandDrawPhongColoredMesh->SpecularColor = SpecularColor;
    PushCommandDrawPhongColoredMesh->Shininess = Shininess;
    
    DRAW_INFO(PushCommandDrawPhongColoredMesh);
    
    PushCommand(Graphics, PushCommandDrawPhongColoredMesh);    
}

void PushDrawPhongTexturedMesh(graphics* Graphics, i64 MeshID, sqt Transform, i64 DiffuseID, i64 SpecularID, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_phong_textured_mesh* PushCommandDrawPhongTexturedMesh = PushStruct(push_command_draw_phong_textured_mesh, NoClear, 0);
    PushCommandDrawPhongTexturedMesh->Type = PUSH_COMMAND_DRAW_PHONG_TEXTURED_MESH;
    PushCommandDrawPhongTexturedMesh->MeshID = MeshID;
    PushCommandDrawPhongTexturedMesh->DiffuseID = DiffuseID;
    PushCommandDrawPhongTexturedMesh->SpecularID = SpecularID;
    PushCommandDrawPhongTexturedMesh->Shininess = Shininess;
    PushCommandDrawPhongTexturedMesh->WorldTransform = TransformM4(Transform);
    
    DRAW_INFO(PushCommandDrawPhongTexturedMesh);
    
    PushCommand(Graphics, PushCommandDrawPhongTexturedMesh);
}

//NOTE(EVERYONE): Joints does not get copied over, make sure the pointer is alive until you have executed the push commands (like storing in a frame allocator)
void PushDrawPhongColoredSkinningMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 DiffuseColor, c4 SpecularColor, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_phong_colored_skinning_mesh* PushCommandDrawPhongColoredSkinningMesh = PushStruct(push_command_draw_phong_colored_skinning_mesh, NoClear, 0);
    PushCommandDrawPhongColoredSkinningMesh->Type = PUSH_COMMAND_DRAW_PHONG_COLORED_SKINNING_MESH;
    PushCommandDrawPhongColoredSkinningMesh->MeshID = MeshID;
    PushCommandDrawPhongColoredSkinningMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawPhongColoredSkinningMesh->DiffuseColor = DiffuseColor;
    PushCommandDrawPhongColoredSkinningMesh->SpecularColor = SpecularColor;
    PushCommandDrawPhongColoredSkinningMesh->Shininess = Shininess;
    
    DRAW_INFO(PushCommandDrawPhongColoredSkinningMesh);
    
    PushCommandDrawPhongColoredSkinningMesh->Joints = Joints;
    PushCommandDrawPhongColoredSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawPhongColoredSkinningMesh);
}

void PushDrawPhongTexturedSkinningMesh(graphics* Graphics, i64 MeshID, sqt Transform, i64 DiffuseID, i64 SpecularID, i32 Shininess, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_phong_textured_skinning_mesh* PushCommandDrawPhongTexturedSkinningMesh = PushStruct(push_command_draw_phong_textured_skinning_mesh, NoClear, 0);
    PushCommandDrawPhongTexturedSkinningMesh->Type = PUSH_COMMAND_DRAW_PHONG_TEXTURED_SKINNING_MESH;
    PushCommandDrawPhongTexturedSkinningMesh->MeshID = MeshID;
    PushCommandDrawPhongTexturedSkinningMesh->DiffuseID = DiffuseID;
    PushCommandDrawPhongTexturedSkinningMesh->SpecularID = SpecularID;
    PushCommandDrawPhongTexturedSkinningMesh->WorldTransform = TransformM4(Transform);    
    PushCommandDrawPhongTexturedSkinningMesh->Shininess = Shininess;
    
    DRAW_INFO(PushCommandDrawPhongTexturedSkinningMesh);
    
    PushCommandDrawPhongTexturedSkinningMesh->Joints = Joints;
    PushCommandDrawPhongTexturedSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawPhongTexturedSkinningMesh);
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

void PushLightForShadowMap(graphics* Graphics, graphics_directional_light* DirectionalLight)
{        
    push_command_light_for_shadow_map* PushCommandLightForShadowMap = PushStruct(push_command_light_for_shadow_map, NoClear, 0);
    PushCommandLightForShadowMap->Type = PUSH_COMMAND_LIGHT_FOR_SHADOW_MAP;
    
    PushCommandLightForShadowMap->LightView = GetLightViewMatrix(DirectionalLight->Position, DirectionalLight->Direction);
    PushCommandLightForShadowMap->LightProjection = GetLightProjectionMatrix();    
    
    PushCommand(Graphics, PushCommandLightForShadowMap);
}


void PushDrawShadowedMesh(graphics* Graphics, i64 MeshID, sqt Transform, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_shadowed_mesh* PushCommandDrawShadowedMesh = PushStruct(push_command_draw_shadowed_mesh, NoClear, 0);
    PushCommandDrawShadowedMesh->Type = PUSH_COMMAND_DRAW_SHADOWED_MESH;
    PushCommandDrawShadowedMesh->MeshID = MeshID;
    PushCommandDrawShadowedMesh->WorldTransform = TransformM4(Transform);
    
    DRAW_INFO(PushCommandDrawShadowedMesh);
    
    PushCommand(Graphics, PushCommandDrawShadowedMesh);
}

void PushViewportAndScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushCameraCommands(graphics* Graphics, camera* Camera)
{    
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    PushProjection(Graphics, Perspective);
    PushViewTransform(Graphics, Camera->Position, Camera->Orientation);    
}

void PushWorldShadingCommands(graphics* Graphics, world* World, camera* Camera, assets* Assets)
{
    graphics_light_buffer LightBuffer = {};
    LightBuffer.DirectionalLightCount = 1;
    LightBuffer.DirectionalLights[0] = CreateDirectionalLight(V3(0.0f, -5.0f, 5.0f), White3(), 1.0f, Normalize(V3(0.0f, 0.3f, -0.6f)));
    
    LightBuffer.PointLightCount = 0;
    LightBuffer.PointLights[0] = CreatePointLight(White3(), 5.0f, V3(1.0f, 0.0f, 3.0f), 10.0f);
    //LightBuffer.PointLights[1] = CreatePointLight(White3(), 5.0f, V3(-5.0f, 0.0f, 3.0f), 10.0f);
    
    PushDepth(Graphics, true);
    
#if 1
    PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    PushViewportAndScissor(Graphics, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);        
    for(u32 DirectionalLightIndex = 0; DirectionalLightIndex < LightBuffer.DirectionalLightCount; DirectionalLightIndex++)
    {                   
        graphics_directional_light* DirectionalLight = LightBuffer.DirectionalLights + DirectionalLightIndex;                
        
        PushLightForShadowMap(Graphics, DirectionalLight);
        PushClearDepth(Graphics, 1.0f);        
        FOR_EACH(Entity, &World->EntityPool)
        {
            PushDrawShadowedMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Mesh->IndexCount, 0, 0);            
        }
    }
#endif
    
#if 0 
    for(u32 PointLightIndex = 0; PointLightIndex < LightBuffer.PointLights; PointLightIndex++)
    {
        graphics_point_light* PointLight = LightBuffer.PointLights + PointLightIndex;
        PushOmniShadowMap(PointLight->ShadowMap);
        PushClearDepth(Graphics, Block4(), 1.0f);
        FOR_EACH(Entity, &World->EntityPool)
        {
            PushDrawOmniShadowedMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Mesh->IndexCount, 0, 0);
        }
    }
#endif
        
    PushSubmitLightBuffer(Graphics, &LightBuffer);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
        
    PushViewportAndScissor(Graphics, 0, 0, Graphics->RenderDim.width, Graphics->RenderDim.height);
    PushClearColorAndDepth(Graphics, Black4(), 1.0f);
    
    PushCameraCommands(Graphics, Camera);
    FOR_EACH(Entity, &World->EntityPool)        
    {
        if(Entity->Mesh)            
        {
            //PushDrawPhongColoredMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, RGBA(0.5f, 0.5f, 0.5f, 1.0f), 8, Entity->Mesh->IndexCount, 0, 0);                     
            //PushDrawLambertianColoredMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, Entity->Mesh->IndexCount, 0, 0);
            //PushDrawTexturedMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Assets->TestDiffuse.GDIHandle, Entity->Mesh->IndexCount, 0, 0);
            //PushDrawLambertianTexturedMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Assets->TestDiffuse.GDIHandle, Entity->Mesh->IndexCount, 0, 0);
            PushDrawPhongTexturedMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Assets->TestDiffuse.GDIHandle, Assets->TestSpecular.GDIHandle, 8, Entity->Mesh->IndexCount, 0, 0);
        }
    }
}
