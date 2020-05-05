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

void PushDrawShadedColoredMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_shaded_colored_mesh* PushCommandDrawShadedColoredMesh = PushStruct(push_command_draw_shaded_colored_mesh, NoClear, 0);
    PushCommandDrawShadedColoredMesh->Type = PUSH_COMMAND_DRAW_SHADED_COLORED_MESH;
    PushCommandDrawShadedColoredMesh->MeshID = MeshID;
    PushCommandDrawShadedColoredMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawShadedColoredMesh->R = Color.r;
    PushCommandDrawShadedColoredMesh->G = Color.g;
    PushCommandDrawShadedColoredMesh->B = Color.b;
    PushCommandDrawShadedColoredMesh->A = Color.a;
    
    PushCommandDrawShadedColoredMesh->IndexCount = IndexCount;
    PushCommandDrawShadedColoredMesh->IndexOffset = IndexOffset;
    PushCommandDrawShadedColoredMesh->VertexOffset = VertexOffset;
    
    PushCommand(Graphics, PushCommandDrawShadedColoredMesh);    
}

//NOTE(EVERYONE): Joints does not get copied over, make sure the pointer is alive until you have executed the push commands (like storing in a frame allocator)
void PushDrawShadedColoredSkinningMesh(graphics* Graphics, i64 MeshID, sqt Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset, m4* Joints, u32 JointCount)
{
    push_command_draw_shaded_colored_skinning_mesh* PushCommandDrawShadedColoredSkinningMesh = PushStruct(push_command_draw_shaded_colored_skinning_mesh, NoClear, 0);
    PushCommandDrawShadedColoredSkinningMesh->Type = PUSH_COMMAND_DRAW_SHADED_COLORED_SKINNING_MESH;
    PushCommandDrawShadedColoredSkinningMesh->MeshID = MeshID;
    PushCommandDrawShadedColoredSkinningMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawShadedColoredSkinningMesh->R = Color.r;
    PushCommandDrawShadedColoredSkinningMesh->G = Color.g;
    PushCommandDrawShadedColoredSkinningMesh->B = Color.b;
    PushCommandDrawShadedColoredSkinningMesh->A = Color.a;
    
    PushCommandDrawShadedColoredSkinningMesh->IndexCount = IndexCount;
    PushCommandDrawShadedColoredSkinningMesh->IndexOffset = IndexOffset;
    PushCommandDrawShadedColoredSkinningMesh->VertexOffset = VertexOffset;
    
    PushCommandDrawShadedColoredSkinningMesh->Joints = Joints;
    PushCommandDrawShadedColoredSkinningMesh->JointCount = JointCount;
    
    PushCommand(Graphics, PushCommandDrawShadedColoredSkinningMesh);
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

void PushDrawQuad(graphics* Graphics, v3f P0, v3f P1, v3f P2, v3f P3, f32 R, f32 G, f32 B, f32 A)
{
    push_command_draw_quad* PushCommandDrawQuad = PushStruct(push_command_draw_quad, NoClear, 0);
    PushCommandDrawQuad->Type = PUSH_COMMAND_DRAW_QUAD;
    PushCommandDrawQuad->P0 = P0;
    PushCommandDrawQuad->P1 = P1;
    PushCommandDrawQuad->P2 = P2;
    PushCommandDrawQuad->P3 = P3;
    PushCommandDrawQuad->R = R;
    PushCommandDrawQuad->G = G;
    PushCommandDrawQuad->B = B;
    PushCommandDrawQuad->A = A;
    
    PushCommand(Graphics, PushCommandDrawQuad);
}

void PushDrawQuad(graphics* Graphics, v3f P0, v3f P1, v3f P2, v3f P3, c4 Color)
{
    PushDrawQuad(Graphics, P0, P1, P2, P3, Color.r, Color.g, Color.b, Color.a);    
}

void PushDrawQuad(graphics* Graphics, v3f* P, c4 Color)
{
    PushDrawQuad(Graphics, P[0], P[1], P[2], P[3], Color.r, Color.g, Color.b, Color.a);    
}

void PushViewportAndScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    PushViewport(Graphics, X, Y, Width, Height);
    PushScissor(Graphics, X, Y, Width, Height);
}

void PushWorldCommands(graphics* Graphics, world* World, camera* Camera)
{            
    m4 Perspective = PerspectiveM4(CAMERA_FIELD_OF_VIEW, SafeRatio(Graphics->RenderDim.width, Graphics->RenderDim.height), CAMERA_ZNEAR, CAMERA_ZFAR);
    m4 CameraView = InverseTransformM4(Camera->Position, Camera->Orientation);        
    
    PushProjection(Graphics, Perspective); 
    PushCameraView(Graphics, CameraView);
    
    pool_iter<world_entity> Iter = BeginIter(&World->EntityPool);
    for(world_entity* Entity = GetFirst(&Iter); Entity; Entity = GetNext(&Iter))
    {
        if(Entity->Mesh)
        {                        
            if(Entity->Type == WORLD_ENTITY_TYPE_PLAYER)
            {
#if 0 
                player* Player = (player*)Entity->UserData;
                animation_controller* AnimationController = &Player->AnimationController;                
                PushDrawShadedColoredSkinningMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, Entity->Mesh->IndexCount, 0, 0, 
                                                  AnimationController->GlobalPoses, AnimationController->Skeleton->JointCount);
#else
                PushDrawShadedColoredMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, Entity->Mesh->IndexCount, 0, 0); 
#endif
                
            }
            else
            {
                PushDrawShadedColoredMesh(Graphics, Entity->Mesh->GDIHandle, Entity->Transform, Entity->Color, Entity->Mesh->IndexCount, 0, 0); 
            }
        }
    }    
}

void PushWorldCommands(graphics* Graphics, world* World)
{            
    PushWorldCommands(Graphics, World, &World->Camera);        
}