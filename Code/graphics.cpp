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

void PushScissor(graphics* Graphics, i32 X, i32 Y, i32 Width, i32 Height)
{
    push_command_scissor* PushCommandScissor = PushStruct(push_command_scissor, NoClear, 0);
    PushCommandScissor->Type = PUSH_COMMAND_SCISSOR;
    PushCommandScissor->X = X;
    PushCommandScissor->Y = Y;
    PushCommandScissor->Width = Width;
    PushCommandScissor->Height = Height;
    
    PushCommand(Graphics, PushCommandScissor);
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

void PushDrawShadedColoredMesh(graphics* Graphics, graphics_mesh* Mesh, sqt Transform, c4 Color)
{
    push_command_draw_shaded_colored_mesh* PushCommandDrawShadedColoredMesh = PushStruct(push_command_draw_shaded_colored_mesh, NoClear, 0);
    PushCommandDrawShadedColoredMesh->Type = PUSH_COMMAND_DRAW_SHADED_COLORED_MESH;
    PushCommandDrawShadedColoredMesh->Mesh = Mesh;
    PushCommandDrawShadedColoredMesh->WorldTransform = TransformM4(Transform);
    PushCommandDrawShadedColoredMesh->R = Color.r;
    PushCommandDrawShadedColoredMesh->G = Color.g;
    PushCommandDrawShadedColoredMesh->B = Color.b;
    PushCommandDrawShadedColoredMesh->A = Color.a;
    
    PushCommand(Graphics, PushCommandDrawShadedColoredMesh);    
}

void PushDrawLineMesh(graphics* Graphics, graphics_mesh* Mesh, m4 Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_line_mesh* PushCommandDrawLineMesh = PushStruct(push_command_draw_line_mesh, NoClear, 0);
    PushCommandDrawLineMesh->Type = PUSH_COMMAND_DRAW_LINE_MESH;
    PushCommandDrawLineMesh->Mesh = Mesh;
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

void PushDrawLineMesh(graphics* Graphics, graphics_mesh* Mesh, sqt Transform, c4 Color, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    PushDrawLineMesh(Graphics, Mesh, TransformM4(Transform), Color, IndexCount, IndexOffset, VertexOffset);
}

void PushDrawImGuiUI(graphics* Graphics, graphics_mesh* Mesh, graphics_texture* Texture, u32 IndexCount, u32 IndexOffset, u32 VertexOffset)
{
    push_command_draw_imgui_ui* PushCommandDrawImGuiUI = PushStruct(push_command_draw_imgui_ui, NoClear, 0);
    PushCommandDrawImGuiUI->Type = PUSH_COMMAND_DRAW_IMGUI_UI;
    PushCommandDrawImGuiUI->Mesh = Mesh;
    PushCommandDrawImGuiUI->Texture = Texture;
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

void RecordGameCommands(game* Game, graphics* Graphics, m4 Projection, m4 View)
{       
    PushClearColorAndDepth(Graphics, Black(), 1.0f);            
    PushDepth(Graphics, true);
    
    PushProjection(Graphics, Projection); 
    PushCameraView(Graphics, View);
    
    world* World = GetCurrentWorld(Game);    
    for(world_entity* Entity = GetFirstEntity(&World->EntityPool); Entity; Entity = GetNextEntity(&World->EntityPool, Entity))
    {
        if(Entity->Mesh)
        {                        
            PushDrawShadedColoredMesh(Graphics, Entity->Mesh, Entity->Transform, Entity->Color); 
        }
    }
    
}