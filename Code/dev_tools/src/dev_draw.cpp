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