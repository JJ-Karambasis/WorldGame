global ak_u32 StaticEntity_HighestIndex;
global ak_u32 ButtonEntity_HighestIndex;
global ak_u32 MovableEntity_HighestIndex;
global ak_u32 PointLight_HighestIndex;

global const ak_char* StaticEntity_DefaultName = "Static_%d";
global const ak_char* ButtonEntity_DefaultName = "Button_%d";
global const ak_char* MovableEntity_DefaultName = "Movable_%d";
global const ak_char* PointLight_DefaultName = "PointLight_%d";

ak_string UI_GetDefaultNames(ak_arena* Scratch, ak_hash_map<ak_string, ak_u64>* Tables, 
                             const ak_char* DefaultName, ak_u32* HighestIndex)
{
    ak_string Name = AK_CreateEmptyString();
    for(;;)
    {
        Name = AK_FormatString(Scratch, DefaultName, *HighestIndex);
        
        ak_bool Found = false;
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            if(Tables[WorldIndex].Find(Name))
                Found = true;
        }
        
        if(Found)
        {
            (*HighestIndex)++;
        }
        else
            break;
    }
    
    return Name;
}

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
    
    if(ImGui::IsItemActivated())
    {
    }
    
    if(ImGui::IsItemDeactivated())
    {
    }
    
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

void UI_SpawnerRefreshName(editor* Editor, entity_spawner* Spawner)
{
    const ak_char* DefaultName = NULL;
    ak_u32* DefaultIndex = NULL;
    
    switch(Spawner->EntityType)
    {
        case ENTITY_TYPE_STATIC:
        {
            DefaultIndex = &StaticEntity_HighestIndex;
            DefaultName = StaticEntity_DefaultName;
        } break;
        
        case ENTITY_TYPE_BUTTON:
        {
            DefaultIndex = &ButtonEntity_HighestIndex;
            DefaultName = ButtonEntity_DefaultName;
        } break;
        
        case ENTITY_TYPE_MOVABLE:
        {
            DefaultIndex = &MovableEntity_HighestIndex;
            DefaultName = MovableEntity_DefaultName;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    ak_string Name = UI_GetDefaultNames(Editor->Scratch, Editor->WorldManagement.EntityTables, 
                                        DefaultName, DefaultIndex);
    AK_MemoryCopy(Spawner->Name, Name.Data, Name.Length);
}

void UI_SpawnerRefreshName(editor* Editor, light_spawner* Spawner)
{
    ak_string Name = UI_GetDefaultNames(Editor->Scratch, Editor->WorldManagement.PointLightTables, 
                                        PointLight_DefaultName, &PointLight_HighestIndex);
    AK_MemoryCopy(Spawner->Name, Name.Data, Name.Length);
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
        UI_SpawnerRefreshName(Editor, Spawner);
    }
    
    world_management* WorldManagement = &Editor->WorldManagement;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
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
        UI_SpawnerRefreshName(Editor, Spawner);
    }
    
    ImGui::Separator();
    UI_NameTool(AK_HashFunction("Name Spawner"), Spawner->Name, MAX_OBJECT_NAME_LENGTH);
    ImGui::SameLine();
    ImGui::Separator();
    switch(Type)
    {
        case ENTITY_TYPE_BUTTON:
        {
            UI_TranslationTool(AK_HashFunction("Translation Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Translation);
            ImGui::Separator();
            UI_ScaleTool(AK_HashFunction("Scale Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Scale);
            ImGui::Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
            UI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, 
                              WorldIndexList, AK_Count(WorldIndexList));
            ImGui::Separator();
            Spawner->MeshID = MESH_ASSET_ID_BUTTON;
            UI_MaterialTool(Assets, &Spawner->MaterialContext);
            ImGui::Separator();
            
        } break;
        
        case ENTITY_TYPE_MOVABLE:
        {
            UI_TranslationTool(AK_HashFunction("Translation Spawner"), EDITOR_ITEM_WIDTH, 
                               &Spawner->Translation);
            ImGui::Separator();
            UI_ScaleTool(AK_HashFunction("Scale Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Scale);
            ImGui::Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both", "Linked"};
            UI_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex,
                              WorldIndexList, AK_Count(WorldIndexList));
            ImGui::Separator();
            Spawner->MeshID = MESH_ASSET_ID_BOX;
            UI_MaterialTool(Assets, &Spawner->MaterialContext);
            ImGui::Separator();
            
        } break;
        
        case ENTITY_TYPE_STATIC:
        {
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
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    } 
    
    if(UI_Button(AK_HashFunction("Create Entity Button"), "Create"))
    {
        ak_string Name = AK_CreateString(Spawner->Name);
        if(!AK_StringIsNullOrEmpty(Name))
        {
            UI->ShowEntityNameNullErrorText = false;
            material Material = UI_MaterialFromContext(&Spawner->MaterialContext);
            if(Spawner->WorldIndex < 2)
            {
                if(!WorldManagement->EntityTables[Spawner->WorldIndex].Find(Name))
                {
                    UI->ShowEntityNameErrorText = false;
                    dev_entity* Entity = WorldManagement->CreateDevEntity(Spawner->WorldIndex, Name.Data, 
                                                                          Spawner->EntityType, Spawner->Translation, 
                                                                          Spawner->Axis, Spawner->Angle, Spawner->Scale, 
                                                                          Material, Spawner->MeshID);
                    
                    EditRecordings->PushCreateEntry(Spawner->WorldIndex, Entity);
                }
                else
                    UI->ShowEntityNameErrorText = true;
            }
            else
            {
                if(WorldManagement->EntityTables[0].Find(Name) ||
                   WorldManagement->EntityTables[1].Find(Name))
                {
                    UI->ShowEntityNameErrorText = true;
                }
                else
                {
                    UI->ShowEntityNameErrorText = false;
                    
                    dev_entity* Entity0 = WorldManagement->CreateDevEntity(0, Name.Data, 
                                                                           Spawner->EntityType, Spawner->Translation, 
                                                                           Spawner->Axis, Spawner->Angle, 
                                                                           Spawner->Scale, Material, Spawner->MeshID);
                    
                    dev_entity* Entity1 = WorldManagement->CreateDevEntity(1, Name.Data, 
                                                                           Spawner->EntityType, Spawner->Translation, 
                                                                           Spawner->Axis, Spawner->Angle, 
                                                                           Spawner->Scale, Material,
                                                                           Spawner->MeshID);
                    
                    if(Spawner->WorldIndex == 3)
                    {
                        Entity0->LinkName = Entity1->Name;
                        Entity1->LinkName = Entity0->Name;
                    }
                    
                    EditRecordings->PushCreateEntry(0, Entity0, Entity1);
                }
            }
            
            UI_SpawnerRefreshName(Editor, Spawner);
        }
        else
            UI->ShowEntityNameNullErrorText = true;
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
        UI_SpawnerRefreshName(Editor, Spawner);
        Spawner->Init = true;
    }
    
    ui* UI = &Editor->UI;
    world_management* WorldManagement = &Editor->WorldManagement;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
    
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
        ak_string Name = AK_CreateString(Spawner->Name);
        if(AK_StringIsNullOrEmpty(Name))
        {
            UI->ShowLightNameNullErrorText = true;
        }
        else
        {
            UI->ShowLightNameNullErrorText = false;
            
            if(Spawner->WorldIndex == 2)
            {
                if(WorldManagement->PointLightTables[0].Find(Name) ||
                   WorldManagement->PointLightTables[1].Find(Name))
                {
                    UI->ShowLightNameErrorText = true;
                }
                else
                {
                    UI->ShowLightNameErrorText = false;
                    
                    dev_point_light* PointLight0 = WorldManagement->CreateDevPointLight(0, Name.Data, 
                                                                                        Spawner->Translation, 
                                                                                        Spawner->Radius, 
                                                                                        Spawner->Color, 
                                                                                        Spawner->Intensity);
                    
                    dev_point_light* PointLight1 = 
                        WorldManagement->CreateDevPointLight(1, Name.Data, 
                                                             Spawner->Translation, 
                                                             Spawner->Radius, 
                                                             Spawner->Color, 
                                                             Spawner->Intensity);
                    
                    EditRecordings->PushCreateEntry(0, PointLight0, PointLight1);
                }
            }
            else
            {
                if(WorldManagement->PointLightTables[Spawner->WorldIndex].Find(Name))
                {
                    UI->ShowLightNameErrorText = true;
                }
                else
                {
                    UI->ShowLightNameErrorText = false;
                    dev_point_light* PointLight = WorldManagement->CreateDevPointLight(Spawner->WorldIndex, Name.Data, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);
                    
                    EditRecordings->PushCreateEntry(Spawner->WorldIndex, PointLight);
                }
            }
        }
        
        UI_SpawnerRefreshName(Editor, Spawner);
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
    object* SelectedObject = Editor_GetSelectedObject(Editor);
    
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
                            if((WorldIndex == CurrentWorldIndex) && SelectedObject && SelectedObject->Type == OBJECT_TYPE_ENTITY)
                            {
                                dev_entity* SelectedEntity = SelectedObject->GetEntity(WorldManagement, WorldIndex);
                                Selected = AK_StringEquals(SelectedEntity->Name, DevEntity->Name);
                            }
                            
                            if(ImGui::Selectable(DevEntity->Name.Data, Selected))
                            {
                                if(WorldIndex == CurrentWorldIndex)
                                {
                                    ak_u64 ID = *WorldManagement->EntityTables[WorldIndex].Find(DevEntity->Name);
                                    GizmoState->SelectedObject = Editor_GizmoSelectedObject(ID, OBJECT_TYPE_ENTITY);
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
                               SelectedObject->Type == OBJECT_TYPE_LIGHT)
                            {
                                dev_point_light* SelectedPointLight = 
                                    SelectedObject->GetPointLight(WorldManagement, WorldIndex);
                                Selected = AK_StringEquals(SelectedPointLight->Name, DevLight->Name);
                            }
                            
                            if(ImGui::Selectable(DevLight->Name.Data, Selected))
                            {
                                if(WorldIndex == CurrentWorldIndex)
                                {
                                    ak_u64 ID = *WorldManagement->PointLightTables[WorldIndex].Find(DevLight->Name);
                                    GizmoState->SelectedObject = 
                                        Editor_GizmoSelectedObject(ID, 
                                                                   OBJECT_TYPE_LIGHT);
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

void Details_SetTempObject(editor* Editor, object* Object)
{
    Editor->UI.TempObject.Type = Object->Type;
    switch(Object->Type)
    {
        case OBJECT_TYPE_ENTITY:
        {
            Editor->UI.TempObject.Entity = *Object->GetEntity(&Editor->WorldManagement, 
                                                              Editor->CurrentWorldIndex);
        } break;
        
        case OBJECT_TYPE_LIGHT:
        {
            Editor->UI.TempObject.PointLight = *Object->GetPointLight(&Editor->WorldManagement, 
                                                                      Editor->CurrentWorldIndex);
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
}

void Details_CheckObjectChange(editor* Editor, object* Object)
{
    AK_Assert(Object->Type == Editor->UI.TempObject.Type, "Deactivated object cannot be a different type than the temp object");
    switch(Object->Type)
    {
        case OBJECT_TYPE_ENTITY:
        {
            dev_entity* Entity = Object->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
            
            material NewMaterial = UI_MaterialFromContext(&Editor->UI.TempContext);
            if(!Editor_AreEntitiesEqual(Entity, &Editor->UI.TempObject.Entity) ||
               !AreMaterialsEqual(Entity->Material, NewMaterial))
            {
                Entity->Material = NewMaterial;
                Editor->EditRecordings.PushPropertyEntry(Editor->CurrentWorldIndex, &Editor->UI.TempObject.Entity, 
                                                         Entity);
            }
        } break;
        
        case OBJECT_TYPE_LIGHT:
        {
            dev_point_light* PointLight = Object->GetPointLight(&Editor->WorldManagement, 
                                                                Editor->CurrentWorldIndex);
            if(!Editor_ArePointLightsEqual(PointLight, &Editor->UI.TempObject.PointLight))
            {
                Editor->EditRecordings.PushPropertyEntry(Editor->CurrentWorldIndex, 
                                                         &Editor->UI.TempObject.PointLight, 
                                                         PointLight);
            }
        } break;
    }
}

void Details_ActiveEvents(editor* Editor, object* Object)
{
    if(ImGui::IsItemActivated())
    {
        Details_SetTempObject(Editor, Object);
    }
    
    if(ImGui::IsItemDeactivated())
    {
        Details_CheckObjectChange(Editor, Object);
    }
}

void Details_DragInt(editor* Editor, object* Object, const ak_char* Label, 
                     ak_i32* Data, ak_f32 Speed, ak_i32 Min, ak_i32 Max)
{
    UI_SameLineLabel(Label);
    ImGui::PushItemWidth(40);
    ImGui::DragInt("", Data, Speed, Min, Max);
    Details_ActiveEvents(Editor, Object);
    ImGui::PopItemWidth();
}

void Details_DragFloatTool(editor* Editor, object* Object, const ak_char* Label, 
                           ak_f32* Data, ak_f32 Speed, ak_f32 Min, ak_f32 Max)
{
    ImGui::PushItemWidth(EDITOR_ITEM_WIDTH);
    ImGui::DragFloat(Label, Data, Speed, Min, Max, "%.3f");
    Details_ActiveEvents(Editor, Object);
    ImGui::PopItemWidth();
}

void Details_DragAngleTool(editor* Editor, object* Object, const ak_char* Label, 
                           ak_f32* Radians, ak_f32 Speed, ak_f32 Min, ak_f32 Max)
{
    ak_f32 Degree = AK_ToDegree(*Radians);
    
    ImGui::PushItemWidth(EDITOR_ITEM_WIDTH);
    ImGui::DragFloat(Label, &Degree, Speed, Min, Max, "%.3f");
    Details_ActiveEvents(Editor, Object);
    ImGui::PopItemWidth();
    
    *Radians = AK_ToRadians(Degree);
}

void Details_TranslationTool(editor* Editor, object* Object, ak_v3f* Translation)
{
    UI_SameLineLabel("Position");
    
    ImGui::PushID(AK_HashFunction("Position X"));
    Details_DragFloatTool(Editor, Object, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Position Y"));
    Details_DragFloatTool(Editor, Object, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Position Z"));
    Details_DragFloatTool(Editor, Object, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
}

void Details_ScaleTool(editor* Editor, object* Object, ak_v3f* Scale)
{
    UI_SameLineLabel("Scale");
    
    ImGui::PushID(AK_HashFunction("Scale X"));
    Details_DragFloatTool(Editor, Object, "X", &Scale->x, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Scale Y"));
    Details_DragFloatTool(Editor, Object, "Y", &Scale->y, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Scale Z"));
    Details_DragFloatTool(Editor, Object, "Z", &Scale->z, 0.1f, -1000.0f, 1000.0f);
    ImGui::PopID();
}

void Details_RotationTool(editor* Editor, object* Object, ak_v3f* Rotation)
{
    UI_SameLineLabel("Rotation");
    
    ImGui::PushID(AK_HashFunction("Rotation X"));
    Details_DragAngleTool(Editor, Object, "X", &Rotation->x, 0.1f, -180.0f, 180.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Rotation Y"));
    Details_DragAngleTool(Editor, Object, "Y", &Rotation->y, 0.1f, -180.0f, 180.0f);
    ImGui::PopID();
    ImGui::SameLine();
    
    ImGui::PushID(AK_HashFunction("Rotation Z"));
    Details_DragAngleTool(Editor, Object, "Z", &Rotation->z, 0.1f, -180.0f, 180.0f);
    ImGui::PopID();
}

void Details_Checkbox(editor* Editor, object* Object, const ak_char* Label, ak_bool* Flag)
{
    UI_SameLineLabel(Label);
    ImGui::Checkbox("", (bool*)Flag);
    Details_ActiveEvents(Editor, Object);
}

void Details_Combo(editor* Editor, object* Object, const ak_char* Label, int* Data, 
                   const ak_char** Names, ak_u32 Size)
{
    UI_SameLineLabel(Label);
    ImGui::Combo("", Data, Names, Size);
    Details_ActiveEvents(Editor, Object);
}

void Details_Color3EditTool(editor* Editor, object* Object, const ak_char* Label, ak_f32* Data)
{
    UI_SameLineLabel(Label);
    ImGui::ColorEdit3("", Data, ImGuiColorEditFlags_RGB);
    Details_ActiveEvents(Editor, Object);
}

void Details_MaterialTool(editor* Editor, object* Object, assets* Assets, material_context* MaterialContext)
{    
    ak_fixed_array<const ak_char*> TextureNames = UI_GetAllTextureInfoNames(Assets);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Material");        
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Diffuse: "); ImGui::SameLine(); 
        
        ImGui::PushID(AK_HashFunction("Diffuse Is Texture"));
        Details_Checkbox(Editor, Object, "Is Texture", &MaterialContext->DiffuseIsTexture);
        ImGui::PopID();
        ImGui::SameLine();
        
        if(MaterialContext->DiffuseIsTexture)
        {
            if((MaterialContext->DiffuseID == INVALID_TEXTURE_ID) || (MaterialContext->DiffuseID > TEXTURE_ASSET_COUNT))
                MaterialContext->DiffuseID = (texture_asset_id)0;
            
            ImGui::PushID(AK_HashFunction("Diffuse Texture"));
            Details_Combo(Editor, Object, "Texture", (int*)&MaterialContext->DiffuseID, TextureNames.Data, 
                          TextureNames.Size);
            ImGui::PopID();
        }
        else
        {
            ImGui::PushID(AK_HashFunction("Diffuse Color"));
            Details_Color3EditTool(Editor, Object, "Color", MaterialContext->Diffuse.Data);
            ImGui::PopID();
        }
    }
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Specular: "); ImGui::SameLine();
        
        ImGui::PushID(AK_HashFunction("Specular In Use"));
        Details_Checkbox(Editor, Object, "In Use", &MaterialContext->SpecularInUse);
        ImGui::PopID();
        
        if(MaterialContext->SpecularInUse)
        {
            ImGui::PushID(AK_HashFunction("Specular Is Texture"));
            Details_Checkbox(Editor, Object, "Is Texture", &MaterialContext->SpecularIsTexture);
            ImGui::PopID();
            if(MaterialContext->SpecularIsTexture)
            {
                if((MaterialContext->SpecularID == INVALID_TEXTURE_ID) || (MaterialContext->SpecularID > TEXTURE_ASSET_COUNT))
                    MaterialContext->SpecularID = (texture_asset_id)0;
                
                ImGui::PushID(AK_HashFunction("Specular Texture"));
                Details_Combo(Editor, Object, "Texture", (int*)&MaterialContext->SpecularID, 
                              TextureNames.Data, TextureNames.Size);
                ImGui::PopID();
            }
            else
            {   
                ImGui::PushID(AK_HashFunction("Specular Color"));
                Details_DragFloatTool(Editor, Object, "Value", &MaterialContext->Specular, 0.01f, 0.0f, 1.0f);
                ImGui::PopID();
                ImGui::SameLine();
                
                ak_v3f SpecularDisplay = AK_V3(MaterialContext->Specular, MaterialContext->Specular, MaterialContext->Specular);
                ImGui::PushID(AK_HashFunction("Specular Display"));
                ImGui::ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
                ImGui::PopID();
            }
            
            ImGui::PushID(AK_HashFunction("Specular Shininess"));
            Details_DragInt(Editor, Object, "Shininess", &MaterialContext->Shininess, 0.1f, 1, 512);
            ImGui::PopID();
        }                                                
    }                    
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Normal: "); ImGui::SameLine();
        
        ImGui::PushID(AK_HashFunction("Normal In Use"));
        Details_Checkbox(Editor, Object, "In Use", &MaterialContext->NormalInUse);
        ImGui::PopID();
        
        if(MaterialContext->NormalInUse)
        {
            if((MaterialContext->NormalID == INVALID_TEXTURE_ID) || (MaterialContext->NormalID > TEXTURE_ASSET_COUNT))
                MaterialContext->NormalID = (texture_asset_id)0;
            
            ImGui::PushID(AK_HashFunction("Normal Texture"));
            Details_Combo(Editor, Object, "Texture", (int*)&MaterialContext->NormalID, TextureNames.Data, TextureNames.Size);
            ImGui::PopID();
        }
    }    
}

template <typename type>
void Details_ObjectName(editor* Editor, object_type Type, ak_hash_map<ak_string, ak_u64>* Tables, type* Object)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Name: %.*s", Object->Name.Length, Object->Name.Data);
    ImGui::SameLine();
    
    ui* UI = &Editor->UI;
    ak_u32 WorldIndex = Editor->CurrentWorldIndex;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
    
    ak_bool OldRenameModalState = UI->RenameModalState;
    if(ImGui::Button("Rename"))
        UI->RenameModalState = true; 
    
    if(UI->RenameModalState)
    {
        if(!ImGui::IsPopupOpen("Rename Modal"))
            ImGui::OpenPopup("Rename Modal");
        
        if(ImGui::BeginPopupModal("Rename Modal"))
        {
            local ak_char Name[MAX_OBJECT_NAME_LENGTH];
            if(OldRenameModalState != UI->RenameModalState)
                AK_MemoryCopy(Name, Object->Name.Data, Object->Name.Length);
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("New Name");
            ImGui::InputText("", Name, MAX_OBJECT_NAME_LENGTH, 
                             ImGuiInputTextFlags_CharsNoBlank);
            
            ak_string NewName = AK_CreateString(Name);
            
            ak_u64* ID = Tables[WorldIndex].Find(NewName);
            
            ak_bool Disabled = ID || AK_StringIsNullOrEmpty(NewName);
            
            if(Disabled) UI_PushDisabledItem();
            
            if(ImGui::Button("Rename"))
            {
                ak_string OldName = Object->Name;
                
                ak_u64* pEntityID = Tables[WorldIndex].Find(Object->Name);
                AK_Assert(pEntityID, "Object must be allocated for it to be renamed");
                ak_u64 EntityID = *pEntityID;
                Tables[WorldIndex].Remove(Object->Name);
                Object->Name = AK_PushString(NewName, Editor->WorldManagement.StringArena);
                Tables[WorldIndex].Insert(Object->Name, EntityID);
                ImGui::CloseCurrentPopup();
                UI->RenameModalState = false;
                
                EditRecordings->PushRenameEntry(Editor->CurrentWorldIndex, Type, OldName, Object->Name);
            }
            
            ImGui::SameLine();
            
            if(Disabled) UI_PopDisabledItem();
            
            if(ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
                UI->RenameModalState = false;
            }
            
            if(ID)
            {
                UI_ErrorText("Entity with name '%.*s' already exists", NewName.Length, NewName.Data);
            }
            
            ImGui::EndPopup();
        }
    }
}

ak_v2f UI_DetailsWindow(editor* Editor, assets* Assets)
{
    ak_v2f Result = {};
    
    object* SelectedObject = Editor_GetSelectedObject(Editor);
    world_management* WorldManagement = &Editor->WorldManagement;
    
    if(SelectedObject)
    {
        if(ImGui::Begin("Details", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            switch(SelectedObject->Type)
            {
                case OBJECT_TYPE_ENTITY:
                {
                    dev_entity* Entity = SelectedObject->GetEntity(WorldManagement, Editor->CurrentWorldIndex);
                    
                    Editor->UI.TempContext = UI_ContextFromMaterial(&Entity->Material);
                    
                    ak_bool DisableName = (Entity->Type == ENTITY_TYPE_PLAYER);
                    if(DisableName) UI_PushDisabledItem();
                    
                    Details_ObjectName(Editor, SelectedObject->Type, WorldManagement->EntityTables, Entity);
                    if(!AK_StringIsNullOrEmpty(Entity->LinkName))
                    {
                        ak_u32 WorldIndex = !Editor->CurrentWorldIndex;
                        ak_u64* ID = WorldManagement->EntityTables[WorldIndex].Find(Entity->LinkName);
                        AK_Assert(ID, "Linked entity cannot be deleted without the other linked entry as well");
                        
                        dev_entity* LinkObject = WorldManagement->DevEntities[WorldIndex].Get(*ID);
                        LinkObject->LinkName = Entity->Name;
                    }
                    
                    if(DisableName) UI_PopDisabledItem();
                    
                    Details_TranslationTool(Editor, SelectedObject, &Entity->Transform.Translation);
                    
                    Details_ScaleTool(Editor, SelectedObject, &Entity->Transform.Scale);
                    
                    ak_bool DisableRotation = (Entity->Type == ENTITY_TYPE_BUTTON ||
                                               Entity->Type == ENTITY_TYPE_MOVABLE);
                    
                    if(DisableRotation) UI_PushDisabledItem();
                    ak_v3f Rotation = Entity->Euler;
                    Details_RotationTool(Editor, SelectedObject, &Rotation);
                    if(DisableRotation) UI_PopDisabledItem();
                    
                    ImGui::Text("Type: %s", UI_GetEntityType(Entity->Type));
                    
                    Details_MaterialTool(Editor, SelectedObject, Assets, &Editor->UI.TempContext);
                    Entity->Material = UI_MaterialFromContext(&Editor->UI.TempContext);
                    
                    ak_bool DisableMeshEdit = (Entity->Type == ENTITY_TYPE_BUTTON ||
                                               Entity->Type == ENTITY_TYPE_MOVABLE ||
                                               Entity->Type == ENTITY_TYPE_PLAYER);
                    
                    if(DisableMeshEdit) UI_PushDisabledItem();
                    
                    ak_fixed_array<const ak_char*> MeshNames = UI_GetAllMeshInfoNames(Assets);
                    ImGui::PushID(AK_HashFunction("Mesh Edit"));
                    Details_Combo(Editor, SelectedObject, "Mesh", (int*)&Entity->MeshID, MeshNames.Data, 
                                  MeshNames.Size);
                    ImGui::PopID();
                    
                    if(DisableMeshEdit) UI_PopDisabledItem();
                    
                    ak_v3f PointDiff = Rotation-Entity->Euler;
                    
                    ak_m3f OriginalRotation = AK_Transpose(AK_QuatToMatrix(Entity->Transform.Orientation));
                    
                    ak_quatf XOrientation = AK_RotQuat(OriginalRotation.XAxis, PointDiff.x);
                    ak_quatf YOrientation = AK_RotQuat(OriginalRotation.YAxis, PointDiff.y);
                    ak_quatf ZOrientation = AK_RotQuat(OriginalRotation.ZAxis, PointDiff.z);
                    
                    Entity->Transform.Orientation *= AK_Normalize(XOrientation*YOrientation*ZOrientation);
                    
                    Entity->Euler = Rotation;
                    
                    if(Entity->Type == ENTITY_TYPE_BUTTON)
                    {
                        ImGui::PushID(AK_HashFunction("Button IsToggled"));
                        Details_Checkbox(Editor, SelectedObject, "Is Toggled", &Entity->IsToggled);
                        ImGui::PopID();
                    }
                } break;
                
                case OBJECT_TYPE_LIGHT:
                {
                    dev_point_light* PointLight = SelectedObject->GetPointLight(WorldManagement, Editor->CurrentWorldIndex);
                    
                    Details_ObjectName(Editor, SelectedObject->Type, WorldManagement->PointLightTables, PointLight);
                    
                    Details_TranslationTool(Editor, SelectedObject, &PointLight->Light.Position);
                    
                    ImGui::PushID(AK_HashFunction("Edit Point Light Radius"));
                    Details_DragFloatTool(Editor, SelectedObject, "Radius", &PointLight->Light.Radius, 0.01f, 0.0f, 100.0f);
                    ImGui::PopID();
                    
                    ImGui::PushID(AK_HashFunction("Edit Point Light Intensity"));
                    Details_DragFloatTool(Editor, SelectedObject, "Intensity", &PointLight->Light.Intensity, 0.01f, 1.0f, 100.0f);
                    ImGui::PopID();
                    
                    ImGui::PushID(AK_HashFunction("Edit Light Color"));
                    Details_Color3EditTool(Editor, SelectedObject, "Color", (ak_f32*)&PointLight->Light.Color);
                    ImGui::PopID();
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
    if(ImGui::Begin("Logs"))
    {
        if(ImGui::Button("Clear"))
        {
            Internal__Logs.Clear();
            Internal__LogArena->Clear();
        }
        
        ImGui::SameLine();
        
        if(ImGui::Button("Copy")) ImGui::LogToClipboard();
        ImGui::Separator();
        
        ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
        
        for(ak_u32 LogIndex = 0; LogIndex < Internal__Logs.Size; LogIndex++)
        {
            ak_string Log = Internal__Logs[LogIndex];
            ImGui::TextUnformatted(Log.Data, Log.Data+Log.Length);
        }
        
        if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        
        ImGui::EndChild();
    }
    ImGui::End();
}

void UI_Timers(timed_entry* TimeEntries)
{
    if(ImGui::Begin("Performance Timers"))
    {
        ImGui::Columns(5, NULL, false);
        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::Text("Cycles");
        ImGui::NextColumn();
        ImGui::Text("Calls");
        ImGui::NextColumn();
        ImGui::Text("Cycles/Call");
        ImGui::NextColumn();
        ImGui::Text("Time");
        ImGui::NextColumn();
        for(ak_u32 TimerIndex = 0; TimerIndex < TIMED_BLOCK_ENTRY_COUNT; TimerIndex++)
        {
            timed_entry* Entry = TimeEntries + TimerIndex;
            if(Entry->Count > 0)
            {
                ImGui::Text(Entry->Name);
                ImGui::NextColumn();
                ImGui::Text("%dhz", Entry->Cycles);
                ImGui::NextColumn();
                ImGui::Text("%d", Entry->Count);
                ImGui::NextColumn();
                ImGui::Text("%dhz", Entry->Cycles/Entry->Count);
                ImGui::NextColumn();
                ImGui::Text("%fms", Entry->ElapsedTime*1000.0);
                ImGui::NextColumn();
            }
        }
    }
    ImGui::End();
}

void UI_GameLister(editor* Editor)
{
    game* Game = Editor->GameContext.Game;
    if(ImGui::Begin("Game Lister"))
    {
        if(ImGui::TreeNode("Entities"))
        {
            for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
            {
                ak_array<ak_string>* GameEntityNames = &Editor->GameContext.GameEntityNames[WorldIndex];
                ak_pool<entity>* Entities = &Game->World->EntityStorage[WorldIndex];
                ak_array<physics_object>* PhysicsObjects = &Game->World->PhysicsObjects[WorldIndex];
                ak_array<button_state>* ButtonStates = &Game->World->ButtonStates[WorldIndex];
                ak_array<movable>* Movables = &Game->World->Movables[WorldIndex];
                
                if(ImGui::TreeNode((void*)(ak_uaddr)WorldIndex, "World %c", WorldIndex == 0 ? 'A' : 'B'))
                {
                    AK_ForEach(Entity, Entities)
                    {
                        ak_u32 EntityIndex = AK_PoolIndex(Entity->ID);
                        ak_string Name = *GameEntityNames->Get(EntityIndex);
                        if(AK_StringIsNullOrEmpty(Name))
                            Name = AK_CreateString("Unknown");
                        
                        ImGui::Columns(2, "", false);
                        ak_bool Open = ImGui::TreeNode((void*)(ak_uaddr)Entity->ID, "%.*s", Name.Length, Name.Data);
                        ImGui::NextColumn();
                        if(Open)
                        {
                            physics_object* PhysicsObject = PhysicsObjects->Get(EntityIndex);
                            
                            ImGui::Text("Type: %s", UI_GetEntityType(Entity->Type));
                            ImGui::Text("Position (%.3f, %.3f, %.3f)", PhysicsObject->Position.x, PhysicsObject->Position.y, 
                                        PhysicsObject->Position.z);
                            ImGui::Text("Scale (%.3f, %.3f, %.3f)", 
                                        PhysicsObject->Scale.x, PhysicsObject->Scale.y, 
                                        PhysicsObject->Scale.z);
                            ImGui::Text("Orientation (%.3f, %.3f, %.3f, %.3f)",
                                        PhysicsObject->Orientation.x, PhysicsObject->Orientation.y, PhysicsObject->Orientation.z, PhysicsObject->Orientation.w);
                            ImGui::Text("Velocity (%.3f, %.3f, %.3f)", 
                                        PhysicsObject->Velocity.x, 
                                        PhysicsObject->Velocity.y, 
                                        PhysicsObject->Velocity.z);
                            
                            switch(Entity->Type)
                            {
                                case ENTITY_TYPE_PLAYER:
                                {
                                    player* Player = &Game->World->Players[WorldIndex];
                                    ImGui::Text("Gravity Velocity (%.3f, %.3f, %.3f)", 
                                                Player->GravityVelocity.x, 
                                                Player->GravityVelocity.y, 
                                                Player->GravityVelocity.z);
                                } break;
                                
                                case ENTITY_TYPE_MOVABLE:
                                {
                                    movable* Movable = Movables->Get(EntityIndex);
                                    ImGui::Text("Gravity Velocity (%.3f, %.3f, %.3f)", 
                                                Movable->GravityVelocity.x, 
                                                Movable->GravityVelocity.y, 
                                                Movable->GravityVelocity.z);
                                    
                                    ImGui::Text("Children: (%d)", Movable->ChildIDs.Size);
                                    for(ak_u32 Index = 0; Index < Movable->ChildIDs.Size; Index++)
                                    {
                                        ak_u32 ChildIndex = AK_PoolIndex(Movable->ChildIDs.IDs[Index]);
                                        movable* ChildMovable = Movables->Get(ChildIndex);
                                        ak_string ChildName = *GameEntityNames->Get(ChildIndex);
                                        ImGui::Text("\t%.*s", ChildName.Length, ChildName.Data);
                                    }
                                    
                                    ImGui::Text("Parents: (%d)", Movable->ParentIDs.Size);
                                    for(ak_u32 Index = 0; Index < Movable->ParentIDs.Size; Index++)
                                    {
                                        ak_u32 ParentIndex = AK_PoolIndex(Movable->ParentIDs.IDs[Index]);
                                        movable* ParentMovable = Movables->Get(ParentIndex);
                                        ak_string ParentName = *GameEntityNames->Get(ParentIndex);
                                        ImGui::Text("\t%.*s", ParentName.Length, ParentName.Data);
                                    }
                                    
                                } break;
                                
                                case ENTITY_TYPE_BUTTON:
                                {
                                    button_state* ButtonState = ButtonStates->Get(EntityIndex);
                                    ImGui::Text("Is Down %s", ButtonState->IsDown ? "true" : "false");
                                    ImGui::Text("Was Down %s", ButtonState->WasDown ? "true" : "false");
                                    ImGui::Text("Is Toggled %s", ButtonState->IsToggled ? "true" : "false");
                                } break;
                            }
                            
                        }
                        ImGui::NextColumn();
                        if(Open)
                            ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}