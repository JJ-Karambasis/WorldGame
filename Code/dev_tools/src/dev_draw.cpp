void DevDraw_LineCapsule(dev_context* Context, ak_v3f P0, ak_v3f P1, ak_f32 Radius, ak_color3f Color)
{    
    ak_v3f ZAxis = P0-P1;
    ak_f32 ZScale = AK_Magnitude(ZAxis);
    ZAxis /= ZScale;
    
    dev_capsule_mesh* Mesh = &Context->LineCapsuleMesh;    
    
    PushDrawLineMesh(Context->Graphics, Mesh->MeshID, AK_TransformM4(P0, AK_Basis(ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(Context->Graphics, Mesh->MeshID, AK_TransformM4(P1, AK_Basis(-ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);    
    
    PushDrawLineMesh(Context->Graphics, Mesh->MeshID, AK_TransformM4(P1 + (ZAxis*ZScale*0.5f), AK_Basis(ZAxis), AK_V3(Radius, Radius, ZScale)),
                     Color, Mesh->BodyIndexCount, Mesh->CapIndexCount, Mesh->CapVertexCount);    
}

void DevDraw_LineCapsule(dev_context* Context, capsule* Capsule, ak_color3f Color)
{
    DevDraw_LineCapsule(Context, Capsule->P0, Capsule->P1, Capsule->Radius, Color);    
}

void DevDraw_Sphere(dev_context* Context, ak_v3f CenterP, ak_f32 Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, AK_V3(Radius, Radius, Radius));
    PushDrawUnlitMesh(Context->Graphics, Context->TriangleSphereMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), Context->TriangleSphereMesh.IndexCount, 0, 0);    
}

void DevDraw_Sphere(dev_context* Context, sphere* Sphere, ak_color3f Color)
{
    DevDraw_Sphere(Context, Sphere->CenterP, Sphere->Radius, Color);
}


void DevDraw_OrientedBox(dev_context* DevContext, ak_v3f P, ak_v3f Dim, ak_v3f XAxis, ak_v3f YAxis, ak_v3f ZAxis, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(P, AK_M3(XAxis, YAxis, ZAxis), Dim);
    PushDrawUnlitMesh(DevContext->Graphics, DevContext->TriangleBoxMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), DevContext->TriangleBoxMesh.IndexCount, 0, 0);
}


void DevDraw_Box(dev_context* DevContext, ak_v3f P, ak_v3f Dim, ak_color3f Color)
{
    DevDraw_OrientedBox(DevContext, P, Dim, AK_XAxis(), AK_YAxis(), AK_ZAxis(), Color);    
}

void DevDraw_Point(dev_context* DevContext, ak_v3f P, ak_f32 Thickness, ak_color3f Color)
{
    DevDraw_Box(DevContext, P, AK_V3(Thickness, Thickness, Thickness), Color);
}


void DevDraw_LineEllipsoid(dev_context* DevContext, ak_v3f CenterP, ak_v3f Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, Radius);
    PushDrawLineMesh(DevContext->Graphics, DevContext->LineSphereMesh.MeshID, Model, Color, DevContext->LineSphereMesh.IndexCount, 0, 0); 
}

void DevDraw_Edge(dev_context* DevContext, ak_v3f P0, ak_v3f P1, ak_color3f Color)
{
    ak_v3f ZAxis = P1-P0;
    ak_f32 ZLength = AK_Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    ak_v3f XAxis, YAxis;
    AK_Basis(ZAxis, &XAxis, &YAxis);
    DevDraw_OrientedBox(DevContext, P0, AK_V3(0.025f, 0.025f, ZLength), XAxis, YAxis, ZAxis, Color);
}


ak_color3f DevDraw_GetGizmoColor(dev_gizmo Gizmo)
{
    switch(Gizmo.MovementDirection)
    {
        case DEV_GIZMO_MOVEMENT_DIRECTION_X:
        {
            return AK_Red3();
        } break;
        
        case DEV_GIZMO_MOVEMENT_DIRECTION_Y:
        {
            return AK_Green3();
        } break;
        
        case DEV_GIZMO_MOVEMENT_DIRECTION_Z:
        {
            return AK_Blue3();
        } break;
        
        case DEV_GIZMO_MOVEMENT_DIRECTION_XY:
        {
            return AK_Blue3();
        } break;
        
        case DEV_GIZMO_MOVEMENT_DIRECTION_XZ:
        {
            return AK_Green3();
        } break;
        
        case DEV_GIZMO_MOVEMENT_DIRECTION_YZ:
        {
            return AK_Red3();
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    return AK_White3();
}

void DevDraw_GizmoState(dev_context* Context, dev_gizmo_state* GizmoState, ak_v3f Position)
{
    if(GizmoState->TransformMode != DEV_GIZMO_MOVEMENT_TYPE_ROTATE)
    {   
        for(ak_i32 i = 0; i < 3; i++)
        {
            dev_gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = DevDraw_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Context->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Context->Graphics, GRAPHICS_CULL_MODE_NONE);
        
        for(ak_i32 i = 3; i < 6; i++)
        {
            dev_gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = DevDraw_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Context->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Context->Graphics, GRAPHICS_CULL_MODE_FRONT);
    }
    else
    {
        PushCull(Context->Graphics, GRAPHICS_CULL_MODE_NONE);
        for(ak_i32 i = 0; i < 3; i++)
        {
            dev_gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = DevDraw_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Context->Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Context->Graphics, GRAPHICS_CULL_MODE_FRONT);
    }
    
    
    DevDraw_Sphere(Context, Position, 0.04f, AK_White3());    
}

void DevDraw_Grid(dev_context* DevContext, ak_i32 xLeftBound, ak_i32 xRightBound, ak_i32 yTopBound, ak_i32 yBottomBound, ak_color3f Color)
{
    for(ak_f32 x = 0; x <= xRightBound; x+= DevContext->GizmoState.GridDistance)
    {
        if(x != 0)
        {
            DevDraw_Edge(DevContext, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), Color);
        }
    }
    
    for(ak_f32 x = 0; x >= xLeftBound; x-=DevContext->GizmoState.GridDistance)
    {
        if(x != 0)
        {
            DevDraw_Edge(DevContext, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), Color);
        }
    }
    
    for(ak_f32 y = 0; y <= yBottomBound; y+=DevContext->GizmoState.GridDistance)
    {
        if(y != 0)
        {
            DevDraw_Edge(DevContext, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), Color);
        }
    }
    
    for(ak_f32 y = 0; y >= yTopBound; y-=DevContext->GizmoState.GridDistance)
    {
        if(y != 0)
        {
            DevDraw_Edge(DevContext, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), Color);
        }
    }
    
    DevDraw_Edge(DevContext, AK_V3f(0.0f, (ak_f32)yTopBound, 0.0f), AK_V3f(0.0f, (ak_f32)yBottomBound, 0.0f), AK_Green3());
    DevDraw_Edge(DevContext, AK_V3f((ak_f32)xLeftBound, 0.0f, 0.0f), AK_V3f((ak_f32)xRightBound, 0.0f, 0.0f), AK_Red3());
}