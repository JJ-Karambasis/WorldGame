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
    Spawner->Material = {};
    Spawner->Mass = 1.0f;
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

void DevUI_WorldIndexTool(ak_u32* WorldIndex, const ak_char** WorldIndexList, ak_u32 WorldIndexCount)
{
    AlignTextToFramePadding();
    Text("World Index");
    SameLine();
    Combo(AK_HashFunction("World Index"), "", (int*)WorldIndex, WorldIndexList, WorldIndexCount);    
}

void DevUI_MeshTool(assets* Assets, mesh_asset_id* MeshID)
{
    ak_fixed_array<const ak_char*> MeshNames = DevUI_GetAllMeshInfoNames(Assets);
    AlignTextToFramePadding();
    Text("Mesh");
    SameLine();
    Combo(AK_HashFunction("Mesh"), "", (int*)MeshID, MeshNames.Data, MeshNames.Size);
}

void DevUI_MassTool(ak_f32* Mass)
{
    AlignTextToFramePadding();
    Text("Mass");                    
    SameLine();
    DragFloat(AK_HashFunction("Mass"), "", Mass, 0.01f, 1.0f, 10000.0f);
}

void DevUI_MaterialTool(assets* Assets, material* Material)
{    
    ak_fixed_array<const ak_char*> TextureNames = DevUI_GetAllTextureInfoNames(Assets);
    
    Text("Material");    
    {
        material_diffuse* Diffuse = &Material->Diffuse;
        
        AlignTextToFramePadding();
        Text("Diffuse: "); SameLine(); 
        AlignTextToFramePadding();
        Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Diffuse Is Texture"), "", &Diffuse->IsTexture); SameLine();
        if(Diffuse->IsTexture)
        {
            if(Diffuse->DiffuseID == INVALID_TEXTURE_ID)
                Diffuse->DiffuseID = (texture_asset_id)0;
            
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Diffuse Texture"), "", (int*)&Diffuse->DiffuseID, TextureNames.Data, TextureNames.Size);
        }
        else
        {
            AlignTextToFramePadding();
            Text("Color");
            SameLine();
            ColorEdit3(AK_HashFunction("Diffuse Color"), "", (ak_f32*)&Diffuse->Diffuse, ImGuiColorEditFlags_RGB);
        }
    }
    
    {
        material_specular* Specular = &Material->Specular;
        
        AlignTextToFramePadding();
        Text("Specular: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Specular In Use"), "", &Specular->InUse); 
        
        if(Specular->InUse)
        {
            SameLine();
            AlignTextToFramePadding();
            Text("Is Texture"); SameLine(); Checkbox(AK_HashFunction("Specular Is Texture"), "", &Specular->IsTexture); SameLine();
            if(Specular->IsTexture)
            {
                if(Specular->SpecularID == INVALID_TEXTURE_ID)
                    Specular->SpecularID = (texture_asset_id)0;
                
                AlignTextToFramePadding();
                Text("Texture");
                SameLine();
                Combo(AK_HashFunction("Specular Texture"), "", (int*)&Specular->SpecularID, TextureNames.Data, TextureNames.Size);
            }
            else
            {
                AlignTextToFramePadding();
                Text("Value");
                SameLine();
                PushItemWidth(60);
                DragFloat(AK_HashFunction("Specular Color"), "", &Specular->Specular, 0.01f, 0.0f, 1.0f); SameLine();                                
                PopItemWidth();
                
                ak_v3f SpecularDisplay = AK_V3(Specular->Specular, Specular->Specular, Specular->Specular);
                ColorEdit3("", (ak_f32*)&SpecularDisplay, ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoPicker);                                                                
            }
            
            SameLine();
            Text("Shininess");
            PushItemWidth(40);
            SameLine();
            DragInt(AK_HashFunction("Specular Shininess"), "", &Specular->Shininess, 0.1f, 1, 512);
            PopItemWidth();
        }                                                
    }                    
    
    {
        material_normal* Normal = &Material->Normal;
        
        AlignTextToFramePadding();
        Text("Normal: "); SameLine();
        AlignTextToFramePadding();
        Text("In Use"); SameLine(); Checkbox(AK_HashFunction("Normal In Use"), "", &Normal->InUse);
        
        if(Normal->InUse)
        {
            if(Normal->NormalID == INVALID_TEXTURE_ID)
                Normal->NormalID = (texture_asset_id)0;
            
            SameLine();
            AlignTextToFramePadding();
            Text("Texture");
            SameLine();
            Combo(AK_HashFunction("Normal Texture"), "", (int*)&Normal->NormalID, TextureNames.Data, TextureNames.Size);
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
    Text("Radius");
    SameLine();
    PushItemWidth(ItemWidth);
    DragFloat(Hash, "", Radius, 0.01f, 0.0f, 100.0f);
    PopItemWidth();
}

void DevUI_RestitutionTool(ak_u32 Hash, ak_f32 ItemWidth, ak_f32* Restitution)
{
    Text("Restitution");
    SameLine();
    PushItemWidth(ItemWidth);
    DragFloat(Hash, "", Restitution, 0.001f, 0.0f, 1.0f);
    PopItemWidth();
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

void DevUI_EntitySpawner(game* Game, entity_spawner* Spawner)
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
            DevUI_WorldIndexTool(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
            Separator();                    
            DevUI_MeshTool(Game->Assets, &Spawner->MeshID);                    
            Separator();                    
            DevUI_MaterialTool(Game->Assets, &Spawner->Material);
            Separator();
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                if(DevUI_ValidateMaterial(&Spawner->Material))                        
                {                    
                    if(Spawner->WorldIndex == 2)
                    {
                        CreateDualStaticEntity(Game, Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle),
                                               Spawner->MeshID, Spawner->Material);                        
                    }
                    else
                    {                            
                        CreateStaticEntity(Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Scale, AK_RotQuat(Spawner->Axis, Spawner->Angle), 
                                           Spawner->MeshID, Spawner->Material);                        
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
            DevUI_WorldIndexTool(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));                    
            Separator();
            DevUI_MassTool(&Spawner->Mass);
            Separator();
            DevUI_MaterialTool(Game->Assets, &Spawner->Material);
            Separator();                    
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                if(DevUI_ValidateMaterial(&Spawner->Material))                        
                {                    
                    entity_id ID = CreateSphereRigidBody(Game, Spawner->WorldIndex, Spawner->Translation, Spawner->Radius, 
                                                         Spawner->Mass, Spawner->Restitution, Spawner->Material);                    
                }
            }
        } break;
        
        case ENTITY_TYPE_PUSHABLE:
        {
            Separator();
            DevUI_RadiusTool(AK_HashFunction("Radius Spawner"), TRANSFORM_ITEM_WIDTH, &Spawner->Radius);
            Separator();
            const ak_char* WorldIndexList[] = {"World A", "World B", "Both", "Linked"};
            DevUI_WorldIndexTool(&Spawner->WorldIndex, WorldIndexList, AK_Count(WorldIndexList));
            Separator();
            DevUI_MassTool(&Spawner->Mass);
            Separator();
            DevUI_MaterialTool(Game->Assets, &Spawner->Material);
            Separator();
            
            if(Button(AK_HashFunction("Create Entity Button"), "Create"))
            {
                if(DevUI_ValidateMaterial(&Spawner->Material))
                {
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
                }
            }
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }        
}

void DevUI_Update(dev_ui* UI, game* Game)
{    
    NewFrame();
#if SHOW_IMGUI_DEMO_WINDOW
    //IMPORTANT(EVERYONE): If you need help figuring out how to use ImGui you can always switch this to 1 and look at the imgui demo window
    //for some functionality that you are trying to create. It doesn't have everything but it's probably a good start    
    local ak_bool demo_window;
    ShowDemoWindow((bool*)&demo_window);
#endif    
    
    SetNextWindowPos(ImVec2(0, 0));
    if(Begin("Dev Tools", (bool*)&UI->DevToolsOpen, ImGuiWindowFlags_AlwaysAutoResize))
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
            
            dev_ui_transform_state* TransformState = &UI->TransformState;
            
            const char* TransformTypes[] = {"Translate", "Scale", "Rotate"};
            AlignTextToFramePadding();
            Text("Object Transform Mode");
            SameLine();
            Combo(AK_HashFunction("Object Transform Mode"), "", (int*)&TransformState->TransformMode, TransformTypes, AK_Count(TransformTypes));
            
            switch(TransformState->TransformMode)
            {
                case GIZMO_MOVEMENT_TYPE_TRANSLATE: 
                { 
                    DragFloat("GridSize", &TransformState->GridDistance, 0.1f, 0.1f, 10); 
                    AK_Clamp(TransformState->GridDistance, 0.1f, 10.0f);  
                } break;
                
                case GIZMO_MOVEMENT_TYPE_SCALE: { DragFloat("Scale Snap", &TransformState->ScaleSnap, 0.1f, 0.1f, 10.0f); } break;
                case GIZMO_MOVEMENT_TYPE_ROTATE: { DragFloat("Rotate Angle Snap", &TransformState->RotationAngleSnap, 0.1f, 0.1f, 180.0f); } break;
            }                                    
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
        if(Begin("Entity Spawner", (bool*)&UI->EntitySpawnerOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            DevUI_EntitySpawner(Game, &UI->Spawner);
        }
        End();
    }
       
    Render();
}

void DevUI_Render(graphics* Graphics, game* Game, dev_ui* UI)
{   
    UpdateRenderBuffer(&UI->UIRenderBuffer, Graphics, Graphics->RenderDim);
    
    PushRenderBufferViewportAndScissor(Graphics, UI->UIRenderBuffer);     
    PushProjection(Graphics, AK_Orthographic(0.0f, (ak_f32)UI->UIRenderBuffer->Resolution.w, 0.0f, (ak_f32)UI->UIRenderBuffer->Resolution.h, -1.0f, 1.0f));
    
    PushClearColor(Graphics, AK_Black4());    
    if(Game->RenderBuffer && UI->PlayGame)
        PushCopyToRenderBuffer(Graphics, Game->RenderBuffer, AK_V2<ak_i32>(), UI->UIRenderBuffer->Resolution);        
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