#include "graphics_push_commands.cpp"

inline void
GetFrustumCorners(ak_v3f* Result, view_settings* ViewSettings, ak_v2i RenderDim)
{
    AK_GetFrustumCorners(Result, ViewSettings->Projection);
    AK_TransformPoints(Result, 8, AK_TransformM4(ViewSettings->Transform.Position, ViewSettings->Transform.Orientation));    
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

ak_rigid_transformf GetCameraTransform(perspective_camera* Camera)
{
    ak_rigid_transformf CameraTransform = {};
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
    
    CameraTransform.Position = Camera->Target + AK_SphericalToCartesian(Camera->SphericalCoordinates);            
    CameraTransform.Orientation = AK_OrientAt(CameraTransform.Position, Camera->Target, Up);       
    return CameraTransform;
}

inline ak_v3f GetCameraPosition(ortho_camera* Camera)
{
    return Camera->Target - Camera->Z*Camera->Distance;
}

inline ak_rigid_transformf GetCameraTransform(ortho_camera* Camera)
{
    ak_rigid_transformf CameraTransform = {};
    CameraTransform.Position = GetCameraPosition(Camera);
    CameraTransform.Orientation = AK_M3(Camera->X, Camera->Y, Camera->Z);
    return CameraTransform;
}

view_settings GetViewSettings(perspective_camera* Camera, ak_v2i Resolution)
{
    view_settings ViewSettings = {};
    ViewSettings.Transform = GetCameraTransform(Camera);
    ViewSettings.Projection = AK_Perspective(PERSPECTIVE_CAMERA_FOV, AK_SafeRatio(Resolution.w, Resolution.h), CAMERA_ZNEAR, CAMERA_ZFAR);
    return ViewSettings;
}

view_settings GetViewSettings(ortho_camera* Camera)
{
    view_settings ViewSettings = {};
    ViewSettings.Transform = GetCameraTransform(Camera);
    ViewSettings.Projection = AK_Orthographic(Camera->Left, Camera->Right, Camera->Top, Camera->Bottom, CAMERA_ZNEAR, CAMERA_ZFAR);
    return ViewSettings;
}

graphics_point_light ToGraphicsPointLight(point_light* PointLight)
{
    graphics_point_light Result;
    
    Result.Color = PointLight->Color;
    Result.Intensity = PointLight->Intensity;
    Result.Position = PointLight->Position;
    Result.Radius = PointLight->Radius;
    
    return Result;
}