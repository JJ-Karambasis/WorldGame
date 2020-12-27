#include "editor.h"

#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_demo.cpp>

#include <assets.cpp>
#include <game_common_source.cpp>
#include <src/graphics_state.cpp>

EDITOR_DEBUG_LOG(Editor_DebugLog)
{
    if(!Internal__LogArena)
        Internal__LogArena = AK_CreateArena();
    
    va_list Args;
    va_start(Args, Format);
    Internal__Logs.Add(AK_FormatString(Internal__LogArena, Format, Args));
    va_end(Args);
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

global editor_create_new_world_modal* Editor_CreateNewWorldModal;
global editor_load_world_modal* Editor_LoadWorldModal;
global editor_delete_world_modal* Editor_DeleteWorldModal;

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

gizmo_selected_object Editor_GizmoSelectedObject(ak_u64 ID, selected_object_type Type)
{
    gizmo_selected_object Result;
    Result.IsSelected = true;
    Result.SelectedObject.Type = Type;
    Result.SelectedObject.ID = ID;
    return Result;
}

#include "src/ui.cpp"
#include "src/world_management.cpp"
#include "src/dev_mesh.cpp"
#include "src/frame_playback.cpp"
#include "src/generated_string_templates.cpp"

dev_entity* selected_object::GetEntity(world_management* WorldManagement, ak_u32 WorldIndex)
{
    AK_Assert(Type == SELECTED_OBJECT_TYPE_ENTITY, "Cannot get entity of a selected object that is not an entity");
    dev_entity* Entity = WorldManagement->DevEntities[WorldIndex].Get(ID);
    return Entity;
}

dev_point_light* selected_object::GetPointLight(world_management* WorldManagement, ak_u32 WorldIndex)
{
    AK_Assert(Type == SELECTED_OBJECT_TYPE_LIGHT, "Cannot get point light of a selected object that is not a point light");
    dev_point_light* PointLight = WorldManagement->DevPointLights[WorldIndex].Get(ID);
    return PointLight;
}

ak_v3f selected_object::GetPosition(world_management* WorldManagement, ak_u32 WorldIndex)
{
    ak_v3f Result = {};
    switch(Type)
    {
        case SELECTED_OBJECT_TYPE_ENTITY:
        {
            dev_entity* Entity = GetEntity(WorldManagement, WorldIndex);
            Result = Entity->Transform.Translation;
        } break;
        
        case SELECTED_OBJECT_TYPE_LIGHT:
        {
            dev_point_light* PointLight = GetPointLight(WorldManagement, WorldIndex);
            Result = PointLight->Light.Position;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    return Result;
}

ray Editor_GetRayCastFromMouse(editor* Editor, view_settings* ViewSettings, ak_v2i Resolution)
{
    ak_m4f Perspective = AK_Perspective(ViewSettings->FieldOfView, AK_SafeRatio(Resolution.w, Resolution.h), ViewSettings->ZNear, ViewSettings->ZFar);
    ak_m4f View = AK_InvTransformM4(ViewSettings->Position, ViewSettings->Orientation);
    
    dev_input* Input = &Editor->Input;
    
    ray RayCast;
    RayCast.Origin = ViewSettings->Position;
    RayCast.Direction = Ray_PixelToWorld(Editor->Input.MouseCoordinates, Resolution, Perspective, View);
    
    return RayCast;
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

#define UPDATE_BEST_HIT(type, id) \
do \
{ \
if(RayCast.Intersected) \
{ \
if((tBest > RayCast.t) && (RayCast.t > ZNear)) \
{ \
tBest = RayCast.t; \
Result.IsSelected = true; \
Result.SelectedObject.Type = type; \
Result.SelectedObject.ID = id; \
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
        
        UPDATE_BEST_HIT(SELECTED_OBJECT_TYPE_ENTITY, DevEntity->ID);
    }
    
    AK_ForEach(DevPointLight, DevPointLights)
    {
        ray_cast RayCast = Ray_SphereCast(Ray.Origin, Ray.Direction, DevPointLight->Light.Position,
                                          POINT_LIGHT_RADIUS);
        UPDATE_BEST_HIT(SELECTED_OBJECT_TYPE_LIGHT, DevPointLight->ID);
    }
    
    
    return Result;
}

#undef UPDATE_BEST_HIT

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

ak_v3f Editor_GetSelectorDiff(editor* Editor, ray RayCast)
{
    ak_v3f Result = {};
    
    gizmo_state* GizmoState = &Editor->GizmoState;
    AK_Assert(GizmoState->SelectedObject.IsSelected, "Selected object must be selected to get selector diff");
    if(GizmoState->GizmoHit.Hit)
    {
        selected_object* SelectedObject = &GizmoState->SelectedObject.SelectedObject;
        
        ak_v3f SelectedObjectPosition = SelectedObject->GetPosition(&Editor->WorldManagement, Editor->CurrentWorldIndex);
        
        
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
            
            if(SelectedObject->Type == SELECTED_OBJECT_TYPE_ENTITY)
            {
                dev_entity* Entity = SelectedObject->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                
                if(GizmoState->UseLocalTransforms && GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_TRANSLATE)
                {
                    AK_Assert(SelectedObject->Type == SELECTED_OBJECT_TYPE_ENTITY, "Selector type must be an entity");
                    
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

selected_object* Editor_GetSelectedObject(editor* Editor)
{
    return Editor->GizmoState.SelectedObject.IsSelected ? &Editor->GizmoState.SelectedObject.SelectedObject : NULL;
}

void Editor_SelectObjects(editor* Editor, assets* Assets, ray RayCast, ak_v2i Resolution, ak_f32 ZNear)
{
    gizmo_state* GizmoState = &Editor->GizmoState;
    
    dev_input* Input = &Editor->Input;
    
    ak_u32 GizmoCount = (GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_ROTATE) ? 3 : 6;
    for(ak_u32 GizmoIndex = 0; GizmoIndex < GizmoCount; GizmoIndex++)
        GizmoState->Gizmos[GizmoIndex].IsHighLighted = false;
    
    if(!IsDown(&Input->Alt) && !ImGui::GetIO().WantCaptureMouse)
    {
        gizmo_intersection_result GizmoHitTest = Editor_CastToGizmos(Editor, GizmoState, RayCast, ZNear);
        
        if(GizmoHitTest.Hit) GizmoHitTest.Gizmo->IsHighLighted = true;
        
        if(IsPressed(&Input->LMB))
        {
            if(!GizmoHitTest.Hit)
            {
                GizmoState->GizmoHit = {};
                GizmoState->SelectedObject = Editor_CastToAllObjects(Editor, Assets, RayCast, ZNear);
            }
            else
            {
                AK_Assert(GizmoState->SelectedObject.IsSelected, "Cannot be selecting a gizmo without selecting an object");
                GizmoState->GizmoHit = GizmoHitTest;
                GizmoState->OriginalRotation = AK_IdentityM3<ak_f32>();
                selected_object* SelectedObject = &GizmoState->SelectedObject.SelectedObject;
                
                switch(SelectedObject->Type)
                {
                    case SELECTED_OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* Entity = SelectedObject->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                        GizmoState->OriginalRotation = AK_Transpose(AK_QuatToMatrix(Entity->Transform.Orientation));
                    } break;
                }
                
            }
        }
        
        if(IsReleased(&Input->LMB))
        {
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

void Editor_DrawGrid(editor* Editor, graphics* Graphics, ak_i32 xLeftBound, ak_i32 xRightBound, ak_i32 yTopBound, ak_i32 yBottomBound, ak_color3f Color, ak_f32 GridDistance)
{
    const ak_f32 EdgeThickness = 0.025f;
    for(ak_f32 x = 0; x <= xRightBound; x+= GridDistance)
    {
        if(x != 0)
        {
            Editor_DrawEdge(Editor, Graphics, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), EdgeThickness, Color);
        }
    }
    
    for(ak_f32 x = 0; x >= xLeftBound; x-=GridDistance)
    {
        if(x != 0)
        {
            Editor_DrawEdge(Editor, Graphics, AK_V3f(x, (ak_f32)yTopBound, 0.0f), AK_V3f(x, (ak_f32)yBottomBound, 0.0f), EdgeThickness, Color);
        }
    }
    
    for(ak_f32 y = 0; y <= yBottomBound; y+=GridDistance)
    {
        if(y != 0)
        {
            Editor_DrawEdge(Editor, Graphics, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), EdgeThickness, Color);
        }
    }
    
    for(ak_f32 y = 0; y >= yTopBound; y-=GridDistance)
    {
        if(y != 0)
        {
            Editor_DrawEdge(Editor, Graphics, AK_V3f((ak_f32)xLeftBound, y, 0.0f), AK_V3f((ak_f32)xRightBound, y, 0.0f), EdgeThickness, Color);
        }
    }
    
    Editor_DrawEdge(Editor, Graphics, AK_V3f(0.0f, (ak_f32)yTopBound, 0.0f), AK_V3f(0.0f, (ak_f32)yBottomBound, 0.0f), EdgeThickness, AK_Green3());
    Editor_DrawEdge(Editor, Graphics, AK_V3f((ak_f32)xLeftBound, 0.0f, 0.0f), AK_V3f((ak_f32)xRightBound, 0.0f, 0.0f), EdgeThickness, AK_Red3());
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

editor* Editor_Initialize(graphics* Graphics, ImGuiContext* Context, platform* Platform, assets* Assets)
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
    
    Editor->Cameras[0].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));    
    Editor->Cameras[1].SphericalCoordinates = AK_V3(6.0f, AK_ToRadians(90.0f), AK_ToRadians(-35.0f));
    
    Editor->Scratch->EndTemp(&TempArena);
    
    Editor_LoadWorldModal = LoadWorldModal_Stub;
    Editor_CreateNewWorldModal = CreateNewWorldModal_Stub;
    Editor_DeleteWorldModal = DeleteWorldModal_Stub;
    
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
    
    return Editor;
}

void Editor_RenderGrid(editor* Editor, graphics* Graphics, view_settings* ViewSettings, ak_v2i Resolution)
{
    ak_v3f* FrustumCorners = GetFrustumCorners(ViewSettings, Resolution);
    
    ak_v3f FrustumPlaneIntersectionPoints[4];
    ak_i8 IntersectedCount = 0;
    for(int i = 0; i < 4; i++)
    {
        ray FrustumRay = {};
        FrustumRay.Origin = FrustumCorners[i];
        FrustumRay.Direction = AK_Normalize(FrustumCorners[i + 4] - FrustumCorners[i]);
        
        ray_cast RayCast = Ray_PlaneCast(FrustumRay.Origin, FrustumRay.Direction, AK_ZAxis(), AK_V3<ak_f32>());
        
        if(RayCast.Intersected)
        {
            IntersectedCount++;            
            RayCast.t = AK_Min(RayCast.t, ViewSettings->ZFar);
            FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin + (FrustumRay.Direction * RayCast.t);
        }
        else
        {            
            FrustumPlaneIntersectionPoints[i] = FrustumRay.Origin;
        }
    }
    
    if(IntersectedCount != 0)
    {
        //if not all frustum rays intersected, we want to use the less efficient method for getting grid bounds
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
        
        Editor_DrawGrid(Editor, Graphics, AK_Floor(MinX), AK_Ceil(MaxX), AK_Floor(MinY), AK_Ceil(MaxY), AK_RGB(0.1f, 0.1f, 0.1f), Editor->GizmoState.GridDistance);
    }    
}

ak_bool Editor_Update(editor* Editor, assets* Assets, platform* Platform, dev_platform* DevPlatform, ak_f32 dt)
{
    if(!DevPlatform->Update(Editor, dt))
        return false;
    
    ak_v2i Resolution = Platform->GetResolution();
    
    dev_input* DevInput = &Editor->Input;
    
    graphics_camera* DevCamera = &Editor->Cameras[Editor->CurrentWorldIndex];
    ak_v2i MouseDelta = DevInput->MouseCoordinates - DevInput->LastMouseCoordinates;
    
    if(Editor->WorldManagement.NewState == WORLD_MANAGEMENT_STATE_NONE)
    {
        
        if(!Editor->Game || Editor->UI.GameUseDevCamera)
        {
            ak_v3f* SphericalCoordinates = &DevCamera->SphericalCoordinates;                
            
            ak_f32 Roll = 0;
            ak_f32 Pitch = 0;        
            
            ak_v2f PanDelta = AK_V2<ak_f32>();
            ak_f32 Scroll = 0;
            
            if(IsDown(&DevInput->Alt))
            {
                if(IsDown(&DevInput->LMB))
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
                
                if(IsDown(&DevInput->MMB))        
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
        }
        
        if(!Editor->Game)
        {
            if(IsPressed(&DevInput->Q))
            {
                Editor->CurrentWorldIndex = !Editor->CurrentWorldIndex;
                Editor->GizmoState.SelectedObject = {};
            }
            
            view_settings ViewSettings = GetViewSettings(DevCamera);
            ray RayCast = Editor_GetRayCastFromMouse(Editor, &ViewSettings, Resolution);
            
            Editor_SelectObjects(Editor, Assets, RayCast, Resolution, ViewSettings.ZNear);
            
            selected_object* SelectedObject = Editor_GetSelectedObject(Editor);
            if(SelectedObject)
            {
                if(SelectedObject->Type != SELECTED_OBJECT_TYPE_ENTITY)
                    Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_TRANSLATE;
                else
                {
                    if(IsPressed(&DevInput->W)) 
                        Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_TRANSLATE;
                    if(IsPressed(&DevInput->E)) 
                        Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_SCALE;
                    if(IsPressed(&DevInput->R)) 
                        Editor->GizmoState.TransformMode = SELECTOR_TRANSFORM_MODE_ROTATE;
                }
                
                ak_v3f SelectorDiff = Editor_GetSelectorDiff(Editor, RayCast);
                
                switch(SelectedObject->Type)
                {
                    case SELECTED_OBJECT_TYPE_ENTITY:
                    {
                        dev_entity* Entity = SelectedObject->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                        
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
                                Entity->Euler -= SelectorDiff;
                                Entity->Transform.Orientation *= Editor_GetOrientationDiff(Editor, SelectorDiff);
                            } break;
                        }
                    } break;
                }
                
                if(IsPressed(&DevInput->F))
                {
                    DevCamera->Target = SelectedObject->GetPosition(&Editor->WorldManagement, Editor->CurrentWorldIndex);
                }
                
                if(IsPressed(&DevInput->Delete))
                {
                    switch(SelectedObject->Type)
                    {
                        case SELECTED_OBJECT_TYPE_ENTITY:
                        {
                            Editor->WorldManagement.DeleteDevEntity(Editor->CurrentWorldIndex, SelectedObject->ID);
                        } break;
                        
                        case SELECTED_OBJECT_TYPE_LIGHT:
                        {
                            Editor->WorldManagement.DeleteDevPointLight(Editor->CurrentWorldIndex, 
                                                                        SelectedObject->ID);
                        } break;
                    }
                    
                    Editor->GizmoState.SelectedObject = {};
                }
            }
        }
    }
    
    
    DevInput->LastMouseCoordinates = DevInput->MouseCoordinates;
    DevInput->MouseCoordinates = {};
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
    
    selected_object* SelectedObject = Editor_GetSelectedObject(Editor);
    AK_Assert(SelectedObject, "Selected object must be present before rendering");
    
    if(SelectedObject->Type == SELECTED_OBJECT_TYPE_ENTITY)
    {
        if(GizmoState->UseLocalTransforms || GizmoState->TransformMode == SELECTOR_TRANSFORM_MODE_SCALE)
        {
            dev_entity* Entity = SelectedObject->GetEntity(&Editor->WorldManagement, Editor->CurrentWorldIndex);
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
    selected_object* SelectedObject = Editor_GetSelectedObject(Editor);
    if(SelectedObject)
    {
        PushDepth(Graphics, false);
        
        gizmo_state* GizmoState = &Editor->GizmoState;
        ak_v3f Position = SelectedObject->GetPosition(&Editor->WorldManagement, Editor->CurrentWorldIndex);
        
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

view_settings Editor_RenderDevWorld(editor* Editor, graphics* Graphics, assets* Assets, ak_u32 WorldIndex)
{
    graphics_render_buffer* RenderBuffer = Editor->RenderBuffers[WorldIndex];
    view_settings ViewSettings = GetViewSettings(&Editor->Cameras[WorldIndex]);
    graphics_light_buffer LightBuffer = {};
    
    world_management* WorldManagement = &Editor->WorldManagement;
    
    AK_ForEach(DevLight, &WorldManagement->DevPointLights[WorldIndex])
    {
        AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
        LightBuffer.PointLights[LightBuffer.PointLightCount++] = DevLight->Light;
    }
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
    PushLightBuffer(Graphics, &LightBuffer);
    AK_ForEach(DevEntity, &WorldManagement->DevEntities[WorldIndex])
    {
        graphics_material Material = ConvertToGraphicsMaterial(Assets, Graphics, &DevEntity->Material);
        graphics_mesh_id MeshHandle = GetOrLoadGraphicsMesh(Assets, Graphics, DevEntity->MeshID);
        
        PushMaterial(Graphics, Material);
        PushDrawMesh(Graphics, MeshHandle, AK_TransformM4(DevEntity->Transform), GetMeshIndexCount(Assets, DevEntity->MeshID), 0, 0);
    }
    
    AK_ForEach(PointLight, &WorldManagement->DevPointLights[WorldIndex])
    {
        Editor_DrawSphere(Editor, Graphics, PointLight->Light.Position, POINT_LIGHT_RADIUS, AK_Yellow3());
    }
    
    if(WorldIndex == (Editor->CurrentWorldIndex))
    {
        Editor_RenderSelectedObjectGizmos(Editor, Graphics);
        if(Editor->UI.EditorDrawGrid)
            Editor_RenderGrid(Editor, Graphics, &ViewSettings, RenderBuffer->Resolution);
    }
    
    return ViewSettings;
}

view_settings Editor_RenderGameWorld(editor* Editor, graphics* Graphics, game* Game, ak_u32 WorldIndex, ak_f32 tInterpolated)
{
    assets* Assets = Game->Assets;
    world* World = Game->World;
    
    graphics_camera* Camera = &Game->Cameras[WorldIndex];
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
    
    view_settings ViewSettings = GetViewSettings(Camera);
    
    graphics_light_buffer LightBuffer = {};
    AK_ForEach(Light, PointLightStorage)
    {
        AK_Assert(LightBuffer.PointLightCount < MAX_POINT_LIGHT_COUNT, "Point light overflow. Too many point lights being rendered");
        LightBuffer.PointLights[LightBuffer.PointLightCount++] = ToGraphicsPointLight(Light);
    }
    
    PushDepth(Graphics, true);
    PushSRGBRenderBufferWrites(Graphics, true);
    PushRenderBufferViewportScissorAndView(Graphics, RenderBuffer, &ViewSettings);    
    PushClearColorAndDepth(Graphics, AK_Black4(), 1.0f);
    PushCull(Graphics, GRAPHICS_CULL_MODE_BACK);
    
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
    
    return ViewSettings;
}

void Editor_Render(editor* Editor, graphics* Graphics, platform* Platform, assets* Assets, 
                   ak_f32 tInterpolated)
{
    
    ak_v2i Resolution = Platform->GetResolution();
    
    ak_u32 CurrentWorldIndex;
    if(Editor->Game)
        CurrentWorldIndex = Editor->Game->CurrentWorldIndex;
    else
        CurrentWorldIndex = Editor->CurrentWorldIndex;
    
    UpdateRenderBuffer(Graphics, &Editor->RenderBuffers[CurrentWorldIndex], Resolution);
    UpdateRenderBuffer(Graphics, &Editor->RenderBuffers[!CurrentWorldIndex], Resolution/5);
    
    if(Editor->Game)
    {
        view_settings ViewSettings = Editor_RenderGameWorld(Editor, Graphics, Editor->Game, CurrentWorldIndex, tInterpolated);
        if(Editor->UI.EditorDrawOtherWorld)
        {
            Editor_RenderGameWorld(Editor, Graphics, Editor->Game, !CurrentWorldIndex, tInterpolated);
            PushRenderBufferViewportScissorAndView(Graphics, Editor->RenderBuffers[CurrentWorldIndex], 
                                                   &ViewSettings);
        }
        
        if(Editor->UI.GameDrawColliders)
        {
            world* World = Editor->Game->World;
            
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
    
    switch(DevEntity->Type)
    {
        case ENTITY_TYPE_PLAYER:
        {
            Result = CreatePlayerEntity(Editor->Game, WorldIndex, DevEntity->Transform.Translation, 
                                        DevEntity->Material)->ID;
        } break;
        
        case ENTITY_TYPE_BUTTON:
        {
            Result = CreateButtonEntity(Editor->Game, WorldIndex, DevEntity->Transform.Translation, 
                                        DevEntity->Transform.Scale, DevEntity->Material, DevEntity->IsToggled)->ID;
        } break;
        
        case ENTITY_TYPE_STATIC:
        {
            Result = CreateStaticEntity(Editor->Game, WorldIndex, DevEntity->Transform.Translation, DevEntity->Transform.Scale, DevEntity->Transform.Orientation, 
                                        DevEntity->MeshID, DevEntity->Material)->ID;
        } break;
        
        AK_INVALID_DEFAULT_CASE;
    }
    
    ak_u32 Index = AK_PoolIndex(Result);
    ak_u32 IndexPlusOne = Index+1;
    if(IndexPlusOne > Editor->GameEntityNames[WorldIndex].Size)
        Editor->GameEntityNames[WorldIndex].Resize(IndexPlusOne*2);
    
    AK_MemoryCopy(Editor->GameEntityNames[WorldIndex][AK_PoolIndex(Result)], 
                  DevEntity->Name, MAX_OBJECT_NAME_LENGTH);
    Editor->GameEntityNameHash[WorldIndex].Insert(DevEntity->Name, Result);
    
    return Result;
}

ak_u64 GetEntryID(editor* Editor, dev_point_light* DevPointLight, ak_u32 WorldIndex)
{
    ak_u64 Result = CreatePointLight(Editor->Game, WorldIndex, DevPointLight->Light.Position, DevPointLight->Light.Radius, 
                                     DevPointLight->Light.Color, DevPointLight->Light.Intensity)->ID;
    return Result;
}

template <typename type>
ak_u32 Editor_BuildIDs(editor* Editor, ak_u64* WorldIDs, ak_pool<type>* Pool, ak_hash_map<ak_char*, ak_u32>* HashMap, ak_u32 WorldIndex)
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

void Editor_StopGame(editor* Editor, platform* Platform)
{
    Editor->Game->Shutdown(Editor->Game);
    Editor->Game = NULL;
    
    Editor->Cameras[0] = Editor->OldCameras[0];
    Editor->Cameras[1] = Editor->OldCameras[1];
    
    AK_DeleteHashMap(&Editor->GameEntityNameHash[0]);
    AK_DeleteHashMap(&Editor->GameEntityNameHash[1]);
    AK_DeleteArray(&Editor->GameEntityNames[0]);
    AK_DeleteArray(&Editor->GameEntityNames[1]);
    
    Editor->UI.GameUseDevCamera = false;
    
    Platform->UnloadGameCode();
    Platform->UnloadWorldCode();
}

ak_bool Editor_PlayGame(editor* Editor, graphics* Graphics, assets* Assets, platform* Platform, 
                        dev_platform* DevPlatform)
{
    world_management* WorldManagement = &Editor->WorldManagement;
    
    game_startup* GameStartup = Platform->LoadGameCode();
    world_startup* WorldStartup = Platform->LoadWorldCode(WorldManagement->CurrentWorldPath, WorldManagement->CurrentWorldName);
    if(!GameStartup || !WorldStartup)
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    if(!DevPlatform->SetGameDebugEditor(Editor))
    {
        //TODO(JJ): Diagnostic and error logging
        AK_InvalidCode();
        return false;
    }
    
    Editor->Game = GameStartup(Graphics, Assets);
    if(!Editor->Game)
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    if(!WorldStartup(Editor->Game))
    {
        //TODO(JJ): Diagnostic and error logging
        return false;
    }
    
    ak_u64* WorldIDs = (ak_u64*)(Editor->Game->World+1);
    WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevEntities[0], &WorldManagement->EntityIndices[0], 0);
    WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevEntities[1], &WorldManagement->EntityIndices[1], 1);
    WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevPointLights[0], &WorldManagement->PointLightIndices[0], 0);
    WorldIDs += Editor_BuildIDs(Editor, WorldIDs, &WorldManagement->DevPointLights[1], &WorldManagement->PointLightIndices[1], 1);
    
    Editor->OldCameras[0] = Editor->Cameras[0];
    Editor->OldCameras[1] = Editor->Cameras[1];
    
    Editor->GizmoState.SelectedObject = {};
    
    return true;
}

extern "C" 
AK_EXPORT EDITOR_RUN(Editor_Run)
{
    assets* Assets = InitAssets(Platform->AssetPath);
    if(!Assets)
        return -1;
    
    editor* Editor = Editor_Initialize(Graphics, Context, Platform, Assets);
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
        
        world_management* WorldManagement = &Editor->WorldManagement;
        
        ak_f32 LogHeight = 0;
        if(!Editor->Game)
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
                            &Editor->UI.EditorDrawOtherWorld);
                
                UI_Checkbox(AK_HashFunction("Editor Draw Colliders"), "Draw Colliders", &Editor->UI.EditorDrawColliders);
                
                UI_Checkbox(AK_HashFunction("Editor Draw Grid"), "Draw Grid", 
                            &Editor->UI.EditorDrawGrid);
                
                ImGui::Text("Snap Settings");
                
                UI_Checkbox(AK_HashFunction("Snap Checkbox"),  "Snap", &Editor->GizmoState.ShouldSnap);
                
                UI_SameLineLabel("Grid Size");
                const ak_f32 GridSizes[] = {0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
                const ak_char* GridSizesText[] = {"0.1", "0.25", "0.5", "1", "2", "5", "10"}; 
                UI_Combo(AK_HashFunction("Grid Size"), "", (int*)&Editor->UI.EditorGridSizeIndex, GridSizesText, AK_Count(GridSizesText));
                Editor->GizmoState.GridDistance = GridSizes[Editor->UI.EditorGridSizeIndex];
                
                if(!Editor->GizmoState.ShouldSnap)
                    UI_PushDisabledItem();
                
                UI_SameLineLabel("Scale Snap");
                UI_Combo(AK_HashFunction("Scale Snap"), "", (int*)&Editor->UI.EditorScaleSnapIndex, 
                         GridSizesText, AK_Count(GridSizesText));
                Editor->GizmoState.ScaleSnap = GridSizes[Editor->UI.EditorScaleSnapIndex];
                
                UI_SameLineLabel("Rotate Angle Snap");
                const ak_f32 RotateAngleSnaps[] = {1.0f, 5.0f, 10.0f, 20.0f, 45.0f, 90.0f, 180.0f};
                const ak_char* RotateAngleSnapsText[] = {"1", "5", "10", "20", "45", "90", "180"};
                UI_Combo(AK_HashFunction("Rotate Angle Snap"), "", (int*)&Editor->UI.EditorRotateSnapIndex, 
                         RotateAngleSnapsText, AK_Count(RotateAngleSnapsText));
                
                Editor->GizmoState.RotationAngleSnap = RotateAngleSnaps[Editor->UI.EditorRotateSnapIndex];
                
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
                    if(ImGui::BeginTabItem("Entity"))
                    {
                        UI_ResetSpawner(&Editor->UI, &Editor->UI.LightSpawner);
                        UI_EntitySpawner(Editor, &Editor->UI.EntitySpawner, Assets);
                        ImGui::EndTabItem();
                    }
                    
                    if(ImGui::BeginTabItem("Light"))
                    {
                        UI_ResetSpawner(&Editor->UI, &Editor->UI.EntitySpawner);
                        UI_LightSpawner(Editor, &Editor->UI.LightSpawner);
                        ImGui::EndTabItem();
                    }
                }
                
                ImGui::EndTabBar();
            }
            LogHeight = MenuHeight+OptionHeight+ImGui::GetWindowHeight();
            ImGui::End();
            
            ImGui::SetNextWindowPos(ImVec2((ak_f32)Resolution.x-Editor->ListerDim.x, 0));
            Editor->ListerDim = UI_ListerWindow(Editor);
            
            ImGui::SetNextWindowPos(ImVec2((ak_f32)Resolution.x-Editor->DetailsDim.x, Editor->ListerDim.y));
            
            Editor->DetailsDim = UI_DetailsWindow(Editor, Assets);
        }
        
        if(Editor->Game)
        {
            game* Game = Editor->Game;
            frame_playback* FramePlayback = &Editor->FramePlayback;
            
            if(!((FramePlayback->NewState == FRAME_PLAYBACK_STATE_RECORDING) ||
                 (FramePlayback->NewState == FRAME_PLAYBACK_STATE_NONE)))
            {
                Game->Input = {};
            }
            
            Game->LoopAccum.IncrementAccum();
            while(Game->LoopAccum.ShouldUpdate())
            {
                ak_temp_arena GameTemp = Game->Scratch->BeginTemp();
                
                if(FramePlayback->NewState == FRAME_PLAYBACK_STATE_RECORDING)
                    FramePlayback->RecordFrame(Editor);
                
                if((FramePlayback->NewState == FRAME_PLAYBACK_STATE_PLAYING) ||
                   (FramePlayback->NewState == FRAME_PLAYBACK_STATE_INSPECTING))
                    FramePlayback->PlayFrame(Editor);
                
                Game->Update(Game);
                
                Game->LoopAccum.DecrementAccum();
                Game->Scratch->EndTemp(&GameTemp);
            }
            
            tInterpolated = Game->LoopAccum.GetRemainder();
            
            UpdateButtons(Game->Input.Buttons, AK_Count(Game->Input.Buttons));
            
            
            ak_v2i Resolution = Platform->GetResolution();
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            if(ImGui::Begin("Options", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if(UI_Button(AK_HashFunction("Stop Game Button"), "Stop"))
                {
                    Editor_StopGame(Editor, Platform);
                }
                
                UI_Checkbox(AK_HashFunction("Draw Other World"), "Draw Other World", 
                            &Editor->UI.EditorDrawOtherWorld);
                
                UI_Checkbox(AK_HashFunction("Game Draw Colliders"), "Draw Colliders", &Editor->UI.GameDrawColliders);
                
                ak_bool OldUseDevCamera = Editor->UI.GameUseDevCamera;
                UI_Checkbox(AK_HashFunction("Use Dev Camera"), "Use Dev Camera", 
                            &Editor->UI.GameUseDevCamera);
                if(OldUseDevCamera != Editor->UI.GameUseDevCamera)
                {
                    if(Editor->UI.GameUseDevCamera)
                    {
                        Editor->Cameras[0] = Game->Cameras[0];
                        Editor->Cameras[1] = Game->Cameras[1];
                    }
                }
                
                FramePlayback->Update(Editor, Graphics, Assets, Platform, DevPlatform);
            }
            LogHeight = ImGui::GetWindowHeight();
            ImGui::End(); 
        }
        
        UI_Logs(LogHeight);
        
        ImGui::Render();
        
        Editor_Render(Editor, Graphics, Platform, Assets, tInterpolated);
        
        Editor->Scratch->EndTemp(&TempArena);
        
        dt = (ak_f32)AK_GetElapsedTime(AK_WallClock(), Start);
    }
}



#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>