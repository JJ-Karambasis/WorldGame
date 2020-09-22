ray DevRay_GetRayFromMouse(ak_v2i MouseCoordinates, view_settings* ViewSettings, ak_v2i RenderDim)
{
    ray Result;
    
    ak_v3f NDC = AK_ToNormalizedDeviceCoordinates(AK_V2f(MouseCoordinates), AK_V2f(RenderDim));
    ak_v4f Clip = AK_V4(NDC.xy, -1.0f, 1.0f);
    
    ak_m4f InvPerspective = AK_Inverse(AK_Perspective(ViewSettings->FieldOfView, AK_SafeRatio(RenderDim.w, RenderDim.h), 
                                                      ViewSettings->ZNear, ViewSettings->ZFar));
    ak_m4f InvView = AK_TransformM4(ViewSettings->Position, ViewSettings->Orientation);
    ak_v4f RayView = Clip*InvPerspective;
    
    ak_v3f RayWorld = AK_Normalize((AK_V4(RayView.xy, -1.0f, 0.0f)*InvView).xyz);
    
    Result.Origin = ViewSettings->Position;
    Result.Direction = RayWorld;
    return Result;    
}

dev_selected_object DevRay_CastToAllSelectables(dev_context* DevContext, ray Ray, ak_u32 WorldIndex, ak_f32 ZNear)
{
    dev_selected_object Result = {};    
    ak_f32 tBest = INFINITY;
    AK_ForEach(Entity, &DevContext->InitialEntityStorage[WorldIndex])
    {
        mesh_info* MeshInfo = GetMeshInfo(DevContext->Game->Assets, Entity->MeshID);
        mesh* Mesh = GetMesh(DevContext->Game->Assets, Entity->MeshID);
        if(!Mesh)
            Mesh = LoadMesh(DevContext->Game->Assets, Entity->MeshID);
        
        ray_mesh_intersection_result IntersectionResult = RayMeshIntersection(Ray.Origin, Ray.Direction, Mesh, MeshInfo, Entity->Transform);
        if(IntersectionResult.FoundCollision)
        {
            if((tBest > IntersectionResult.t) && (IntersectionResult.t > ZNear))
            {
                tBest = IntersectionResult.t;
                Result.Type = DEV_SELECTED_OBJECT_TYPE_ENTITY;
                Result.EntityID = Entity->ID;
            }
        }
    }
    
    return Result;
}