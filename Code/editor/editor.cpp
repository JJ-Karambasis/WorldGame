#include "editor.h"

#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_demo.cpp>

#include <assets.cpp>
#include <game_common_source.cpp>
#include <src/graphics_state.cpp>

#define GetEditorViewSettings(Editor, Resolution, WorldIndex) ((Editor->UI.ViewModeType == VIEW_MODE_TYPE_PERSPECTIVE) ? GetViewSettings(&Editor->Cameras[WorldIndex], Resolution) : GetViewSettings(&Editor->OrthoCameras[WorldIndex][Editor->UI.ViewModeType-1]))

global const ak_char* Global_RenderModeStrings[] = 
{
    "Lit", 
    "Unlit", 
    "Wireframe", 
    "Lit Wireframe"
};

global const ak_char* Global_ViewModeStrings[] = 
{
    "Perspective", 
    "Top (Ortho)", 
    "Bottom (Ortho)", 
    "Left (Ortho)", 
    "Right (Ortho)", 
    "Near (Ortho)", 
    "Far (Ortho)"
};

global const ak_f32 Global_GridSizes[] = {0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
global const ak_char* Global_GridSizesText[] = {"0.1", "0.25", "0.5", "1.0", "2.0", "5.0", "10.0"};
global const ak_f32 RotateAngleSnaps[] = {1.0f, 5.0f, 10.0f, 20.0f, 45.0f, 90.0f, 180.0f};
global const ak_char* RotateAngleSnapsText[] = {"1", "5", "10", "20", "45", "90", "180"};


EDITOR_DEBUG_LOG(Editor_DebugLog)
{
    if(!Internal__LogArena)
        Internal__LogArena = AK_CreateArena();
    
    va_list Args;
    va_start(Args, Format);
    Internal__Logs.Add(AK_FormatString(Internal__LogArena, Format, Args));
    va_end(Args);
}

void Editor_AddEntity(editor* Editor, ak_u32 WorldIndex, ak_u64 ID, ak_string Name)
{
    game_context* GameContext = &Editor->GameContext;
    ak_u32 Index = AK_PoolIndex(ID);
    ak_u32 IndexPlusOne = Index+1;
    if(IndexPlusOne > GameContext->GameEntityNames[WorldIndex].Size)
        GameContext->GameEntityNames[WorldIndex].Resize(IndexPlusOne*2);
    
    GameContext->GameEntityNames[WorldIndex][Index] = Name;
    GameContext->GameEntityNameHash[WorldIndex].Insert(Name, ID);
}

EDITOR_ADD_ENTITY(Editor_AddEntity)
{
    ak_string StringName = AK_PushString(Name, Editor->WorldManagement.StringArena);
    Editor_AddEntity(Editor, WorldIndex, ID, StringName);
}

EDITOR_DRAW_POINT(Editor_DrawPoint)
{
    render_primitive Primitive = {}; 
    Primitive.Type = RENDER_PRIMITIVE_TYPE_POINT;
    Primitive.Point.P = Position;
    Primitive.Point.Size = Size;
    Primitive.Point.Color = Color;
    Editor->RenderPrimitives.Add(Primitive);
}

EDITOR_DRAW_SEGMENT(Editor_DrawSegment)
{
    render_primitive Primitive = {};
    Primitive.Type = RENDER_PRIMITIVE_TYPE_SEGMENT;
    Primitive.Segment.P0 = P0;
    Primitive.Segment.P1 = P1;
    Primitive.Segment.Size = Size;
    Primitive.Segment.Color = Color;
    Editor->RenderPrimitives.Add(Primitive);
}

void Editor_DebugLog(ak_string String)
{
    if(!Internal__LogArena)
        Internal__LogArena = AK_CreateArena();
    Internal__Logs.Add(AK_PushString(String, Internal__LogArena));
}

#define EDITOR_ITEM_WIDTH 80.0f

world_file_header Editor_GetWorldFileHeader(ak_u16 EntityCountA, ak_u16 EntityCountB, ak_u16 PointLightCountA, ak_u16 PointLightCountB)
{
    world_file_header Header = {};
    AK_MemoryCopy(Header.Signature, WORLD_FILE_SIGNATURE, sizeof(WORLD_FILE_SIGNATURE));
    Header.MajorVersion = WORLD_FILE_MAJOR_VERSION;
    Header.MinorVersion = WORLD_FILE_MINOR_VERSION;
    Header.EntityCount[0] = EntityCountA;
    Header.EntityCount[1] = EntityCountB;
    Header.PointLightCount[0] = PointLightCountA;
    Header.PointLightCount[1] = PointLightCountB;
    return Header;
}

gizmo_selected_object Editor_GizmoSelectedObject(ak_u64 ID, object_type Type)
{
    gizmo_selected_object Result;
    Result.IsSelected = true;
    Result.SelectedObject.Type = Type;
    Result.SelectedObject.ID = ID;
    return Result;
}

ak_bool Editor_AreEntitiesEqual(dev_entity* EntityA, dev_entity* EntityB)
{
    ak_bool Result = (AK_StringEquals(EntityA->Name, EntityB->Name) &&
                      AK_StringEquals(EntityA->LinkName, EntityB->LinkName) &&
                      (EntityA->Type == EntityB->Type) &&
                      (EntityA->Transform == EntityB->Transform) &&
                      AreMaterialsEqual(EntityA->Material, EntityB->Material) &&
                      (EntityA->MeshID == EntityB->MeshID) &&
                      (EntityA->IsToggled == EntityB->IsToggled));
    return Result;
}

ak_bool Editor_ArePointLightsEqual(dev_point_light* PointLightA, dev_point_light* PointLightB)
{
    ak_bool Result = (AK_StringEquals(PointLightA->Name, PointLightB->Name) &&
                      PointLightA->Light.Color == PointLightB->Light.Color &&
                      PointLightA->Light.Intensity == PointLightB->Light.Intensity &&
                      PointLightA->Light.Position == PointLightB->Light.Position &&
                      PointLightA->Light.Radius == PointLightB->Light.Radius);
    return Result;
}

void Editor_SetSelectedObject(editor* Editor, object_type Type, ak_u64 ID)
{
    gizmo_selected_object* SelectedObject = &Editor->GizmoState.SelectedObject;
    SelectedObject->IsSelected = true;
    SelectedObject->SelectedObject.Type = Type;
    SelectedObject->SelectedObject.ID = ID;
}

object* Editor_GetSelectedObject(editor* Editor)
{
    gizmo_selected_object* SelectedObject = &Editor->GizmoState.SelectedObject;
    if(SelectedObject->IsSelected)
    {
        SelectedObject->IsSelected = SelectedObject->SelectedObject.IsAlive(Editor, Editor->CurrentWorldIndex);
    }
    return SelectedObject->IsSelected ? &SelectedObject->SelectedObject : NULL;
}

ak_quatf Editor_GetOrientationDiff(ak_m3f OriginalRotation, ak_v3f SelectorDiff)
{
    ak_quatf XOrientation = AK_RotQuat(OriginalRotation.XAxis, -SelectorDiff.x);
    ak_quatf YOrientation = AK_RotQuat(OriginalRotation.YAxis, -SelectorDiff.y);
    ak_quatf ZOrientation = AK_RotQuat(OriginalRotation.ZAxis, -SelectorDiff.z);
    
    return AK_Normalize(XOrientation*YOrientation*ZOrientation);
}

ak_quatf Editor_GetOrientationDiff(editor* Editor, ak_v3f SelectorDiff)
{
    return Editor_GetOrientationDiff(Editor->GizmoState.OriginalRotation, SelectorDiff);
}

#include "src/ui.cpp"
#include "src/world_management.cpp"
#include "src/dev_mesh.cpp"
#include "src/frame_playback.cpp"
#include "src/generated_string_templates.cpp"
#include "src/edit_recordings.cpp"

dev_entity* object::GetEntity(editor* Editor, ak_u32 WorldIndex)
{
    AK_Assert(Type == OBJECT_TYPE_ENTITY, "Cannot get entity of a selected object that is not an entity");
    dev_entity* Entity = Editor->WorldManagement.DevEntities[WorldIndex].Get(ID);
    return Entity;
}

dev_point_light* object::GetPointLight(editor* Editor, ak_u32 WorldIndex)
{
    AK_Assert(Type == OBJECT_TYPE_LIGHT, "Cannot get point light of a selected object that is not a point light");
    dev_point_light* PointLight = Editor->WorldManagement.DevPointLights[WorldIndex].Get(ID);
    return PointLight;
}

ak_v3f object::GetPosition(editor* Editor, ak_u32 WorldIndex)
{
    ak_v3f Result = {};
    switch(Type)
    {
        case OBJECT_TYPE_ENTITY:
        {
            dev_entity* Entity = GetEntity(Editor, WorldIndex);
            Result = Entity->Transform.Translation;
        } break;
        
        case OBJECT_TYPE_LIGHT:
        {
            dev_point_light* PointLight = GetPointLight(Editor, WorldIndex);
            Result = PointLight->Light.Position;
        } break;
        
        case OBJECT_TYPE_ENTITY_SPAWNER:
        {
            Result = Editor->UI.EntitySpawner.Translation;
        } break;
        
        case OBJECT_TYPE_LIGHT_SPAWNER:
        {
            Result = Editor->UI.LightSpawner.Translation;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    return Result;
}

ak_bool object::IsAlive(editor* Editor, ak_u32 WorldIndex)
{
    switch(Type)
    {
        case OBJECT_TYPE_ENTITY:
        {
            return GetEntity(Editor, WorldIndex) != NULL;
        } break;
        
        case OBJECT_TYPE_LIGHT:
        {
            return GetPointLight(Editor, WorldIndex) != NULL;
        } break;
        
        case OBJECT_TYPE_ENTITY_SPAWNER:
        case OBJECT_TYPE_LIGHT_SPAWNER:
        {
            return true;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    return false;
}

void Editor_GetPixelOrthoUV(ak_f32* u, ak_f32* v, ak_f32 Left, ak_f32 Right, ak_f32 Top, ak_f32 Bottom, ak_v2i MouseCoordinates, ak_v2i Resolution)
{
    MouseCoordinates.y = Resolution.y - MouseCoordinates.y;
    *u = Left + (Right-Left)*(MouseCoordinates.x+0.5f)/Resolution.x;
    *v = Bottom + (Top-Bottom)*(MouseCoordinates.y+0.5f)/Resolution.y;
}

void Editor_GetPixelOrthoUV(ak_f32* u, ak_f32* v, ortho_camera* Camera, ak_v2i MouseCoordinates, ak_v2i Resolution)
{
    Editor_GetPixelOrthoUV(u, v, Camera->Left, Camera->Right, Camera->Top, Camera->Bottom, MouseCoordinates, Resolution);
}

gizmo_intersection_result Editor_CastToGizmos(editor* Editor, gizmo_state* GizmoState, ray Ray, ak_f32 ZNear)
{
    gizmo_intersection_result Result = {};
    if(!GizmoState->SelectedObject.IsSelected)
        return Result;
    
    if(GizmoState->GizmoHit.Hit)
        return GizmoState->GizmoHit;
    
    ak_f32 tBest = INFINITY;    
    
    ak_u32 GizmoCount = (GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_ROTATE) ? 3 : 6;
    for(ak_u32 GizmoIndex = 0; GizmoIndex < GizmoCount; GizmoIndex++)
    {
        dev_mesh* DevMesh = GizmoState->Gizmos[GizmoIndex].Mesh;
        if(DevMesh != NULL)
        {
            ray_cast RayCast = Ray_TriangleMeshCastNoCull(Ray.Origin, Ray.Direction, (ak_v3f*)DevMesh->Vertices, 
                                                          (ak_u16*)DevMesh->Indices, DevMesh->IndexCount, AK_TransformM4(GizmoState->Gizmos[GizmoIndex].Transform));
            
            if(RayCast.Intersected)
            {
                if((tBest > RayCast.t) && (RayCast.t > ZNear))
                {
                    tBest = RayCast.t;
                    Result.Gizmo = &GizmoState->Gizmos[GizmoIndex];
                    Result.HitMousePosition = Ray.Origin + (tBest*Ray.Direction);
                    Result.Hit = true;                    
                }
            }
        }
    }
    
    return Result;
}

#define UPDATE_BEST_HIT(type, name, tables) \
do \
{ \
if(RayCast.Intersected) \
{ \
if((tBest > RayCast.t) && (RayCast.t > ZNear)) \
{ \
tBest = RayCast.t; \
Result.IsSelected = true; \
Result.SelectedObject.Type = type; \
ak_u64 ID = *tables[Editor->CurrentWorldIndex].Find(name); \
Result.SelectedObject.ID = ID; \
} \
} \
} while(0)

#define UPDATE_BEST_HIT_SPAWNER(type) \
do \
{ \
if(RayCast.Intersected) \
{ \
if((tBest > RayCast.t) && (RayCast.t > ZNear)) \
{ \
tBest = RayCast.t; \
Result.IsSelected = true; \
Result.SelectedObject.Type = type; \
} \
} \
} while(0)

gizmo_selected_object Editor_CastToAllObjects(editor* Editor, assets* Assets, ray Ray, ak_f32 ZNear)
{
    gizmo_selected_object Result = {};
    ak_f32 tBest = INFINITY;
    
    world_management* WorldManagement = &Editor->WorldManagement;
    ak_pool<dev_entity>* DevEntities = &WorldManagement->DevEntities[Editor->CurrentWorldIndex];
    ak_pool<dev_point_light>* DevPointLights = &WorldManagement->DevPointLights[Editor->CurrentWorldIndex];
    
    AK_ForEach(DevEntity, DevEntities)
    {
        mesh_info* MeshInfo = GetMeshInfo(Assets, DevEntity->MeshID);
        mesh* Mesh = GetMesh(Assets, DevEntity->MeshID);
        if(!Mesh)
            Mesh = LoadMesh(Assets, DevEntity->MeshID);
        
        ray_cast RayCast = {};
        if(MeshInfo->Header.IsIndexFormat32)
        {
            ak_u32* Indices = (ak_u32*)Mesh->Indices;
            RayCast = Ray_TriangleMeshCast(Ray.Origin, Ray.Direction, Mesh->Positions, Indices, 
                                           MeshInfo->Header.IndexCount, AK_TransformM4(DevEntity->Transform));
        }
        else
        {
            ak_u16* Indices = (ak_u16*)Mesh->Indices;
            RayCast = Ray_TriangleMeshCast(Ray.Origin, Ray.Direction, Mesh->Positions, Indices, 
                                           MeshInfo->Header.IndexCount, 
                                           AK_TransformM4(DevEntity->Transform));
        }
        
        UPDATE_BEST_HIT(OBJECT_TYPE_ENTITY, DevEntity->Name, WorldManagement->EntityTables);
    }
    
    AK_ForEach(DevPointLight, DevPointLights)
    {
        ray_cast RayCast = Ray_SphereCast(Ray.Origin, Ray.Direction, DevPointLight->Light.Position,
                                          POINT_LIGHT_RADIUS);
        UPDATE_BEST_HIT(OBJECT_TYPE_LIGHT, DevPointLight->Name, WorldManagement->PointLightTables);
    }
    
    if(Editor->UI.EntitySpawnerOpen)
    {
        entity_spawner* Spawner = &Editor->UI.EntitySpawner;
        mesh_info* MeshInfo = GetMeshInfo(Assets, Spawner->MeshID);
        mesh* Mesh = GetMesh(Assets, Spawner->MeshID);
        if(!Mesh)
            Mesh = LoadMesh(Assets, Spawner->MeshID);
        
        ak_m4f Transform = AK_TransformM4(AK_SQT(Spawner->Translation, Spawner->Orientation, Spawner->Scale));
        
        ray_cast RayCast = {};
        if(MeshInfo->Header.IsIndexFormat32)
        {
            ak_u32* Indices = (ak_u32*)Mesh->Indices;
            RayCast = Ray_TriangleMeshCast(Ray.Origin, Ray.Direction, Mesh->Positions, Indices, 
                                           MeshInfo->Header.IndexCount, Transform);
        }
        else
        {
            ak_u16* Indices = (ak_u16*)Mesh->Indices;
            RayCast = Ray_TriangleMeshCast(Ray.Origin, Ray.Direction, Mesh->Positions, Indices, 
                                           MeshInfo->Header.IndexCount, 
                                           Transform);
        }
        
        UPDATE_BEST_HIT_SPAWNER(OBJECT_TYPE_ENTITY_SPAWNER);
    }
    
    if(Editor->UI.LightSpawnerOpen)
    {
        light_spawner* LightSpawner = &Editor->UI.LightSpawner;
        ray_cast RayCast = Ray_SphereCast(Ray.Origin, Ray.Direction, LightSpawner->Translation, 
                                          POINT_LIGHT_RADIUS);
        UPDATE_BEST_HIT_SPAWNER(OBJECT_TYPE_LIGHT_SPAWNER);
    }
    
    return Result;
}

#undef UPDATE_BEST_HIT
#undef UPDATE_BEST_HIT_SPAWNER

ak_v3f Editor_GetSelectorDiff(editor* Editor, ray RayCast)
{
    ak_v3f Result = {};
    
    gizmo_state* GizmoState = &Editor->GizmoState;
    AK_Assert(GizmoState->SelectedObject.IsSelected, "Selected object must be selected to get selector diff");
    if(GizmoState->GizmoHit.Hit)
    {
        object* SelectedObject = &GizmoState->SelectedObject.SelectedObject;
        
        ak_v3f SelectedObjectPosition = SelectedObject->GetPosition(Editor, Editor->CurrentWorldIndex);
        
        
        gizmo_intersection_result* GizmoHit = &GizmoState->GizmoHit;
        ray_cast RayHit = Ray_PlaneCast(RayCast.Origin, RayCast.Direction, GizmoHit->Gizmo->IntersectionPlane, GizmoHit->HitMousePosition);        
        if(RayHit.Intersected != INFINITY)
        {   
            ak_f32 t = RayHit.t;
            
            ak_v3f PointDiff = AK_V3<ak_f32>();
            ak_v3f NewPoint = RayCast.Origin + (RayCast.Direction*t);
            ak_v3f MouseDiff = GizmoHit->HitMousePosition - NewPoint;
            ak_v3f XAxis = AK_XAxis();
            ak_v3f YAxis = AK_YAxis();
            ak_v3f ZAxis = AK_ZAxis();
            
            if(SelectedObject->Type == OBJECT_TYPE_ENTITY)
            {
                dev_entity* Entity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                
                if(GizmoState->UseLocalTransforms && GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_TRANSLATE)
                {
                    AK_Assert(SelectedObject->Type == OBJECT_TYPE_ENTITY, "Selector type must be an entity");
                    
                    ak_m3f Orientation = AK_QuatToMatrix(Entity->Transform.Orientation);
                    XAxis = Orientation.XAxis;
                    YAxis = Orientation.YAxis;
                    ZAxis = Orientation.ZAxis;
                } 
                else
                {
                    if(GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_SCALE && GizmoHit->HitMousePosition != NewPoint)
                    {
                        ak_quatf Orientation = Entity->Transform.Orientation;
                        ak_v3f RotatedOPoint = AK_Rotate(GizmoHit->HitMousePosition, AK_Conjugate(Orientation));
                        ak_v3f RotatedNewPoint = AK_Rotate(NewPoint, AK_Conjugate(Orientation));
                        MouseDiff = RotatedOPoint - RotatedNewPoint;
                    }
                }
            }
            
            
            if(GizmoState->TransformMode != SELECTOR_TRANSFORM_MODE_ROTATE)
            {
                switch(GizmoHit->Gizmo->MovementDirection)
                {
                    case GIZMO_MOVEMENT_DIRECTION_X:
                    {
                        //PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, 0.0f, 0.0f);
                        PointDiff = AK_Dot(MouseDiff, XAxis) * XAxis;
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_Y:
                    {
                        //PointDiff = AK_V3(0.0f, GizmoHit->HitMousePosition.y - NewPoint.y, 0.0f);
                        PointDiff = AK_Dot(MouseDiff, YAxis) * YAxis;
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_Z:
                    {
                        //PointDiff = AK_V3(0.0f, 0.0f, GizmoHit->HitMousePosition.z - NewPoint.z);
                        PointDiff = AK_Dot(MouseDiff, ZAxis) * ZAxis;
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_XY:
                    {
                        //PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, GizmoHit->HitMousePosition.y - NewPoint.y, 0.0f);
                        PointDiff = (AK_Dot(MouseDiff, XAxis) * XAxis) + AK_Dot(MouseDiff, YAxis) * YAxis;
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_XZ:
                    {
                        //PointDiff = AK_V3(GizmoHit->HitMousePosition.x - NewPoint.x, 0.0f, GizmoHit->HitMousePosition.z - NewPoint.z);
                        PointDiff = (AK_Dot(MouseDiff, XAxis) * XAxis) + AK_Dot(MouseDiff, ZAxis) * ZAxis;
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_YZ:
                    {
                        //PointDiff = AK_V3(0.0f, GizmoHit->HitMousePosition.y - NewPoint.y, GizmoHit->HitMousePosition.z - NewPoint.z);
                        PointDiff = (AK_Dot(MouseDiff, ZAxis) * ZAxis) + AK_Dot(MouseDiff, YAxis) * YAxis;
                    } break;                    
                }
            }
            else
            {
                ak_v3f DirectionToOld = GizmoHit->HitMousePosition - SelectedObjectPosition;
                ak_v3f DirectionToNew = NewPoint - SelectedObjectPosition;
                ak_f32 AngleDiff = AK_ATan2(AK_Dot(AK_Cross(DirectionToNew, DirectionToOld), 
                                                   GizmoHit->Gizmo->IntersectionPlane), 
                                            AK_Dot(DirectionToOld, DirectionToNew));
                
                
                switch(GizmoHit->Gizmo->MovementDirection)
                {
                    case GIZMO_MOVEMENT_DIRECTION_X:
                    {
                        PointDiff = AK_V3(AngleDiff, 0.0f, 0.0f);
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_Y:
                    {
                        PointDiff = AK_V3(0.0f, AngleDiff, 0.0f);
                    } break;
                    
                    case GIZMO_MOVEMENT_DIRECTION_Z:
                    {
                        PointDiff = AK_V3(0.0f, 0.0f, AngleDiff);
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }                
            }
            
            if(GizmoState->ShouldSnap)
            {    
                ak_f32 Snap;
                if(GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_SCALE) Snap = GizmoState->ScaleSnap;                
                else if(GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_ROTATE) Snap = AK_ToRadians(GizmoState->RotationAngleSnap);                                    
                else Snap = GizmoState->GridDistance;
                
                for(ak_u32 PlaneIndex = 0; PlaneIndex < 3; PlaneIndex++)
                {           
                    if((AK_Abs(PointDiff[PlaneIndex]) < Snap) && (PointDiff[PlaneIndex] != 0))
                    {
                        if(GizmoState->TransformMode != SELECTOR_TRANSFORM_MODE_ROTATE)
                        {
                            NewPoint[PlaneIndex] = GizmoHit->HitMousePosition[PlaneIndex];
                            PointDiff[PlaneIndex] = 0.0f;
                        }
                        else
                        {
                            NewPoint = GizmoHit->HitMousePosition;
                            PointDiff = AK_V3<ak_f32>();
                        }
                    }
                    else if(PointDiff[PlaneIndex] < 0) PointDiff[PlaneIndex] = -Snap;
                    else if(PointDiff[PlaneIndex] > 0) PointDiff[PlaneIndex] = Snap;
                }
            }
            
            GizmoHit->HitMousePosition = NewPoint;
            Result = PointDiff;
        }
    }
    
    return Result;
}

void Editor_SelectObjects(editor* Editor, assets* Assets, ray RayCast, ak_v2i Resolution)
{
    gizmo_state* GizmoState = &Editor->GizmoState;
    world_management* WorldManagement = &Editor->WorldManagement;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
    
    dev_input* Input = &Editor->Input;
    
    ak_u32 GizmoCount = (GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_ROTATE) ? 3 : 6;
    for(ak_u32 GizmoIndex = 0; GizmoIndex < GizmoCount; GizmoIndex++)
        GizmoState->Gizmos[GizmoIndex].IsHighLighted = false;
    
    if(!IsDown(&Input->Alt) && !ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard)
    {
        gizmo_intersection_result GizmoHitTest = Editor_CastToGizmos(Editor, GizmoState, RayCast, CAMERA_ZNEAR);
        
        if(GizmoHitTest.Hit) GizmoHitTest.Gizmo->IsHighLighted = true;
        
        if(IsPressed(&Input->LMB))
        {
            if(!GizmoHitTest.Hit)
            {
                GizmoState->GizmoHit = {};
                GizmoState->SelectedObject = Editor_CastToAllObjects(Editor, Assets, RayCast, CAMERA_ZNEAR);
            }
            else
            {
                AK_Assert(GizmoState->SelectedObject.IsSelected, "Cannot be selecting a gizmo without selecting an object");
                GizmoState->GizmoHit = GizmoHitTest;
                GizmoState->OriginalRotation = AK_IdentityM3<ak_f32>();
                object* SelectedObject = &GizmoState->SelectedObject.SelectedObject;
                
                switch(SelectedObject->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* Entity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                        GizmoState->OriginalRotation = AK_Transpose(AK_QuatToMatrix(Entity->Transform.Orientation));
                        
                        GizmoState->Transform = Entity->Transform;
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        dev_point_light* PointLight = 
                            SelectedObject->GetPointLight(Editor, 
                                                          Editor->CurrentWorldIndex);
                        GizmoState->Transform.Translation = PointLight->Light.Position;
                        GizmoState->Transform.Scale = AK_V3(PointLight->Light.Radius, PointLight->Light.Radius, PointLight->Light.Radius);
                    } break;
                    
                    case OBJECT_TYPE_ENTITY_SPAWNER:
                    {
                        entity_spawner* Spawner = &Editor->UI.EntitySpawner;
                        GizmoState->OriginalRotation = AK_Transpose(AK_QuatToMatrix(Spawner->Orientation));
                        GizmoState->Transform = AK_SQT(Spawner->Translation, Spawner->Orientation, 
                                                       Spawner->Scale);
                    } break;
                    
                    case OBJECT_TYPE_LIGHT_SPAWNER:
                    {
                        light_spawner* Spawner = &Editor->UI.LightSpawner;
                        GizmoState->Transform.Translation = Spawner->Translation;
                        GizmoState->Transform.Scale = AK_V3(Spawner->Radius, Spawner->Radius, Spawner->Radius);
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
            }
        }
        
        if(IsReleased(&Input->LMB))
        {
            object* SelectedObject = Editor_GetSelectedObject(Editor);
            if(SelectedObject && GizmoState->GizmoHit.Hit)
            {
                switch(SelectedObject->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* Entity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                        
                        ak_v3f TranslationDiff = Entity->Transform.Translation-GizmoState->Transform.Translation;
                        ak_v3f ScaleDiff = Entity->Transform.Scale-GizmoState->Transform.Scale;
                        ak_quatf OrientationDiff = AK_QuatDiff(GizmoState->Transform.Orientation, 
                                                               Entity->Transform.Orientation);
                        
                        object Object = {OBJECT_TYPE_ENTITY, SelectedObject->ID};
                        
                        EditRecordings->PushTransformEntry(Editor->CurrentWorldIndex, Object, TranslationDiff, ScaleDiff, OrientationDiff);
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        dev_point_light* DevPointLight = SelectedObject->GetPointLight(Editor, Editor->CurrentWorldIndex);
                        
                        ak_v3f TranslationDiff = DevPointLight->Light.Position-GizmoState->Transform.Translation;
                        ak_v3f ScaleDiff = DevPointLight->Light.Radius-GizmoState->Transform.Scale;
                        
                        object Object = {OBJECT_TYPE_LIGHT, SelectedObject->ID};
                        EditRecordings->PushTransformEntry(Editor->CurrentWorldIndex, Object, 
                                                           TranslationDiff, ScaleDiff, AK_IdentityQuat<ak_f32>());
                    } break;
                    
                    case OBJECT_TYPE_ENTITY_SPAWNER:
                    {
                        entity_spawner* Spawner = &Editor->UI.EntitySpawner;
                        ak_v3f TranslationDiff = Spawner->Translation-GizmoState->Transform.Translation;
                        ak_v3f ScaleDiff = Spawner->Scale-GizmoState->Transform.Scale;
                        ak_quatf OrientationDiff = AK_QuatDiff(GizmoState->Transform.Orientation, 
                                                               Spawner->Orientation);
                        
                        object Object = {OBJECT_TYPE_ENTITY_SPAWNER};
                        EditRecordings->PushTransformEntry(Editor->CurrentWorldIndex, Object, TranslationDiff, ScaleDiff, 
                                                           OrientationDiff);
                    } break;
                    
                    case OBJECT_TYPE_LIGHT_SPAWNER:
                    {
                        light_spawner* Spawner = &Editor->UI.LightSpawner;
                        
                        ak_v3f TranslationDiff = Spawner->Translation-GizmoState->Transform.Translation;
                        ak_v3f ScaleDiff = Spawner->Radius-GizmoState->Transform.Scale;
                        
                        object Object = {OBJECT_TYPE_LIGHT_SPAWNER};
                        EditRecordings->PushTransformEntry(Editor->CurrentWorldIndex, Object, TranslationDiff, ScaleDiff, 
                                                           AK_IdentityQuat<ak_f32>());
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
            }
            
            GizmoState->GizmoHit = {};
        }
    }
}

void Editor_DrawLineCapsule(editor* Editor, graphics* Graphics, ak_v3f P0, ak_v3f P1, ak_f32 Radius, ak_color3f Color)
{    
    ak_v3f ZAxis = P0-P1;
    ak_f32 ZScale = AK_Magnitude(ZAxis);
    ZAxis /= ZScale;
    
    dev_capsule_mesh* Mesh = &Editor->LineCapsuleMesh;    
    
    PushDrawLineMesh(Graphics, Mesh->MeshID, AK_TransformM4(P0, AK_Basis(ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);
    
    PushDrawLineMesh(Graphics, Mesh->MeshID, AK_TransformM4(P1, AK_Basis(-ZAxis), AK_V3(Radius, Radius, Radius)), 
                     Color, Mesh->CapIndexCount, 0, 0);    
    
    PushDrawLineMesh(Graphics, Mesh->MeshID, AK_TransformM4(P1 + (ZAxis*ZScale*0.5f), AK_Basis(ZAxis), AK_V3(Radius, Radius, ZScale)),
                     Color, Mesh->BodyIndexCount, Mesh->CapIndexCount, Mesh->CapVertexCount);    
}

void Editor_DrawLineCapsule(editor* Editor, graphics* Graphics, capsule* Capsule, ak_color3f Color)
{
    Editor_DrawLineCapsule(Editor, Graphics, Capsule->P0, Capsule->P1, Capsule->Radius, Color);    
}

void Editor_DrawSphere(editor* Editor, graphics* Graphics, ak_v3f CenterP, ak_f32 Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, AK_V3(Radius, Radius, Radius));
    PushDrawUnlitMesh(Graphics, Editor->TriangleSphereMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), Editor->TriangleSphereMesh.IndexCount, 0, 0);    
}

void Editor_DrawSphere(editor* Editor, graphics* Graphics, sphere* Sphere, ak_color3f Color)
{
    Editor_DrawSphere(Editor, Graphics, Sphere->CenterP, Sphere->Radius, Color);
}


void Editor_DrawOrientedBox(editor* Editor, graphics* Graphics, ak_v3f P, ak_v3f Dim, ak_v3f XAxis, ak_v3f YAxis, ak_v3f ZAxis, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(P, AK_M3(XAxis, YAxis, ZAxis), Dim);
    PushDrawUnlitMesh(Graphics, Editor->TriangleBoxMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), Editor->TriangleBoxMesh.IndexCount, 0, 0);
}

void Editor_DrawOrientedBox(editor* Editor, graphics* Graphics, ak_v3f P, ak_v3f Dim, ak_v3f XAxis, ak_v3f YAxis, ak_v3f ZAxis, ak_color3f Color, 
                            ak_f32 Alpha)
{
    ak_m4f Model = AK_TransformM4(P, AK_M3(XAxis, YAxis, ZAxis), Dim);
    PushDrawUnlitMesh(Graphics, Editor->TriangleBoxMesh.MeshID, Model, CreateDiffuseMaterialSlot(Color), Editor->TriangleBoxMesh.IndexCount, 0, 0, 
                      CreateTransparentMaterialSlot(Alpha));
}

void Editor_DrawBox(editor* Editor, graphics* Graphics, ak_v3f P, ak_v3f Dim, ak_color3f Color)
{
    Editor_DrawOrientedBox(Editor, Graphics, P, Dim, AK_XAxis(), AK_YAxis(), AK_ZAxis(), Color);    
}

void Editor_DrawPoint(editor* Editor, graphics* Graphics, ak_v3f P, ak_f32 Thickness, ak_color3f Color)
{
    Editor_DrawBox(Editor, Graphics, P, AK_V3(Thickness, Thickness, Thickness), Color);
}

void Editor_DrawLineEllipsoid(editor* Editor, graphics* Graphics, ak_v3f CenterP, ak_v3f Radius, ak_color3f Color)
{
    ak_m4f Model = AK_TransformM4(CenterP, Radius);
    PushDrawLineMesh(Graphics, Editor->LineSphereMesh.MeshID, Model, Color, Editor->LineSphereMesh.IndexCount, 0, 0); 
}

void Editor_DrawEdge(editor* Editor, graphics* Graphics, ak_v3f P0, ak_v3f P1, ak_f32 Thickness, ak_color3f Color)
{
    ak_v3f ZAxis = P1-P0;
    ak_f32 ZLength = AK_Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    ak_v3f XAxis, YAxis;
    AK_Basis(ZAxis, &XAxis, &YAxis);
    Editor_DrawOrientedBox(Editor, Graphics, P0, AK_V3(Thickness, Thickness, ZLength), XAxis, YAxis, ZAxis, Color);
}

void Editor_DrawEdge(editor* Editor, graphics* Graphics, ak_v3f P0, ak_v3f P1, ak_f32 ThicknessX, ak_f32 ThicknessY, ak_color3f Color, ak_f32 Alpha)
{
    ak_v3f ZAxis = P1-P0;
    ak_f32 ZLength = AK_Magnitude(ZAxis);
    ZAxis /= ZLength;
    
    ak_v3f XAxis, YAxis;
    AK_Basis(ZAxis, &XAxis, &YAxis);
    Editor_DrawOrientedBox(Editor, Graphics, P0, AK_V3(ThicknessX, ThicknessY, ZLength), XAxis, YAxis, ZAxis, Color, Alpha);
}

void Editor_DrawEdge(editor* Editor, graphics* Graphics, ak_v3f P0, ak_v3f P1, ak_f32 Thickness, ak_color3f Color, ak_f32 Alpha)
{
    Editor_DrawEdge(Editor, Graphics, P0, P1, Thickness, Thickness, Color, Alpha);
}

void Editor_DrawFrame(editor* Editor, graphics* Graphics, ak_v3f Position, ak_v3f XAxis = AK_XAxis(), ak_v3f YAxis = AK_YAxis(), ak_v3f ZAxis = AK_ZAxis())
{            
    {
        ak_v3f X, Y, Z;
        Z = XAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(Graphics, Editor->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Red3()), Editor->TriangleArrowMesh.IndexCount, 0, 0);    
    }
    
    {
        ak_v3f X, Y, Z;
        Z = YAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(Graphics, Editor->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Green3()), Editor->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    {
        ak_v3f X, Y, Z;
        Z = ZAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        PushDrawUnlitMesh(Graphics, Editor->TriangleArrowMesh.MeshID, Transform, CreateDiffuseMaterialSlot(AK_Blue3()), Editor->TriangleArrowMesh.IndexCount, 0, 0);            
    }
    
    Editor_DrawSphere(Editor, Graphics, Position, 0.04f, AK_White3());    
}

#define GRID_EDGE_THICKNESS_X 0.05f
#define GRID_EDGE_THICKNESS_Y 0.01f

void Editor_DrawGridX(editor* Editor, graphics* Graphics, ak_i32 zLeftBound, ak_i32 zRightBound, 
                      ak_i32 yTopBound, ak_i32 yBottomBound, ak_color3f Color, ak_f32 Alpha, ak_f32 GridDistance)
{
    ak_f32 MinZ = (ak_f32)zLeftBound;
    ak_f32 MaxZ = (ak_f32)zRightBound;
    ak_f32 MinY = (ak_f32)yBottomBound;
    ak_f32 MaxY = (ak_f32)yTopBound;
    
    if(MinZ > MaxZ) AK_Swap(MinZ, MaxZ);
    if(MinY > MaxY) AK_Swap(MinY, MaxY);
    
    ak_i32 ZCount = AK_Ceil((MaxZ-MinZ)/GridDistance);
    ak_i32 YCount = AK_Ceil((MaxY-MinY)/GridDistance);
    
    for(ak_i32 ZIndex = 0; ZIndex < ZCount; ZIndex++)
    {
        ak_f32 z = MinZ + ZIndex*GridDistance;
        ak_v3f P0 = AK_V3(0.0f, MinY, z);
        ak_v3f P1 = AK_V3(0.0f, MaxY, z);
        Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_X, Color, Alpha);
    }
    
    for(ak_i32 YIndex = 0; YIndex < YCount; YIndex++)
    {
        ak_f32 y = MinY + YIndex*GridDistance;
        ak_v3f P0 = AK_V3(0.0f, y, MinZ);
        ak_v3f P1 = AK_V3(0.0f, y, MaxZ);
        Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_X, Color, Alpha);
    }
    
    Editor_DrawEdge(Editor, Graphics, AK_V3f(0.0f, MinY, 0.0f), AK_V3f(0.0f, MaxY, 0.0f), 
                    GRID_EDGE_THICKNESS_X, AK_Green3(), Alpha);
    Editor_DrawEdge(Editor, Graphics, AK_V3f(0.0f, 0.0f, MinZ), AK_V3f(0.0f, 0.0f, MaxZ), 
                    GRID_EDGE_THICKNESS_X, AK_Red3(), Alpha);
}

void Editor_DrawGridY(editor* Editor, graphics* Graphics, ak_i32 xLeftBound, ak_i32 xRightBound, 
                      ak_i32 zTopBound, ak_i32 zBottomBound, ak_color3f Color, ak_f32 Alpha, ak_f32 GridDistance)
{
    ak_f32 MinX = (ak_f32)xLeftBound;
    ak_f32 MaxX = (ak_f32)xRightBound;
    ak_f32 MinZ = (ak_f32)zBottomBound;
    ak_f32 MaxZ = (ak_f32)zTopBound;
    
    if(MinX > MaxX) AK_Swap(MinX, MaxX);
    if(MinZ > MaxZ) AK_Swap(MinZ, MaxZ);
    
    ak_i32 XCount = AK_Ceil((MaxX-MinX)/GridDistance);
    ak_i32 ZCount = AK_Ceil((MaxZ-MinZ)/GridDistance);
    
    for(ak_i32 XIndex = 0; XIndex < XCount; XIndex++)
    {
        ak_f32 x = MinX + XIndex*GridDistance;
        ak_v3f P0 = AK_V3(x, 0.0f, MinZ);
        ak_v3f P1 = AK_V3(x, 0.0f, MaxZ);
        Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_X, Color, Alpha);
    }
    
    for(ak_i32 ZIndex = 0; ZIndex < ZCount; ZIndex++)
    {
        ak_f32 z = MinZ + ZIndex*GridDistance;
        ak_v3f P0 = AK_V3(MinX, 0.0f, z);
        ak_v3f P1 = AK_V3(MaxX, 0.0f, z);
        Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_X, Color, Alpha);
    }
    
    Editor_DrawEdge(Editor, Graphics, AK_V3f(MinX, 0.0f, 0.0f), AK_V3f(MaxX, 0.0f, 0.0f), 
                    GRID_EDGE_THICKNESS_X, AK_Green3(), Alpha);
    Editor_DrawEdge(Editor, Graphics, AK_V3f(0.0f, 0.0f, MinZ), AK_V3f(0.0f, 0.0f, MaxZ), 
                    GRID_EDGE_THICKNESS_X, AK_Red3(), Alpha);
}

void Editor_DrawGridZ(editor* Editor, graphics* Graphics, ak_i32 xLeftBound, ak_i32 xRightBound, ak_i32 yTopBound, ak_i32 yBottomBound, ak_color3f Color, ak_f32 Alpha, ak_f32 GridDistance)
{
    ak_f32 MinX = (ak_f32)xLeftBound;
    ak_f32 MaxX = (ak_f32)xRightBound;
    ak_f32 MinY = (ak_f32)yBottomBound;
    ak_f32 MaxY = (ak_f32)yTopBound;
    
    if(MinX > MaxX) AK_Swap(MinX, MaxX);
    if(MinY > MaxY) AK_Swap(MinY, MaxY);
    
    ak_i32 XCount = AK_Ceil((MaxX-MinX)/GridDistance);
    ak_i32 YCount = AK_Ceil((MaxY-MinY)/GridDistance);
    
    for(ak_i32 XIndex = 0; XIndex < XCount; XIndex++)
    {
        ak_f32 x = MinX + XIndex*GridDistance;
        if(x != 0.0f)
        {
            ak_v3f P0 = AK_V3(x, MinY, 0.0f);
            ak_v3f P1 = AK_V3(x, MaxY, 0.0f);
            Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_X, GRID_EDGE_THICKNESS_Y, Color, Alpha);
        }
    }
    
    for(ak_i32 YIndex = 0; YIndex < YCount; YIndex++)
    {
        ak_f32 y = MinY + YIndex*GridDistance;
        if(y != 0.0f)
        {
            ak_v3f P0 = AK_V3(MinX, y, 0.0f);
            ak_v3f P1 = AK_V3(MaxX, y, 0.0f);
            Editor_DrawEdge(Editor, Graphics, P0, P1, GRID_EDGE_THICKNESS_Y, GRID_EDGE_THICKNESS_X, Color, Alpha);
        }
    }
    
    Editor_DrawEdge(Editor, Graphics, AK_V3f(0.0f, MinY, 0.001f), AK_V3f(0.0f, MaxY, 0.001f), GRID_EDGE_THICKNESS_X, GRID_EDGE_THICKNESS_Y, AK_Red3(), Alpha);
    Editor_DrawEdge(Editor, Graphics, AK_V3f(MinX, 0.0f, 0.001f), AK_V3f(MaxX, 0.0f, 0.001f), GRID_EDGE_THICKNESS_Y, GRID_EDGE_THICKNESS_X, AK_Green3(), Alpha);
}

ak_color3f Editor_GetGizmoColor(gizmo Gizmo)
{
    ak_color3f GizmoColor = AK_White3();
    switch(Gizmo.MovementDirection)
    {
        case GIZMO_MOVEMENT_DIRECTION_X:
        {
            GizmoColor = AK_Red3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_Y:
        {
            GizmoColor = AK_Green3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_Z:
        {
            GizmoColor = AK_Blue3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_XY:
        {
            GizmoColor = AK_Blue3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_XZ:
        {
            GizmoColor = AK_Green3();
        } break;
        
        case GIZMO_MOVEMENT_DIRECTION_YZ:
        {
            GizmoColor = AK_Red3();
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    if(Gizmo.IsHighLighted)
    {
        GizmoColor = AK_Yellow3();
    }
    return GizmoColor;
}

void Editor_RenderGizmoState(editor* Editor, graphics* Graphics, gizmo_state* GizmoState, ak_v3f Position)
{
    if(GizmoState->TransformMode != SELECTOR_TRANSFORM_MODE_ROTATE)
    {   
        for(ak_i32 i = 0; i < 3; i++)
        {
            gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = Editor_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);
        
        for(ak_i32 i = 3; i < 6; i++)
        {
            gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = Editor_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    }
    else
    {
        PushCull(Graphics, GRAPHICS_CULL_MODE_NONE);
        for(ak_i32 i = 0; i < 3; i++)
        {
            gizmo CurrentGizmo = GizmoState->Gizmos[i];
            ak_v3f Color = Editor_GetGizmoColor(CurrentGizmo);
            PushDrawUnlitMesh(Graphics, CurrentGizmo.Mesh->MeshID, CurrentGizmo.Transform, CreateDiffuseMaterialSlot(Color), CurrentGizmo.Mesh->IndexCount, 0, 0);  
        }
        PushCull(Graphics, GRAPHICS_CULL_MODE_FRONT);
    }
    
    Editor_DrawSphere(Editor, Graphics, Position, 0.04f, AK_White3());    
}

void Editor_SetDefaultWorld(editor* Editor)
{
    ak_string DefaultWorldPath = AK_StringConcat(WORLDS_PATH, "default_world.txt", Editor->Scratch);
    AK_WriteEntireFile(DefaultWorldPath, Editor->WorldManagement.CurrentWorldName.Data, 
                       Editor->WorldManagement.CurrentWorldName.Length);
}

void Editor_LoadDefaultWorld(editor* Editor, assets* Assets, dev_platform* DevPlatform)
{
    ak_string DefaultWorldPath = AK_StringConcat(WORLDS_PATH, "default_world.txt", Editor->Scratch);
    ak_buffer Buffer = AK_ReadEntireFile(DefaultWorldPath, Editor->Scratch);
    if(Buffer.IsValid())
    {
        ak_string WorldName = AK_CreateString((ak_char*)Buffer.Data, AK_SafeU32(Buffer.Size));
        if(!Editor->WorldManagement.LoadWorld(WorldName, Editor, Assets, DevPlatform))
        {
            AK_MessageBoxOk("Error loading", "Could not load default world, clearing configuration");
            AK_FileRemove(DefaultWorldPath);
        }
    }
}

editor* Editor_Initialize(graphics* Graphics, ImGuiContext* Context, platform* Platform, dev_platform* DevPlatform, assets* Assets)
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
    
    ak_temp_arena TempArena = Editor->Scratch->BeginTemp();
    
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
    
    
    Editor->WorldSelectedIndex = (ak_u32)-1;
    
    Editor->LineCapsuleMesh = DevMesh_CreateLineCapsuleMesh(Graphics, 1.0f, 60);
    Editor->LineBoxMesh = DevMesh_CreateLineBoxMesh(Graphics);
    Editor->LineSphereMesh = DevMesh_CreateLineSphereMesh(Graphics, 60);
    Editor->TriangleBoxMesh = DevMesh_CreateTriangleBoxMesh(Graphics);
    Editor->TriangleSphereMesh = DevMesh_CreateTriangleSphereMesh(Graphics);
    Editor->TriangleCylinderMesh = DevMesh_CreateTriangleCylinderMesh(Graphics, 60);
    Editor->TriangleArrowMesh = DevMesh_CreateTriangleArrowMesh(Graphics, 60, 0.02f, 0.85f, 0.035f, 0.15f);
    Editor->TrianglePlaneMesh = DevMesh_CreatePlaneMesh(Graphics, 0.4f, 0.4f);
    Editor->TriangleCircleMesh = DevMesh_CreateTriangleCircleMesh(Graphics, 60, 0.05f);
    Editor->TriangleScaleMesh = DevMesh_CreateTriangleScaleMesh(Graphics, 60, 0.02f, 0.85f, 0.1f);
    Editor->TriangleTorusMesh = DevMesh_CreateTriangleTorusMesh(Graphics, 20, 0.03f);
    
    for(ak_u32 MeshIndex = 0; MeshIndex < MESH_ASSET_COUNT; MeshIndex++)
    {
        mesh_info* MeshInfo = GetMeshInfo(Assets, (mesh_asset_id)MeshIndex);
        if(MeshInfo->Header.ConvexHullCount)
        {
            Editor->ConvexHullMeshes[MeshIndex] = (dev_slim_mesh*)AK_Allocate(sizeof(dev_slim_mesh) * 
                                                                              MeshInfo->Header.ConvexHullCount);
            
            for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
            {
                Editor->ConvexHullMeshes[MeshIndex][ConvexHullIndex] =
                    DevMesh_CreateConvexHullMesh(Graphics, &MeshInfo->ConvexHulls[ConvexHullIndex]);
            }
        }
    }
    
    gizmo_state* GizmoState = &Editor->GizmoState;
    GizmoState->GridDistance = 1.0f;
    GizmoState->ScaleSnap = 1.0f;
    GizmoState->RotationAngleSnap = 5.0f;
    
    ui* UI = &Editor->UI;
    UI->EditorGridSizeIndex = 3;
    UI->EditorScaleSnapIndex = 3;
    UI->EditorRotateSnapIndex = 1;
    
    Editor->DebugLog = Editor_DebugLog;
    Editor->DrawPoint = Editor_DrawPoint;
    Editor->DrawSegment = Editor_DrawSegment;
    Editor->AddEntity = Editor_AddEntity;
    
    Editor->Scratch->EndTemp(&TempArena);
    
    ortho_camera* OrthoCamera = NULL;
    
    const ak_f32 DefaultDistance = 20;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_NEAR-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = -AK_ZAxis();
    OrthoCamera->Y = AK_YAxis();
    OrthoCamera->X = AK_XAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_FAR-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = AK_ZAxis();
    OrthoCamera->Y = AK_YAxis();
    OrthoCamera->X = -AK_XAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_LEFT-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = AK_XAxis();
    OrthoCamera->Y = AK_ZAxis();
    OrthoCamera->X = -AK_YAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_RIGHT-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = -AK_XAxis();
    OrthoCamera->Y = AK_ZAxis();
    OrthoCamera->X = AK_YAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_BOTTOM-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = AK_YAxis();
    OrthoCamera->Y = AK_ZAxis();
    OrthoCamera->X = AK_XAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    OrthoCamera = &Editor->OrthoCameras[0][VIEW_MODE_TYPE_TOP-1];
    OrthoCamera->Target = AK_V3<ak_f32>();
    OrthoCamera->Distance = DefaultDistance;
    OrthoCamera->Z = -AK_YAxis();
    OrthoCamera->Y = AK_ZAxis();
    OrthoCamera->X = -AK_XAxis();
    OrthoCamera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
    OrthoCamera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
    OrthoCamera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
    OrthoCamera->Top = DEFAULT_ORTHO_CAMERA_TOP;
    
    Editor->Cameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));    
    Editor->Cameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    AK_CopyArray(Editor->OrthoCameras[1], Editor->OrthoCameras[0], 6);
    
    Editor_LoadDefaultWorld(Editor, Assets, DevPlatform);
    
    return Editor;
}

void Editor_RenderGrid(editor* Editor, graphics* Graphics, view_settings* ViewSettings, ak_v2i Resolution, view_mode_type ViewMode)
{
    ak_v3f FrustumCorners[8];
    GetFrustumCorners(FrustumCorners, ViewSettings, Resolution);
    
    ray FrustumRays[4] = 
    {
        {FrustumCorners[0], AK_Normalize(FrustumCorners[5]-FrustumCorners[0])},
        {FrustumCorners[1], AK_Normalize(FrustumCorners[4]-FrustumCorners[1])},
        {FrustumCorners[2], AK_Normalize(FrustumCorners[7]-FrustumCorners[2])},
        {FrustumCorners[3], AK_Normalize(FrustumCorners[6]-FrustumCorners[3])}
    };
    
    ak_v3f PlaneNormal = {};
    switch(ViewMode)
    {
        case VIEW_MODE_TYPE_PERSPECTIVE:
        case VIEW_MODE_TYPE_NEAR:
        case VIEW_MODE_TYPE_FAR:
        {
            PlaneNormal = AK_ZAxis();
        } break;
        
        case VIEW_MODE_TYPE_TOP:
        case VIEW_MODE_TYPE_BOTTOM:
        {
            PlaneNormal = AK_YAxis();
        } break;
        
        case VIEW_MODE_TYPE_RIGHT:
        case VIEW_MODE_TYPE_LEFT:
        {
            PlaneNormal = AK_XAxis();
        } break;
    }
    
    ak_v3f FrustumPlaneIntersectionPoints[4];
    ak_i8 IntersectedCount = 0;
    for(int i = 0; i < 4; i++)
    {
        ray FrustumRay = FrustumRays[i];
        ray_cast RayCast = Ray_PlaneCast(FrustumRay.Origin, FrustumRay.Direction, PlaneNormal, AK_V3<ak_f32>());
        
        if(RayCast.Intersected)
        {
            IntersectedCount++;            
            RayCast.t = AK_Min(RayCast.t, CAMERA_ZFAR);
            FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin + (FrustumRay.Direction * RayCast.t);
        }
        else
        {            
            FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin;
        }
    }
    
    if(ViewMode != VIEW_MODE_TYPE_PERSPECTIVE)
        PushDepth(Graphics, false);
    
    const ak_f32 Alpha = 0.15f;
    PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);
    if(IntersectedCount != 0)
    {
        switch(ViewMode)
        {
            case VIEW_MODE_TYPE_PERSPECTIVE:
            case VIEW_MODE_TYPE_NEAR:
            case VIEW_MODE_TYPE_FAR:
            {
                ak_f32 MinX;
                ak_f32 MaxX;
                ak_f32 MinY;
                ak_f32 MaxY;
                if(IntersectedCount == 4)
                {
                    MinX = FrustumPlaneIntersectionPoints[0].x;
                    MaxX = FrustumPlaneIntersectionPoints[0].x;
                    MinY = FrustumPlaneIntersectionPoints[0].y;
                    MaxY = FrustumPlaneIntersectionPoints[0].y;
                    for(int i = 0; i < 4; i++)
                    {
                        MinX = AK_Min(FrustumPlaneIntersectionPoints[i].x, MinX);                                             
                        MaxX = AK_Max(FrustumPlaneIntersectionPoints[i].x, MaxX);
                        MinY = AK_Min(FrustumPlaneIntersectionPoints[i].y, MinY);
                        MaxY = AK_Max(FrustumPlaneIntersectionPoints[i].y, MaxY);
                    }
                }
                else
                {
                    MinX = FrustumCorners[0].x;
                    MaxX = FrustumCorners[0].x;
                    MinY = FrustumCorners[0].y;
                    MaxY = FrustumCorners[0].y;
                    for(int i = 0; i < 8; i++)
                    {
                        MinX = AK_Min(FrustumCorners[i].x, MinX);                                             
                        MaxX = AK_Max(FrustumCorners[i].x, MaxX);
                        MinY = AK_Min(FrustumCorners[i].y, MinY);
                        MaxY = AK_Max(FrustumCorners[i].y, MaxY);
                    }
                }
                
                Editor_DrawGridZ(Editor, Graphics, AK_Floor(MinX), AK_Ceil(MaxX), AK_Floor(MinY), AK_Ceil(MaxY), AK_RGB(0.1f, 0.1f, 0.1f),
                                 Alpha, Editor->GizmoState.GridDistance);
            } break;
            
            case VIEW_MODE_TYPE_LEFT:
            case VIEW_MODE_TYPE_RIGHT:
            {
                ak_f32 MinZ;
                ak_f32 MaxZ;
                ak_f32 MinY;
                ak_f32 MaxY;
                
                if(IntersectedCount == 4)
                {
                    MinZ = FrustumPlaneIntersectionPoints[0].z;
                    MaxZ = FrustumPlaneIntersectionPoints[0].z;
                    MinY = FrustumPlaneIntersectionPoints[0].y;
                    MaxY = FrustumPlaneIntersectionPoints[0].y;
                    for(int i = 0; i < 4; i++)
                    {
                        MinZ = AK_Min(FrustumPlaneIntersectionPoints[i].z, MinZ);                                             
                        MaxZ = AK_Max(FrustumPlaneIntersectionPoints[i].z, MaxZ);
                        MinY = AK_Min(FrustumPlaneIntersectionPoints[i].y, MinY);
                        MaxY = AK_Max(FrustumPlaneIntersectionPoints[i].y, MaxY);
                    }
                }
                else
                {
                    MinZ = FrustumCorners[0].z;
                    MaxZ = FrustumCorners[0].z;
                    MinY = FrustumCorners[0].y;
                    MaxY = FrustumCorners[0].y;
                    for(int i = 0; i < 8; i++)
                    {
                        MinZ = AK_Min(FrustumCorners[i].z, MinZ);                                             
                        MaxZ = AK_Max(FrustumCorners[i].z, MaxZ);
                        MinY = AK_Min(FrustumCorners[i].y, MinY);
                        MaxY = AK_Max(FrustumCorners[i].y, MaxY);
                    }
                }
                
                Editor_DrawGridX(Editor, Graphics, AK_Floor(MinZ), AK_Ceil(MaxZ), AK_Floor(MinY), 
                                 AK_Ceil(MaxY), AK_RGB(0.1f, 0.1f, 0.1f), Alpha, Editor->GizmoState.GridDistance);
            } break;
            
            case VIEW_MODE_TYPE_TOP:
            case VIEW_MODE_TYPE_BOTTOM:
            {
                ak_f32 MinX;
                ak_f32 MaxX;
                ak_f32 MinZ;
                ak_f32 MaxZ;
                
                if(IntersectedCount == 4)
                {
                    MinZ = FrustumPlaneIntersectionPoints[0].z;
                    MaxZ = FrustumPlaneIntersectionPoints[0].z;
                    MinX = FrustumPlaneIntersectionPoints[0].x;
                    MaxX = FrustumPlaneIntersectionPoints[0].x;
                    for(int i = 0; i < 4; i++)
                    {
                        MinZ = AK_Min(FrustumPlaneIntersectionPoints[i].z, MinZ);                                             
                        MaxZ = AK_Max(FrustumPlaneIntersectionPoints[i].z, MaxZ);
                        MinX = AK_Min(FrustumPlaneIntersectionPoints[i].x, MinX);
                        MaxX = AK_Max(FrustumPlaneIntersectionPoints[i].x, MaxX);
                    }
                }
                else
                {
                    MinZ = FrustumCorners[0].z;
                    MaxZ = FrustumCorners[0].z;
                    MinX = FrustumCorners[0].x;
                    MaxX = FrustumCorners[0].x;
                    for(int i = 0; i < 8; i++)
                    {
                        MinZ = AK_Min(FrustumCorners[i].z, MinZ);                                             
                        MaxZ = AK_Max(FrustumCorners[i].z, MaxZ);
                        MinX = AK_Min(FrustumCorners[i].x, MinX);
                        MaxX = AK_Max(FrustumCorners[i].x, MaxX);
                    }
                }
                
                Editor_DrawGridY(Editor, Graphics, AK_Floor(MinX), AK_Ceil(MaxX), AK_Floor(MinZ), 
                                 AK_Ceil(MaxZ), AK_RGB(0.1f, 0.1f, 0.1f), Alpha, Editor->GizmoState.GridDistance);
            } break;
        }
    }    
    PushBlend(Graphics, false);
    
    if(ViewMode != VIEW_MODE_TYPE_PERSPECTIVE)
        PushDepth(Graphics, true);
}

ak_bool Editor_Update(editor* Editor, assets* Assets, platform* Platform, dev_platform* DevPlatform, ak_f32 dt)
{
    if(!DevPlatform->Update(Editor, dt))
        return false;
    
    ak_v2i Resolution = Platform->GetResolution();
    
    dev_input* DevInput = &Editor->Input;
    
    game_context* GameContext = &Editor->GameContext;
    game* Game = GameContext->Game;
    world_management* WorldManagement = &Editor->WorldManagement;
    edit_recordings* EditRecordings = &Editor->EditRecordings;
    
    if(WorldManagement->NewState == WORLD_MANAGEMENT_STATE_NONE)
    {
        if(Editor->UI.ViewModeType == VIEW_MODE_TYPE_PERSPECTIVE)
        {
            if(!Game || Editor->UI.GameUseDevCamera)
            {
                perspective_camera* Camera = &Editor->Cameras[Editor->CurrentWorldIndex];
                ak_v3f* SphericalCoordinates = &Camera->SphericalCoordinates;
                
                view_settings ViewSettings = GetViewSettings(Camera, Resolution);
                ak_m4f View = AK_InvTransformM4(ViewSettings.Transform.Position, ViewSettings.Transform.Orientation);
                ak_v2f PanDelta = AK_V2<ak_f32>();
                
                if(IsDown(&DevInput->Alt))
                {
                    if(IsDown(&DevInput->LMB))
                    {
                        ak_v3f StartDirection = Ray_PixelToView(DevInput->MouseCoordinates, Resolution, ViewSettings.Projection);
                        ak_v3f EndDirection = Ray_PixelToView(DevInput->MouseCoordinates+DevInput->MouseDelta, Resolution, ViewSettings.Projection);
                        ak_v3f MouseDiff = EndDirection-StartDirection;
                        
                        SphericalCoordinates->inclination += MouseDiff.y;
                        SphericalCoordinates->azimuth += MouseDiff.x;
                        
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
                    
                    if(IsDown(&DevInput->MMB))
                    {
                        ak_v3f StartDirection = Ray_PixelToView(DevInput->MouseCoordinates, Resolution, ViewSettings.Projection);
                        ak_v3f EndDirection = Ray_PixelToView(DevInput->MouseCoordinates+DevInput->MouseDelta, Resolution, ViewSettings.Projection);
                        ak_v3f MouseDiff = EndDirection-StartDirection;
                        PanDelta += MouseDiff.xy*2.5f;
                    }
                    
                    if(AK_Abs(DevInput->Scroll) > 0) SphericalCoordinates->radius -= DevInput->Scroll*0.5f;
                }
                
                if(SphericalCoordinates->radius < 1e-3f)
                    SphericalCoordinates->radius = 1e-3f;
                
                ak_rigid_transformf CameraTransform = GetCameraTransform(Camera);
                Camera->Target += (CameraTransform.Orientation.XAxis*PanDelta.x - CameraTransform.Orientation.YAxis*PanDelta.y);        
            }
        }
        else
        {
            if(!Game)
            {
                ortho_camera* Camera = &Editor->OrthoCameras[Editor->CurrentWorldIndex][Editor->UI.ViewModeType-1];
                
                ak_v2f PanDelta = AK_V2<ak_f32>();
                
                if(IsDown(&DevInput->Alt))
                {
                    if(IsDown(&DevInput->MMB))
                    {
                        ak_f32 startU, startV;
                        ak_f32 endU, endV;
                        
                        Editor_GetPixelOrthoUV(&startU, &startV, Camera, DevInput->MouseCoordinates, Resolution);
                        Editor_GetPixelOrthoUV(&endU, &endV, Camera, DevInput->MouseCoordinates+DevInput->MouseDelta, Resolution);
                        
                        ak_v3f CameraPosition = GetCameraPosition(Camera);
                        
                        ak_v3f StartPosition = CameraPosition + startU*Camera->X + startV*Camera->Y;
                        ak_v3f EndPosition = CameraPosition + endU*Camera->X + endV*Camera->Y;
                        
                        ak_v3f MouseDiff = EndPosition-StartPosition;
                        switch(Editor->UI.ViewModeType)
                        {
                            case VIEW_MODE_TYPE_NEAR: { PanDelta = MouseDiff.xy; } break;
                            case VIEW_MODE_TYPE_FAR: { PanDelta = AK_V2(-MouseDiff.x, MouseDiff.y); } break;
                            case VIEW_MODE_TYPE_LEFT: { PanDelta = AK_V2(-MouseDiff.y, MouseDiff.z); } break;
                            case VIEW_MODE_TYPE_RIGHT: { PanDelta = MouseDiff.yz; } break;
                            case VIEW_MODE_TYPE_BOTTOM: { PanDelta = AK_V2(MouseDiff.x, MouseDiff.z); } break;
                            case VIEW_MODE_TYPE_TOP: { PanDelta = AK_V2(-MouseDiff.x, MouseDiff.z); } break;
                        }
                    }
                    
                    if(AK_Abs(DevInput->Scroll) > 0) 
                    {
                        ak_f32 Constant = DevInput->Scroll < 0 ? 1.1f : 0.9f;
                        ak_f32 Right = Camera->Right * Constant;
                        if(Right > 1.0f)
                        {
                            Camera->Left *= Constant;
                            Camera->Right = Right;
                            Camera->Top *= Constant;
                            Camera->Bottom *= Constant;
                        }
                    }
                }
                
                ak_v3f Diff = Camera->X*PanDelta.x - Camera->Y*PanDelta.y;
                Camera->Target += Diff;
            }
        }
        
        if(!Game)
        {
            if(IsPressed(&DevInput->Q))
            {
                Editor->CurrentWorldIndex = !Editor->CurrentWorldIndex;
                Editor->GizmoState.SelectedObject = {};
            }
            
            if(IsDown(&DevInput->Ctrl))
            {
                if(IsPressed(&DevInput->Z))
                    EditRecordings->Undo(Editor);
                
                if(IsPressed(&DevInput->Y))
                    EditRecordings->Redo(Editor);
            }
            
            
            ray RayCast = {};
            if(Editor->UI.ViewModeType == VIEW_MODE_TYPE_PERSPECTIVE)
            {
                view_settings ViewSettings = GetViewSettings(&Editor->Cameras[Editor->CurrentWorldIndex], Resolution);
                ak_m4f View = AK_InvTransformM4(ViewSettings.Transform.Position, ViewSettings.Transform.Orientation);
                RayCast.Origin = ViewSettings.Transform.Position;
                RayCast.Direction = Ray_PixelToWorld(Editor->Input.MouseCoordinates, Resolution, ViewSettings.Projection, View);
            }
            else
            {
                ortho_camera* Camera = &Editor->OrthoCameras[Editor->CurrentWorldIndex][Editor->UI.ViewModeType-1];
                
                ak_f32 u, v;
                Editor_GetPixelOrthoUV(&u, &v, Camera, Editor->Input.MouseCoordinates, Resolution);
                
                RayCast.Origin = GetCameraPosition(Camera) + u*Camera->X + v*Camera->Y;
                RayCast.Direction = Camera->Z;
            }
            
            Editor_SelectObjects(Editor, Assets, RayCast, Resolution);
            
            object* SelectedObject = Editor_GetSelectedObject(Editor);
            if(SelectedObject)
            {
                switch(SelectedObject->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    case OBJECT_TYPE_ENTITY_SPAWNER:
                    {
                        if(IsPressed(&DevInput->W)) 
                            Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_TRANSLATE;
                        if(IsPressed(&DevInput->E)) 
                            Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_SCALE;
                        if(IsPressed(&DevInput->R)) 
                            Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_ROTATE;
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    case OBJECT_TYPE_LIGHT_SPAWNER:
                    {
                        if(IsPressed(&DevInput->W)) 
                            Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_TRANSLATE;
                        if(IsPressed(&DevInput->E)) 
                            Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_SCALE;
                    } break;
                }
                
                ak_v3f SelectorDiff = Editor_GetSelectorDiff(Editor, RayCast);
                
                switch(SelectedObject->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* Entity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                        
                        switch(Editor->GizmoState.TransformMode)
                        {
                            case SELECTOR_TRANSFORM_MODE_TRANSLATE:
                            {
                                Entity->Transform.Translation -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_SCALE:
                            {
                                Entity->Transform.Scale -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_ROTATE:
                            {
                                Entity->Transform.Orientation *= Editor_GetOrientationDiff(Editor, SelectorDiff);
                            } break;
                        }
                        
                    } break;
                    
                    case OBJECT_TYPE_ENTITY_SPAWNER:
                    {
                        entity_spawner* Spawner = &Editor->UI.EntitySpawner;
                        
                        switch(Editor->GizmoState.TransformMode)
                        {
                            case SELECTOR_TRANSFORM_MODE_TRANSLATE:
                            {
                                Spawner->Translation -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_SCALE:
                            {
                                Spawner->Scale -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_ROTATE:
                            {
                                Spawner->Orientation *= Editor_GetOrientationDiff(Editor, SelectorDiff);
                            } break;
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        dev_point_light* DevPointLight = SelectedObject->GetPointLight(Editor, Editor->CurrentWorldIndex);
                        switch(Editor->GizmoState.TransformMode)
                        {
                            case SELECTOR_TRANSFORM_MODE_TRANSLATE:
                            {
                                DevPointLight->Light.Position -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_SCALE:
                            {
                                DevPointLight->Light.Radius -= SelectorDiff[SelectorDiff.LargestComp()];
                            } break;
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT_SPAWNER:
                    {
                        light_spawner* Spawner = &Editor->UI.LightSpawner;
                        switch(Editor->GizmoState.TransformMode)
                        {
                            case SELECTOR_TRANSFORM_MODE_TRANSLATE:
                            {
                                Spawner->Translation -= SelectorDiff;
                            } break;
                            
                            case SELECTOR_TRANSFORM_MODE_SCALE:
                            {
                                Spawner->Radius -= SelectorDiff[SelectorDiff.LargestComp()];
                            } break;
                        }
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
                if(IsPressed(&DevInput->F))
                {
                    if(Editor->UI.ViewModeType == VIEW_MODE_TYPE_PERSPECTIVE)
                    {
                        perspective_camera* Camera = &Editor->Cameras[Editor->CurrentWorldIndex];
                        Camera->Target = SelectedObject->GetPosition(Editor, Editor->CurrentWorldIndex);
                        Camera->SphericalCoordinates.radius = 6.0f;
                    }
                    else
                    {
                        ortho_camera* Camera = &Editor->OrthoCameras[Editor->CurrentWorldIndex][Editor->UI.ViewModeType-1];
                        Camera->Target = SelectedObject->GetPosition(Editor, Editor->CurrentWorldIndex);
                        Camera->Left = DEFAULT_ORTHO_CAMERA_LEFT;
                        Camera->Right = DEFAULT_ORTHO_CAMERA_RIGHT;
                        Camera->Top = DEFAULT_ORTHO_CAMERA_TOP;
                        Camera->Bottom = DEFAULT_ORTHO_CAMERA_BOTTOM;
                    }
                }
                
                if(IsPressed(&DevInput->Delete))
                {
                    switch(SelectedObject->Type)
                    {
                        case OBJECT_TYPE_ENTITY:
                        {
                            dev_entity* DevEntity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                            EditRecordings->PushDeleteEntry(Editor->CurrentWorldIndex, DevEntity);
                            WorldManagement->DeleteDevEntity(Editor->CurrentWorldIndex, DevEntity->Name);
                        } break;
                        
                        case OBJECT_TYPE_LIGHT:
                        {
                            dev_point_light* DevPointLight = SelectedObject->GetPointLight(Editor, Editor->CurrentWorldIndex);
                            EditRecordings->PushDeleteEntry(Editor->CurrentWorldIndex, 
                                                            DevPointLight);
                            WorldManagement->DeleteDevPointLight(Editor->CurrentWorldIndex, 
                                                                 DevPointLight->Name);
                        } break;
                        
                        case OBJECT_TYPE_ENTITY_SPAWNER:
                        case OBJECT_TYPE_LIGHT_SPAWNER:
                        {
                            //Do nothing
                        } break;
                        
                        AK_INVALID_DEFAULT_CASE;
                    }
                    
                    Editor->GizmoState.SelectedObject = {};
                }
                
                if(IsDown(&DevInput->Ctrl))
                {
                    if(IsPressed(&DevInput->D))
                    {
                        switch(SelectedObject->Type)
                        {
                            case OBJECT_TYPE_ENTITY:
                            {
                                dev_entity* DevEntity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
                                
                                dev_entity* DuplicateEntity = 
                                    WorldManagement->DuplicateEntity(Editor->Scratch, DevEntity, 
                                                                     Editor->CurrentWorldIndex);
                                ak_u64 ID = *WorldManagement->EntityTables[Editor->CurrentWorldIndex].Find(DuplicateEntity->Name);
                                
                                if(!AK_StringIsNullOrEmpty(DuplicateEntity->LinkName))
                                {
                                    ak_u32 WorldIndex = !Editor->CurrentWorldIndex;
                                    ak_u64* LinkID = WorldManagement->EntityTables[WorldIndex].Find(DuplicateEntity->LinkName);
                                    AK_Assert(LinkID, "Cannot have an entity with a linked object that is deleted");
                                    dev_entity* LinkEntity = WorldManagement->DevEntities[WorldIndex].Get(*LinkID);
                                    
                                    EditRecordings->PushCreateEntry(Editor->CurrentWorldIndex, 
                                                                    DuplicateEntity, LinkEntity);
                                    
                                }
                                else
                                {
                                    EditRecordings->PushCreateEntry(Editor->CurrentWorldIndex, DuplicateEntity);
                                }
                                
                                Editor_SetSelectedObject(Editor, SelectedObject->Type, ID);
                            } break;
                            
                            case OBJECT_TYPE_LIGHT:
                            {
                                dev_point_light* DevPointLight = 
                                    SelectedObject->GetPointLight(Editor, Editor->CurrentWorldIndex);
                                dev_point_light* DuplicatePointLight = WorldManagement->DuplicatePointLight(Editor->Scratch, 
                                                                                                            DevPointLight, 
                                                                                                            Editor->CurrentWorldIndex);
                                
                                EditRecordings->PushCreateEntry(Editor->CurrentWorldIndex, 
                                                                DuplicatePointLight);
                                
                                ak_u64 ID = *WorldManagement->PointLightTables[Editor->CurrentWorldIndex].Find(DuplicatePointLight->Name);
                                Editor_SetSelectedObject(Editor, SelectedObject->Type, ID);
                            } break;
                            
                            AK_INVALID_DEFAULT_CASE;
                        }
                    }
                }
            }
        }
    }
    
    
    DevInput->MouseCoordinates = {};
    DevInput->MouseDelta = {};
    DevInput->Scroll = 0.0f;
    
    UpdateButtons(DevInput->Buttons, AK_Count(DevInput->Buttons));
    
    return true;
}

void Editor_PopulateNonRotationGizmos(editor* Editor, gizmo_state* GizmoState, dev_mesh* GizmoMesh, dev_mesh* TrianglePlaneMesh, ak_v3f Position)
{   
    ak_v3f XAxis = AK_XAxis();
    ak_v3f YAxis = AK_YAxis();
    ak_v3f ZAxis = AK_ZAxis();
    ak_v3f Gizmo1 = AK_V3f(0, 0, 0);
    ak_v3f Gizmo2 = AK_V3f(0, 0, 0);
    ak_v3f Gizmo3 = AK_V3f(0, 0, 0);
    
    object* SelectedObject = Editor_GetSelectedObject(Editor);
    AK_Assert(SelectedObject, "Selected object must be present before rendering");
    
    if(SelectedObject->Type == OBJECT_TYPE_ENTITY)
    {
        if(GizmoState->UseLocalTransforms || GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_SCALE)
        {
            dev_entity* Entity = SelectedObject->GetEntity(Editor, Editor->CurrentWorldIndex);
            ak_m3f Orientation = AK_QuatToMatrix(Entity->Transform.Orientation); 
            XAxis = Orientation.XAxis;
            YAxis = Orientation.YAxis;
            ZAxis = Orientation.ZAxis;
        }
        
    }
    
    Gizmo1 = Gizmo1 + GIZMO_PLANE_DISTANCE*XAxis;
    Gizmo1 = Gizmo1 + GIZMO_PLANE_DISTANCE*YAxis;
    Gizmo2 = Gizmo2 + GIZMO_PLANE_DISTANCE*XAxis;
    Gizmo2 = Gizmo2 + GIZMO_PLANE_DISTANCE*ZAxis;
    Gizmo3 = Gizmo3 + GIZMO_PLANE_DISTANCE*YAxis;
    Gizmo3 = Gizmo3 + GIZMO_PLANE_DISTANCE*ZAxis;
    
    {
        ak_v3f X, Y, Z;
        Z = XAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = ZAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_X;
        Gizmo.IsHighLighted = GizmoState->Gizmos[0].IsHighLighted;
        GizmoState->Gizmos[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = YAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = ZAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Y;
        Gizmo.IsHighLighted = GizmoState->Gizmos[1].IsHighLighted;
        GizmoState->Gizmos[1] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = ZAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = GizmoMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = YAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Z;
        Gizmo.IsHighLighted = GizmoState->Gizmos[2].IsHighLighted;
        GizmoState->Gizmos[2] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = ZAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + Gizmo1, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = ZAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_XY;
        Gizmo.IsHighLighted = GizmoState->Gizmos[3].IsHighLighted;
        GizmoState->Gizmos[3] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = YAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + Gizmo2, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = YAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_XZ;
        Gizmo.IsHighLighted = GizmoState->Gizmos[4].IsHighLighted;
        GizmoState->Gizmos[4] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = XAxis;
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position + Gizmo3, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TrianglePlaneMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = XAxis;
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_YZ;
        Gizmo.IsHighLighted = GizmoState->Gizmos[5].IsHighLighted;
        GizmoState->Gizmos[5] =  Gizmo;
    }    
}

void Editor_PopulateRotationGizmos(gizmo_state* GizmoState, dev_mesh* TriangleTorusMesh, ak_v3f Position)
{
    {
        ak_v3f X, Y, Z;
        Z = AK_XAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_XAxis();
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_X;
        Gizmo.IsHighLighted = GizmoState->Gizmos[0].IsHighLighted;
        GizmoState->Gizmos[0] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_YAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_YAxis();
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Y;
        Gizmo.IsHighLighted = GizmoState->Gizmos[1].IsHighLighted;
        GizmoState->Gizmos[1] =  Gizmo;
    }
    
    {
        ak_v3f X, Y, Z;
        Z = AK_ZAxis();
        AK_Basis(Z, &X, &Y);
        
        ak_m4f Transform = AK_TransformM4(Position, X, Y, Z);
        gizmo Gizmo;
        Gizmo.Mesh = TriangleTorusMesh;
        Gizmo.Transform = AK_SQT(Transform);
        Gizmo.IntersectionPlane = AK_ZAxis();
        Gizmo.MovementDirection = GIZMO_MOVEMENT_DIRECTION_Z;
        Gizmo.IsHighLighted = GizmoState->Gizmos[2].IsHighLighted;
        GizmoState->Gizmos[2] =  Gizmo;
    }
    
    GizmoState->Gizmos[3] = {};                                                                
    GizmoState->Gizmos[4] = {};                                                                 
    GizmoState->Gizmos[5] = {};                
}

void Editor_RenderSelectedObjectGizmos(editor* Editor, graphics* Graphics)
{
    object* SelectedObject = Editor_GetSelectedObject(Editor);
    if(SelectedObject)
    {
        PushDepth(Graphics, false);
        
        gizmo_state* GizmoState = &Editor->GizmoState;
        ak_v3f Position = SelectedObject->GetPosition(Editor, Editor->CurrentWorldIndex);
        
        switch(GizmoState->TransformMode)
        {
            case SELECTOR_TRANSFORM_MODE_TRANSLATE:
            case SELECTOR_TRANSFORM_MODE_SCALE:
            {
                dev_mesh* GizmoMesh = (GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_SCALE) ? 
                    &Editor->TriangleScaleMesh : &Editor->TriangleArrowMesh;
                
                Editor_PopulateNonRotationGizmos(Editor, GizmoState, GizmoMesh, &Editor->TrianglePlaneMesh, Position);
            } break;
            
            case SELECTOR_TRANSFORM_MODE_ROTATE:
            {
                Editor_PopulateRotationGizmos(GizmoState, &Editor->TriangleTorusMesh, Position);
            } break;
        }
        
        Editor_RenderGizmoState(Editor, Graphics, GizmoState, Position);
        
        PushDepth(Graphics, true);
    }
}

ak_bool Editor_ShouldDrawEntitySpawnerMesh(editor* Editor, ak_u32 WorldIndex)
{
    return Editor->UI.EntitySpawnerOpen && (Editor->CurrentWorldIndex == WorldIndex);
}

ak_bool Editor_ShouldDrawLightSpawner(editor* Editor, ak_u32 WorldIndex)
{
    return Editor->UI.LightSpawnerOpen && (Editor->CurrentWorldIndex == WorldIndex);
}

ak_fixed_array<ak_u64> 
Editor_MergeEntities(ak_arena* Scratch, ak_pool<dev_entity>* DevEntities,
                     ak_v3f CameraPosition, ak_fixed_array<ak_u64> Left, ak_fixed_array<ak_u64> Right)
{
    
    ak_u32 ResultIndex = 0;
    ak_u32 LeftIndex = 0;
    ak_u32 RightIndex = 0;
    
    ak_fixed_array<ak_u64> Result = AK_CreateArray<ak_u64>(Scratch, Left.Size+Right.Size);
    
    while((LeftIndex != Left.Size) && (RightIndex != Right.Size))
    {
        ak_v3f LeftPosition = DevEntities->Get(Left[LeftIndex])->Transform.Translation;
        ak_v3f RightPosition = 
            DevEntities->Get(Right[RightIndex])->Transform.Translation;
        
        if(AK_SqrMagnitude(CameraPosition-RightPosition) >
           AK_SqrMagnitude(CameraPosition-LeftPosition))
        {
            Result[ResultIndex++] = Right[RightIndex++];
        }
        else
        {
            Result[ResultIndex++] = Left[LeftIndex++];
        }
    }
    
    while(LeftIndex != Left.Size)
        Result[ResultIndex++] = Left[LeftIndex++];
    
    while(RightIndex != Right.Size)
        Result[ResultIndex++] = Right[RightIndex++];
    
    return Result;
}

ak_fixed_array<ak_u64> Editor_MergeSortEntities(ak_arena* Scratch, ak_pool<dev_entity>* DevEntities, 
                                                ak_v3f CameraPosition, ak_fixed_array<ak_u64> IDList)
{
    if(IDList.Size <= 1)
        return IDList;
    
    ak_u32 HalfSize = (IDList.Size)/2;
    
    ak_fixed_array<ak_u64> Left = AK_CreateArray<ak_u64>(Scratch, HalfSize);
    ak_fixed_array<ak_u64> Right = AK_CreateArray<ak_u64>(Scratch, IDList.Size-HalfSize);
    
    ak_u32 LeftCount = 0;
    ak_u32 RightCount = 0;
    for(ak_u32 Index = 0; Index < IDList.Size; Index++)
    {
        if(Index < HalfSize)
            Left[LeftCount++] = IDList[Index];
        else
            Right[RightCount++] = IDList[Index];
    }
    
    Left = Editor_MergeSortEntities(Scratch, DevEntities, CameraPosition, Left);
    Right = Editor_MergeSortEntities(Scratch, DevEntities, CameraPosition, Right);
    
    return Editor_MergeEntities(Scratch, DevEntities, CameraPosition, Left, Right);
}

ak_fixed_array<ak_u64> Editor_SortDevEntities(editor* Editor, ak_u32 WorldIndex, 
                                              ak_v3f CameraPosition)
{
    ak_pool<dev_entity>* DevEntities = &Editor->WorldManagement.DevEntities[WorldIndex];
    
    ak_u32 Count = 0;
    ak_fixed_array<ak_u64> Array = AK_CreateArray<ak_u64>(Editor->Scratch, DevEntities->Size);
    for(ak_u32 EntityIndex = 0; EntityIndex < DevEntities->MaxUsed; EntityIndex++)
    {
        ak_u64 ID = DevEntities->IDs[EntityIndex];
        if(AK_PoolIsAllocatedID(ID))
        {
            Array[Count++] = ID;
        }
    }
    
    return Editor_MergeSortEntities(Editor->Scratch, DevEntities, CameraPosition, Array);
}

view_settings Editor_RenderDevWorld(editor* Editor, graphics* Graphics, assets* Assets, ak_u32 WorldIndex)
{
    graphics_render_buffer* RenderBuffer = Editor->RenderBuffers[WorldIndex];
    
    world_management* WorldManagement = &Editor->WorldManagement;
    view_settings ViewSettings = GetEditorViewSettings(Editor, RenderBuffer->Resolution, WorldIndex);
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    ak_bool DrawEntitySpawnerMesh = Editor_ShouldDrawEntitySpawnerMesh(Editor, WorldIndex);
    ak_bool DrawLightSpawner = Editor_ShouldDrawLightSpawner(Editor, WorldIndex);
    
    graphics_light_buffer LightBuffer = {};
    AK_ForEach(DevLight, &WorldManagement->DevPointLights[WorldIndex])
        AddPointLight(&LightBuffer, DevLight->Light);
    
    if(DrawLightSpawner)
    {
        light_spawner* Spawner = &Editor->UI.LightSpawner;
        graphics_point_light Light = CreatePointLight(Spawner->Color, Spawner->Intensity, Spawner->Translation, Spawner->Radius);
        AddPointLight(&LightBuffer, Light);
    }
    
    switch(Editor->UI.RenderModeType)
    {
        case RENDER_MODE_TYPE_LIT:
        {
            PushLightBuffer(Graphics, &LightBuffer);
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
                
                PushMaterial(Graphics, Material);
                PushDrawMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
            }
            
        } break;
        
        case RENDER_MODE_TYPE_UNLIT:
        {
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
                
                PushDrawUnlitMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), Material.Diffuse, 
                                  GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
            }
            
        } break;
        
        case RENDER_MODE_TYPE_WIREFRAME:
        {
            PushWireframe(Graphics, true);
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
                PushDrawUnlitMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), CreateDiffuseMaterialSlot(AK_Blue3()), 
                                  GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
            }
            PushWireframe(Graphics, false);
        } break;
        
        case RENDER_MODE_TYPE_LIT_WIREFRAME:
        {
            PushLightBuffer(Graphics, &LightBuffer);
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
                
                PushMaterial(Graphics, Material);
                PushDrawMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
            }
            
            PushWireframe(Graphics, true);
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
                PushDrawUnlitMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), CreateDiffuseMaterialSlot(AK_Blue3()), 
                                  GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
            }
            PushWireframe(Graphics, false);
        } break;
    }
    
    if(Editor->UI.EditorOverlayOtherWorld)
    {
        ak_fixed_array<ak_u64> EntityIDs =  Editor_SortDevEntities(Editor, !WorldIndex, ViewSettings.Transform.Position);
        
        
        PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);
        AK_ForEach(EntityID, &EntityIDs)
        {
            dev_entity* DevEntity = Editor->WorldManagement.DevEntities[!WorldIndex].Get(*EntityID);
            
            graphics_material GraphicsMaterial = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
            
            graphics_mesh_id MeshID = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
            
            switch(Editor->UI.RenderModeType)
            {
                case RENDER_MODE_TYPE_LIT:
                case RENDER_MODE_TYPE_UNLIT:
                {
                    PushDrawUnlitMesh(Graphics, MeshID, DevEntity->Transform, GraphicsMaterial.Diffuse, 
                                      GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0, CreateTransparentMaterialSlot(0.25f));
                } break;
                
                case RENDER_MODE_TYPE_WIREFRAME:
                {
                    PushWireframe(Graphics, true);
                    PushDrawUnlitMesh(Graphics, MeshID, DevEntity->Transform, CreateDiffuseMaterialSlot(AK_Red3()), 
                                      GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
                    PushWireframe(Graphics, false);
                } break;
                
                case RENDER_MODE_TYPE_LIT_WIREFRAME:
                {
                    PushDrawUnlitMesh(Graphics, MeshID, DevEntity->Transform, GraphicsMaterial.Diffuse, 
                                      GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0, CreateTransparentMaterialSlot(0.25f));
                    PushWireframe(Graphics, true);
                    PushDrawUnlitMesh(Graphics, MeshID, DevEntity->Transform, CreateDiffuseMaterialSlot(AK_Red3()), 
                                      GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
                    PushWireframe(Graphics, false);
                } break;
            }
        }
        PushBlend(Graphics, false);
    }
    
    if(DrawEntitySpawnerMesh)
    {
        entity_spawner* Spawner = &Editor->UI.EntitySpawner;
        material Material = UI_MaterialFromContext(&Spawner->MaterialContext);
        mesh_asset_id MeshID = Spawner->MeshID;
        
        graphics_material GraphicsMaterial = ConvertToGraphicsMaterial(Assets, Graphics, &Material);
        GraphicsMaterial.Alpha = CreateTransparentMaterialSlot(0.75f);
        
        ak_m4f Transform = AK_TransformM4(AK_SQT(Spawner->Translation, Spawner->Orientation, Spawner->Scale));
        
        graphics_mesh_id GraphicsMeshID = GetOrLoadGraphicsMesh(Assets, Graphics, MeshID);
        
        PushBlend(Graphics, true, GRAPHICS_BLEND_SRC_ALPHA, GRAPHICS_BLEND_ONE_MINUS_SRC_ALPHA);
        
        switch(Editor->UI.RenderModeType)
        {
            case RENDER_MODE_TYPE_LIT:
            {
                PushLightBuffer(Graphics, &LightBuffer);
                PushMaterial(Graphics, GraphicsMaterial);
                PushDrawMesh(Graphics, GraphicsMeshID, Transform, GetMeshIndexCount(Assets, MeshID), 0, 0);
            } break;
            
            case RENDER_MODE_TYPE_UNLIT:
            {
                PushDrawUnlitMesh(Graphics, GraphicsMeshID, Transform, GraphicsMaterial.Diffuse, 
                                  GetMeshIndexCount(Assets, MeshID), 0, 0, GraphicsMaterial.Alpha);
            } break;
            
            case RENDER_MODE_TYPE_WIREFRAME:
            {
                PushWireframe(Graphics, true);
                PushDrawUnlitMesh(Graphics, GraphicsMeshID, Transform, CreateDiffuseMaterialSlot(AK_Yellow3()), 
                                  GetMeshIndexCount(Assets, MeshID), 0, 0);
                PushWireframe(Graphics, false);
            } break;
            
            case RENDER_MODE_TYPE_LIT_WIREFRAME:
            {
                PushLightBuffer(Graphics, &LightBuffer);
                PushMaterial(Graphics, GraphicsMaterial);
                PushDrawMesh(Graphics, GraphicsMeshID, Transform, GetMeshIndexCount(Assets, MeshID), 0, 0);
                
                PushWireframe(Graphics, true);
                PushDrawUnlitMesh(Graphics, GraphicsMeshID, Transform, CreateDiffuseMaterialSlot(AK_Yellow3()), 
                                  GetMeshIndexCount(Assets, MeshID), 0, 0);
                PushWireframe(Graphics, false);
            } break;
        }
        PushBlend(Graphics, false);
    }
    
    AK_ForEach(PointLight, &WorldManagement->DevPointLights[WorldIndex])
    {
        Editor_DrawSphere(Editor, Graphics, PointLight->Light.Position, POINT_LIGHT_RADIUS, AK_Yellow3());
    }
    
    if(DrawLightSpawner)
    {
        light_spawner* LightSpawner = &Editor->UI.LightSpawner;
        Editor_DrawSphere(Editor, Graphics, LightSpawner->Translation, POINT_LIGHT_RADIUS, AK_Yellow3()*0.5f);
    }
    
    if(WorldIndex == (Editor->CurrentWorldIndex))
    {
        Editor_RenderSelectedObjectGizmos(Editor, Graphics);
        if(Editor->UI.EditorDrawGrid)
        {
            Editor_RenderGrid(Editor, Graphics, &ViewSettings, RenderBuffer->Resolution, Editor->UI.ViewModeType);
        }
    }
    
    return ViewSettings;
}

view_settings Editor_RenderGameWorld(editor* Editor, graphics* Graphics, game* Game, ak_u32 WorldIndex, ak_f32 tInterpolated)
{
    assets* Assets = Game->Assets;
    world* World = Game->World;
    
    perspective_camera* Camera = &Game->Cameras[WorldIndex];
    ak_pool<entity>* EntityStorage = &World->EntityStorage[WorldIndex];
    ak_pool<point_light>* PointLightStorage = &World->PointLightStorage[WorldIndex];
    ak_array<ak_sqtf>* OldTransforms = &World->OldTransforms[WorldIndex];
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    ak_array<graphics_object>* GraphicsObjects = &World->GraphicsObjects[WorldIndex];
    graphics_render_buffer* RenderBuffer = Editor->RenderBuffers[WorldIndex];
    
    for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
    {
        ak_u64 ID = EntityStorage->IDs[EntityIndex];
        if(AK_PoolIsAllocatedID(ID))
        {
            ak_u32 Index = AK_PoolIndex(ID);
            ak_sqtf* NewTransform = &PhysicsObjects->Get(Index)->Transform;
            ak_sqtf* OldTransform = OldTransforms->Get(Index);
            graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
            
            ak_sqtf InterpState;
            InterpState.Translation = AK_Lerp(OldTransform->Translation, tInterpolated, NewTransform->Translation);
            InterpState.Orientation = AK_Lerp(OldTransform->Orientation, tInterpolated, NewTransform->Orientation);
            InterpState.Scale = NewTransform->Scale;
            
            GraphicsObject->Transform = AK_TransformM4(InterpState);
            
            entity* Entity = EntityStorage->GetByIndex(Index);
            if(Entity->Type == ENTITY_TYPE_PLAYER)
                Camera->Target = GraphicsObject->Transform.Translation.xyz;
        }
    }
    
    if(Editor->UI.GameUseDevCamera)
        Camera = &Editor->Cameras[WorldIndex];
    
    view_settings ViewSettings = GetViewSettings(Camera, RenderBuffer->Resolution);
    
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    switch(Editor->UI.RenderModeType)
    {
        case RENDER_MODE_TYPE_LIT:
        {
            graphics_light_buffer LightBuffer = {};
            AK_ForEach(Light, PointLightStorage)
            {
                AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
                LightBuffer.PointLights[LightBuffer.PointLightCount++] = ToGraphicsPointLight(Light);
            }
            
            PushLightBuffer(Graphics, &LightBuffer);
            for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
            {
                ak_u64 ID = EntityStorage->IDs[EntityIndex];
                if(AK_PoolIsAllocatedID(ID))
                {
                    ak_u32 Index = AK_PoolIndex(ID);
                    graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                    graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &GraphicsObject->Material);
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsObject->MeshID);
                    
                    PushMaterial(Graphics, Material);
                    PushDrawMesh(Graphics, MeshHandle, GraphicsObject->Transform, GetMeshIndexCount(Assets, GraphicsObject->MeshID), 0, 0);
                }
            }
        } break;
        
        case RENDER_MODE_TYPE_UNLIT:
        {
            for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
            {
                ak_u64 ID = EntityStorage->IDs[EntityIndex];
                if(AK_PoolIsAllocatedID(ID))
                {
                    ak_u32 Index = AK_PoolIndex(ID);
                    graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                    graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &GraphicsObject->Material);
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsObject->MeshID);
                    PushDrawUnlitMesh(Graphics, MeshHandle, GraphicsObject->Transform, Material.Diffuse, 
                                      GetMeshIndexCount(Assets, GraphicsObject->MeshID), 0, 0);
                }
            }
        } break;
        
        case RENDER_MODE_TYPE_WIREFRAME:
        {
            PushWireframe(Graphics, true);
            for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
            {
                ak_u64 ID = EntityStorage->IDs[EntityIndex];
                if(AK_PoolIsAllocatedID(ID))
                {
                    ak_u32 Index = AK_PoolIndex(ID);
                    graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsObject->MeshID);
                    PushDrawUnlitMesh(Graphics, MeshHandle, GraphicsObject->Transform, CreateDiffuseMaterialSlot(AK_Blue3()), 
                                      GetMeshIndexCount(Assets, GraphicsObject->MeshID), 0, 0);
                }
            }
            PushWireframe(Graphics, false);
        } break;
        
        case RENDER_MODE_TYPE_LIT_WIREFRAME:
        {
            graphics_light_buffer LightBuffer = {};
            AK_ForEach(Light, PointLightStorage)
            {
                AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
                LightBuffer.PointLights[LightBuffer.PointLightCount++] = ToGraphicsPointLight(Light);
            }
            
            PushLightBuffer(Graphics, &LightBuffer);
            for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
            {
                ak_u64 ID = EntityStorage->IDs[EntityIndex];
                if(AK_PoolIsAllocatedID(ID))
                {
                    ak_u32 Index = AK_PoolIndex(ID);
                    graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                    graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &GraphicsObject->Material);
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsObject->MeshID);
                    
                    PushMaterial(Graphics, Material);
                    PushDrawMesh(Graphics, MeshHandle, GraphicsObject->Transform, GetMeshIndexCount(Assets, GraphicsObject->MeshID), 0, 0);
                }
            }
            
            PushWireframe(Graphics, true);
            for(ak_u32 EntityIndex = 0; EntityIndex < EntityStorage->MaxUsed; EntityIndex++)
            {
                ak_u64 ID = EntityStorage->IDs[EntityIndex];
                if(AK_PoolIsAllocatedID(ID))
                {
                    ak_u32 Index = AK_PoolIndex(ID);
                    graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                    graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, GraphicsObject->MeshID);
                    PushDrawUnlitMesh(Graphics, MeshHandle, GraphicsObject->Transform, CreateDiffuseMaterialSlot(AK_Blue3()), 
                                      GetMeshIndexCount(Assets, GraphicsObject->MeshID), 0, 0);
                }
            }
            PushWireframe(Graphics, false);
        } break;
    }
    
    return ViewSettings;
}

void Editor_Render(editor* Editor, graphics* Graphics, platform* Platform, assets* Assets, 
                   ak_f32 tInterpolated)
{
    
    ak_v2i Resolution = Platform->GetResolution();
    
    game_context* GameContext = &Editor->GameContext;
    game* Game = GameContext->Game;
    
    ak_u32 CurrentWorldIndex;
    if(Game)
        CurrentWorldIndex = Game->CurrentWorldIndex;
    else
        CurrentWorldIndex = Editor->CurrentWorldIndex;
    
    UpdateRenderBuffer(Graphics, &Editor->RenderBuffers[CurrentWorldIndex], Resolution);
    UpdateRenderBuffer(Graphics, &Editor->RenderBuffers[!CurrentWorldIndex], Resolution/5);
    
    if(Game)
    {
        view_settings ViewSettings = Editor_RenderGameWorld(Editor, Graphics, Game, CurrentWorldIndex, tInterpolated);
        if(Editor->UI.EditorDrawOtherWorld)
        {
            Editor_RenderGameWorld(Editor, Graphics, Game, !CurrentWorldIndex, tInterpolated);
            PushRenderBufferViewportScissorAndView(Graphics, Editor->RenderBuffers[CurrentWorldIndex], 
                                                   &ViewSettings);
        }
        
        if(Editor->UI.GameDrawColliders)
        {
            world* World = Game->World;
            
            ak_u32 WorldIndex = CurrentWorldIndex;
            ak_array<graphics_object>* GraphicsObjects = &World->GraphicsObjects[WorldIndex];
            ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
            ak_pool<collision_volume>* CollisionVolumes = &World->CollisionVolumeStorage;
            
            PushDepth(Graphics, false);
            
            AK_ForEach(Entity, &World->EntityStorage[WorldIndex])
            {
                ak_u32 Index = AK_PoolIndex(Entity->ID);
                
                graphics_object* GraphicsObject = GraphicsObjects->Get(Index);
                physics_object* PhysicsObject = PhysicsObjects->Get(Index);
                
                ak_sqtf Transform = AK_SQT(GraphicsObject->Transform);
                
                collision_volume* Volume = CollisionVolumes->Get(PhysicsObject->CollisionVolumeID);
                
                dev_slim_mesh* ConvexHullMeshes = Editor->ConvexHullMeshes[GraphicsObject->MeshID];
                
                ak_u32 ConvexHullIndex = 0;
                while(Volume)
                {
                    switch(Volume->Type)
                    {
                        case COLLISION_VOLUME_TYPE_SPHERE:
                        {                    
                            sphere Sphere = TransformSphere(&Volume->Sphere, Transform);                    
                            Editor_DrawLineEllipsoid(Editor, Graphics, Sphere.CenterP, AK_V3(Sphere.Radius, Sphere.Radius, Sphere.Radius), AK_Blue3());
                        } break;
                        
                        case COLLISION_VOLUME_TYPE_CAPSULE:
                        { 
                            capsule Capsule = TransformCapsule(&Volume->Capsule, Transform);
                            Editor_DrawLineCapsule(Editor, Graphics, Capsule.P0, Capsule.P1, Capsule.Radius, AK_Blue3());                                                
                        } break;
                        
                        case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                        {
                            ak_sqtf NewTransform = Volume->ConvexHull.Header.Transform*Transform;
                            ak_m4f Model = AK_TransformM4(NewTransform);
                            
                            PushDrawLineMesh(Graphics, ConvexHullMeshes[ConvexHullIndex].MeshID, Model, AK_Blue3(),
                                             ConvexHullMeshes[ConvexHullIndex].IndexCount, 0, 0);                    
                            ConvexHullIndex++;
                        } break;
                    }
                    Volume = CollisionVolumes->Get(Volume->NextID);
                }
            }
            PushDepth(Graphics, true);
        }
    }
    else
    {
        view_settings ViewSettings = Editor_RenderDevWorld(Editor, Graphics, Assets, CurrentWorldIndex);
        if(Editor->UI.EditorDrawOtherWorld)
        {
            Editor_RenderDevWorld(Editor, Graphics, Assets, !CurrentWorldIndex);
            PushRenderBufferViewportScissorAndView(Graphics, Editor->RenderBuffers[CurrentWorldIndex], 
                                                   &ViewSettings);
        }
        
        if(Editor->UI.EditorDrawColliders)
        {
            PushDepth(Graphics, false);
            
            AK_ForEach(DevEntity, &Editor->WorldManagement.DevEntities[CurrentWorldIndex])
            {
                ak_sqtf Transform = DevEntity->Transform;
                
                mesh_info* MeshInfo = GetMeshInfo(Assets, DevEntity->MeshID);
                dev_slim_mesh* ConvexHullMeshes = Editor->ConvexHullMeshes[DevEntity->MeshID];
                
                for(ak_u32 ConvexHullIndex = 0; ConvexHullIndex < MeshInfo->Header.ConvexHullCount; ConvexHullIndex++)
                {
                    ak_sqtf NewTransform = MeshInfo->ConvexHulls[ConvexHullIndex].Header.Transform*Transform;
                    ak_m4f Model = AK_TransformM4(NewTransform);
                    PushDrawLineMesh(Graphics, ConvexHullMeshes[ConvexHullIndex].MeshID, Model, AK_Blue3(),
                                     ConvexHullMeshes[ConvexHullIndex].IndexCount, 0, 0);
                }
                
                PushDepth(Graphics, true);
            }
        }
    }
    
    PushDepth(Graphics, false);
    
    AK_ForEach(RenderPrimitive, &Editor->RenderPrimitives)
    {
        switch(RenderPrimitive->Type)
        {
            case RENDER_PRIMITIVE_TYPE_POINT:
            {
                Editor_DrawPoint(Editor, Graphics, RenderPrimitive->Point.P, 
                                 RenderPrimitive->Point.Size, RenderPrimitive->Point.Color);
            } break;
            
            case RENDER_PRIMITIVE_TYPE_SEGMENT:
            {
                Editor_DrawEdge(Editor, Graphics, RenderPrimitive->Segment.P0, RenderPrimitive->Segment.P1, RenderPrimitive->Segment.Size, 
                                RenderPrimitive->Segment.Color);
            } break;
        }
    }
    Editor->RenderPrimitives.Clear();
    
    PushDepth(Graphics, true);
    
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
    PushCopyToOutput(Graphics, Editor->RenderBuffers[CurrentWorldIndex], AK_V2(0, 0), Resolution);
    
    if(Editor->UI.EditorDrawOtherWorld)
    {
        ak_v2i CopyResolution = Editor->RenderBuffers[CurrentWorldIndex]->Resolution / 5;
        ak_v2i CopyOffset = Editor->RenderBuffers[CurrentWorldIndex]->Resolution - CopyResolution; 
        PushCopyToOutput(Graphics, Editor->RenderBuffers[!CurrentWorldIndex], CopyOffset, CopyResolution);
    }
    
    
    Platform->ExecuteRenderCommands(Graphics);
}


ak_u64 GetEntryID(editor* Editor, dev_entity* DevEntity, ak_u32 WorldIndex)
{
    ak_u64 Result = 0;
    
    game_context* GameContext = &Editor->GameContext;
    
    switch(DevEntity->Type)
    {
        case ENTITY_TYPE_PLAYER:
        {
            Result = CreatePlayerEntity(GameContext->Game, WorldIndex, DevEntity->Transform.Translation, 
                                        DevEntity->Material)->ID;
        } break;
        
        case ENTITY_TYPE_BUTTON:
        {
            Result = CreateButtonEntity(GameContext->Game, WorldIndex, DevEntity->Transform.Translation, 
                                        DevEntity->Transform.Scale, DevEntity->Material, DevEntity->IsToggled)->ID;
        } break;
        
        case ENTITY_TYPE_STATIC:
        {
            Result = CreateStaticEntity(GameContext->Game, WorldIndex, DevEntity->Transform.Translation, DevEntity->Transform.Scale, DevEntity->Transform.Orientation, 
                                        DevEntity->MeshID, DevEntity->Material)->ID;
        } break;
        
        case ENTITY_TYPE_MOVABLE:
        {
            Result = CreateMovableEntity(GameContext->Game, WorldIndex, 
                                         DevEntity->Transform.Translation, 
                                         DevEntity->Transform.Scale, 
                                         DevEntity->Material)->ID;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    Editor_AddEntity(Editor, WorldIndex, Result, DevEntity->Name);
    return Result;
}

ak_u64 GetEntryID(editor* Editor, dev_point_light* DevPointLight, ak_u32 WorldIndex)
{
    ak_u64 Result = CreatePointLight(Editor->GameContext.Game, WorldIndex, DevPointLight->Light.Position, DevPointLight->Light.Radius, 
                                     DevPointLight->Light.Color, DevPointLight->Light.Intensity)->ID;
    return Result;
}

template <typename type>
ak_u32 Editor_BuildIDs(editor* Editor, ak_u64* WorldIDs, ak_pool<type>* Pool, ak_hash_map<ak_string, ak_u32>* HashMap, ak_u32 WorldIndex)
{
    AK_ForEach(Entry, Pool)
    {
        ak_u64 EntityID = GetEntryID(Editor, Entry, WorldIndex);
        ak_u32* Index = HashMap->Find(Entry->Name);
        if(Index)
            WorldIDs[*Index] = EntityID;
    }
    return HashMap->Size;
}

void Editor_UpdateGame(editor* Editor, platform* Platform, ak_f32* tInterpolated)
{
    frame_playback* FramePlayback = &Editor->FramePlayback;
    game* Game = Editor->GameContext.Game;
    
    if(!((FramePlayback->NewState == FRAME_PLAYBACK_STATE_RECORDING) ||
         (FramePlayback->NewState == FRAME_PLAYBACK_STATE_NONE)))
    {
        Game->Input = {};
    }
    
    Game->LoopAccum.IncrementAccum();
    while(Game->LoopAccum.ShouldUpdate())
    {
        if(FramePlayback->NewState == FRAME_PLAYBACK_STATE_RECORDING)
            FramePlayback->RecordFrame(Editor);
        
        if((FramePlayback->NewState == FRAME_PLAYBACK_STATE_PLAYING) ||
           (FramePlayback->NewState == FRAME_PLAYBACK_STATE_INSPECTING))
            FramePlayback->PlayFrame(Editor);
        
        __try
        {
            ak_temp_arena GameTemp = Game->Scratch->BeginTemp();
            AK_ClearArray(Editor->TimedBlockEntries, TIMED_BLOCK_ENTRY_COUNT);
            Game->Update(Game);
            Game->Scratch->EndTemp(&GameTemp);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //TODO(JJ): Stack trace exception handling
            AK_MessageBoxOk("Error", "Unhandled exception occurred when running the game. Game must be stopped");
            
            if(FramePlayback->NewState == FRAME_PLAYBACK_STATE_RECORDING)
                FramePlayback->StopRecording(Editor);
            
            if((FramePlayback->NewState == FRAME_PLAYBACK_STATE_PLAYING) ||
               (FramePlayback->NewState == FRAME_PLAYBACK_STATE_INSPECTING))
                FramePlayback->StopPlaying(Editor);
            
            Editor_StopGame(Editor, Platform);
            return;
        }
        
        Game->LoopAccum.DecrementAccum();
    }
    
    UpdateButtons(Game->Input.Buttons, AK_Count(Game->Input.Buttons));
    
    if(FramePlayback->NewState == FRAME_PLAYBACK_STATE_NONE)
        *tInterpolated = Game->LoopAccum.GetRemainder();
    else
        *tInterpolated = 1.0f;
}

void Editor_StopGame(editor* Editor, platform* Platform)
{
    game_context* GameContext = &Editor->GameContext;
    game* Game = GameContext->Game;
    
    Game->Shutdown(Game);
    GameContext->Game = NULL;
    
    Editor->Cameras[0] = Editor->OldCameras[0];
    Editor->Cameras[1] = Editor->OldCameras[1];
    
    AK_DeleteHashMap(&GameContext->GameEntityNameHash[0]);
    AK_DeleteHashMap(&GameContext->GameEntityNameHash[1]);
    AK_DeleteArray(&GameContext->GameEntityNames[0]);
    AK_DeleteArray(&GameContext->GameEntityNames[1]);
    
    Editor->UI.GameUseDevCamera = false;
    AK_ClearArray(Editor->TimedBlockEntries, TIMED_BLOCK_ENTRY_COUNT);
    
    Platform->UnloadGameCode();
    Platform->UnloadWorldCode();
}

ak_bool Editor_PlayGame(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform, 
                        dev_platform* DevPlatform)
{
    game_context* GameContext = &Editor->GameContext;
    world_management* WorldManagement = &Editor->WorldManagement;
    
    game_startup* GameStartup = Platform->LoadGameCode();
    world_startup* WorldStartup = Platform->LoadWorldCode(WorldManagement->CurrentWorldPath, WorldManagement->CurrentWorldName);
    if(!GameStartup || !WorldStartup || !DevPlatform->SetGameDebugEditor(Editor))
    {
        AK_MessageBoxOk("Error", "Could not start the game. Please save the world and try again. Game/World libraries are missing or corrupted.");
        
        if(GameStartup)
            Platform->UnloadGameCode();
        
        if(WorldStartup)
            Platform->UnloadWorldCode();
        
        return false;
    }
    
    __try
    {
        
        GameContext->Game = GameStartup(Graphics, Assets);
        if(!GameContext->Game)
        {
            //TODO(JJ): Diagnostic and error logging
            return false;
        }
        
        if(!WorldStartup(GameContext->Game))
        {
            //TODO(JJ): Diagnostic and error logging
            return false;
        }
        
        ak_u64* WorldIDs = (ak_u64*)(GameContext->Game->World+1);
        WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevEntities[0], &WorldManagement->EntityIndices[0], 0);
        WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevEntities[1], &WorldManagement->EntityIndices[1], 1);
        WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevPointLights[0], &WorldManagement->PointLightIndices[0], 0);
        WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevPointLights[1], &WorldManagement->PointLightIndices[1], 1);
        
        for(ak_u32 WorldIndex = 0; WorldIndex < 2; WorldIndex++)
        {
            AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
            {
                if(!AK_StringIsNullOrEmpty(DevEntity->LinkName))
                {
                    ak_u64* EntityID = GameContext->GameEntityNameHash[WorldIndex].Find(DevEntity->Name);
                    AK_Assert(EntityID, "Dev Entity has no game entity associated at startup");
                    
                    ak_u64* LinkID = GameContext->GameEntityNameHash[!WorldIndex].Find(DevEntity->LinkName);
                    AK_Assert(LinkID, "Dev Entity has a link dev entity that has no game entity associated at startup");
                    
                    entity* Entity = GameContext->Game->World->EntityStorage[WorldIndex].Get(*EntityID);
                    Entity->LinkID = *LinkID;
                }
            }
        }
        
        Editor->OldCameras[0] = Editor->Cameras[0];
        Editor->OldCameras[1] = Editor->Cameras[1];
        
        Editor->GizmoState.SelectedObject = {};
        
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //TODO(JJ): Stack trace exception handling
        AK_MessageBoxOk("Error", "Could not start the game. An unhandled exception occurred during startup.");
        
        //TODO(JJ): We want a more sophisticated cleanup when our actual editor/game allocators are created
        if(GameContext->Game)
        {
            if(GameContext->Game->World)
                AK_Free(GameContext->Game->World);
            AK_Free(GameContext->Game);
        }
        
        Editor->UI.GameUseDevCamera = false;
        
        Platform->UnloadWorldCode();
        Platform->UnloadGameCode();
        
        return false;
    }
}

extern "C" 
AK_EXPORT EDITOR_RUN(Editor_Run)
{
    assets* Assets = InitAssets(Platform->AssetPath);
    if(!Assets)
        return -1;
    
    editor* Editor = Editor_Initialize(Graphics, Context, Platform, DevPlatform, Assets);
    if(!Editor)
        return -1;
    
    ak_bool Initialized = false;
    
    ak_f32 dt = TARGET_DT;
    
    for(;;)
    { 
        ak_high_res_clock Start = AK_WallClock();
        ak_temp_arena TempArena = Editor->Scratch->BeginTemp();
        
        ak_f32 tInterpolated = INFINITY;
        
        if(!Editor_Update(Editor, Assets, Platform, DevPlatform, dt))
            return 0;
        ImGui::NewFrame();
        
#if SHOW_IMGUI_DEMO_WINDOW
        local ak_bool Internal__DemoWindow;
        ImGui::ShowDemoWindow((bool*)&Internal__DemoWindow);
#endif
        
        game_context* GameContext = &Editor->GameContext;
        world_management* WorldManagement = &Editor->WorldManagement;
        ui* UI = &Editor->UI;
        
        AK_Assert(!UI->EntitySpawnerOpen || !UI->LightSpawnerOpen, "Corrupted spawner state");
        
        ak_f32 LogHeight = 0;
        if(!GameContext->Game)
        {
            ak_v2i Resolution = Platform->GetResolution();
            
            dev_input* DevInput = &Editor->Input;
            
            if(IsDown(&DevInput->Ctrl))
            {
                if(IsDown(&DevInput->S)) 
                    WorldManagement->SetState(WORLD_MANAGEMENT_STATE_SAVE);
            }
            
            if(IsDown(&DevInput->Alt))
            {
                if(IsDown(&DevInput->L)) 
                    WorldManagement->SetState(WORLD_MANAGEMENT_STATE_LOAD);
                
                if(IsDown(&DevInput->N))
                    WorldManagement->SetState(WORLD_MANAGEMENT_STATE_CREATE);
                
                if(IsDown(&DevInput->D))
                    WorldManagement->SetState(WORLD_MANAGEMENT_STATE_DELETE);
            }
            
            WorldManagement->Update(Editor, Platform, DevPlatform, Assets);
            
            ak_f32 MenuHeight = 0;
            if(ImGui::BeginMainMenuBar())
            {
                if(ImGui::BeginMenu("Menu"))
                {
                    if(ImGui::MenuItem("Save World", "CTRL-S")) 
                        WorldManagement->SetState(WORLD_MANAGEMENT_STATE_SAVE);
                    
                    if(ImGui::MenuItem("Load World", "ALT-L")) 
                        WorldManagement->SetState(WORLD_MANAGEMENT_STATE_LOAD);
                    
                    if(ImGui::MenuItem("New World", "ALT-N")) 
                        WorldManagement->SetState(WORLD_MANAGEMENT_STATE_CREATE);
                    
                    if(ImGui::MenuItem("Delete World", "ALT-D")) 
                        WorldManagement->SetState(WORLD_MANAGEMENT_STATE_DELETE);
                    
                    if(ImGui::MenuItem("Set Default World"))
                        Editor_SetDefaultWorld(Editor);
                    
                    if(ImGui::MenuItem("Load Default World"))
                        Editor_LoadDefaultWorld(Editor, Assets, DevPlatform);
                    
                    ImGui::EndMenu();
                }
                
                ImGui::Text("Loaded World: %.*s", WorldManagement->CurrentWorldName.Length, WorldManagement->CurrentWorldName.Data);
                
                MenuHeight = ImGui::GetWindowHeight();
                ImGui::EndMainMenuBar();
            }
            
            ImGui::SetNextWindowPos(ImVec2(0, MenuHeight));
            if(ImGui::Begin("Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(UI_Button(AK_HashFunction("Play Game Button"), "Play"))
                {
                    Editor_PlayGame(Editor, Graphics, Assets, Platform, DevPlatform);
                }
                
                UI_Checkbox(AK_HashFunction("Draw Other World"), "Draw Other World", 
                            &UI->EditorDrawOtherWorld);
                
                UI_Checkbox(AK_HashFunction("Overlay Other World"), "Overlay Other World", 
                            &UI->EditorOverlayOtherWorld);
                
                UI_Checkbox(AK_HashFunction("Editor Draw Colliders"), "Draw Colliders", &UI->EditorDrawColliders);
                
                UI_Checkbox(AK_HashFunction("Editor Draw Grid"), "Draw Grid", 
                            &UI->EditorDrawGrid);
                
                ImGui::PushID(AK_HashFunction("Editor Render Modes"));
                UI_Combo("Render Modes", (ak_i32*)&UI->RenderModeType, Global_RenderModeStrings, AK_Count(Global_RenderModeStrings));
                ImGui::PopID();
                
                ImGui::PushID(AK_HashFunction("Editor View Modes"));
                UI_Combo("View Modes", (ak_i32*)&UI->ViewModeType, Global_ViewModeStrings, AK_Count(Global_ViewModeStrings));
                ImGui::PopID();
                
                ImGui::Text("Snap Settings");
                
                UI_Checkbox(AK_HashFunction("Snap Checkbox"),  "Snap", &Editor->GizmoState.ShouldSnap);
                
                ImGui::PushID(AK_HashFunction("Grid Size"));
                UI_Combo("Grid Size", (ak_i32*)&UI->EditorGridSizeIndex, Global_GridSizesText, AK_Count(Global_GridSizesText));
                ImGui::PopID();
                
                Editor->GizmoState.GridDistance = Global_GridSizes[UI->EditorGridSizeIndex];
                
                if(!Editor->GizmoState.ShouldSnap)
                    UI_PushDisabledItem();
                
                ImGui::PushID(AK_HashFunction("Scale Snap"));
                UI_Combo("Scale Snap", (ak_i32*)&UI->EditorScaleSnapIndex, Global_GridSizesText, AK_Count(Global_GridSizesText));
                ImGui::PopID();
                
                Editor->GizmoState.ScaleSnap = Global_GridSizes[UI->EditorScaleSnapIndex];
                
                ImGui::PushID(AK_HashFunction("Rotate Angle Snap"));
                UI_Combo("Rotate Angle Snap", (ak_i32*)&UI->EditorRotateSnapIndex, RotateAngleSnapsText, AK_Count(RotateAngleSnapsText));
                ImGui::PopID();
                
                Editor->GizmoState.RotationAngleSnap = RotateAngleSnaps[UI->EditorRotateSnapIndex];
                
                if(!Editor->GizmoState.ShouldSnap)
                    UI_PopDisabledItem();
                
                UI_SameLineLabel("Use Local Transforms");
                UI_Checkbox(AK_HashFunction("Use Local Transforms"), "", &Editor->GizmoState.UseLocalTransforms);
                
            }
            
            ak_f32 OptionHeight = ImGui::GetWindowHeight();
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2(0, MenuHeight+OptionHeight));
            if(ImGui::Begin("Spawners", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(ImGui::BeginTabBar("Spawner Tabs"))
                {
                    UI->EntitySpawnerOpen = ImGui::BeginTabItem("Entity");
                    if(UI->EntitySpawnerOpen)
                    {
                        UI_ResetSpawner(UI, &UI->LightSpawner);
                        UI_EntitySpawner(Editor, &UI->EntitySpawner, Assets);
                        ImGui::EndTabItem();
                    }
                    
                    UI->LightSpawnerOpen = ImGui::BeginTabItem("Light");
                    if(UI->LightSpawnerOpen)
                    {
                        UI_ResetSpawner(UI, &UI->EntitySpawner);
                        UI_LightSpawner(Editor, &UI->LightSpawner);
                        ImGui::EndTabItem();
                    }
                }
                
                ImGui::EndTabBar();
            }
            else
            {
                UI->EntitySpawnerOpen = false;
                UI->LightSpawnerOpen = false;
            }
            
            LogHeight = MenuHeight+OptionHeight+ImGui::GetWindowHeight();
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2((ak_f32)Resolution.x-Editor->ListerDim.x, 0));
            Editor->ListerDim = UI_ListerWindow(Editor);
            
            ImGui::SetNextWindowPos(ImVec2((ak_f32)Resolution.x-Editor->DetailsDim.x, Editor->ListerDim.y));
            
            Editor->DetailsDim = UI_DetailsWindow(Editor, Assets);
        }
        
        if(GameContext->Game)
        {
            frame_playback* FramePlayback = &Editor->FramePlayback;
            
            DevPlatform->HandleHotReload(Editor);
            
            Editor_UpdateGame(Editor, Platform, &tInterpolated);
            if(GameContext->Game)
            {
                ak_v2i Resolution = Platform->GetResolution();
                
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                if(ImGui::Begin("Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    if(UI_Button(AK_HashFunction("Stop Game Button"), "Stop"))
                    {
                        Editor_StopGame(Editor, Platform);
                    }
                    else
                    {
                        
                        UI_Checkbox(AK_HashFunction("Draw Other World"), "Draw Other World", 
                                    &UI->EditorDrawOtherWorld);
                        
                        UI_Checkbox(AK_HashFunction("Game Draw Colliders"), "Draw Colliders", &UI->GameDrawColliders);
                        
                        ak_bool OldUseDevCamera = UI->GameUseDevCamera;
                        UI_Checkbox(AK_HashFunction("Use Dev Camera"), "Use Dev Camera", 
                                    &UI->GameUseDevCamera);
                        if(OldUseDevCamera != UI->GameUseDevCamera)
                        {
                            if(UI->GameUseDevCamera)
                            {
                                game* Game = Editor->GameContext.Game;
                                Editor->Cameras[0] = Game->Cameras[0];
                                Editor->Cameras[1] = Game->Cameras[1];
                            }
                        }
                        
                        ImGui::PushID(AK_HashFunction("Editor Render Modes"));
                        UI_Combo("Render Modes", (ak_i32*)&UI->RenderModeType, Global_RenderModeStrings, AK_Count(Global_RenderModeStrings));
                        ImGui::PopID();
                        
                        FramePlayback->Update(Editor, Graphics, Assets, Platform, DevPlatform);
                        LogHeight = ImGui::GetWindowHeight();
                    }
                    ImGui::End(); 
                }
                
                if(GameContext->Game)
                    UI_GameLister(Editor);
            }
        }
        
        UI_Timers(Editor->TimedBlockEntries);
        UI_Logs(LogHeight);
        
        ImGui::Render();
        
        Editor_Render(Editor, Graphics, Platform, Assets, tInterpolated);
        
        Editor->Scratch->EndTemp(&TempArena);
        
        dt = (ak_f32)AK_GetElapsedTime(AK_WallClock(), Start);
    }
}



#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>