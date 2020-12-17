#include "editor.h"


#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_demo.cpp>

#include <assets.cpp>
#include <game_common_source.cpp>
#include <src/graphics_state.cpp>

#define EDITOR_ITEM_WIDTH 80.0f

dev_entity* Editor_CreateDevEntity(editor* Editor, ak_u32 WorldIndex, ak_char* Name, entity_type Type, ak_v3f Position, 
                                   ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    ak_u64 ID = Editor->DevEntities[WorldIndex].Allocate();
    dev_entity* Entity = Editor->DevEntities[WorldIndex].Get(ID);
    
    AK_CopyArray(Entity->Name, Name, MAX_OBJECT_NAME_LENGTH);
    Entity->Type = Type;
    Entity->ID = ID;
    Entity->LinkID = 0;
    Entity->Transform = AK_SQT(Position, AK_RotQuat(Axis, Angle), Scale);
    Entity->Material = Material;
    Entity->MeshID = MeshID;
    
    return Entity;
}

dual_dev_entity Editor_CreateDevEntityInBothWorlds(editor* Editor, ak_char* Name, entity_type Type, ak_v3f Position, 
                                                   ak_v3f Axis, ak_f32 Angle, ak_v3f Scale, material Material, mesh_asset_id MeshID)
{
    dual_dev_entity DualEntities;
    DualEntities.EntityA = Editor_CreateDevEntity(Editor, 0, Name, Type, Position, Axis, Angle, Scale, Material, MeshID);
    DualEntities.EntityB = Editor_CreateDevEntity(Editor, 1, Name, Type, Position, Axis, Angle, Scale, Material, MeshID);
    
    return DualEntities;
}

dev_point_light* Editor_CreateDevPointLight(editor* Editor, ak_u32 WorldIndex, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    ak_u64 ID = Editor->DevPointLights[WorldIndex].Allocate();
    dev_point_light* PointLight = Editor->DevPointLights[WorldIndex].Get(ID);
    
    AK_CopyArray(PointLight->Name, Name, MAX_OBJECT_NAME_LENGTH);
    PointLight->Light.Color = Color;
    PointLight->Light.Intensity = Intensity;
    PointLight->Light.Position = Position;
    PointLight->Light.Radius = Radius;
    
    return PointLight;
}

dual_dev_point_light Editor_CreateDevPointLightInBothWorlds(editor* Editor, ak_char* Name, ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    dual_dev_point_light DualPointLights;
    DualPointLights.PointLightA = Editor_CreateDevPointLight(Editor, 0, Name, Position, Radius, Color, Intensity);
    DualPointLights.PointLightB = Editor_CreateDevPointLight(Editor, 1, Name, Position, Radius, Color, Intensity);
    
    return DualPointLights;
}

editor* Editor_Initialize(graphics* Graphics, ImGuiContext* Context, platform* Platform)
{
    editor* Editor = (editor*)AK_Allocate(sizeof(editor));
    if(!Editor)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    Editor->Scratch = AK_CreateArena(AK_Megabyte(1));
    if(!Editor->Scratch)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return NULL;
    }
    
    ImGui::SetCurrentContext(Context);
    
    ImGuiIO* IO = &ImGui::GetIO();
    void* ImGuiFontData;
    
    ak_i32 Width, Height;        
    IO->Fonts->GetTexDataAsRGBA32((unsigned char**)&ImGuiFontData, &Width, &Height);
    
    graphics_sampler_info SamplerInfo = {};
    SamplerInfo.MinFilter = GRAPHICS_FILTER_LINEAR;
    SamplerInfo.MagFilter = GRAPHICS_FILTER_LINEAR;
    
    graphics_texture_id FontTexture = Graphics->AllocateTexture(Graphics, ImGuiFontData, (ak_u32)Width, (ak_u32)Height, GRAPHICS_TEXTURE_FORMAT_R8G8B8_ALPHA8, &SamplerInfo);        
    IO->Fonts->TexID = (ImTextureID)FontTexture;
    
    
    Editor->Cameras[0].SphericalCoordinates.radius = Editor->Cameras[1].SphericalCoordinates.radius = 5.0f;
    
    ak_temp_arena TempArena = Editor->Scratch->BeginTemp();
    
    Editor->Scratch->EndTemp(&TempArena);
    
    return Editor;
}

void Editor_ErrorText(const ak_char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
    ImGui::TextV(Format, Args);
    ImGui::PopStyleColor();
    va_end(Args);
}

const ak_char* Editor_GetEntityTypeUI(entity_type Type)
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

ak_fixed_array<const ak_char*> Editor_GetAllEntityTypesNotPlayerUI()
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_u32 Size = ENTITY_TYPE_COUNT-1;
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(Size), Size);    
    ak_u32 Counter = 0;
    
    for(ak_u32 TypeIndex = 0; TypeIndex < ENTITY_TYPE_COUNT; TypeIndex++)
    {
        if((entity_type)TypeIndex != ENTITY_TYPE_PLAYER)
            Result[Counter++] = Editor_GetEntityTypeUI((entity_type)TypeIndex);
    }
    
    return Result;
}

ak_fixed_array<const ak_char*> Editor_GetAllMeshInfoNamesUI(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(MESH_ASSET_COUNT), MESH_ASSET_COUNT);
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)    
        Result[MeshIndex] = Assets->MeshInfos[MeshIndex].Name;          
    return Result;
}

ak_fixed_array<const ak_char*> Editor_GetAllTextureInfoNamesUI(assets* Assets)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_fixed_array<const ak_char*> Result = AK_CreateArray(GlobalArena->PushArray<const ak_char*>(TEXTURE_ASSET_COUNT), TEXTURE_ASSET_COUNT);
    for(ak_u32 TextureIndex = 0; TextureIndex < TEXTURE_ASSET_COUNT; TextureIndex++)    
        Result[TextureIndex] = Assets->TextureInfos[TextureIndex].Name;          
    return Result;
}

void Editor_DragInt(ak_u32 ID, const ak_char* Label, ak_i32* Value, ak_f32 Speed, ak_i32 Min, ak_i32 Max)
{
    ImGui::PushID(ID);    
    ImGui::DragInt(Label, Value, Speed, Min, Max);    
    ImGui::PopID();
}

void Editor_DragFloat(ak_u32 ID, const ak_char* Label, ak_f32* Value, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ImGui::PushID(ID);
    ImGui::DragFloat(Label, Value, Speed, Min, Max);
    ImGui::PopID();
}

void Editor_DragFloatTool(ak_u32 Hash, const ak_char* Name, ak_f32 ItemWidth, ak_f32* Data, ak_f32 Speed, ak_f32 Min, ak_f32 Max)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text(Name);
    ImGui::SameLine();
    ImGui::PushItemWidth(ItemWidth);
    Editor_DragFloat(Hash, "", Data, Speed, Min, Max);
    ImGui::PopItemWidth();
}

void Editor_DragAngle(ak_u32 ID, const ak_char* Label, ak_f32* Radians, ak_f32 Speed, ak_f32 Min, ak_f32 Max, const ak_char* Format = "%.3f")
{
    ak_f32 Degree = AK_ToDegree(*Radians);
    ImGui::PushID(ID);
    ImGui::DragFloat(Label, &Degree, Speed, Min, Max, Format);
    ImGui::PopID();
    *Radians = AK_ToRadians(Degree);
}

ak_bool Editor_Combo(ak_u32 ID, const ak_char* Label, ak_i32* Data, const ak_char** List, ak_i32 ListCount)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::Combo(Label, Data, List, ListCount);
    ImGui::PopID();
    return Result;
}


ak_bool Editor_ColorEdit3(ak_u32 ID, const ak_char* Label, ak_f32* Data, ImGuiColorEditFlags Flags)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::ColorEdit3(Label, Data, Flags);
    ImGui::PopID();
    return Result;
}

ak_bool Editor_Checkbox(ak_u32 ID, const ak_char* Label, ak_bool* Flag)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::Checkbox(Label, (bool*)Flag);
    ImGui::PopID();
    return Result;
}

ak_bool Editor_Button(ak_u32 ID, const ak_char* ButtonText)
{
    ImGui::PushID(ID);
    ak_bool Result = ImGui::Button(ButtonText);
    ImGui::PopID();
    return Result;
}

void Editor_ClearSpawner(entity_spawner* Spawner)
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

void Editor_ResetSpawner(editor* Editor, entity_spawner* Spawner)
{
    Editor_ClearSpawner(Spawner);
    Editor->ShowEntityNameNullErrorText = false;
    Editor->ShowEntityNameErrorText = false;
}

void Editor_ClearSpawner(light_spawner* Spawner)
{
    Spawner->Translation = {};
    Spawner->Radius = 1.0f;
    Spawner->WorldIndex = 0;
    Spawner->Intensity = 1.0f;
    Spawner->Color = AK_White3();    
}

void Editor_ResetSpawner(editor* Editor, light_spawner* Spawner)
{
    Editor_ClearSpawner(&Editor->LightSpawner);
    Editor->ShowLightNameNullErrorText = false;
    Editor->ShowLightNameErrorText = false;
}

void Editor_TranslationTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    Editor_DragFloat(Hash+0, "X", &Translation->x, 0.1f, -1000.0f, 1000.0f); ImGui::SameLine();                                
    Editor_DragFloat(Hash+1, "Y", &Translation->y, 0.1f, -1000.0f, 1000.0f); ImGui::SameLine();                                
    Editor_DragFloat(Hash+2, "Z", &Translation->z, 0.1f, -1000.0f, 1000.0f);                                
    
    ImGui::PopItemWidth();
}

void Editor_TranslationTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Translation)
{
    Editor_TranslationTool("Translation", Hash, ItemWidth, Translation);                          
}

void Editor_ScaleTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    Editor_DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    Editor_DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    Editor_DragFloat(Hash+2, "Z", &Scale->z, 0.1f, 0.0f, 100.0f);                 
    
    ImGui::PopItemWidth();
}

void Editor_ScaleTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Scale)
{
    Editor_ScaleTool("Scale", Hash, ItemWidth, Scale);
}

void Editor_AngleAxisTool(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Axis, ak_f32* Angle)
{
    ImGui::Text(Label);
    
    ImGui::PushItemWidth(ItemWidth);
    
    Editor_DragFloat(Hash+0, "Axis X", &Axis->x, 0.01f, -1, 1); ImGui::SameLine();
    Editor_DragFloat(Hash+1, "Axis Y", &Axis->y, 0.01f, -1, 1); ImGui::SameLine();
    Editor_DragFloat(Hash+2, "Axis Z", &Axis->z, 0.01f, -1, 1); ImGui::SameLine();
    Editor_DragAngle(Hash+3, "Angle", Angle,   0.1f, -180.0f, 180.0f);
    
    *Axis = AK_Normalize(*Axis);                                                                        
    
    ImGui::PopItemWidth();
}

void Editor_AngleAxisTool(ak_u32 Hash, ak_f32 ItemWidth, ak_v3f* Axis, ak_f32* Angle)
{
    Editor_AngleAxisTool("Rotation", Hash, ItemWidth, Axis, Angle);
}

void Editor_ScaleTool2D(const ak_char* Label, ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    ImGui::Text(Label);
    ImGui::PushItemWidth(ItemWidth);
    
    Editor_DragFloat(Hash+0, "X", &Scale->x, 0.1f, 0.0f, 100.0f); ImGui::SameLine();                                
    Editor_DragFloat(Hash+1, "Y", &Scale->y, 0.1f, 0.0f, 100.0f);     
    
    ImGui::PopItemWidth();
}

void Editor_ScaleTool2D(ak_u32 Hash, ak_f32 ItemWidth, ak_v2f* Scale)
{
    Editor_ScaleTool2D("Scale", Hash, ItemWidth, Scale);
}

void Editor_WorldIndexTool(ak_u32 Hash, ak_u32* WorldIndex, const ak_char** WorldIndexList, ak_u32 WorldIndexCount)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("World Index");
    ImGui::SameLine();
    Editor_Combo(Hash, "", (int*)WorldIndex, WorldIndexList, WorldIndexCount);    
}

void Editor_MeshTool(assets* Assets, mesh_asset_id* MeshID)
{
    ak_fixed_array<const ak_char*> MeshNames = Editor_GetAllMeshInfoNamesUI(Assets);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Mesh");
    ImGui::SameLine();
    Editor_Combo(AK_HashFunction("Mesh"), "", (int*)MeshID, MeshNames.Data, MeshNames.Size);
}

void Editor_NameTool(ak_u32 Hash, ak_char* Name, ak_u32 MaxLength)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::PushID(Hash);
    ImGui::InputText("", Name, MaxLength);
    ImGui::PopID();
}

void Editor_Color3EditTool(ak_char* Label, ak_u32 Hash, ak_f32* Value)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text(Label);
    ImGui::SameLine();
    Editor_ColorEdit3(Hash, "", Value, ImGuiColorEditFlags_RGB);
}

void Editor_RadiusTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Radius)
{
    Editor_DragFloatTool(Hash, "Radius", ItemWidth, Radius, 0.01f, 0.0f, 100.0f);    
}

void Editor_IntensityTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Intensity)
{
    Editor_DragFloatTool(Hash, "Intensity", ItemWidth, Intensity, 0.01f, 1.0f, 100.0f);
}

void Editor_MaterialTool(assets* Assets, material_context* MaterialContext)
{    
    ak_fixed_array<const ak_char*> TextureNames = Editor_GetAllTextureInfoNamesUI(Assets);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Material");        
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Diffuse: "); ImGui::SameLine(); 
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Is Texture"); ImGui::SameLine(); Editor_Checkbox(AK_HashFunction("Diffuse Is Texture"), "", &MaterialContext->DiffuseIsTexture); ImGui::SameLine();
        if(MaterialContext->DiffuseIsTexture)
        {
            if((MaterialContext->DiffuseID == INVALID_TEXTURE_ID) || (MaterialContext->DiffuseID > TEXTURE_ASSET_COUNT))
                MaterialContext->DiffuseID = (texture_asset_id)0;
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture");
            ImGui::SameLine();
            Editor_Combo(AK_HashFunction("Diffuse Texture"), "", (int*)&MaterialContext->DiffuseID, TextureNames.Data, TextureNames.Size);
        }
        else
        {
            Editor_Color3EditTool("Color", AK_HashFunction("Diffuse Color"), MaterialContext->Diffuse.Data);
        }
    }
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Specular: "); ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("In Use"); ImGui::SameLine(); Editor_Checkbox(AK_HashFunction("Specular In Use"), "", &MaterialContext->SpecularInUse); 
        
        if(MaterialContext->SpecularInUse)
        {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Is Texture"); ImGui::SameLine(); Editor_Checkbox(AK_HashFunction("Specular Is Texture"), "", &MaterialContext->SpecularIsTexture); ImGui::SameLine();
            if(MaterialContext->SpecularIsTexture)
            {
                if((MaterialContext->SpecularID == INVALID_TEXTURE_ID) || (MaterialContext->SpecularID > TEXTURE_ASSET_COUNT))
                    MaterialContext->SpecularID = (texture_asset_id)0;
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Texture");
                ImGui::SameLine();
                Editor_Combo(AK_HashFunction("Specular Texture"), "", (int*)&MaterialContext->SpecularID, TextureNames.Data, TextureNames.Size);
            }
            else
            {                
                Editor_DragFloatTool(AK_HashFunction("Specular Color"), "Value", 60, &MaterialContext->Specular, 0.01f, 0.0f, 1.0f); ImGui::SameLine();                                
                
                ak_v3f SpecularDisplay = AK_V3(MaterialContext->Specular, MaterialContext->Specular, MaterialContext->Specular);
                ImGui::ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
            }
            
            ImGui::SameLine();
            ImGui::Text("Shininess");
            ImGui::PushItemWidth(40);
            ImGui::SameLine();
            Editor_DragInt(AK_HashFunction("Specular Shininess"), "", &MaterialContext->Shininess, 0.1f, 1, 512);
            ImGui::PopItemWidth();
        }                                                
    }                    
    
    {        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Normal: "); ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("In Use"); ImGui::SameLine(); Editor_Checkbox(AK_HashFunction("Normal In Use"), "", &MaterialContext->NormalInUse);
        
        if(MaterialContext->NormalInUse)
        {
            if((MaterialContext->NormalID == INVALID_TEXTURE_ID) || (MaterialContext->NormalID > TEXTURE_ASSET_COUNT))
                MaterialContext->NormalID = (texture_asset_id)0;
            
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture");
            ImGui::SameLine();
            Editor_Combo(AK_HashFunction("Normal Texture"), "", (int*)&MaterialContext->NormalID, TextureNames.Data, TextureNames.Size);
        }
    }    
}

material_context Editor_ContextFromMaterial(material* Material)
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

material Editor_MaterialFromContext(material_context* MaterialContext)
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

void Editor_EntitySpawner(editor* Editor, entity_spawner* Spawner, assets* Assets)
{
    if(!Spawner->Init)
    {
        Spawner->Init = true;
        Editor_ClearSpawner(Spawner);                
    }
    
    ak_fixed_array<const ak_char*> EntityTypes = Editor_GetAllEntityTypesNotPlayerUI();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Entity Type");            
    ImGui::SameLine();            
    
    entity_type PrevType = Spawner->EntityType;
    
    ak_i32 SpawnType = Spawner->EntityType-1;            
    Editor_Combo(AK_HashFunction("Entity Type"), "", (int*)&SpawnType, EntityTypes.Data, EntityTypes.Size);            
    entity_type Type = (entity_type)(SpawnType+1);
    
    if(PrevType != Type)
    {
        Editor_ClearSpawner(Spawner);
        Spawner->EntityType = Type;
    }
    
    switch(Type)
    {
        case ENTITY_TYPE_STATIC:
        {
            ImGui::Separator();
            Editor_NameTool(AK_HashFunction("Name Spawner"), Spawner->Name, MAX_OBJECT_NAME_LENGTH);
            ImGui::Separator();
            Editor_TranslationTool(AK_HashFunction("Translation Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Translation);
            ImGui::Separator();
            Editor_ScaleTool(AK_HashFunction("Scale Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Scale);
            ImGui::Separator();
            Editor_AngleAxisTool(AK_HashFunction("Rotation Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Axis, &Spawner->Angle);
            ImGui::Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
            Editor_WorldIndexTool(AK_HashFunction("Entity World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
            ImGui::Separator();
            Editor_MeshTool(Assets, &Spawner->MeshID);
            ImGui::Separator();
            Editor_MaterialTool(Assets, &Spawner->MaterialContext);
            ImGui::Separator();
            
            if(Editor_Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                if(AK_StringIsNullOrEmpty(AK_CreateString(Spawner->Name)))
                {
                    Editor->ShowEntityNameNullErrorText = true;
                }
                else
                {
                    Editor->ShowEntityNameNullErrorText = false;
                    material Material = Editor_MaterialFromContext(&Spawner->MaterialContext);
                    if(Spawner->WorldIndex == 2)
                    {
                        if(Editor->EntityNameCollisionMap[0].Find(Spawner->Name) ||
                           Editor->EntityNameCollisionMap[1].Find(Spawner->Name))
                        {
                            Editor->ShowEntityNameErrorText = true;
                        }
                        else
                        {
                            Editor->ShowEntityNameErrorText = false;
                        }
                    }
                    else
                    {
                        if(Editor->EntityNameCollisionMap[Spawner->WorldIndex].Find(Spawner->Name))
                        {
                            Editor->ShowEntityNameErrorText = true;
                        }
                        else
                        {
                            Editor->ShowEntityNameErrorText = false;
                            
                            Editor_CreateDevEntity(Editor, Spawner->WorldIndex, Spawner->Name, Spawner->EntityType, Spawner->Translation, Spawner->Axis, Spawner->Angle, 
                                                   Spawner->Scale, Material, Spawner->MeshID);
                            
                            Editor->EntityNameCollisionMap[Spawner->WorldIndex].Insert(Spawner->Name, true);
                        }
                    }
                }
            }
        } break;
    } 
    
    if(Editor->ShowEntityNameErrorText)
    {
        ImGui::SameLine();
        Editor_ErrorText("Error: Entity with name already exists");
    }
    
    if(Editor->ShowEntityNameNullErrorText)
    {
        ImGui::SameLine();
        Editor_ErrorText("Error: Name must be supplied");
    }
}

void Editor_LightSpawner(editor* Editor, light_spawner* Spawner)
{
    if(!Spawner->Init)
    {
        Editor_ClearSpawner(Spawner);
        Spawner->Init = true;
    }
    
    Editor_NameTool(AK_HashFunction("Light Name Spawner"), Spawner->Name, MAX_OBJECT_NAME_LENGTH);
    ImGui::Separator();
    Editor_TranslationTool(AK_HashFunction("Translation Light Spawner"), EDITOR_ITEM_WIDTH, &Spawner->Translation);
    ImGui::Separator();
    const ak_char* WorldIndexList[] = {"World A", "World B", "Both"};
    Editor_WorldIndexTool(AK_HashFunction("Light World Index"), &Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
    ImGui::Separator();
    Editor_RadiusTool(AK_HashFunction("Light Radius"), EDITOR_ITEM_WIDTH, &Spawner->Radius);
    ImGui::Separator();
    Editor_IntensityTool(AK_HashFunction("Light Intensity"), EDITOR_ITEM_WIDTH, &Spawner->Intensity);
    ImGui::Separator();
    Editor_Color3EditTool("Light Color", AK_HashFunction("Light Color"), Spawner->Color.Data);
    ImGui::Separator();
    
    if(Editor_Button(AK_HashFunction("Create Point Light Button"), "Create"))
    {
        if(AK_StringIsNullOrEmpty(AK_CreateString(Spawner->Name)))
        {
            Editor->ShowLightNameNullErrorText = true;
        }
        else
        {
            Editor->ShowLightNameNullErrorText = false;
            
            if(Spawner->WorldIndex == 2)
            {
                if(Editor->LightNameCollisionMap[0].Find(Spawner->Name) ||
                   Editor->LightNameCollisionMap[1].Find(Spawner->Name))
                {
                    Editor->ShowLightNameErrorText = true;
                }
                else
                {
                    Editor->ShowLightNameErrorText = false;
                }
            }
            else
            {
                if(Editor->LightNameCollisionMap[Spawner->WorldIndex].Find(Spawner->Name))
                {
                    Editor->ShowLightNameErrorText = true;
                }
                else
                {
                    Editor->ShowLightNameErrorText = false;
                    
                    Editor_CreateDevPointLight(Editor, Spawner->WorldIndex, Spawner->Name, Spawner->Translation, Spawner->Radius, Spawner->Color, Spawner->Intensity);
                    
                    Editor->LightNameCollisionMap[Spawner->WorldIndex].Insert(Spawner->Name, true);
                }
            }
        }
    }
    
    if(Editor->ShowLightNameErrorText)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::Text("Error: Light with name already exists");
        ImGui::PopStyleColor();
    }
    
    if(Editor->ShowLightNameNullErrorText)
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ImGui::Text("Error: Name must be supplied");
        ImGui::PopStyleColor();
    }
}

ak_bool Editor_Update(editor* Editor, game* Game, dev_platform* DevPlatform, ak_f32 dt)
{
    if(!DevPlatform->Update(Editor, Game, dt))
        return false;
    
    dev_input* DevInput = &Editor->Input;
    
    graphics_camera* DevCamera = &Editor->Cameras[Editor->CurrentWorldIndex];
    ak_v2i MouseDelta = DevInput->MouseCoordinates - DevInput->LastMouseCoordinates;
    
    ak_v3f* SphericalCoordinates = &DevCamera->SphericalCoordinates;                
    
    ak_f32 Roll = 0;
    ak_f32 Pitch = 0;        
    
    ak_v2f PanDelta = AK_V2<ak_f32>();
    ak_f32 Scroll = 0;
    
#if 0 
    if(IsDown(DevInput->Ctrl))
    {            
        if(IsDown(DevInput->S)) DevContext_SaveWorld(Context, &Context->LoadedWorld, false);            
    }
    
    if(IsDown(DevInput->Alt))
    {
        if(IsDown(DevInput->L)) DevContext_LoadWorld(Context, &Context->LoadedWorld);
        if(IsDown(DevInput->S)) DevContext_SaveWorld(Context, &Context->LoadedWorld, true);
    }
#endif
    
    if(IsDown(DevInput->Alt))
    {
        if(IsDown(DevInput->LMB))
        {
            SphericalCoordinates->inclination += MouseDelta.y*1e-3f;
            SphericalCoordinates->azimuth += MouseDelta.x*1e-3f;
            
            ak_f32 InclindationDegree = AK_ToDegree(SphericalCoordinates->inclination);
            if(InclindationDegree < -180.0f)
            {
                ak_f32 Diff = InclindationDegree + 180.0f;
                InclindationDegree = 180.0f - Diff;
                InclindationDegree = AK_Min(180.0f, InclindationDegree);
                SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
            }
            else if(InclindationDegree > 180.0f)
            {
                ak_f32 Diff = InclindationDegree - 180.0f;
                InclindationDegree = -180.0f + Diff;
                InclindationDegree = AK_Min(-180.0f, InclindationDegree);
                SphericalCoordinates->inclination = AK_ToRadians(InclindationDegree);
            }            
        }
        
        if(IsDown(DevInput->MMB))        
            PanDelta += AK_V2f(MouseDelta)*1e-3f;        
        
        if(AK_Abs(DevInput->Scroll) > 0)        
        {
            SphericalCoordinates->radius -= DevInput->Scroll*0.5f;                    
        }
    }    
    
    if(SphericalCoordinates->radius < 1e-3f)
        SphericalCoordinates->radius = 1e-3f;
    
    view_settings ViewSettings = GetViewSettings(DevCamera);    
    DevCamera->Target += (ViewSettings.Orientation.XAxis*PanDelta.x - ViewSettings.Orientation.YAxis*PanDelta.y);        
    
    DevInput->LastMouseCoordinates = DevInput->MouseCoordinates;
    DevInput->MouseCoordinates = {};
    DevInput->Scroll = 0.0f;
    
    UpdateButtons(DevInput->Buttons, AK_Count(DevInput->Buttons));
    
    return true;
}

void Editor_Render(editor* Editor, graphics* Graphics, platform* Platform, assets* Assets)
{
    ak_v2i Resolution = Platform->GetResolution();
    UpdateRenderBuffer(Graphics, &Editor->RenderBuffer, Resolution);
    
    view_settings ViewSettings = GetViewSettings(&Editor->Cameras[Editor->CurrentWorldIndex]);
    
    graphics_light_buffer LightBuffer = {};
    
    AK_ForEach(DevLight, &Editor->DevPointLights[Editor->CurrentWorldIndex])
    {
        AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
        LightBuffer.PointLights[LightBuffer.PointLightCount++] = DevLight->Light;
    }
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, Editor->RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    PushLightBuffer(Graphics, &LightBuffer);
    AK_ForEach(DevEntity, &Editor->DevEntities[Editor->CurrentWorldIndex])
    {
        graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
        
        PushMaterial(Graphics, Material);
        PushDrawMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
    }
    
    
    PushProjection(Graphics, AK_Orthographic(0.0f, (ak_f32)Resolution.w, 0.0f, (ak_f32)Resolution.h, -1.0f, 1.0f));
    PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);        
    PushDepth(Graphics, false);        
    
    ak_u32 IndexSize = sizeof(ImDrawIdx);    
    ImDrawData* DrawData = ImGui::GetDrawData();        
    
    for(ak_i32 CmdListIndex = 0; CmdListIndex < DrawData->CmdListsCount; CmdListIndex++)
    {
        if(Editor->ImGuiMeshes.Size == (ak_u32)CmdListIndex)
            Editor->ImGuiMeshes.Add(Graphics->AllocateDynamicMesh(Graphics, GRAPHICS_VERTEX_FORMAT_P2_UV_C, GRAPHICS_INDEX_FORMAT_16_BIT));
        
        ImDrawList* CmdList = DrawData->CmdLists[CmdListIndex];        
        Graphics->StreamMeshData(Graphics, Editor->ImGuiMeshes[CmdListIndex], 
                                 CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size*sizeof(ak_vertex_p2_uv_c), 
                                 CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size*IndexSize);                
        
        for(ak_i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
        {
            ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
            AK_Assert(!Cmd->UserCallback, "ImGui User callback is not supported");
            
            ImVec4 ClipRect = Cmd->ClipRect;
            if((ClipRect.x < Resolution.w) && (ClipRect.y < Resolution.h) && (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
            {                
                ak_i32 X = (ak_i32)ClipRect.x;
                ak_i32 Y = (ak_i32)(Resolution.h-ClipRect.w);
                ak_i32 Width = (ak_i32)(ClipRect.z - ClipRect.x);
                ak_i32 Height = (ak_i32)(ClipRect.w - ClipRect.y);
                
                PushScissor(Graphics, X, Y, Width, Height);
                
                graphics_texture_id TextureID = (graphics_texture_id)Cmd->TextureId;
                PushDrawImGuiUI(Graphics, Editor->ImGuiMeshes[CmdListIndex], TextureID, Cmd->ElemCount, Cmd->IdxOffset, Cmd->VtxOffset);                             
            }
        }
    }
    
    PushBlend(Graphics, false);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    PushDepth(Graphics, true);            
    PushSRGBRenderBufferWrites(Graphics, false);
    
    PushScissor(Graphics, 0, 0, Resolution.w, Resolution.h);
    PushCopyToOutput(Graphics, Editor->RenderBuffer, AK_V2(0, 0), Resolution);
    
    Platform->ExecuteRenderCommands(Graphics);
}

template <typename type> 
void Editor_WriteStruct(ak_string_builder* Builder, ak_pool<type>* Pool, const ak_char* Name)
{
    Builder->WriteLine("struct %s", Name);
    Builder->WriteLine("{");
    AK_ForEach(Entry, Pool)
        Builder->WriteLine("\tak_u64 %s;", Entry->Name);
    Builder->WriteLine("};");
}

template <typename type>
void Editor_InsertIndices(ak_pool<type>* Pool, ak_hash_map<ak_char*, ak_u32>* HashMap)
{
    ak_u32 Index = 0;
    AK_ForEach(Entry, Pool)
        HashMap->Insert(Entry->Name, Index++);
}

ak_u64 GetEntryID(game* Game, dev_entity* DevEntity, ak_u32 WorldIndex)
{
    ak_u64 Result = 0;
    if(DevEntity->Type == ENTITY_TYPE_PLAYER)
    {
        Result= CreatePlayerEntity(Game, WorldIndex, DevEntity->Transform.Translation, DevEntity->Material)->ID;
    }
    else
    {
        Result = CreateEntity(Game, WorldIndex, DevEntity->Type, DevEntity->Transform.Translation, DevEntity->Transform.Scale, 
                              DevEntity->Transform.Orientation, DevEntity->MeshID, DevEntity->Material)->ID;
    }
    return Result;
}

ak_u64 GetEntryID(game* Game, dev_point_light* DevPointLight, ak_u32 WorldIndex)
{
    ak_u64 Result = CreatePointLight(Game, WorldIndex, DevPointLight->Light.Position, DevPointLight->Light.Radius, 
                                     DevPointLight->Light.Color, DevPointLight->Light.Intensity)->ID;
    return Result;
}

template <typename type>
ak_u32 Editor_BuildIDs(game* Game, ak_u64* WorldIDs, ak_pool<type>* Pool, ak_hash_map<ak_char*, ak_u32>* HashMap, ak_u32 WorldIndex)
{
    AK_ForEach(Entry, Pool)
    {
        ak_u64 EntityID = GetEntryID(Game, Entry, WorldIndex);
        ak_u32* Index = HashMap->Find(Entry->Name);
        if(Index)
            WorldIDs[*Index] = EntityID;
    }
    return HashMap->Size;
}

ak_bool Editor_BuildWorld(editor* Editor, dev_platform* DevPlatform)
{
    ak_string_builder HeaderFile = {};
    
    HeaderFile.WriteLine("#ifndef GENERATED_H");
    HeaderFile.WriteLine("#define GENERATED_H");
    
    Editor_WriteStruct(&HeaderFile, &Editor->DevEntities[0], "entities_a");
    Editor_WriteStruct(&HeaderFile, &Editor->DevEntities[1], "entities_b");
    Editor_WriteStruct(&HeaderFile, &Editor->DevPointLights[0], "point_lights_a");
    Editor_WriteStruct(&HeaderFile, &Editor->DevPointLights[1], "point_lights_b");
    
    Editor_InsertIndices(&Editor->DevEntities[0], &Editor->EntityIndices[0]);
    Editor_InsertIndices(&Editor->DevEntities[1], &Editor->EntityIndices[1]);
    Editor_InsertIndices(&Editor->DevPointLights[0], &Editor->PointLightIndices[0]);
    Editor_InsertIndices(&Editor->DevPointLights[1], &Editor->PointLightIndices[1]);
    
#if 0 
    ak_u32 Index = 0;
    AK_ForEach(DevEntity, &Editor->DevEntities[0])
        Editor->EntityIndices[0].Insert(DevEntity->Name, Index++);
#endif
    
    HeaderFile.WriteLine("#endif");
    
    ak_string HeaderFileString = HeaderFile.PushString(Editor->Scratch);
    ak_string HeaderPath = AK_StringConcat(Editor->WorldContext.WorldPath, "generated.h", Editor->Scratch);
    ak_string RenameHeaderPath = AK_StringConcat(Editor->WorldContext.WorldPath, "generated_rename.h", Editor->Scratch);
    
    if(AK_FileExists(HeaderPath))
        AK_FileRename(HeaderPath, RenameHeaderPath);
    
    AK_WriteEntireFile(HeaderPath, HeaderFileString.Data, HeaderFileString.Length);
    
    if(!DevPlatform->BuildWorld(Editor->WorldContext.WorldName))
    {
        AK_FileRemove(HeaderPath);
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRename(RenameHeaderPath, HeaderPath);
        return false;
    }
    else
    {
        if(AK_FileExists(RenameHeaderPath))
            AK_FileRemove(RenameHeaderPath);
    }
    
    //TODO(JJ): Task to actual build the world file (make it async if it takes awhile)
    return true;
}

ak_bool Editor_CreateNewWorld(editor* Editor, ak_string WorldName, dev_platform* DevPlatform)
{
    if(AK_StringIsNullOrEmpty(WorldName))
    {
        Editor->ShowEmptyWorldNameErrorText = true;
        return false;
    }
    Editor->ShowEmptyWorldNameErrorText = false;
    
    ak_array<ak_string> Files = AK_GetAllFilesInDirectory(WORLDS_PATH, Editor->Scratch);
    AK_ForEach(File, &Files)
    {
        if(AK_DirectoryExists(File->Data))
        {
            ak_string DirectoryPathName = AK_GetFilename(*File);
            if(AK_StringEquals(DirectoryPathName, WorldName))
            {
                Editor->ShowDuplicateWorldErrorText = true;
                AK_DeleteArray(&Files);
                return false;
            }
        }
    }
    Editor->ShowDuplicateWorldErrorText = false;
    
    ak_string_builder WorldHeader = {};
    ak_string_builder WorldSource = {};
    
    ak_string WorldNameUpper = AK_ToUpper(WorldName, Editor->Scratch);
    ak_string WorldNameLower = AK_ToLower(WorldName, Editor->Scratch);
    ak_string HeaderGuard = AK_StringConcat(WorldNameUpper, "_H", Editor->Scratch);
    
    WorldHeader.WriteLine("#ifndef %.*s", HeaderGuard.Length, HeaderGuard.Data);
    WorldHeader.WriteLine("#define %.*s", HeaderGuard.Length, HeaderGuard.Data);
    WorldHeader.WriteLine("#include <game.h>");
    WorldHeader.WriteLine("#include \"generated.h\"");
    WorldHeader.NewLine();
    WorldHeader.WriteLine("struct %.*s : public world", WorldNameLower.Length, WorldNameLower.Data);
    WorldHeader.WriteLine("{");
    WorldHeader.WriteLine("\tentities_a WorldEntitiesA;");
    WorldHeader.WriteLine("\tentities_b WorldEntitiesB;");
    WorldHeader.WriteLine("\tpoint_lights_a PointLightsA;");
    WorldHeader.WriteLine("\tpoint_lights_b PointLightsB;");
    WorldHeader.WriteLine("};");
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_STARTUP(%.*s_Startup);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_UPDATE(%.*s_Update);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("extern \"C\" AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown);", WorldName.Length, WorldName.Data);
    WorldHeader.WriteLine("#endif");
    
    WorldSource.WriteLine("#include \"%.*s.h\"", WorldName.Length, WorldName.Data);
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_STARTUP(%.*s_Startup)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("\t%.*s* World = (%.*s*)AK_Allocate(sizeof(%.*s));", WorldNameLower.Length, WorldNameLower.Data, WorldNameLower.Length, 
                          WorldNameLower.Data, WorldNameLower.Length, WorldNameLower.Data);
    //TODO(JJ): Generate some error handling when we have some proper error handling :)
    WorldSource.WriteLine("\tWorld->Update = %.*s_Update;", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("\tWorld->Shutdown = %.*s_Shutdown;", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("\tGame->World = World;");
    WorldSource.WriteLine("\treturn true;");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_UPDATE(%.*s_Update)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    WorldSource.WriteLine("extern \"C\"");
    WorldSource.WriteLine("AK_EXPORT WORLD_SHUTDOWN(%.*s_Shutdown)", WorldName.Length, WorldName.Data);
    WorldSource.WriteLine("{");
    WorldSource.WriteLine("\tAK_DeletePool(&Game->World->EntityStorage[0]);");
    WorldSource.WriteLine("\tAK_DeletePool(&Game->World->EntityStorage[1]);");
    WorldSource.WriteLine("\tAK_DeleteArray(&Game->World->OldTransforms[0]);");
    WorldSource.WriteLine("\tAK_DeleteArray(&Game->World->OldTransforms[1]);");
    WorldSource.WriteLine("\tAK_DeleteArray(&Game->World->PhysicObjects[0]);");
    WorldSource.WriteLine("\tAK_DeleteArray(&Game->World->PhysicObjects[1]);");
    WorldSource.WriteLine("\tAK_Free(Game->World);");
    WorldSource.WriteLine("\tGame->World = NULL;");
    WorldSource.WriteLine("}");
    WorldSource.NewLine();
    
    //WorldSource.WriteLine("#include \"generated.cpp\"");
    WorldSource.WriteLine("#define AK_COMMON_IMPLEMENTATION");
    WorldSource.WriteLine("#include <ak_common.h>");
    
    material PlayerMaterial = {CreateDiffuse(AK_Blue3()), InvalidNormal(), CreateSpecular(0.5f, 8)};
    Editor_CreateDevEntityInBothWorlds(Editor, "Player", ENTITY_TYPE_PLAYER, AK_V3(0.0f, 0.0f, 0.0f), AK_XAxis(), 0.0f, AK_V3(1.0f, 1.0f, 1.0f), PlayerMaterial, MESH_ASSET_ID_PLAYER);
    
    material FloorMaterial = { CreateDiffuse(AK_White3()) };
    Editor_CreateDevEntityInBothWorlds(Editor, "Default_Floor", ENTITY_TYPE_STATIC, AK_V3(0.0f, 0.0f, -1.0f), AK_XAxis(), 0.0f, AK_V3(10.0f, 10.0f, 1.0f), FloorMaterial, MESH_ASSET_ID_BOX);
    
    Editor_CreateDevPointLightInBothWorlds(Editor, "Default_Light", AK_V3(0.0f, 0.0f, 10.0f), 20.0f, AK_White3(), 1.0f);
    
    ak_string NewWorldDirectoryPath = AK_StringConcat(WORLDS_PATH, WorldName, Editor->Scratch);
    NewWorldDirectoryPath = AK_StringConcat(NewWorldDirectoryPath, AK_OS_PATH_DELIMITER, Editor->Scratch);
    
    ak_string WorldHeaderString = WorldHeader.PushString(Editor->Scratch);
    ak_string WorldSourceString = WorldSource.PushString(Editor->Scratch);
    
    ak_string WorldHeaderPath = AK_FormatString(Editor->Scratch, "%.*s%.*s.h", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    ak_string WorldSourcePath = AK_FormatString(Editor->Scratch, "%.*s%.*s.cpp", NewWorldDirectoryPath.Length, NewWorldDirectoryPath.Data, WorldName.Length, WorldName.Data);
    
    if(!AK_DirectoryExists(NewWorldDirectoryPath))
        AK_CreateDirectory(NewWorldDirectoryPath);
    
    AK_WriteEntireFile(WorldHeaderPath, WorldHeaderString.Data, WorldHeaderString.Length);
    AK_WriteEntireFile(WorldSourcePath, WorldSourceString.Data, WorldSourceString.Length);
    
    Editor->WorldContext.WorldName = AK_PushString(WorldName);
    Editor->WorldContext.WorldPath = AK_PushString(NewWorldDirectoryPath);
    
    ak_bool Result = Editor_BuildWorld(Editor, DevPlatform);
    if(!Result)
    {
        AK_FileRemove(WorldHeaderPath);
        AK_FileRemove(WorldSourcePath);
        AK_DirectoryRemove(NewWorldDirectoryPath);
        AK_Assert(Result, "Game build is broken");
        //TODO(JJ): Should we return false in here?
    }
    
    WorldHeader.ReleaseMemory();
    WorldSource.ReleaseMemory();
    
    return true;
}

global editor_create_new_world_modal* Editor_CreateNewWorldModal;

EDITOR_CREATE_NEW_WORLD_MODAL(CreateNewWorldModal)
{
    if(!ImGui::IsPopupOpen("New World"))
        ImGui::OpenPopup("New World");
    
    
    if(ImGui::BeginPopupModal("New World", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        local ak_char WorldNameData[MAX_WORLD_NAME] = {};
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("World Name");
        ImGui::InputText("", WorldNameData, MAX_WORLD_NAME, ImGuiInputTextFlags_CharsNoBlank);
        
        ak_string WorldName = AK_CreateString(WorldNameData);
        
        if(ImGui::Button("Create"))
        {
            if(Editor_CreateNewWorld(Editor, WorldName, DevPlatform))
            {
                Editor_CreateNewWorldModal = CreateNewWorldModal_Stub;
                ImGui::CloseCurrentPopup();
            }
        }
        
        if(HasCloseButton)
        {
            ImGui::SameLine();
            if(ImGui::Button("Close"))
            {
                Editor_CreateNewWorldModal = CreateNewWorldModal_Stub;
                ImGui::CloseCurrentPopup();
                Editor->ShowDuplicateWorldErrorText = false;
            }
        }
        
        if(Editor->ShowDuplicateWorldErrorText)
            Editor_ErrorText("World with name '%.*s' already exists, cannot create world without deleting already existing world or renaming new one", 
                             WorldName.Length, WorldName.Data); 
        
        if(Editor->ShowEmptyWorldNameErrorText)
            Editor_ErrorText("Please give the new world a name, it cannot be an empty name");
        
        ImGui::EndPopup();
    }
}

ak_array<ak_string> Editor_GetAllWorldNames(editor* Editor)
{
    return {};
}

void Editor_LoadWorldModal(editor* Editor, dev_platform* DevPlatform, ak_bool HasCloseButton=true)
{
    if(ImGui::IsPopupOpen("Load World"))
    {
        if(ImGui::BeginPopupModal("Load World", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ak_array<ak_string> Worlds = Editor_GetAllWorldNames(Editor);
            AK_Assert(!(!HasCloseButton && (Worlds.Size == 0)), "Invalid load world modal state");
            AK_NotImplemented();
        }
    }
}

extern "C" 
AK_EXPORT EDITOR_RUN(Editor_Run)
{
    editor* Editor = Editor_Initialize(Graphics, Context, Platform);
    if(!Editor)
        return -1;
    
    assets* Assets = InitAssets(Platform->AssetPath);
    if(!Assets)
        return -1;
    
    ak_bool Initialized = false;
    
    game* Game = NULL;
    
    ak_f32 dtFixed = 1.0f/60.0f;
    ak_f32 dt = dtFixed;
    
    game_loop_accum GameLoopAccum = {};
    for(;;)
    { 
        ak_high_res_clock Start = AK_WallClock();
        ak_temp_arena TempArena = Editor->Scratch->BeginTemp();
        
        if(!Editor_Update(Editor, Game, DevPlatform, dt))
            return 0;
        ImGui::NewFrame();
        
        if(!Initialized)
        {
            ak_array<ak_string> Worlds = Editor_GetAllWorldNames(Editor);
            if(Worlds.Size == 0)
            {
                Editor_CreateNewWorldModal = CreateNewWorldModal;
            }
            else
            {
                ImGui::OpenPopup("Load World");
            }
            AK_DeleteArray(&Worlds);
            Initialized = true;
        }
        
        if(!Game)
        {
            ak_v2i Resolution = Platform->GetResolution();
            
            Editor_CreateNewWorldModal(Editor, DevPlatform, Editor->ShowCloseButtonInModals);
            Editor_LoadWorldModal(Editor, DevPlatform, Editor->ShowCloseButtonInModals);
            
            ak_f32 MenuHeight = 0;
            if(ImGui::BeginMainMenuBar())
            {
                if(ImGui::BeginMenu("Menu"))
                {
                    if(ImGui::MenuItem("Save World", "CTRL-S")) { Editor_BuildWorld(Editor, DevPlatform); }
                    if(ImGui::MenuItem("Load World", "ALT-L")) { ImGui::OpenPopup("Load World"); Editor->ShowCloseButtonInModals = true; }
                    if(ImGui::MenuItem("New World", "ALT-N")) { Editor_CreateNewWorldModal = CreateNewWorldModal; Editor->ShowCloseButtonInModals = true; }
                    //if(ImGui::MenuItem("Delete World", "ALT-D")) { ImGui::OpenPopup("Delete World"); }
                    
                    ImGui::EndMenu();
                }
                
                ImGui::Text("Loaded World: %.*s", Editor->WorldContext.WorldName.Length, Editor->WorldContext.WorldName.Data);
                
                MenuHeight = ImGui::GetWindowHeight();
                ImGui::EndMainMenuBar();
            }
            
            ImGui::SetNextWindowPos(ImVec2(0, MenuHeight));
            if(ImGui::Begin("Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(Editor_Button(AK_HashFunction("Play Game Button"), "Play"))
                {
                    
                    game_startup* GameStartup = Platform->LoadGameCode();
                    world_startup* WorldStartup = Platform->LoadWorldCode(Editor->WorldContext.WorldName);
                    if(!GameStartup || !WorldStartup)
                    {
                        AK_InvalidCode();
                        return -1;
                    }
                    
                    Game = GameStartup(Graphics, Assets, dtFixed);
                    if(!Game)
                        return -1;
                    
                    if(!WorldStartup(Game))
                        return -1;
                    
                    ak_u64* WorldIDs = (ak_u64*)(Game->World+1);
                    WorldIDs += Editor_BuildIDs(Game, WorldIDs, &Editor->DevEntities[0], &Editor->EntityIndices[0], 0);
                    WorldIDs += Editor_BuildIDs(Game, WorldIDs, &Editor->DevEntities[1], &Editor->EntityIndices[1], 1);
                    WorldIDs += Editor_BuildIDs(Game, WorldIDs, &Editor->DevPointLights[0], &Editor->PointLightIndices[0], 0);
                    WorldIDs += Editor_BuildIDs(Game, WorldIDs, &Editor->DevPointLights[1], &Editor->PointLightIndices[1], 1);
                    
                    GameLoopAccum = InitGameLoopAccum(Game->dtFixed);
                }
            }
            
            ak_f32 OptionHeight = ImGui::GetWindowHeight();
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2(0, MenuHeight+OptionHeight));
            if(ImGui::Begin("Spawners", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(ImGui::BeginTabBar("Spawner Tabs"))
                {
                    if(ImGui::BeginTabItem("Entity"))
                    {
                        Editor_ResetSpawner(Editor, &Editor->LightSpawner);
                        Editor_EntitySpawner(Editor, &Editor->EntitySpawner, Assets);
                        ImGui::EndTabItem();
                    }
                    
                    if(ImGui::BeginTabItem("Light"))
                    {
                        Editor_ResetSpawner(Editor, &Editor->EntitySpawner);
                        Editor_LightSpawner(Editor, &Editor->LightSpawner);
                        ImGui::EndTabItem();
                    }
                }
                
                ImGui::EndTabBar();
            }
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2((ak_f32)Resolution.x-Editor->ListerWidth, 0));
            if(ImGui::Begin("Lister", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(ImGui::BeginTabBar("Worlds"))
                {
                    ak_u32 WorldIndex = Editor->CurrentWorldIndex;
                    for(ak_u32 World = 0; World < 2; World++)
                    {
                        if(ImGui::BeginTabItem(AK_FormatString(Editor->Scratch, "World %d", WorldIndex).Data))
                        {
                            if(ImGui::TreeNode("Entities"))
                            {
                                AK_ForEach(DevEntity, &Editor->DevEntities[WorldIndex])
                                {
                                    ImGui::Selectable(DevEntity->Name);
                                }
                                ImGui::TreePop();
                            }
                            
                            if(ImGui::TreeNode("Lights"))
                            {
                                AK_ForEach(DevLight, &Editor->DevPointLights[WorldIndex])
                                {
                                    ImGui::Selectable(DevLight->Name);
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
            Editor->ListerWidth = ImGui::GetWindowWidth();
            ImGui::End();
        }
        else
        {
            GameLoopAccum.IncrementAccum();
            while(GameLoopAccum.ShouldUpdate())
            {
                ak_temp_arena GameTemp = Game->Scratch->BeginTemp();
                
                Game->Update(Game);
                
                GameLoopAccum.DecrementAccum();
                Game->Scratch->EndTemp(&GameTemp);
            }
            
            UpdateButtons(Game->Input.Buttons, AK_Count(Game->Input.Buttons));
            
            ak_v2i Resolution = Platform->GetResolution();
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            if(ImGui::Begin("Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(Editor_Button(AK_HashFunction("Stop Game Button"), "Stop"))
                {
                    Game->Shutdown(Game);
                    Game = NULL;
                }
            }
            ImGui::End(); 
        }
        
        ImGui::Render();
        
        Editor_Render(Editor, Graphics, Platform, Assets);
        
        Editor->Scratch->EndTemp(&TempArena);
        
        dt = (ak_f32)AK_GetElapsedTime(AK_WallClock(), Start);
    }
}

#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>