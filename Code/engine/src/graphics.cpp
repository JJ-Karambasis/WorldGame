void PushWorldShadingCommands(graphics_state* GraphicsState, graphics_render_buffer* RenderBuffer, 
                              view_settings* Camera, assets* Assets)
{    
    graphics_light_buffer LightBuffer = {};
    
    AK_ForEach(PointLight, PointLights)
    {
        if(PointLight->On)
        {
            LightBuffer.PointLights[LightBuffer.PointLightCount++] = 
                CreatePointLight(PointLight->Color, PointLight->Intensity, PointLight->Position, PointLight->Radius);
        }
    }
    
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
        
        AK_ForEach(Object, GraphicsObjects)            
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
        ak_m4f LightPerspective = AK_Perspective(AK_PI*1.0f, AK_SafeRatio(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT), 0.01f, PointLight->Radius);
        
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
            
            AK_ForEach(Object, GraphicsObjects)            
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
    
    AK_ForEach(Object, GraphicsObjects)            
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
        *RenderBuffer = Graphics->AllocateRenderBuffer(Graphics, RenderDim);
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