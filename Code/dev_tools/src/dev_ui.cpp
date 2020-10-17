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

void DevUI_Checkbox(ak_u32 ID, const ak_char* Label, ak_bool* Flag)
{
    AlignTextToFramePadding();
    Text(Label);
    SameLine();
    Checkbox(ID, "", Flag);
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

void DevUI_ClearSpawner(jumping_quad_spawner* Spawner)
{
    Spawner->Translation[0] = {};
    Spawner->Translation[1] = {};
    Spawner->Dimension = AK_V2(1.0f, 1.0f);
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
    
    AlignTextToFramePadding();
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

void DevUI_TranslationTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    Text(Label);
    PushItemWidth(ItemWidth);
    
    DragFloat(Hash+0, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);                                
    
    PopItemWidth();
}

void DevUI_TranslationTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    DevUI_TranslationTool("Translation", Hash, ItemWidth, Translation);                          
}

void DevUI_ScaleTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    Text(Label);
    PushItemWidth(ItemWidth);
    
    DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+2, "Z", &Scale->z, 0.1f, 0.0f, 100.0f);                 
    
    PopItemWidth();
}

void DevUI_ScaleTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    DevUI_ScaleTool("Scale", Hash, ItemWidth, Scale);
}

void DevUI_ScaleTool2D(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    Text(Label);
    PushItemWidth(ItemWidth);
    
    DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); SameLine();                                
    DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f);     
    
    PopItemWidth();
}

void DevUI_ScaleTool2D(ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    DevUI_ScaleTool2D("Scale", Hash, ItemWidth, Scale);
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

void DevUI_EntitySpawner(dev_context* DevContext, entity_spawner* Spawner, ak_u32 CurrentWorldIndex)
{
    game* Game = DevContext->Game;
    assets* Assets = Game->Assets;
    
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
                    
                    world_id AID = CreateStaticEntity(&Game->World, Game->Assets, 0, 
                                                      Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle), 
                                                      Spawner->MeshID, Material);
                    world_id BID = CreateStaticEntity(&Game->World, Game->Assets, 1, 
                                                      Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle), 
                                                      Spawner->MeshID, Material);
                    
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, AID);
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, BID);
                    
                    if(CurrentWorldIndex == AID.WorldIndex)                    
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, AID, &Material);                                            
                    else                    
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, BID, &Material);                                            
                }
                else
                {                           
                    world_id EntityID = CreateStaticEntity(&Game->World, Game->Assets, Spawner->WorldIndex, 
                                                           Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle), 
                                                           Spawner->MeshID, Material);                                            
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, EntityID);
                    
                    if(EntityID.WorldIndex == CurrentWorldIndex)                    
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, EntityID, &Material);
                    dev_object_edit Undo;
                    Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_CREATE;
                    Undo.Entity.ID = EntityID;
                    Undo.Entity.Type = ENTITY_TYPE_STATIC;
                    DevContext->UndoStack.Add(Undo);
                    DevContext->RedoStack.Clear();
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
                material Material = DevUI_MaterialFromContext(&Spawner->MaterialContext);
                world_id EntityID = CreateSphereRigidBody(&Game->World, Game->Assets, Spawner->WorldIndex, Spawner->Translation, 
                                                          Spawner->Radius, Spawner->Mass, Spawner->Restitution, Material);
                DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, EntityID);
                if(EntityID.WorldIndex = CurrentWorldIndex)                
                    DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, EntityID, &Material);
                dev_object_edit Undo;
                Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_CREATE;
                Undo.Entity.ID = EntityID;
                Undo.Entity.Type = ENTITY_TYPE_RIGID_BODY;
                DevContext->UndoStack.Add(Undo);
                DevContext->RedoStack.Clear();
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
                material Material = DevUI_MaterialFromContext(&Spawner->MaterialContext);
                
                if(Spawner->WorldIndex == 2)
                {
                    world_id A = CreatePushableBox(&Game->World, Game->Assets, 0, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Material);
                    world_id B = CreatePushableBox(&Game->World, Game->Assets, 1, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Material);
                    
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, A);
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, B);
                    if(CurrentWorldIndex == A.WorldIndex)                    
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, A, &Material);                                            
                    else
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, B, &Material);
                }
                else if(Spawner->WorldIndex == 3)
                {
                    world_id A = CreatePushableBox(&Game->World, Game->Assets, 0, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Material);
                    world_id B = CreatePushableBox(&Game->World, Game->Assets, 1, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Material);
                    
                    entity* EntityA = Game->World.EntityStorage[A.WorldIndex].Get(A.ID);
                    entity* EntityB = Game->World.EntityStorage[B.WorldIndex].Get(B.ID);
                    
                    EntityA->LinkID = B;
                    EntityB->LinkID = A;
                    
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, A);
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, B);
                    if(CurrentWorldIndex == A.WorldIndex)                    
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, A, &Material);                                            
                    else
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, B, &Material);
                }
                else
                {       
                    world_id EntityID = CreatePushableBox(&Game->World, Game->Assets, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius*2, Spawner->Mass, Material);                    
                    DevContext_AddToDevTransform(DevContext->InitialTransforms, &Game->World, EntityID);
                    if(CurrentWorldIndex == EntityID.WorldIndex)
                        DevContext_SetEntityAsSelectedObject(&DevContext->SelectedObject, EntityID, &Material);
                    dev_object_edit Undo;
                    Undo.ObjectEditType = DEV_OBJECT_EDIT_TYPE_CREATE;
                    Undo.Entity.ID = EntityID;
                    Undo.Entity.Type = ENTITY_TYPE_PUSHABLE;
                    DevContext->UndoStack.Add(Undo);
                    DevContext->RedoStack.Clear();
                }                    
                
            }
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }        
}

void DevUI_LightSpawner(dev_context* DevContext, light_spawner* Spawner, ak_u32 CurrentWorldIndex)
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
            graphics_state* GraphicsStateA = &DevContext->Game->World.GraphicsStates[0];
            graphics_state* GraphicsStateB = &DevContext->Game->World.GraphicsStates[1];
            ak_u64 AID = CreatePointLight(GraphicsStateA, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity, true);
            ak_u64 BID = CreatePointLight(GraphicsStateB, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity, true);
            
            DevContext->SelectedObject.Type = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
            if(CurrentWorldIndex == 0)
            {
                DevContext->SelectedObject.PointLightID = MakeWorldID(AID, 0);
            }
            else
            {
                DevContext->SelectedObject.PointLightID = MakeWorldID(BID, 1);
            }
        }
        else
        {
            graphics_state* GraphicsState = &DevContext->Game->World.GraphicsStates[Spawner->WorldIndex];
            ak_u64 ID = CreatePointLight(GraphicsState, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity, true);            
            if(CurrentWorldIndex == Spawner->WorldIndex)
            {
                DevContext->SelectedObject.Type = DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT;
                DevContext->SelectedObject.PointLightID = MakeWorldID(ID, CurrentWorldIndex);
            }
        }
    }
}

void DevUI_JumpingQuadSpawner(dev_context* DevContext, jumping_quad_spawner* Spawner, ak_u32 CurrentWorldIndex)
{
    const ak_f32 TRANSFORM_ITEM_WIDTH = 80.0f;
    if(!Spawner->Init)
    {
        Spawner->Init = true;
        DevUI_ClearSpawner(Spawner);                
    }
    
    const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
    DevUI_WorldIndexTool(AK_HashFunction("Jumping Quad World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
    Separator();
    DevUI_ScaleTool2D("Dimension", AK_HashFunction("Jumping Quad Dimension"), TRANSFORM_ITEM_WIDTH, &Spawner->Dimension);
    Separator();
    DevUI_TranslationTool("Quad 0 Translation", AK_HashFunction("Quad 0 Translation"), TRANSFORM_ITEM_WIDTH, &Spawner->Translation[0]);
    DevUI_TranslationTool("Quad 1 Translation", AK_HashFunction("Quad 1 Translation"), TRANSFORM_ITEM_WIDTH, &Spawner->Translation[1]);
    Separator();
    if(Button(AK_HashFunction("Create Jumping Quads"), "Create"))
    {
        if(Spawner->WorldIndex == 2)
        {
            dual_world_id AIDs = CreateJumpingQuads(&DevContext->Game->World, 0, Spawner->Translation, Spawner->Dimension);
            dual_world_id BIDs = CreateJumpingQuads(&DevContext->Game->World, 1, Spawner->Translation, Spawner->Dimension);
            
            DevContext->SelectedObject.Type = DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD;
            if(CurrentWorldIndex == AIDs.A.WorldIndex)                
                DevContext->SelectedObject.JumpingQuadID = AIDs.A;            
            else            
                DevContext->SelectedObject.JumpingQuadID = BIDs.A;
        }
        else
        {
            dual_world_id IDs = CreateJumpingQuads(&DevContext->Game->World, Spawner->WorldIndex, Spawner->Translation, Spawner->Dimension);
            if(CurrentWorldIndex == IDs.A.WorldIndex)
            {
                DevContext->SelectedObject.Type = DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD;
                DevContext->SelectedObject.JumpingQuadID = IDs.A;
            }
        }                        
    }
    
}

void DevUI_Details(dev_context* DevContext, dev_selected_object* SelectedObject)
{
    game* Game = DevContext->Game;
    world* World = &Game->World;
    
    const ak_f32 TRANSFORM_ITEM_WIDTH = 80.0f;            
    switch(SelectedObject->Type)
    {
        case DEV_SELECTED_OBJECT_TYPE_ENTITY:
        {
            entity* Entity = GetEntity(Game, SelectedObject->EntityID);
            
            ak_array<dev_transform>* DevTransforms = &DevContext->InitialTransforms[SelectedObject->EntityID.WorldIndex];
            ak_u32 Index = AK_PoolIndex(Entity->ID.ID);
            if((Index+1) > DevTransforms->Size)
                DevTransforms->Resize(Index+1);
            
            dev_transform* DevTransform = DevTransforms->Get(Index);
            
            ak_v3f Translation = DevTransform->Translation;
            ak_v3f Scale = DevTransform->Scale;
            ak_v3f Rotation = DevTransform->Euler;
            
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
            
            Text("Type: %s", DevUI_GetEntityType(Entity->Type));
            
            DevUI_MaterialTool(Game->Assets, &SelectedObject->MaterialContext);
            
            DevTransform->Translation = Translation;
            DevTransform->Scale       = Scale;
            DevTransform->Euler       = Rotation;
            
            ak_sqtf* Transform = &World->NewTransforms[SelectedObject->EntityID.WorldIndex][Index];
            Transform->Translation = DevTransform->Translation;
            Transform->Scale = DevTransform->Scale;                        
            Transform->Orientation = AK_Normalize(AK_EulerToQuat(DevTransform->Euler));
            
            World->OldTransforms[SelectedObject->EntityID.WorldIndex][Index] = *Transform;
            
            sim_entity* SimEntity = GetSimEntity(Game, SelectedObject->EntityID);
            SimEntity->Transform = *Transform;                    
            
            graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->EntityID.WorldIndex];
            graphics_entity* GraphicsEntity = GraphicsState->GraphicsEntityStorage.Get(Entity->GraphicsEntityID);
            GraphicsEntity->Transform = AK_TransformM4(*Transform);
            
            GraphicsEntity->Material = DevUI_MaterialFromContext(&SelectedObject->MaterialContext);                        
            
            switch(Entity->Type)
            {                
                case ENTITY_TYPE_RIGID_BODY:
                {
                    rigid_body* RigidBody = Game->World.Simulations[Entity->ID.WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    
                    ak_f32 Mass = 1.0f/RigidBody->InvMass;
                    DevUI_MassTool(AK_HashFunction("Edit Mass"), TRANSFORM_ITEM_WIDTH, &Mass);
                    RigidBody->InvMass = 1.0f/Mass;
                    
                    DevUI_RestitutionTool(AK_HashFunction("Edit Restitution"), TRANSFORM_ITEM_WIDTH, &RigidBody->Restitution);
                    
                    ak_f32 SphereRadius = 1.0f*Transform->Scale.LargestComp();
                    RigidBody->LocalInvInertiaTensor = GetSphereInvInertiaTensor(SphereRadius, Mass);
                } break;
                
                case ENTITY_TYPE_PUSHABLE:
                {
                    rigid_body* RigidBody = Game->World.Simulations[Entity->ID.WorldIndex].GetSimEntity(Entity->SimEntityID)->ToRigidBody();
                    ak_f32 Mass = 1.0f/RigidBody->InvMass;
                    DevUI_MassTool(AK_HashFunction("Edit Mass"), TRANSFORM_ITEM_WIDTH, &Mass);
                    RigidBody->InvMass = 1.0f/Mass;
                    
                    RigidBody->LocalInvInertiaTensor = GetBoxInvInertiaTensor(AK_V3(1.0f, 1.0f, 1.0f), Mass);                    
                } break;
            }
            
        } break;
        
        case DEV_SELECTED_OBJECT_TYPE_JUMPING_QUAD:
        {
            jumping_quad* SelectedJumpingQuad = Game->World.JumpingQuadStorage[SelectedObject->JumpingQuadID.WorldIndex].Get(SelectedObject->JumpingQuadID.ID);
            jumping_quad* OtherJumpingQuad = Game->World.JumpingQuadStorage[SelectedJumpingQuad->OtherQuad.WorldIndex].Get(SelectedJumpingQuad->OtherQuad.ID);
            
            jumping_quad* TranslationA;
            jumping_quad* TranslationB;
            if(SelectedJumpingQuad->ID < OtherJumpingQuad->ID)
            {
                TranslationA = SelectedJumpingQuad;
                TranslationB = OtherJumpingQuad;
            }
            else
            {
                TranslationA = OtherJumpingQuad;
                TranslationB = SelectedJumpingQuad;
            }
            
            AK_Assert(TranslationA->Dimensions == TranslationB->Dimensions, "Dimensions do not synchronize between both jumping quads. Programming error has occurred");
            
            DevUI_TranslationTool("Quad 0 Translation", AK_HashFunction("Quad 0 Edit Translation"), TRANSFORM_ITEM_WIDTH, &TranslationA->CenterP);
            DevUI_TranslationTool("Quad 1 Translation", AK_HashFunction("Quad 1 Edit Translation"), TRANSFORM_ITEM_WIDTH, &TranslationB->CenterP);
            DevUI_ScaleTool2D("Quad Dimensions", AK_HashFunction("Quad Edit Dimensions"), TRANSFORM_ITEM_WIDTH, &TranslationA->Dimensions);            
            TranslationB->Dimensions = TranslationA->Dimensions;            
            
        } break;
        
        case DEV_SELECTED_OBJECT_TYPE_POINT_LIGHT:
        {
            graphics_state* GraphicsState = &World->GraphicsStates[SelectedObject->PointLightID.WorldIndex];
            point_light* PointLight = GetPointLight(GraphicsState, SelectedObject->PointLightID.ID);
            
            DevUI_TranslationTool(AK_HashFunction("Edit Point Light Translation"), TRANSFORM_ITEM_WIDTH, &PointLight->Position);
            DevUI_RadiusTool(AK_HashFunction("Edit Light Radius"), TRANSFORM_ITEM_WIDTH, &PointLight->Radius);
            DevUI_IntensityTool(AK_HashFunction("Edit Light Intensity"), TRANSFORM_ITEM_WIDTH, &PointLight->Intensity);
            
            AlignTextToFramePadding();
            Text("Color");
            SameLine();
            ColorEdit3(AK_HashFunction("Edit Light Color"), "", (ak_f32*)&PointLight->Color, ImGuiColorEditFlags_RGB);
        } break;        
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
    ak_f32 MenuHeight = 0;   
    
    if(!UI->PlayGame)
    {
        if(BeginMainMenuBar())
        {                        
            if(BeginMenu("Menu"))
            {   
                if(MenuItem("Load World", "ALT+L")) DevContext_LoadWorld(DevContext, &DevContext->LoadedWorld);                                 
                if(MenuItem("Save World", "CTRL+S")) DevContext_SaveWorld(DevContext, &DevContext->LoadedWorld, false);                                
                if(MenuItem("Save World As", "ALT+S")) DevContext_SaveWorld(DevContext, &DevContext->LoadedWorld, true);
                
                if(AK_StringIsNullOrEmpty(DevContext->LoadedWorld.LoadedWorldFile))
                    MenuItem("Default World As", NULL, false, false);
                else
                {
                    if(MenuItem("Default World As")) DevContext_SetDefaultWorld(DevContext, &DevContext->LoadedWorld);
                }
                
                ImGui::EndMenu();
            }
            
            ak_char* LoadedWorldString = AK_StringIsNullOrEmpty(DevContext->LoadedWorld.LoadedWorldFile) ? "None" : DevContext->LoadedWorld.LoadedWorldFile.Data;
            Text("\tLoaded World: %s", LoadedWorldString);
            SameLine();
            
            MenuHeight = GetWindowHeight();
            EndMainMenuBar();            
        }
    }
    
    SetNextWindowPos(ImVec2(0, MenuHeight));
    if(Begin("Dev Tools", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {                
        Text("FPS: %f", 1.0f/Game->dt);
        
        ak_bool PrevPlayGame = UI->PlayGame;
        
        ak_char* PlayText = UI->PlayGame ? "Stop" : "Play";
        if(Button(PlayText)) UI->PlayGame = !UI->PlayGame;
        
        DevUI_Checkbox(AK_HashFunction("Draw Other World"), "Draw Other World", &UI->DrawOtherWorld);
        DevUI_Checkbox(AK_HashFunction("Draw Collision Volumes"), "Draw Collision Volumes", &UI->DrawCollisionVolumes);
        
        if(PrevPlayGame != UI->PlayGame)
        {
            if(UI->PlayGame)
            {
                //NOTE(EVERYONE): Just started playing
                
            }
            else
            {                                
                //NOTE(EVERYONE): Just stopped playing
                for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
                {
                    camera* DevCamera = &DevContext->Cameras[WorldIndex];                    
                    
                    entity_storage*   EntityStorage = &Game->World.EntityStorage[WorldIndex];
                    ak_array<ak_sqtf>* OldTransforms = &Game->World.OldTransforms[WorldIndex];
                    ak_array<ak_sqtf>* NewTransforms = &Game->World.NewTransforms[WorldIndex];                    
                    graphics_entity_storage* GraphicsEntityStorage = &Game->World.GraphicsStates[WorldIndex].GraphicsEntityStorage;
                    simulation* Simulation = &Game->World.Simulations[WorldIndex];
                    
                    ak_array<dev_transform>* InitialTransforms = &DevContext->InitialTransforms[WorldIndex];
                    AK_ForEach(Entity, EntityStorage)
                    {                        
                        ak_u32 Index = AK_PoolIndex(Entity->ID.ID);
                        graphics_entity* GraphicsEntity = GraphicsEntityStorage->Get(Entity->GraphicsEntityID);
                        sim_entity* SimEntity = Simulation->GetSimEntity(Entity->SimEntityID);
                        
                        ak_sqtf* NewTransform = NewTransforms->Get(Index);
                        dev_transform* InitialTransform = InitialTransforms->Get(Index);
                        NewTransform->Translation = InitialTransform->Translation;
                        NewTransform->Scale = InitialTransform->Scale;
                        NewTransform->Orientation = AK_EulerToQuat(InitialTransform->Euler);
                        
                        *OldTransforms->Get(Index) = *NewTransform;
                        GraphicsEntity->Transform = AK_TransformM4(*NewTransform);
                        SimEntity->Transform = *NewTransform;                        
                        
                        if(Entity->Type == ENTITY_TYPE_PLAYER)
                        {
                            DevCamera->Target = NewTransform->Translation;
                            DevCamera->SphericalCoordinates.radius = 6.0f;
                        }
                    }
                }                
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
        SetNextWindowPos(ImVec2(0, DevToolsWindowHeight+MenuHeight));
        if(Begin("Spawners", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if(BeginTabBar("Spawner Tabs"))
            {                
                if(BeginTabItem("Entity"))
                {
                    DevUI_EntitySpawner(DevContext, &UI->EntitySpawner, Game->CurrentWorldIndex);
                    EndTabItem();
                }            
                
                if(BeginTabItem("Light"))
                {
                    DevUI_LightSpawner(DevContext, &UI->LightSpawner, Game->CurrentWorldIndex);            
                    EndTabItem();
                }                            
                
                if(BeginTabItem("Jumping Quads"))
                {
                    DevUI_JumpingQuadSpawner(DevContext, &UI->JumpingQuadSpawner, Game->CurrentWorldIndex);                                        
                    EndTabItem();
                }
                
                EndTabBar();
            }
        }
        End();
        
        dev_selected_object* SelectedObject = &DevContext->SelectedObject;
        if(SelectedObject->Type != DEV_SELECTED_OBJECT_TYPE_NONE)
        {   
            ak_f32 StartY = MenuHeight;
            if(UI->DrawOtherWorld)
                StartY = (ak_f32)Game->World.GraphicsStates[Game->CurrentWorldIndex].RenderBuffer->Resolution.y / 5;
            
            SetNextWindowPos(ImVec2((ak_f32)Game->Resolution.x-UI->DetailWidth, StartY));
            if(Begin("Details", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                DevUI_Details(DevContext, SelectedObject);        
            }            
            UI->DetailWidth = GetWindowWidth();
            End();
        }        
    }
    
    Render();
}

void DevUI_Render(graphics* Graphics, dev_ui* UI, graphics_render_buffer* MergeRenderBuffer)
{   
    UpdateRenderBuffer(Graphics, &UI->UIRenderBuffer, MergeRenderBuffer->Resolution);
    
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
            if((ClipRect.x < UI->UIRenderBuffer->Resolution.w) && (ClipRect.y < UI->UIRenderBuffer->Resolution.h) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                ak_i32 X = (ak_i32)ClipRect.x;
                ak_i32 Y = (ak_i32)(UI->UIRenderBuffer->Resolution.h-ClipRect.w);
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