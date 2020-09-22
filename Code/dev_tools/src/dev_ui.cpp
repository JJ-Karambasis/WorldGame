#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

graphics_texture_id DevUI_AllocateImGuiFont(graphics* Graphics)
{    
    ImGuiIO* IO = &GetIO();
    void* ImGuiFontData;
    
    ak_i32 Width, Height;        
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &Width, &Height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture_id FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, (ak_u32)Width, (ak_u32)Height, GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;    
    return FontTexture;
}

void DragInt(ak_u32 ID, const ak_char* Label, ak_i32* Value, ak_f32 Speed, ak_i32 Min, ak_i32 Max)
{
    PushID(ID);    
    DragInt(Label, Value, Speed, Min, Max);    
    PopID();
}

void DragFloat(ak_u32 ID, const ak_char* Label, ak_f32* Value, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    PushID(ID);
    DragFloat(Label, Value, Speed, Min, Max);
    PopID();
}

void DragAngle(ak_u32 ID, const ak_char* Label, ak_f32* Radians, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ak_f32 Degree = AK_ToDegree(*Radians);
    PushID(ID);
    DragFloat(Label, &Degree, Speed, Min, Max, Format);
    PopID();
    *Radians = AK_ToRadians(Degree);
}

ak_bool Combo(ak_u32 ID, const ak_char* Label, ak_i32* Data, const ak_char** List, ak_i32 ListCount)
{
    PushID(ID);
    ak_bool Result = Combo(Label, Data, List, ListCount);
    PopID();
    return Result;
}


ak_bool ColorEdit3(ak_u32 ID, const ak_char* Label, ak_f32* Data, ImGuiColorEditFlags Flags)
{
    PushID(ID);
    ak_bool Result = ColorEdit3(Label, Data, Flags);
    PopID();
    return Result;
}

ak_bool Checkbox(ak_u32 ID, const ak_char* Label, ak_bool* Flag)
{
    PushID(ID);
    ak_bool Result = Checkbox(Label, (bool*)Flag);
    PopID();
    return Result;
}

ak_bool Button(ak_u32 ID, const ak_char* ButtonText)
{
    PushID(ID);
    ak_bool Result = Button(ButtonText);
    PopID();
    return Result;
}

void DevUI_Initialize(dev_ui* UI, graphics* Graphics, void* PlatformWindow, platform_init_imgui* InitImGui)
{
    IMGUI_CHECKVERSION();
    ImGuiContext* Context = CreateContext();
    StyleColorsDark();    
    
    ImGuiIO* IO = &GetIO();
    InitImGui(PlatformWindow, IO);
    DevUI_AllocateImGuiFont(Graphics);
    
    IO->BackendRendererName = "OpenGL";
    IO->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    
    UI->LogArena = AK_CreateArena(AK_Megabyte(1));    
}

material_context DevUI_ContextFromMaterial(material* Material)
{
    material_context Result = {};
    
    Result.DiffuseIsTexture = Material->Diffuse.IsTexture;
    if(Result.DiffuseIsTexture) Result.DiffuseID = Material->Diffuse.DiffuseID;
    else Result.Diffuse = Material->Diffuse.Diffuse;
    
    Result.SpecularInUse = Material->Specular.InUse;
    if(Result.SpecularInUse)
    {
        Result.SpecularIsTexture = Material->Specular.IsTexture;
        if(Result.SpecularIsTexture) Result.SpecularID = Material->Specular.SpecularID;
        else Result.Specular = Material->Specular.Specular;
        Result.Shininess = Material->Specular.Shininess;
    }
    
    Result.NormalInUse = Material->Normal.InUse;
    if(Result.NormalInUse) Result.NormalID = Material->Normal.NormalID;
    
    return Result;
}

material DevUI_MaterialFromContext(material_context* MaterialContext)
{
    material_diffuse Diffuse = {};
    Diffuse.IsTexture = MaterialContext->DiffuseIsTexture;
    if(Diffuse.IsTexture) Diffuse.DiffuseID = MaterialContext->DiffuseID;
    else Diffuse.Diffuse = MaterialContext->Diffuse;
    
    material_specular Specular = {};
    Specular.InUse = MaterialContext->SpecularInUse;
    if(Specular.InUse)
    {
        Specular.IsTexture = MaterialContext->SpecularIsTexture;
        if(Specular.IsTexture) Specular.SpecularID = MaterialContext->SpecularID;
        else Specular.Specular = MaterialContext->Specular;        
        Specular.Shininess = MaterialContext->Shininess;
    }
    
    material_normal Normal = {};
    Normal.InUse = MaterialContext->NormalInUse;
    if(Normal.InUse)    
        Normal.NormalID = MaterialContext->NormalID;            
    
    material Result;
    Result.Diffuse  = Diffuse;
    Result.Specular = Specular;
    Result.Normal   = Normal;
    return Result;
}

void DevUI_ClearSpawner(entity_spawner* Spawner)
{
    Spawner->EntityType = ENTITY_TYPE_STATIC;                
    Spawner->Translation = {};
    Spawner->Scale = AK_V3(1.0f, 1.0f, 1.0f);
    Spawner->Radius = 1.0f;
    Spawner->Restitution = 0.0f;
    Spawner->Axis = {};
    Spawner->Angle = 0;
    Spawner->WorldIndex = 0;
    Spawner->MeshID = (mesh_asset_id)0;
    Spawner->MaterialContext = {};
    Spawner->Mass = 1.0f;
}

void DevUI_ClearSpawner(light_spawner* Spawner)
{
    Spawner->Translation = {};
    Spawner->Radius = 1.0f;
    Spawner->WorldIndex = 0;
    Spawner->Intensity = 1.0f;
    Spawner->Color = AK_White3();    
}

const ak_char* DevUI_GetEntityType(entity_type Type)
{
#define ENUM_TYPE(type) case type: return #type
    switch(Type)
    {
        ENUM_TYPE(ENTITY_TYPE_STATIC);
        ENUM_TYPE(ENTITY_TYPE_PLAYER);
        ENUM_TYPE(ENTITY_TYPE_RIGID_BODY);
        ENUM_TYPE(ENTITY_TYPE_PUSHABLE);
        
        AK_INVALID_DEFAULT_CASE;
    }   
#undef ENUM_TYPE
    
    return NULL;
}

ak_fixed_array<const ak_char*> DevUI_GetAllEntityTypesNotPlayer()
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_u32 Size = ENTITY_TYPE_COUNT-1;
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(Size), Size);    
    ak_u32 Counter = 0;
    
    for(ak_u32 TypeIndex = 0; TypeIndex < ENTITY_TYPE_COUNT; TypeIndex++)
    {
        if((entity_type)TypeIndex != ENTITY_TYPE_PLAYER)
            Result[Counter++] = DevUI_GetEntityType((entity_type)TypeIndex);
    }
    
    return Result;
}

ak_fixed_array<const ak_char*> DevUI_GetAllMeshInfoNames(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(MESH_ASSET_COUNT), MESH_ASSET_COUNT);
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Result[MeshIndex] = Assets->MeshInfos[MeshIndex].Name;          
    return Result;
}

ak_fixed_array<const ak_char*> DevUI_GetAllTextureInfoNames(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(TEXTURE_ASSET_COUNT), TEXTURE_ASSET_COUNT);
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)    
        Result[TextureIndex] = Assets->TextureInfos[TextureIndex].Name;          
    return Result;
}

void DevUI_DragFloatTool(ak_u32 Hash, const ak_char* Name, ak_f32 ItemWidth, ak_f32* Data, ak_f32 Speed, ak_f32 Min, ak_f32 Max)
{
    AlignTextToFramePadding();
    Text(Name);
    SameLine();
    PushItemWidth(ItemWidth);
    DragFloat(Hash, "", Data, Speed, Min, Max);
    PopItemWidth();
}

void DevUI_WorldIndexTool(ak_u32 Hash, ak_u32* WorldIndex, const ak_char** WorldIndexList, ak_u32 WorldIndexCount)
{
    AlignTextToFramePadding();
    Text("World Index");
    SameLine();
    Combo(Hash, "", (int*)WorldIndex, WorldIndexList, WorldIndexCount);    
}

void DevUI_MeshTool(assets* Assets, mesh_asset_id* MeshID)
{
    ak_fixed_array<const ak_char*> MeshNames = DevUI_GetAllMeshInfoNames(Assets);
    AlignTextToFramePadding();
    Text("Mesh");
    SameLine();
    Combo(AK_HashFunction("Mesh"), "", (int*)MeshID, MeshNames.Data, MeshNames.Size);
}

void DevUI_MassTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Mass)
{
    DevUI_DragFloatTool(Hash, "Mass", ItemWidth, Mass, 0.01f, 1.0f, 10000.0f);    
}

void DevUI_MaterialTool(assets* Assets, material_context* MaterialContext)
{    
    ak_fixed_array<const ak_char*> TextureNames = DevUI_GetAllTextureInfoNames(Assets);
    
    Text("Material");    
    {        
        AlignTextToFramePadding();
        Text("Diffuse: "); SameLine(); 
        AlignTextToFramePadding();
        Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Diffuse Is Texture"), "", &MaterialContext->DiffuseIsTexture); SameLine();
        if(MaterialContext->DiffuseIsTexture)
        {
            if((MaterialContext->DiffuseID == INVALID_TEXTURE_ID) || (MaterialContext->DiffuseID > TEXTURE_ASSET_COUNT))
                MaterialContext->DiffuseID = (texture_asset_id)0;
            
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Diffuse Texture"), "", (int*)&MaterialContext->DiffuseID, TextureNames.Data, TextureNames.Size);
        }
        else
        {
            AlignTextToFramePadding();
            Text("Color");
            SameLine();
            ColorEdit3(AK_HashFunction("Diffuse Color"), "", (ak_f32*)&MaterialContext->Diffuse, ImGuiColorEditFlags_RGB);
        }
    }
    
    {        
        AlignTextToFramePadding();
        Text("Specular: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Specular In Use"), "", &MaterialContext->SpecularInUse); 
        
        if(MaterialContext->SpecularInUse)
        {
            SameLine();
            AlignTextToFramePadding();
            Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Specular Is Texture"), "", &MaterialContext->SpecularIsTexture); SameLine();
            if(MaterialContext->SpecularIsTexture)
            {
                if((MaterialContext->SpecularID == INVALID_TEXTURE_ID) || (MaterialContext->SpecularID > TEXTURE_ASSET_COUNT))
                    MaterialContext->SpecularID = (texture_asset_id)0;
                
                AlignTextToFramePadding();
                Text("Texture");
                SameLine();
                Combo(AK_HashFunction("Specular Texture"), "", (int*)&MaterialContext->SpecularID, TextureNames.Data, TextureNames.Size);
            }
            else
            {                
                DevUI_DragFloatTool(AK_HashFunction("Specular Color"), "Value", 60, &MaterialContext->Specular, 0.01f, 0.0f, 1.0f); SameLine();                                
                
                ak_v3f SpecularDisplay = AK_V3(MaterialContext->Specular, MaterialContext->Specular, MaterialContext->Specular);
                ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
            }
            
            SameLine();
            Text("Shininess");
            PushItemWidth(40);
            SameLine();
            DragInt(AK_HashFunction("Specular Shininess"), "", &MaterialContext->Shininess, 0.1f, 1, 512);
            PopItemWidth();
        }                                                
    }                    
    
    {        
        AlignTextToFramePadding();
        Text("Normal: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Normal In Use"), "", &MaterialContext->NormalInUse);
        
        if(MaterialContext->NormalInUse)
        {
            if((MaterialContext->NormalID == INVALID_TEXTURE_ID) || (MaterialContext->NormalID > TEXTURE_ASSET_COUNT))
                MaterialContext->NormalID = (texture_asset_id)0;
            
            SameLine();
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Normal Texture"), "", (int*)&MaterialContext->NormalID, TextureNames.Data, TextureNames.Size);
        }
    }    
}

void DevUI_TranslationTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    Text("Translation");
    PushItemWidth(ItemWidth);
    
    DragFloat(Hash+0, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);                                
    
    PopItemWidth();
}

void DevUI_ScaleTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    Text("Scale");
    PushItemWidth(ItemWidth);
    
    DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Scale->z, 0.1f, 0.0f, 100.0f);                 
    
    PopItemWidth();
}

void DevUI_RadiusTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Radius)
{
    DevUI_DragFloatTool(Hash, "Radius", ItemWidth, Radius, 0.01f, 0.0f, 100.0f);    
}

void DevUI_RestitutionTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Restitution)
{
    DevUI_DragFloatTool(Hash, "Restitution", ItemWidth, Restitution, 0.001f, 0.0f, 1.0f);    
}

void DevUI_IntensityTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Intensity)
{
    DevUI_DragFloatTool(Hash, "Intensity", ItemWidth, Intensity, 0.01f, 1.0f, 100.0f);
}

ak_bool DevUI_ValidateMaterial(material* Material)
{
    if(Material->Diffuse.IsTexture && ((Material->Diffuse.DiffuseID >= TEXTURE_ASSET_COUNT) || (Material->Diffuse.DiffuseID == -1)))
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid diffuse texture. Please select a valid one");
        return false;
    }
    else if(Material->Specular.InUse && Material->Specular.IsTexture && ((Material->Specular.SpecularID >= TEXTURE_ASSET_COUNT) || (Material->Specular.SpecularID == -1)))
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid specular texture. Please select a valid one");
        return false;
    }
    else if(Material->Normal.InUse && ((Material->Normal.NormalID >= TEXTURE_ASSET_COUNT) || (Material->Normal.NormalID == -1)))
    {
        AK_MessageBoxOk("Entity Create Error", "Material specifies an invalid normal texture. Please select a valid one");
        return false;
    }
    
    return true;
}

void DevUI_EntitySpawner(ak_pool<dev_entity>* DevEntityStorage, assets* Assets, dev_selected_object* SelectedObject, 
                         entity_spawner* Spawner, ak_u32 CurrentWorldIndex)
{    
    if(!Spawner->Init)
    {
        Spawner->Init = true;
        DevUI_ClearSpawner(Spawner);                
    }
    
    ak_fixed_array<const ak_char*> EntityTypes = DevUI_GetAllEntityTypesNotPlayer();
    AlignTextToFramePadding();
    Text("Entity Type");            
    SameLine();            
    
    entity_type PrevType = Spawner->EntityType;
    
    ak_i32 SpawnType = Spawner->EntityType-1;            
    Combo(AK_HashFunction("Entity Type"), "", (int*)&SpawnType, EntityTypes.Data, EntityTypes.Size);            
    entity_type Type = (entity_type)(SpawnType+1);
    
    if(PrevType != Type)
    {
        DevUI_ClearSpawner(Spawner);
        Spawner->EntityType = Type;
    }
    
    const ak_f32 TRANSFORM_ITEM_WIDTH = 80.0f;
    
    Separator();                        
    DevUI_TranslationTool(AK_HashFunction("Translation Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Translation);                            
    
    switch(Spawner->EntityType)
    {
        case ENTITY_TYPE_STATIC:
        {                    
            Separator();                    
            DevUI_ScaleTool(AK_HashFunction("Scale Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Scale);                    
            Separator();
            
            {
                ak_u32 Hash = AK_HashFunction("Rotation Spawner");
                Text("Rotation");
                
                PushItemWidth(TRANSFORM_ITEM_WIDTH);
                
                DragFloat(Hash+0, "Axis X", &Spawner->Axis.x, 0.01f, -1, 1); SameLine();
                DragFloat(Hash+1, "Axis Y", &Spawner->Axis.y, 0.01f, -1, 1); SameLine();
                DragFloat(Hash+2, "Axis Z", &Spawner->Axis.z, 0.01f, -1, 1); SameLine();
                DragAngle(Hash+3, "Angle", &Spawner->Angle,   0.1f, -180.0f, 180.0f);
                
                Spawner->Axis = AK_Normalize(Spawner->Axis);                                                                        
                
                PopItemWidth();
            }
            
            Separator();                    
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both"}; 
            DevUI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
            Separator();                    
            DevUI_MeshTool(Assets, &Spawner->MeshID);                    
            Separator();                    
            DevUI_MaterialTool(Assets, &Spawner->MaterialContext);
            Separator();
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {                
                material Material = DevUI_MaterialFromContext(&Spawner->MaterialContext);
                if(Spawner->WorldIndex == 2)
                {                        
                    SelectedObject->Type = DEV_SELECTED_OBJECT_TYPE_ENTITY;
                    world_id A = DevContext_CreateDevEntity(DevEntityStorage, ENTITY_TYPE_STATIC, 0, Spawner->Translation, Spawner->Scale, 
                                                            AK_RotQuat(Spawner->Axis, Spawner->Angle), Spawner->MeshID, Material);
                    world_id B = DevContext_CreateDevEntity(DevEntityStorage, ENTITY_TYPE_STATIC, 1, Spawner->Translation, Spawner->Scale, 
                                                            AK_RotQuat(Spawner->Axis, Spawner->Angle), Spawner->MeshID, Material);                        
                    if(CurrentWorldIndex == 0)
                    {
                        SelectedObject->EntityID = A;
                        SelectedObject->MaterialContext = DevUI_ContextFromMaterial(&Material);
                    }
                    else
                    {
                        SelectedObject->EntityID = B;                        
                        SelectedObject->MaterialContext = DevUI_ContextFromMaterial(&Material);
                    }
                }
                else
                {                           
                    world_id EntityID = DevContext_CreateDevEntity(DevEntityStorage, ENTITY_TYPE_STATIC, Spawner->WorldIndex, Spawner->Translation, Spawner->Scale, 
                                                                   AK_RotQuat(Spawner->Axis, Spawner->Angle), Spawner->MeshID, Material);                        
                    if(Spawner->WorldIndex == CurrentWorldIndex)
                    {
                        SelectedObject->Type = DEV_SELECTED_OBJECT_TYPE_ENTITY;
                        SelectedObject->EntityID = EntityID;
                        SelectedObject->MaterialContext = DevUI_ContextFromMaterial(&Material);
                    }
                }                                    
            }
        } break;
        
        case ENTITY_TYPE_RIGID_BODY:
        {
            Separator();          
            DevUI_RadiusTool(AK_HashFunction("Radius Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
            Separator();
            DevUI_RestitutionTool(AK_HashFunction("Restitution Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Restitution);
            Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B"};
            DevUI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
            Separator();
            DevUI_MassTool(AK_HashFunction("Entity Mass"), TRANSFORM_ITEM_WIDTH, &Spawner->Mass);
            Separator();
            DevUI_MaterialTool(Assets, &Spawner->MaterialContext);
            Separator();                    
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
#if 0 
                    entity_id ID = CreateSphereRigidBody(Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius, 
                                                         Spawner->Mass, Spawner->Restitution, Spawner->Material);                    
#endif
                
            }
        } break;
        
        
        case ENTITY_TYPE_PUSHABLE:
        {
            Separator();
            DevUI_RadiusTool(AK_HashFunction("Radius Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
            Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both", "Linked"};
            DevUI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
            Separator();
            DevUI_MassTool(AK_HashFunction("Entity Mass"), TRANSFORM_ITEM_WIDTH, &Spawner->Mass);
            Separator();
            DevUI_MaterialTool(Assets, &Spawner->MaterialContext);
            Separator();
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                
#if 0 
                if(Spawner->WorldIndex == 2)
                {
                    entity_id A = CreatePushableBox(Game, 0, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);
                    entity_id B = CreatePushableBox(Game, 1, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);                        
                }
                else if(Spawner->WorldIndex == 3)
                {
                    dual_entity_id IDs = CreateDualPushableBox(Game, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);                        
                }
                else
                {
                    entity_id ID = CreatePushableBox(Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Spawner->Material);                        
                }                    
#endif            
            }
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }        
}

void DevUI_LightSpawner(ak_pool<dev_point_light>* InitialPointLights, dev_selected_object* SelectedObject, light_spawner* Spawner, ak_u32 CurrentWorldIndex)
{        
    if(!Spawner->Init)
    {
        DevUI_ClearSpawner(Spawner);
        Spawner->Init = true;
    }
    
    const ak_f32 TRANSFORM_ITEM_WIDTH = 80.0f;            
    DevUI_TranslationTool(AK_HashFunction("Translation Light Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Translation);
    Separator();
    const ak_char* WorldIndexList[] = {"World A", "World B", "Both"}; 
    DevUI_WorldIndexTool(AK_HashFunction("Light World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
    Separator();
    DevUI_RadiusTool(AK_HashFunction("Light Radius"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
    Separator();               
    DevUI_IntensityTool(AK_HashFunction("Light Intensity"), TRANSFORM_ITEM_WIDTH, &Spawner->Intensity);            
    Separator();
    
    AlignTextToFramePadding();
    Text("Color");            
    SameLine();            
    ColorEdit3(AK_HashFunction("Light Color"), "", (ak_f32*)&Spawner->Color, ImGuiColorEditFlags_RGB);            
    
    if(Button(AK_HashFunction("Create Point Light Button"), "Create"))
    {
        point_light PointLight = {};
        PointLight.On = true;
        PointLight.Position = Spawner->Translation;
        PointLight.Radius = Spawner->Radius;
        PointLight.Color = Spawner->Color;
        PointLight.Intensity = Spawner->Intensity;
        
        if(Spawner->WorldIndex == 2)
        {
            world_id A = DevContext_CreatePointLight(InitialPointLights, 0, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);
            world_id B = DevContext_CreatePointLight(InitialPointLights, 1, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);
            
            SelectedObject->Type = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
            if(CurrentWorldIndex == 0)
            {
                SelectedObject->PointLightID = A;
            }
            else
            {
                SelectedObject->PointLightID = B;
            }
        }
        else
        {
            world_id PointLightID = DevContext_CreatePointLight(InitialPointLights, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);            
            if(CurrentWorldIndex == Spawner->WorldIndex)
            {
                SelectedObject->Type = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
                SelectedObject->PointLightID = PointLightID;
            }
        }
    }
}

void DevUI_Update(dev_context* DevContext, dev_ui* UI)
{   
    game* Game = DevContext->Game;
    
    NewFrame();        
#if SHOW_IMGUI_DEMO_WINDOW
    //IMPORTANT(EVERYONE): If you need help figuring out how to use ImGui you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start    
    local ak_bool demo_window;
    ShowDemoWindow((bool*)&demo_window);
#endif    
    
    SetNextWindowPos(ImVec2(0, 0));
    if(Begin("Dev Tools", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        Text("FPS: %f", 1.0f/Game->dt);
        
        ak_bool PrevPlayGame = UI->PlayGame;
        
        ak_char* PlayText = UI->PlayGame ? "Stop" : "Play";
        if(Button(PlayText)) UI->PlayGame = !UI->PlayGame;
        
        if(PrevPlayGame != UI->PlayGame)
        {
            if(UI->PlayGame)
            {
                //NOTE(EVERYONE): Just started playing
                Dev_DebugLog("Just started playing");
            }
            else
            {
                //NOTE(EVERYONE): Just stopped playing
                Dev_DebugLog("Just stopped playing");
            }
        }
        
        if(!UI->PlayGame)
        {        
            const char* ViewModeTypes[] = {"Lit", "Unlit", "Wireframe", "Wireframe on Lit"};
            AlignTextToFramePadding();
            Text("View Modes");
            SameLine();
            Combo(AK_HashFunction("ViewModes"), "", (int*)&UI->ViewModeType, ViewModeTypes, AK_Count(ViewModeTypes));
            
            dev_gizmo_state* GizmoState = &DevContext->GizmoState;            
            
            Text("Snap Settings");
            
            AlignTextToFramePadding();
            Text("Snap"); SameLine();
            Checkbox(AK_HashFunction("Snap Checkbox"), "", &GizmoState->ShouldSnap); 
            
            AlignTextToFramePadding();
            Text("Grid Size"); SameLine();
            DragFloat(AK_HashFunction("Grid Size"), "", &GizmoState->GridDistance, 0.1f, 0.1f, 10);             
            AK_Clamp(GizmoState->GridDistance, 0.1f, 10.0f);  
            if(GizmoState->GridDistance < 0.1f)
                GizmoState->GridDistance = 0.1f;
            
            AlignTextToFramePadding();
            Text("Scale Snap"); SameLine();
            DragFloat(AK_HashFunction("Scale Snap"), "", &GizmoState->ScaleSnap, 0.1f, 0.1f, 10.0f);
            if(GizmoState->ScaleSnap < 0.1f)
                GizmoState->ScaleSnap = 0.1f;
            
            AlignTextToFramePadding();
            Text("Rotate Angle Snap"); SameLine();
            DragFloat(AK_HashFunction("Rotate Angle Snap"), "", &GizmoState->RotationAngleSnap, 0.1f, 0.1f, 180.0f);                                             
            if(GizmoState->RotationAngleSnap < 0.1f)
                GizmoState->RotationAngleSnap = 0.1f;
        }
        
        if(CollapsingHeader("Debug Logs", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if(Button("Clear"))
            {
                UI->Logs.Clear();
                UI->LogArena->Clear();
            }        
            
            SameLine();
            
            if(Button("Copy")) LogToClipboard();       
            
            for(ak_u32 LogIndex = 0; LogIndex < UI->Logs.Size; LogIndex++)
            {
                ak_string Log = UI->Logs[LogIndex];
                TextUnformatted(Log.Data, Log.Data+Log.Length);
            }
        }        
    }   
    ak_f32 DevToolsWindowHeight = GetWindowHeight();
    End();
    
    if(!UI->PlayGame)
    {
        SetNextWindowPos(ImVec2(0, DevToolsWindowHeight));
        if(Begin("Entity Spawner", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            DevUI_EntitySpawner(DevContext->InitialEntityStorage, Game->Assets, &DevContext->SelectedObject, &UI->EntitySpawner, Game->CurrentWorldIndex);
        }
        ak_f32 EntitySpawnerHeight = GetWindowHeight();
        End();                
        
        SetNextWindowPos(ImVec2(0, DevToolsWindowHeight+EntitySpawnerHeight));
        if(Begin("Light Spawner", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            DevUI_LightSpawner(DevContext->InitialPointLights, &DevContext->SelectedObject, &UI->LightSpawner, Game->CurrentWorldIndex);            
        }
        End();    
        
        dev_selected_object* SelectedObject = &DevContext->SelectedObject;
        if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE)
        {        
            const ak_f32 TRANSFORM_ITEM_WIDTH = 80.0f;            
            SetNextWindowPos(ImVec2((ak_f32)DevContext->Graphics->RenderDim.x-UI->DetailWidth, 0));
            if(Begin("Details", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                switch(SelectedObject->Type)
                {
                    case DEV_SELECTED_OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* DevEntity = DevContext_GetEntity(DevContext, SelectedObject->EntityID);
                        ak_array<ak_v3f>* EntityRotations = &DevContext->InitialRotations[SelectedObject->EntityID.WorldIndex];
                        ak_u32 Index = AK_PoolIndex(SelectedObject->EntityID.ID);
                        if((Index+1) > EntityRotations->Size)
                            EntityRotations->Resize(Index+1);
                        
                        ak_v3f Translation = DevEntity->Transform.Translation;
                        ak_v3f Scale = DevEntity->Transform.Scale;
                        ak_v3f Rotation = EntityRotations->Entries[Index];
                        
                        DevUI_TranslationTool(AK_HashFunction("Edit Translation"), TRANSFORM_ITEM_WIDTH, &Translation);
                        DevUI_ScaleTool(AK_HashFunction("Edit Scale"), TRANSFORM_ITEM_WIDTH, &Scale);
                        
                        {
                            ak_u32 Hash = AK_HashFunction("Edit Rotation");
                            Text("Rotation");
                            PushItemWidth(TRANSFORM_ITEM_WIDTH);
                            
                            DragAngle(Hash+0, "X", &Rotation.x, 0.1f, -180.0f, 180.0f); SameLine();
                            DragAngle(Hash+1, "Y", &Rotation.y, 0.1f, -180.0f, 180.0f); SameLine();
                            DragAngle(Hash+2, "Z", &Rotation.z, 0.1f, -180.0f, 180.0f); 
                            
                            PopItemWidth();
                        }
                        
                        Text("Type: %s", DevUI_GetEntityType(DevEntity->Type));
                        
                        DevUI_MaterialTool(DevContext->Game->Assets, &SelectedObject->MaterialContext);
                        
                        DevEntity->Transform.Translation = Translation;
                        DevEntity->Transform.Scale = Scale;
                        DevContext_UpdateObjectOrientation(&DevEntity->Transform.Orientation, EntityRotations->Get(Index), Rotation);
                        
                        DevEntity->Material = DevUI_MaterialFromContext(&SelectedObject->MaterialContext);                        
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
                    {
                        dev_point_light* PointLight = DevContext_GetPointLight(DevContext, SelectedObject->PointLightID);
                        
                        DevUI_TranslationTool(AK_HashFunction("Edit Point Light Translation"), TRANSFORM_ITEM_WIDTH, &PointLight->Position);
                        DevUI_RadiusTool(AK_HashFunction("Edit Light Radius"), TRANSFORM_ITEM_WIDTH, &PointLight->Radius);
                        DevUI_IntensityTool(AK_HashFunction("Edit Light Intensity"), TRANSFORM_ITEM_WIDTH, &PointLight->Intensity);
                        
                        AlignTextToFramePadding();
                        Text("Color");
                        SameLine();
                        ColorEdit3(AK_HashFunction("Edit Light Color"), "", (ak_f32*)&PointLight->Color, ImGuiColorEditFlags_RGB);
                    } break;
                    
                    case DEV_SELECTED_OBJECT_TYPE_PLAYER_CAPSULE:
                    {
                        ak_v3f Position = SelectedObject->PlayerCapsule->GetBottom();                                                
                        ak_v3f PrevPosition = Position;
                        DevUI_TranslationTool(AK_HashFunction("Edit Player Translation"), TRANSFORM_ITEM_WIDTH, &Position);
                        
                        TranslateCapsule(SelectedObject->PlayerCapsule, Position-PrevPosition);
                        
                    } break;
                }                
            }            
            UI->DetailWidth = GetWindowWidth();
            End();
        }
    }
    
    Render();
}

void DevUI_Render(graphics* Graphics, dev_ui* UI, graphics_render_buffer* MergeRenderBuffer)
{   
    UpdateRenderBuffer(&UI->UIRenderBuffer, Graphics, Graphics->RenderDim);
    
    PushRenderBufferViewportAndScissor(Graphics, UI->UIRenderBuffer);     
    PushProjection(Graphics, AK_Orthographic(0.0f, (ak_f32)UI->UIRenderBuffer->Resolution.w, 0.0f, (ak_f32)UI->UIRenderBuffer->Resolution.h, -1.0f, 1.0f));
    
    PushClearColor(Graphics, AK_Black4());    
    PushCopyToRenderBuffer(Graphics, MergeRenderBuffer, AK_V2<ak_i32>(), UI->UIRenderBuffer->Resolution);
    
    PushSRGBRenderBufferWrites(Graphics, true);
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    ak_u32 IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = GetDrawData();        
    
    for(ak_i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(UI->ImGuiMeshes.Size == (ak_u32)CmdListIndex)
            UI->ImGuiMeshes.Add(Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT));
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(Graphics, UI->ImGuiMeshes[CmdListIndex], 
                                 CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(ak_vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);                
        
        for(ak_i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            AK_Assert(!Cmd->UserCallback, "ImGui User callback is not supported");
            
            ImVec4 ClipRect = Cmd->ClipRect;
            if((ClipRect.x < Graphics->RenderDim.w) && (ClipRect.y < Graphics->RenderDim.h) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                ak_i32 X = (ak_i32)ClipRect.x;
                ak_i32 Y = (ak_i32)(Graphics->RenderDim.h-ClipRect.w);
                ak_i32 Width = (ak_i32)(ClipRect.z - ClipRect.x);
                ak_i32 Height = (ak_i32)(ClipRect.w - ClipRect.y);
                
                PushScissor(Graphics, X, Y, Width, Height);
                
                graphics_texture_id TextureID = (graphics_texture_id)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, UI->ImGuiMeshes[CmdListIndex], TextureID, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
            }
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushDepth(Graphics, true);            
    PushSRGBRenderBufferWrites(Graphics, false);
}