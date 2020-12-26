void UI_PushDisabledItem()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha*0.5f);
}

void UI_PopDisabledItem()
{
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

void UI_SameLineLabel(const ak_char* Label)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text(Label);
    ImGui::SameLine();
}

void UI_Checkbox(ak_u32 ID, const ak_char* Label, ak_bool* Flag)
{
    UI_SameLineLabel(Label);
    ImGui::PushID(ID);
    ImGui::Checkbox("", (bool*)Flag);
    ImGui::PopID();
}

void UI_ErrorText(const ak_char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
    ImGui::TextV(Format, Args);
    ImGui::PopStyleColor();
    va_end(Args);
}

const ak_char* UI_GetEntityType(entity_type Type)
{
#define ENUM_TYPE(type) case type: return #type
    switch(Type)
    {
        ENUM_TYPE(ENTITY_TYPE_STATIC);
        ENUM_TYPE(ENTITY_TYPE_PLAYER);
        ENUM_TYPE(ENTITY_TYPE_BUTTON);
        ENUM_TYPE(ENTITY_TYPE_MOVABLE);
        
        AK_INVALID_DEFAULT_CASE;
    }   
#undef ENUM_TYPE
    
    return NULL;
}

ak_fixed_array<const ak_char*> UI_GetAllEntityTypesNotPlayer()
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_u32 Size = ENTITY_TYPE_COUNT-1;
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(Size), Size);    
    ak_u32 Counter = 0;
    
    for(ak_u32 TypeIndex = 0; TypeIndex < ENTITY_TYPE_COUNT; TypeIndex++)
    {
        if((entity_type)TypeIndex != ENTITY_TYPE_PLAYER)
            Result[Counter++] = UI_GetEntityType((entity_type)TypeIndex);
    }
    
    return Result;
}

ak_fixed_array<const ak_char*> UI_GetAllMeshInfoNames(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(MESH_ASSET_COUNT), MESH_ASSET_COUNT);
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Result[MeshIndex] = Assets->MeshInfos[MeshIndex].Name;          
    return Result;
}

ak_fixed_array<const ak_char*> UI_GetAllTextureInfoNames(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(TEXTURE_ASSET_COUNT), TEXTURE_ASSET_COUNT);
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)    
        Result[TextureIndex] = Assets->TextureInfos[TextureIndex].Name;          
    return Result;
}

void UI_DragInt(ak_u32 ID, const ak_char* Label, ak_i32* Value, ak_f32 Speed, ak_i32 Min, ak_i32 Max)
{
    ImGui::PushID(ID);    
    ImGui::DragInt(Label, Value, Speed, Min, Max);    
    ImGui::PopID();
}

void UI_DragFloat(ak_u32 ID, const ak_char* Label, ak_f32* Value, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ImGui::PushID(ID);
    ImGui::DragFloat(Label, Value, Speed, Min, Max);
    ImGui::PopID();
}

void UI_DragFloatTool(ak_u32 Hash, const ak_char* Name, ak_f32 ItemWidth, ak_f32* Data, ak_f32 Speed, ak_f32 Min, ak_f32 Max)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text(Name);
    ImGui::SameLine();
    ImGui::PushItemWidth(ItemWidth);
    UI_DragFloat(Hash, "", Data, Speed, Min, Max);
    ImGui::PopItemWidth();
}

void UI_DragAngle(ak_u32 ID, const ak_char* Label, ak_f32* Radians, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ak_f32 Degree = AK_ToDegree(*Radians);
    ImGui::PushID(ID);
    ImGui::DragFloat(Label, &Degree, Speed, Min, Max, Format);
    ImGui::PopID();
    *Radians = AK_ToRadians(Degree);
}

ak_bool UI_Combo(ak_u32 ID, const ak_char* Label, ak_i32* Data, const ak_char** List, ak_i32 ListCount)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::Combo(Label, Data, List, ListCount);
    ImGui::PopID();
    return Result;
}


ak_bool UI_ColorEdit3(ak_u32 ID, const ak_char* Label, ak_f32* Data, ImGuiColorEditFlags Flags)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::ColorEdit3(Label, Data, Flags);
    ImGui::PopID();
    return Result;
}

ak_bool UI_Button(ak_u32 ID, const ak_char* ButtonText)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::Button(ButtonText);
    ImGui::PopID();
    return Result;
}

void UI_ClearSpawner(entity_spawner* Spawner)
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

void UI_ResetSpawner(ui* UI, entity_spawner* Spawner)
{
    UI_ClearSpawner(Spawner);
    UI->ShowEntityNameNullErrorText = false;
    UI->ShowEntityNameErrorText = false;
}

void UI_ClearSpawner(light_spawner* Spawner)
{
    Spawner->Translation = {};
    Spawner->Radius = 1.0f;
    Spawner->WorldIndex = 0;
    Spawner->Intensity = 1.0f;
    Spawner->Color = AK_White3();    
}

void UI_ResetSpawner(ui* UI, light_spawner* Spawner)
{
    UI_ClearSpawner(&UI->LightSpawner);
    UI->ShowLightNameNullErrorText = false;
    UI->ShowLightNameErrorText = false;
}

void UI_TranslationTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    UI_DragFloat(Hash+0, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f); ImGui::SameLine();                                
    UI_DragFloat(Hash+1, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f); ImGui::SameLine();                                
    UI_DragFloat(Hash+2, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);                                
    
    ImGui::PopItemWidth();
}

void UI_TranslationTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    UI_TranslationTool("Translation", Hash, ItemWidth, Translation);                          
}

void UI_ScaleTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    UI_DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    UI_DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    UI_DragFloat(Hash+2, "Z", &Scale->z, 0.1f, 0.0f, 100.0f);                 
    
    ImGui::PopItemWidth();
}

void UI_ScaleTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    UI_ScaleTool("Scale", Hash, ItemWidth, Scale);
}

void UI_AngleAxisTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Axis, ak_f32* Angle)
{
    ImGui::Text(Label);
    
    ImGui::PushItemWidth(ItemWidth);
    
    UI_DragFloat(Hash+0, "Axis X", &Axis->x, 0.01f, -1, 1); ImGui::SameLine();
    UI_DragFloat(Hash+1, "Axis Y", &Axis->y, 0.01f, -1, 1); ImGui::SameLine();
    UI_DragFloat(Hash+2, "Axis Z", &Axis->z, 0.01f, -1, 1); ImGui::SameLine();
    UI_DragAngle(Hash+3, "Angle", Angle,   0.1f, -180.0f, 180.0f);
    
    *Axis = AK_Normalize(*Axis);                                                                        
    
    ImGui::PopItemWidth();
}

void UI_AngleAxisTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Axis, ak_f32* Angle)
{
    UI_AngleAxisTool("Rotation", Hash, ItemWidth, Axis, Angle);
}

void UI_ScaleTool2D(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    UI_DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    UI_DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f);     
    
    ImGui::PopItemWidth();
}

void UI_ScaleTool2D(ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    UI_ScaleTool2D("Scale", Hash, ItemWidth, Scale);
}

void UI_WorldIndexTool(ak_u32 Hash, ak_u32* WorldIndex, const ak_char** WorldIndexList, ak_u32 WorldIndexCount)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("World Index");
    ImGui::SameLine();
    UI_Combo(Hash, "", (int*)WorldIndex, WorldIndexList, WorldIndexCount);    
}

void UI_MeshTool(assets* Assets, mesh_asset_id* MeshID)
{
    ak_fixed_array<const ak_char*> MeshNames = UI_GetAllMeshInfoNames(Assets);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Mesh");
    ImGui::SameLine();
    UI_Combo(AK_HashFunction("Mesh"), "", (int*)MeshID, MeshNames.Data, MeshNames.Size);
}

void UI_NameTool(ak_u32 Hash, ak_char* Name, ak_u32 MaxLength)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::PushID(Hash);
    ImGui::InputText("", Name, MaxLength);
    ImGui::PopID();
}

void UI_Color3EditTool(ak_char* Label, ak_u32 Hash, ak_f32* Value)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text(Label);
    ImGui::SameLine();
    UI_ColorEdit3(Hash, "", Value, ImGuiColorEditFlags_RGB);
}

void UI_RadiusTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Radius)
{
    UI_DragFloatTool(Hash, "Radius", ItemWidth, Radius, 0.01f, 0.0f, 100.0f);    
}

void UI_IntensityTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Intensity)
{
    UI_DragFloatTool(Hash, "Intensity", ItemWidth, Intensity, 0.01f, 1.0f, 100.0f);
}

void UI_MaterialTool(assets* Assets, material_context* MaterialContext)
{    
    ak_fixed_array<const ak_char*> TextureNames = UI_GetAllTextureInfoNames(Assets);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Material");        
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Diffuse: "); ImGui::SameLine(); 
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Is Texture"); ImGui::SameLine(); UI_Checkbox(AK_HashFunction("Diffuse Is Texture"), "", &MaterialContext->DiffuseIsTexture); ImGui::SameLine();
        if(MaterialContext->DiffuseIsTexture)
        {
            if((MaterialContext->DiffuseID == INVALID_TEXTURE_ID) || (MaterialContext->DiffuseID > TEXTURE_ASSET_COUNT))
                MaterialContext->DiffuseID = (texture_asset_id)0;
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture");
            ImGui::SameLine();
            UI_Combo(AK_HashFunction("Diffuse Texture"), "", (int*)&MaterialContext->DiffuseID, TextureNames.Data, TextureNames.Size);
        }
        else
        {
            UI_Color3EditTool("Color", AK_HashFunction("Diffuse Color"), MaterialContext->Diffuse.Data);
        }
    }
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Specular: "); ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("In Use"); ImGui::SameLine(); UI_Checkbox(AK_HashFunction("Specular In Use"), "", &MaterialContext->SpecularInUse); 
        
        if(MaterialContext->SpecularInUse)
        {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Is Texture"); ImGui::SameLine(); UI_Checkbox(AK_HashFunction("Specular Is Texture"), "", &MaterialContext->SpecularIsTexture); ImGui::SameLine();
            if(MaterialContext->SpecularIsTexture)
            {
                if((MaterialContext->SpecularID == INVALID_TEXTURE_ID) || (MaterialContext->SpecularID > TEXTURE_ASSET_COUNT))
                    MaterialContext->SpecularID = (texture_asset_id)0;
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Texture");
                ImGui::SameLine();
                UI_Combo(AK_HashFunction("Specular Texture"), "", (int*)&MaterialContext->SpecularID, TextureNames.Data, TextureNames.Size);
            }
            else
            {                
                UI_DragFloatTool(AK_HashFunction("Specular Color"), "Value", 60, &MaterialContext->Specular, 0.01f, 0.0f, 1.0f); ImGui::SameLine();                                
                
                ak_v3f SpecularDisplay = AK_V3(MaterialContext->Specular, MaterialContext->Specular, MaterialContext->Specular);
                ImGui::ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
            }
            
            ImGui::SameLine();
            ImGui::Text("Shininess");
            ImGui::PushItemWidth(40);
            ImGui::SameLine();
            UI_DragInt(AK_HashFunction("Specular Shininess"), "", &MaterialContext->Shininess, 0.1f, 1, 512);
            ImGui::PopItemWidth();
        }                                                
    }                    
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Normal: "); ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("In Use"); ImGui::SameLine(); UI_Checkbox(AK_HashFunction("Normal In Use"), "", &MaterialContext->NormalInUse);
        
        if(MaterialContext->NormalInUse)
        {
            if((MaterialContext->NormalID == INVALID_TEXTURE_ID) || (MaterialContext->NormalID > TEXTURE_ASSET_COUNT))
                MaterialContext->NormalID = (texture_asset_id)0;
            
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture");
            ImGui::SameLine();
            UI_Combo(AK_HashFunction("Normal Texture"), "", (int*)&MaterialContext->NormalID, TextureNames.Data, TextureNames.Size);
        }
    }    
}

material_context UI_ContextFromMaterial(material* Material)
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

material UI_MaterialFromContext(material_context* MaterialContext)
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

void UI_EntitySpawner(editor* Editor, entity_spawner* Spawner, assets* Assets)
{
    if(!Spawner->Init)
    {
        Spawner->Init = true;
        UI_ClearSpawner(Spawner);                
    }
    
    world_management* WorldManagement = &Editor->WorldManagement;
    ui* UI = &Editor->UI;
    
    ak_fixed_array<const ak_char*> EntityTypes = UI_GetAllEntityTypesNotPlayer();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Entity Type");            
    ImGui::SameLine();            
    
    entity_type PrevType = Spawner->EntityType;
    
    ak_i32 SpawnType = Spawner->EntityType-1;            
    UI_Combo(AK_HashFunction("Entity Type"), "", (int*)&SpawnType, EntityTypes.Data, EntityTypes.Size);            
    entity_type Type = (entity_type)(SpawnType+1);
    
    if(PrevType != Type)
    {
        UI_ClearSpawner(Spawner);
        Spawner->EntityType = Type;
    }
    
    switch(Type)
    {
        case ENTITY_TYPE_STATIC:
        {
            ImGui::Separator();
            UI_NameTool(AK_HashFunction("Name Spawner"), Spawner->Name, MAX_OBJECT_NAME_LENGTH);
            ImGui::Separator();
            UI_TranslationTool(AK_HashFunction("Translation Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Translation);
            ImGui::Separator();
            UI_ScaleTool(AK_HashFunction("Scale Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Scale);
            ImGui::Separator();
            UI_AngleAxisTool(AK_HashFunction("Rotation Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Axis, &Spawner->Angle);
            ImGui::Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
            UI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
            ImGui::Separator();
            UI_MeshTool(Assets, &Spawner->MeshID);
            ImGui::Separator();
            UI_MaterialTool(Assets, &Spawner->MaterialContext);
            ImGui::Separator();
            
            if(UI_Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                if(AK_StringIsNullOrEmpty(AK_CreateString(Spawner->Name)))
                {
                    UI->ShowEntityNameNullErrorText = true;
                }
                else
                {
                    UI->ShowEntityNameNullErrorText = false;
                    material Material = UI_MaterialFromContext(&Spawner->MaterialContext);
                    if(Spawner->WorldIndex == 2)
                    {
                        if(WorldManagement->EntityNameCollisionMap[0].Find(Spawner->Name) ||
                           WorldManagement->EntityNameCollisionMap[1].Find(Spawner->Name))
                        {
                            UI->ShowEntityNameErrorText = true;
                        }
                        else
                        {
                            UI->ShowEntityNameErrorText = false;
                        }
                    }
                    else
                    {
                        if(WorldManagement->EntityNameCollisionMap[Spawner->WorldIndex].Find(Spawner->Name))
                        {
                            UI->ShowEntityNameErrorText = true;
                        }
                        else
                        {
                            UI->ShowEntityNameErrorText = false;
                            
                            WorldManagement->CreateDevEntity(Spawner->WorldIndex, Spawner->Name, Spawner->EntityType, Spawner->Translation, Spawner->Axis, Spawner->Angle, 
                                                             Spawner->Scale, Material, Spawner->MeshID);
                            
                            WorldManagement->EntityNameCollisionMap[Spawner->WorldIndex].Insert(Spawner->Name, true);
                        }
                    }
                }
            }
        } break;
    } 
    
    if(UI->ShowEntityNameErrorText)
    {
        ImGui::SameLine();
        UI_ErrorText("Error: Entity with name already exists");
    }
    
    if(UI->ShowEntityNameNullErrorText)
    {
        ImGui::SameLine();
        UI_ErrorText("Error: Name must be supplied");
    }
}

void UI_LightSpawner(editor* Editor, light_spawner* Spawner)
{
    if(!Spawner->Init)
    {
        UI_ClearSpawner(Spawner);
        Spawner->Init = true;
    }
    
    ui* UI = &Editor->UI;
    world_management* WorldManagement = &Editor->WorldManagement;
    
    UI_NameTool(AK_HashFunction("Light Name Spawner"), Spawner->Name, MAX_OBJECT_NAME_LENGTH);
    ImGui::Separator();
    UI_TranslationTool(AK_HashFunction("Translation Light Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Translation);
    ImGui::Separator();
    const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
    UI_WorldIndexTool(AK_HashFunction("Light World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
    ImGui::Separator();
    UI_RadiusTool(AK_HashFunction("Light Radius"), EDITOR_ITEM_WIDTH, &Spawner->Radius);
    ImGui::Separator();
    UI_IntensityTool(AK_HashFunction("Light Intensity"), EDITOR_ITEM_WIDTH, &Spawner->Intensity);
    ImGui::Separator();
    UI_Color3EditTool("Light Color", AK_HashFunction("Light Color"), Spawner->Color.Data);
    ImGui::Separator();
    
    if(UI_Button(AK_HashFunction("Create Point Light Button"), "Create"))
    {
        if(AK_StringIsNullOrEmpty(AK_CreateString(Spawner->Name)))
        {
            UI->ShowLightNameNullErrorText = true;
        }
        else
        {
            UI->ShowLightNameNullErrorText = false;
            
            if(Spawner->WorldIndex == 2)
            {
                if(WorldManagement->LightNameCollisionMap[0].Find(Spawner->Name) ||
                   WorldManagement->LightNameCollisionMap[1].Find(Spawner->Name))
                {
                    UI->ShowLightNameErrorText = true;
                }
                else
                {
                    UI->ShowLightNameErrorText = false;
                }
            }
            else
            {
                if(WorldManagement->LightNameCollisionMap[Spawner->WorldIndex].Find(Spawner->Name))
                {
                    UI->ShowLightNameErrorText = true;
                }
                else
                {
                    UI->ShowLightNameErrorText = false;
                    
                    WorldManagement->CreateDevPointLight(Spawner->WorldIndex, Spawner->Name, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);
                    
                    WorldManagement->LightNameCollisionMap[Spawner->WorldIndex].Insert(Spawner->Name, true);
                }
            }
        }
    }
    
    if(UI->ShowLightNameErrorText)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::Text("Error: Light with name already exists");
        ImGui::PopStyleColor();
    }
    
    if(UI->ShowLightNameNullErrorText)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::Text("Error: Name must be supplied");
        ImGui::PopStyleColor();
    }
}

ak_v2f UI_ListerWindow(editor* Editor)
{
    world_management* WorldManagement = &Editor->WorldManagement;
    ak_u32 CurrentWorldIndex = Editor->CurrentWorldIndex;
    
    gizmo_state* GizmoState = &Editor->GizmoState;
    selected_object* SelectedObject = Editor_GetSelectedObject(Editor);
    
    if(ImGui::Begin("Lister", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(ImGui::BeginTabBar("Worlds"))
        {
            ak_u32 WorldIndex = CurrentWorldIndex;
            for(ak_u32 World = 0; World < 2; World++)
            {
                if(ImGui::BeginTabItem(AK_FormatString(Editor->Scratch, "World %d", WorldIndex).Data))
                {
                    if(ImGui::TreeNode("Entities"))
                    {
                        AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
                        {
                            ak_bool Selected = false;
                            if((WorldIndex == CurrentWorldIndex) && SelectedObject && SelectedObject->Type == SELECTED_OBJECT_TYPE_ENTITY)
                            {
                                dev_entity* SelectedEntity = SelectedObject->GetEntity(WorldManagement, WorldIndex);
                                Selected = AK_StringEquals(SelectedEntity->Name, DevEntity->Name);
                            }
                            
                            if(ImGui::Selectable(DevEntity->Name, Selected))
                            {
                                if(WorldIndex == CurrentWorldIndex)
                                {
                                    GizmoState->SelectedObject = Editor_GizmoSelectedObject(DevEntity->ID, SELECTED_OBJECT_TYPE_ENTITY);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                    
                    if(ImGui::TreeNode("Lights"))
                    {
                        AK_ForEach(DevLight, &WorldManagement->DevPointLights[WorldIndex])
                        {
                            ak_bool Selected = false;
                            if((WorldIndex == CurrentWorldIndex) && SelectedObject &&
                               SelectedObject->Type == SELECTED_OBJECT_TYPE_LIGHT)
                            {
                                dev_point_light* SelectedPointLight = 
                                    SelectedObject->GetPointLight(WorldManagement, WorldIndex);
                                Selected = AK_StringEquals(SelectedPointLight->Name, DevLight->Name);
                            }
                            
                            if(ImGui::Selectable(DevLight->Name, Selected))
                            {
                                if(WorldIndex == CurrentWorldIndex)
                                {
                                    GizmoState->SelectedObject = 
                                        Editor_GizmoSelectedObject(DevLight->ID, 
                                                                   SELECTED_OBJECT_TYPE_LIGHT);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                    
                    ImGui::EndTabItem();
                }
                WorldIndex = !WorldIndex;
            }
            ImGui::EndTabBar();
        }
    }
    ak_v2f Result = ImGui::GetWindowSize();
    ImGui::End();
    return Result;
}

ak_v2f UI_DetailsWindow(editor* Editor, assets* Assets)
{
    ak_v2f Result = {};
    
    selected_object* SelectedObject = Editor_GetSelectedObject(Editor);
    if(SelectedObject)
    {
        if(ImGui::Begin("Details", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            switch(SelectedObject->Type)
            {
                case SELECTED_OBJECT_TYPE_ENTITY:
                {
                    dev_entity* Entity = SelectedObject->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                    
                    ImGui::Text("Name: %s", Entity->Name);
                    UI_TranslationTool("Position", AK_HashFunction("Edit Translation"), EDITOR_ITEM_WIDTH, &Entity->Transform.Translation);
                    UI_ScaleTool("Scale", AK_HashFunction("Edit Scale"), EDITOR_ITEM_WIDTH, 
                                 &Entity->Transform.Scale);
                    
                    ak_v3f Rotation = Entity->Euler;
                    
                    UI_SameLineLabel("Rotation");
                    ImGui::PushItemWidth(EDITOR_ITEM_WIDTH);
                    ak_u32 Hash = AK_HashFunction("Edit Rotation");
                    
                    ak_f32 HalfAngleMax = 180.0f;
                    
                    UI_DragAngle(Hash, "X", &Rotation.x, 0.1f, -HalfAngleMax, HalfAngleMax); 
                    ImGui::SameLine();
                    UI_DragAngle(Hash+1, "Y", &Rotation.y, 0.1f, -HalfAngleMax, HalfAngleMax); ImGui::SameLine();
                    UI_DragAngle(Hash+2, "Z", &Rotation.z, 0.1f, -HalfAngleMax, HalfAngleMax);
                    ImGui::PopItemWidth();
                    
                    ImGui::Text("Type: %s", UI_GetEntityType(Entity->Type));
                    material_context MaterialContext = UI_ContextFromMaterial(&Entity->Material);
                    UI_MaterialTool(Assets, &MaterialContext);
                    Entity->Material = UI_MaterialFromContext(&MaterialContext);
                    
                    
                    ak_v3f PointDiff = Rotation-Entity->Euler;
                    
                    ak_m3f OriginalRotation = AK_Transpose(AK_QuatToMatrix(Entity->Transform.Orientation));
                    
                    ak_quatf XOrientation = AK_RotQuat(OriginalRotation.XAxis, PointDiff.x);
                    ak_quatf YOrientation = AK_RotQuat(OriginalRotation.YAxis, PointDiff.y);
                    ak_quatf ZOrientation = AK_RotQuat(OriginalRotation.ZAxis, PointDiff.z);
                    
                    Entity->Transform.Orientation *= AK_Normalize(XOrientation*YOrientation*ZOrientation);
                    
                    Entity->Euler = Rotation;
                } break;
                
                case SELECTED_OBJECT_TYPE_LIGHT:
                {
                    dev_point_light* PointLight = SelectedObject->GetPointLight(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                    
                    ImGui::Text("Name: %s", PointLight->Name);
                    UI_TranslationTool("Position", AK_HashFunction("Edit Point Light Translation"), 
                                       EDITOR_ITEM_WIDTH, &PointLight->Light.Position);
                    UI_RadiusTool(AK_HashFunction("Edit Point Light Radius"), 
                                  EDITOR_ITEM_WIDTH, &PointLight->Light.Radius);
                    UI_IntensityTool(AK_HashFunction("Edit Point Light Intensity"), EDITOR_ITEM_WIDTH, &PointLight->Light.Intensity);
                    UI_Color3EditTool("Color", AK_HashFunction("Edit Light Color"), (ak_f32*)&PointLight->Light.Color);
                } break;
            }
        }
        Result = ImGui::GetWindowSize();
        ImGui::End();
    }
    
    return Result;
}

void UI_Logs(ak_f32 LogHeight)
{
    ImGui::SetNextWindowPos(ImVec2(0, LogHeight));
    if(ImGui::Begin("Logs", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(ImGui::Button("Clear"))
        {
            Internal__Logs.Clear();
            Internal__LogArena->Clear();
        }
        
        ImGui::SameLine();
        
        if(ImGui::Button("Copy")) ImGui::LogToClipboard();
        
        for(ak_u32 LogIndex = 0; LogIndex < Internal__Logs.Size; LogIndex++)
        {
            ak_string Log = Internal__Logs[LogIndex];
            ImGui::TextUnformatted(Log.Data, Log.Data+Log.Length);
        }
    }
    ImGui::End();
}