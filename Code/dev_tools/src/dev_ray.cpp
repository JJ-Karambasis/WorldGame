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

world_id DevRay_CastToAllJumpingQuads(ak_f32* t, jumping_quad_storage* JumpingQuadStorage, ak_u32 WorldIndex, ray Ray, ak_f32 ZNear)
{
    world_id HitJumpingQuadID = InvalidWorldID();
    AK_ForEach(JumpingQuad, &JumpingQuadStorage[WorldIndex])
    {        
        ak_v3f P0 = JumpingQuad->CenterP;
        ak_v3f L0 = Ray.Origin;
        ak_v3f L = Ray.Direction;
        
        if(AK_EqualZeroEps(L.z))
            continue;
        
        ak_f32 d = JumpingQuad->CenterP.z - Ray.Origin.z / L.z;        
        ak_v3f P = Ray.Origin + Ray.Direction*d;
        
        ak_v2f HalfDim = JumpingQuad->Dimensions*0.5f;
        ak_v2f Max = JumpingQuad->CenterP.xy + HalfDim;
        ak_v2f Min = JumpingQuad->CenterP.xy - HalfDim;
        
        if(P.xy <= Max && P.xy >= Min)
        {
            if((*t > (d-1e3f)) && (*t > ZNear))
            {
                *t = d;
                HitJumpingQuadID = MakeWorldID(JumpingQuad->ID, WorldIndex);                
            }
        }        
    }
    
    return HitJumpingQuadID;
}

world_id DevRay_CastToAllEntities(ak_f32* t, assets* Assets, graphics_state* GraphicsState, ak_u32 WorldIndex, ray Ray, ak_f32 ZNear)
{    
    world_id HitEntityID = InvalidWorldID();
    AK_ForEach(Entity, &GraphicsState[WorldIndex].GraphicsEntityStorage)
    {
        mesh_info* MeshInfo = GetMeshInfo(Assets, Entity->MeshID);
        mesh* Mesh = GetMesh(Assets, Entity->MeshID);
        if(!Mesh)
            Mesh = LoadMesh(Assets, Entity->MeshID);
        
        ray_mesh_intersection_result IntersectionResult = RayMeshIntersection(Ray.Origin, Ray.Direction, Mesh, MeshInfo, Entity->Transform);
        if(IntersectionResult.FoundCollision)
        {
            if((*t > IntersectionResult.t) && (IntersectionResult.t > ZNear))
            {
                *t = IntersectionResult.t;  
                HitEntityID = MakeWorldID(Entity->ID, WorldIndex);                
            }
        }
    }    
    return HitEntityID;
}

world_id DevRay_CastToAllPointLights(ak_f32* t, graphics_state* GraphicsState, ak_u32 WorldIndex, ray Ray, ak_f32 ZNear)
{
    world_id HitLightID = InvalidWorldID();
    AK_ForEach(PointLight, &GraphicsState[WorldIndex].PointLightStorage)
    {
        ak_f32 tHit = RaySphereIntersection(Ray.Origin, Ray.Direction, PointLight->Position, DEV_POINT_LIGHT_RADIUS);
        if(tHit != INFINITY)
        {
            if((*t > tHit) && (tHit > ZNear))
            {
                *t = tHit;
                HitLightID = MakeWorldID(PointLight->ID, WorldIndex);                
            }
        }
    }
    
    return HitLightID;
}

dev_selected_object DevRay_CastToAllSelectables(world* World, assets* Assets, ray Ray, ak_u32 WorldIndex, ak_f32 ZNear)
{    
    dev_selected_object Result = {};    
    ak_f32 tBest = INFINITY;
    
    world_id EntityID = DevRay_CastToAllEntities(&tBest, Assets, World->GraphicsStates, WorldIndex, Ray, ZNear);
    if(EntityID.IsValid())
    {
        Result.Type = DEV_SELECTED_OBJECT_TYPE_ENTITY;
        Result.EntityID = EntityID;
    }
    
    world_id JumpingQuadID = DevRay_CastToAllJumpingQuads(&tBest, World->JumpingQuadStorage, WorldIndex, Ray, ZNear);    
    if(JumpingQuadID.IsValid())
    {
        Result.Type = DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD;        
        Result.JumpingQuadID = JumpingQuadID;
    }
    
    world_id LightID = DevRay_CastToAllPointLights(&tBest, World->GraphicsStates, WorldIndex, Ray, ZNear);
    if(LightID.IsValid())
    {
        Result.Type = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
        Result.PointLightID = LightID;
    }        
    
    if(Result.Type == DEV_SELECTED_OBJECT_TYPE_ENTITY)
    {
        entity* Entity = World->EntityStorage[Result.EntityID.WorldIndex].Get(Result.EntityID.ID);
        graphics_state* GraphicsState = &World->GraphicsStates[Result.EntityID.WorldIndex];
        material Material = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID)->Material;
        Result.MaterialContext = DevUI_ContextFromMaterial(&Material);
    }
    
    return Result;
}

gizmo_intersection_result DevRay_CastToGizmos(dev_context* Context, dev_gizmo_state* GizmoState, ray Ray, ak_f32 ZNear)
{
    gizmo_intersection_result Result = {};
    if(Context->SelectedObject.Type == DEV_SELECTED_OBJECT_TYPE_NONE)
        return Result;
    
    if(GizmoState->GizmoHit.Hit)
        return GizmoState->GizmoHit;
    
    ak_f32 tBest = INFINITY;    
    
    ak_u32 GizmoCount = (GizmoState->TransformMode == DEV_GIZMO_MOVEMENT_TYPE_ROTATE) ? 3 : 6;
    for(ak_u32 GizmoIndex = 0; GizmoIndex < GizmoCount; GizmoIndex++)
    {
        dev_mesh* DevMesh = GizmoState->Gizmos[GizmoIndex].Mesh;
        if(DevMesh != NULL)
        {
            mesh_info MeshInfo = DevContext_GetMeshInfoFromDevMesh(DevMesh);
            mesh Mesh = DevContext_GetMeshFromDevMesh(DevMesh);
            
            ray_mesh_intersection_result IntersectionResult = RayMeshIntersection(Ray.Origin, Ray.Direction, &Mesh, &MeshInfo, GizmoState->Gizmos[GizmoIndex].Transform, true, false);
            if(IntersectionResult.FoundCollision)
            {               
                if((tBest > IntersectionResult.t) && (IntersectionResult.t > ZNear))
                {
                    tBest = IntersectionResult.t;
                    Result.Gizmo = &GizmoState->Gizmos[GizmoIndex];
                    Result.HitMousePosition = Ray.Origin + (tBest*Ray.Direction);
                    Result.Hit = true;                    
                }
            }
        }
    }
    
    return Result;
}