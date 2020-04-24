void PushCommand(graphics* Graphics, push_command* Command)
{
    ASSERT(Graphics->CommandList.Count < MAX_COMMAND_COUNT);    
    Graphics->CommandList.Ptr[Graphics->CommandList.Count++] = Command;
}

void PushClear(graphics* Graphics, f32 R, f32 G, f32 B, f32 A)
{    
    push_command_clear* PushCommandClear = PushStruct(push_command_clear, NoClear, 0);
    PushCommandClear->Type = PUSH_COMMAND_CLEAR;
    PushCommandClear->R = R;
    PushCommandClear->G = G;
    PushCommandClear->B = B;
    PushCommandClear->A = A;
    
    PushCommand(Graphics, PushCommandClear);    
}

void PushClear(graphics* Graphics, c4 Color)
{
    PushClear(Graphics, Color.r, Color.g, Color.b, Color.a);
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