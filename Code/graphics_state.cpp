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
    ViewSettings.ZFar = 1000.0f;
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
graphics_mesh_id LoadGraphicsMesh(assets* Assets, graphics* Graphics, mesh_asset_id ID)
{
    mesh_info* MeshInfo = GetMeshInfo(Assets, ID);
    mesh* Mesh = GetMesh(Assets, ID);
    if(!Mesh)    
        Mesh = LoadMesh(Assets, ID);                    
    
    graphics_vertex_format VertexFormat = MeshInfo->Header.IsSkeletalMesh ? GRAPHICS_VERTEX_FORMAT_P3_N3_UV_WEIGHTS : GRAPHICS_VERTEX_FORMAT_P3_N3_UV;
    graphics_index_format IndexFormat = MeshInfo->Header.IsIndexFormat32 ? GRAPHICS_INDEX_FORMAT_32_BIT : GRAPHICS_INDEX_FORMAT_16_BIT;
    
    ak_u32 VerticesSize = GetVertexStride(MeshInfo)*MeshInfo->Header.VertexCount;                
    ak_u32 IndicesSize = GetIndexStride(MeshInfo)*MeshInfo->Header.IndexCount;        
    
    graphics_mesh_id Result = Graphics->AllocateMesh(Graphics, Mesh->Vertices, VerticesSize, VertexFormat, Mesh->Indices, IndicesSize, IndexFormat); 
    Assets->GraphicsMeshes[ID] = Result;
    return Result;
}

graphics_mesh_id GetOrLoadGraphicsMesh(assets* Assets, graphics* Graphics, mesh_asset_id ID)
{    
    graphics_mesh_id MeshHandle = GetGraphicsMeshID(Assets, ID);
    if(MeshHandle == INVALID_GRAPHICS_MESH_ID)
        MeshHandle = LoadGraphicsMesh(Assets, Graphics, ID);
    return MeshHandle;
}

graphics_texture_format GetTextureFormat(ak_u32 ComponentCount, ak_bool IsSRGB)
{
    switch(ComponentCount)
    {
        case 1:
        {
            return GRAPHICS_TEXTURE_FORMAT_R8;
        } break;
        
        case 3:
        {
            return IsSRGB ? GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB : GRAPHICS_TEXTURE_FORMAT_R8G8B8;
        } break;
        
        case 4:
        {
            return IsSRGB ? GRAPHICS_TEXTURE_FORMAT_R8G8B8_SRGB_ALPHA8 : GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return (graphics_texture_format)-1;
}

graphics_texture_format GetTextureFormat(texture_info* Info)
{
    graphics_texture_format Result = GetTextureFormat(Info->Header.ComponentCount, Info->Header.IsSRGB);
    return Result;    
}

graphics_texture_id LoadGraphicsTexture(assets* Assets, graphics* Graphics, texture_asset_id ID)
{   
    texture_info* TextureInfo = GetTextureInfo(Assets, ID);
    texture* Texture = GetTexture(Assets, ID);
    if(!Texture)
        Texture = LoadTexture(Assets, ID);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture_format Format = GetTextureFormat(TextureInfo);
    
    graphics_texture_id Result = Graphics->AllocateTexture(Graphics, Texture->Texels, TextureInfo->Header.Width, TextureInfo->Header.Height, Format, &SamplerInfo);
    Assets->GraphicsTextures[ID] = Result;
    return Result;
}

graphics_texture_id GetOrLoadGraphicsTexture(assets* Assets, graphics* Graphics, texture_asset_id ID)
{
    graphics_texture_id TextureHandle = GetGraphicsTextureID(Assets, ID);
    if(TextureHandle == INVALID_GRAPHICS_TEXTURE_ID)
        TextureHandle = LoadGraphicsTexture(Assets, Graphics, ID);
    return TextureHandle;
}

graphics_diffuse_material_slot ConvertToGraphicsDiffuse(assets* Assets, graphics* Graphics, material_diffuse Diffuse)
{
    graphics_diffuse_material_slot DiffuseSlot;
    
    DiffuseSlot.IsTexture = Diffuse.IsTexture;
    if(DiffuseSlot.IsTexture)            
        DiffuseSlot.DiffuseID = GetOrLoadGraphicsTexture(Assets, Graphics, Diffuse.DiffuseID);    
    else            
        DiffuseSlot.Diffuse = Diffuse.Diffuse;     
    
    return DiffuseSlot;
}

graphics_normal_material_slot ConvertToGraphicsNormal(assets* Assets, graphics* Graphics, material_normal Normal)
{
    graphics_normal_material_slot NormalSlot;
    NormalSlot.InUse = Normal.InUse;
    if(!NormalSlot.InUse)
        return NormalSlot;
    
    NormalSlot.NormalID = GetOrLoadGraphicsTexture(Assets, Graphics, Normal.NormalID);
    return NormalSlot;
}

graphics_specular_material_slot ConvertToGraphicsSpecular(assets* Assets, graphics* Graphics, material_specular Specular)
{
    graphics_specular_material_slot SpecularSlot;
    SpecularSlot.InUse = Specular.InUse;
    if(!SpecularSlot.InUse)
        return SpecularSlot;
    
    SpecularSlot.IsTexture = Specular.IsTexture;
    if(SpecularSlot.IsTexture)
        SpecularSlot.SpecularID = GetOrLoadGraphicsTexture(Assets, Graphics, Specular.SpecularID);    
    else    
        SpecularSlot.Specular = Specular.Specular;    
    
    SpecularSlot.Shininess = Specular.Shininess;
    return SpecularSlot;
}

graphics_material ConvertToGraphicsMaterial(assets* Assets, graphics* Graphics, material* Material)
{
    graphics_material GraphicsMaterial;
    GraphicsMaterial.Diffuse = ConvertToGraphicsDiffuse(Assets, Graphics, Material->Diffuse);
    GraphicsMaterial.Normal = ConvertToGraphicsNormal(Assets, Graphics, Material->Normal);
    GraphicsMaterial.Specular = ConvertToGraphicsSpecular(Assets, Graphics, Material->Specular);
    return GraphicsMaterial;
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
        ak_m4f LightPerspective = AK_Perspective(AK_PI*0.5f, AK_SafeRatio(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT), 0.01f, PointLight->Radius);
        
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

void EntityPass(graphics* Graphics, assets* Assets, graphics_light_buffer* LightBuffer, graphics_entity_storage* GraphicsEntityStorage)
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