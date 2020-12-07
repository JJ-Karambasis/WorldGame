#include "engine.h"

#include "src/assets.cpp"
#include "src/graphics_push_commands.cpp"

engine* Engine_Initialize(graphics* Graphics, ak_string AssetPath)
{
    engine* Engine = (engine*)AK_Allocate(sizeof(engine));
    if(!Engine)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Engine->Scratch = AK_CreateArena(AK_Megabyte(4));
    Engine->Graphics = Graphics;
    
    
    ak_arena* AssetStorage = AK_CreateArena(AK_Megabyte(1));
    Engine->Assets = InitAssets(AssetStorage, AssetPath);
    if(!Engine->Assets)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    return Engine;
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

view_settings GetViewSettings(graphics_camera* Camera)
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

void Engine_Render(engine* Engine, graphics_state* GraphicsState)
{
    graphics_light_buffer LightBuffer = {};
    
    LightBuffer.PointLightCount = GraphicsState->PointLightArray.Count;
    AK_CopyArray(LightBuffer.PointLights, GraphicsState->PointLightArray.Ptr, LightBuffer.PointLightCount);
    
    view_settings ViewSettings = GetViewSettings(&GraphicsState->Camera);
    
    PushDepth(Engine->Graphics, true);
    PushSRGBRenderBufferWrites(Engine->Graphics, true);
    PushRenderBufferViewportScissorAndView(Engine->Graphics, Engine->RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Engine->Graphics, AK_Black4(), 1.0f);
    PushCull(Engine->Graphics, GRAPHICS_CULL_MODE_BACK);
    
    PushLightBuffer(Engine->Graphics, &LightBuffer);
    graphics_entity_array* EntityArray = &GraphicsState->EntityArray;
    for(ak_u32 EntityIndex = 0; EntityIndex < EntityArray->Count; EntityIndex++)
    {
        graphics_entity* GraphicsEntity = EntityArray->Ptr + EntityIndex;
        AK_Assert(GraphicsEntity->MeshID != INVALID_MESH_ID, "Cannot draw an invalid mesh");            
        
        graphics_material Material = ConvertToGraphicsMaterial(Engine->Assets, Engine->Graphics, &GraphicsEntity->Material);            
        PushMaterial(Engine->Graphics, Material);
        
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Engine->Assets, Engine->Graphics, GraphicsEntity->MeshID);            
        PushDrawMesh(Engine->Graphics, MeshHandle, GraphicsEntity->Transform, GetMeshIndexCount(Engine->Assets, GraphicsEntity->MeshID), 0, 0);                        
    }
}

extern "C"
AK_EXPORT ENGINE_RUN(Engine_Run)
{
    game_code GameCode = {};
    if(!Platform->LoadGameCode(&GameCode))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    engine* Engine = Engine_Initialize(Graphics, Platform->AssetPath);
    if(!Engine)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    if(!GameCode.Startup(Engine))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return -1;
    }
    
    Engine->dtFixed = 1.0f/60.0f;
    ak_f32 Accumulator = 0.0f;
    ak_high_res_clock CurrentTime = AK_WallClock();
    for(;;)
    {
        ak_temp_arena TempArena = Engine->Scratch->BeginTemp();
        
        ak_high_res_clock NewTime = AK_WallClock();
        ak_f32 FrameTime = (ak_f32)AK_GetElapsedTime(NewTime, CurrentTime);
        if(FrameTime > 0.05f)
            FrameTime = 0.05f;
        
        CurrentTime = NewTime;
        Accumulator += FrameTime;
        
        while(Accumulator >= Engine->dtFixed)
        {
            input* Input = &Engine->Input;
            if(!Platform->ProcessMessages(Input))
                return 0;
            
            GameCode.Update(Engine->Game);
            for(ak_u32 ButtonIndex = 0; ButtonIndex < AK_Count(Input->Buttons); ButtonIndex++)        
            {
                Input->Buttons[ButtonIndex].WasDown = Input->Buttons[ButtonIndex].IsDown;             
            }
        }
        
        ak_v2i Resolution = Platform->GetResolution();
        UpdateRenderBuffer(Graphics, &Engine->RenderBuffer, Resolution);
        
        ak_f32 tInterpolated = Accumulator / Engine->dtFixed;
        graphics_state GraphicsState = GameCode.GetGraphicsState(Engine->Game, tInterpolated);
        Engine_Render(Engine, &GraphicsState);
        Platform->ExecuteRenderCommands(Graphics);
        
        Engine->Scratch->EndTemp(&TempArena);
    }
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>