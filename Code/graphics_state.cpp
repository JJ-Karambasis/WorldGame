#include "graphics_push_commands.cpp"

void DeleteGraphicsState(graphics* Graphics, graphics_state* GraphicsState)
{
    if(GraphicsState->RenderBuffer)
        Graphics->FreeRenderBuffer(Graphics, GraphicsState->RenderBuffer);
    AK_DeletePool(&GraphicsState->GraphicsEntityStorage);
    AK_DeletePool(&GraphicsState->PointLightStorage);
}

ak_u64 CreateGraphicsEntity(graphics_state* GraphicsState, mesh_asset_id MeshID, material Material, ak_sqtf Transform, void* UserData)
{
    ak_u64 Result = GraphicsState->GraphicsEntityStorage.Allocate();
    graphics_entity* Entity = GraphicsState->GraphicsEntityStorage.Get(Result);
    Entity->ID = Result;
    Entity->MeshID = MeshID;
    Entity->Material = Material;
    Entity->Transform = AK_TransformM4(Transform);
    Entity->UserData = UserData;
    return Result;
}

inline point_light* GetPointLight(graphics_state* GraphicsState, ak_u64 ID)
{
    return GraphicsState->PointLightStorage.Get(ID);
}

ak_u64 CreatePointLight(graphics_state* GraphicsState, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity, ak_bool On)
{
    ak_u64 ID = GraphicsState->PointLightStorage.Allocate();
    point_light* PointLight = GraphicsState->PointLightStorage.Get(ID);
    PointLight->ID = ID;
    PointLight->Position = Position;
    PointLight->Radius = Radius;
    PointLight->Color = Color;
    PointLight->Intensity = Intensity;
    PointLight->On = On;    
    return ID;    
}

void CreatePointLights(graphics_state* GraphicsStates, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity, ak_bool On)
{
    CreatePointLight(&GraphicsStates[0], Position, Radius, Color, Intensity, On);
    CreatePointLight(&GraphicsStates[1], Position, Radius, Color, Intensity, On);
}

inline ak_v3f* 
GetFrustumCorners(view_settings* ViewSettings, ak_v2i RenderDim)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_v3f* Result = GlobalArena->PushArray<ak_v3f>(8);
    ak_m4f Perspective = AK_Perspective(ViewSettings->FieldOfView, AK_SafeRatio(RenderDim.w, RenderDim.h), ViewSettings->ZNear, ViewSettings->ZFar);    
    AK_GetFrustumCorners(Result, Perspective);
    AK_TransformPoints(Result, 8, AK_TransformM4(ViewSettings->Position, ViewSettings->Orientation));    
    return Result;
}

view_settings GetViewSettings(camera* Camera)
{    
    view_settings ViewSettings = {};
    ak_v3f Up = AK_ZAxis();        
    ak_f32 Degree = AK_ToDegree(Camera->SphericalCoordinates.inclination);                    
    if(Degree > 0)
        Up = -AK_ZAxis();    
    if(Degree < -180.0f)
        Up = -AK_YAxis();
    if(AK_EqualZeroEps(Degree))
        Up = AK_YAxis(); 
    if(AK_EqualEps(AK_Abs(Degree), 180.0f))
        Up = -AK_YAxis();        
    
    ViewSettings.Position = Camera->Target + AK_SphericalToCartesian(Camera->SphericalCoordinates);            
    ViewSettings.Orientation = AK_OrientAt(ViewSettings.Position, Camera->Target, Up);       
    
    ViewSettings.ZNear = 0.01f;
    ViewSettings.ZFar = 50.0f;
    ViewSettings.FieldOfView = AK_PI*0.3f;
    return ViewSettings;
}

void UpdateRenderBuffer(graphics* Graphics, graphics_render_buffer** RenderBuffer, ak_v2i RenderDim)
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

void InterpolateState(world* World, ak_u32 WorldIndex, ak_f32 t)
{
    graphics_state* GraphicsState = &World->GraphicsStates[WorldIndex];
    
    ak_sqtf* OldTransforms = World->OldTransforms[WorldIndex].Entries;
    ak_sqtf* NewTransform = World->NewTransforms[WorldIndex].Entries;
    
    AK_ForEach(GraphicsEntity, &GraphicsState->GraphicsEntityStorage)
    {
        ak_u64 EntityID = (ak_u64)GraphicsEntity->UserData;
        ak_u32 Index = AK_PoolIndex(EntityID);
        
        ak_sqtf InterpState;
        ak_sqtf OldState = OldTransforms[Index];
        ak_sqtf NewState = NewTransform[Index];
        
        InterpState.Translation = AK_Lerp(OldState.Translation, t, NewState.Translation);
        InterpState.Orientation = AK_Lerp(OldState.Orientation, t, NewState.Orientation);
        InterpState.Scale = NewState.Scale;        
        GraphicsEntity->Transform = AK_TransformM4(InterpState);
    }
    
    camera* OldCamera = &World->OldCameras[WorldIndex];
    camera* NewCamera = &World->NewCameras[WorldIndex];
    
    camera* InterpCamera = &GraphicsState->Camera;
    InterpCamera->Target = AK_Lerp(OldCamera->Target, t, NewCamera->Target);
    //TODO(JJ): Interpolate spherical coordinates    
    InterpCamera->SphericalCoordinates = NewCamera->SphericalCoordinates;
}

graphics_light_buffer GetLightBuffer(graphics_state* GraphicsState)
{
    graphics_light_buffer LightBuffer = {};    
    AK_ForEach(PointLight, &GraphicsState->PointLightStorage)
    {
        if(PointLight->On)
        {
            AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
            LightBuffer.PointLights[LightBuffer.PointLightCount++] = 
                CreatePointLight(PointLight->Color, PointLight->Intensity, PointLight->Position, PointLight->Radius);
        }
    }   
    
    return LightBuffer;
}

void ShadowPass(graphics* Graphics, assets* Assets, graphics_light_buffer* LightBuffer, graphics_entity_storage* EntityStorage)
{       
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, false);
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    PushViewportAndScissor(Graphics, 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);        
    for(ak_u32 DirectionalLightIndex = 0; DirectionalLightIndex < LightBuffer->DirectionalLightCount; DirectionalLightIndex++)
    {                   
        graphics_directional_light* DirectionalLight = LightBuffer->DirectionalLights + DirectionalLightIndex;                
        
        PushViewProjection(Graphics, DirectionalLight->ViewProjection);
        PushShadowMap(Graphics);
        PushClearDepth(Graphics, 1.0f);
        
        AK_ForEach(GraphicsEntity, EntityStorage)            
        {
            AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
            
            graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsEntity->MeshID);            
            PushDrawMesh(Graphics, MeshHandle, GraphicsEntity->Transform, GetMeshIndexCount(Assets, GraphicsEntity->MeshID), 0, 0);                        
        }
    }
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    for(ak_u32 PointLightIndex = 0; PointLightIndex < LightBuffer->PointLightCount; PointLightIndex++)
    {           
        graphics_point_light* PointLight = LightBuffer->PointLights + PointLightIndex;              
        
#if 1
        ak_f32 LightNear = 0.01f;
#else
        ak_f32 LightNear = 10.0f;
#endif
        ak_m4f LightPerspective = AK_Perspective(AK_PI*0.5f, AK_SafeRatio(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT), LightNear, PointLight->Radius);        
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
            
            AK_ForEach(GraphicsEntity, EntityStorage)            
            {
                AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
                
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsEntity->MeshID);            
                PushDrawMesh(Graphics, MeshHandle, GraphicsEntity->Transform, GetMeshIndexCount(Assets, GraphicsEntity->MeshID), 0, 0);                        
            }            
        }
    }                
}

void StandardEntityCommands(graphics* Graphics, graphics_state* GraphicsState, view_settings* ViewSettings)
{    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, GraphicsState->RenderBuffer, ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);    
}

void EntityUnlitPass(graphics* Graphics, assets* Assets, graphics_entity_storage* GraphicsEntityStorage)
{
    AK_ForEach(GraphicsEntity, GraphicsEntityStorage)            
    {
        AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
        
        graphics_diffuse_material_slot Diffuse = ConvertToGraphicsDiffuse(Assets, Graphics, GraphicsEntity->Material.Diffuse);        
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsEntity->MeshID);            
        PushDrawUnlitMesh(Graphics, MeshHandle, GraphicsEntity->Transform, Diffuse, GetMeshIndexCount(Assets, GraphicsEntity->MeshID), 0, 0);                        
    }
}

void EntityLitPass(graphics* Graphics, assets* Assets, graphics_light_buffer* LightBuffer, graphics_entity_storage* GraphicsEntityStorage)
{        
    PushLightBuffer(Graphics, LightBuffer);                
    AK_ForEach(GraphicsEntity, GraphicsEntityStorage)            
    {
        AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
        
        graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &GraphicsEntity->Material);            
        PushMaterial(Graphics, Material);
        
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsEntity->MeshID);            
        PushDrawMesh(Graphics, MeshHandle, GraphicsEntity->Transform, GetMeshIndexCount(Assets, GraphicsEntity->MeshID), 0, 0);                        
    }
}

void EntityWireframePass(graphics* Graphics, assets* Assets, graphics_entity_storage* GraphicsEntityStorage)
{
    PushWireframe(Graphics, true);
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);        
    
    AK_ForEach(GraphicsEntity, GraphicsEntityStorage)            
    {
        AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
        
        graphics_diffuse_material_slot Diffuse = ConvertToGraphicsDiffuse(Assets, Graphics, GraphicsEntity->Material.Diffuse);        
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsEntity->MeshID);            
        PushDrawUnlitMesh(Graphics, MeshHandle, GraphicsEntity->Transform, Diffuse, GetMeshIndexCount(Assets, GraphicsEntity->MeshID), 0, 0);                        
    }
    
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushWireframe(Graphics, false);
}

void JumpingQuadPass(graphics* Graphics, jumping_quad_storage* JumpingQuads, jumping_quad_graphics_mesh* Mesh)
{
    if(!Mesh->MeshID )
    {
        Mesh->MeshID = Graphics->AllocateMesh(Graphics, Mesh->Vertices, Mesh->VertexCount*sizeof(ak_vertex_p3), GRAPHICS_VERTEX_FORMAT_P3,
                                              Mesh->Indices, Mesh->IndexCount*sizeof(ak_u16), GRAPHICS_INDEX_FORMAT_16_BIT);    
    }
    
    AK_ForEach(JumpingQuad, JumpingQuads)
    {
        graphics_diffuse_material_slot QuadDiffuse = CreateDiffuseMaterialSlot(JumpingQuad->Color);
        ak_m4f Transform = AK_TransformM4(JumpingQuad->CenterP, AK_IdentityQuat<ak_f32>(), AK_V3(JumpingQuad->Dimensions, 0.0f));
        PushDrawUnlitMesh(Graphics, Mesh->MeshID, Transform, QuadDiffuse, Mesh->IndexCount, 0, 0);
    }        
}

void NormalEntityPass(graphics* Graphics, game* Game, graphics_state* GraphicsState, view_settings* ViewSettings)
{
    graphics_light_buffer LightBuffer = GetLightBuffer(GraphicsState);
    ShadowPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);
    
    StandardEntityCommands(Graphics, GraphicsState, ViewSettings);
    EntityLitPass(Graphics, Game->Assets, &LightBuffer, &GraphicsState->GraphicsEntityStorage);            
}

void NormalEntityPassPlusJumpingQuad(graphics* Graphics, game* Game, graphics_state* GraphicsState, jumping_quad_storage* JumpingQuads, view_settings* ViewSettings)
{    
    NormalEntityPass(Graphics, Game, GraphicsState, ViewSettings);
    JumpingQuadPass(Graphics, JumpingQuads, &Game->QuadMesh);    
}