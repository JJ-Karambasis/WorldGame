#ifndef CAMERA_H
#define CAMERA_H

struct game_camera
{
    //TODO(JJ): We will probably need to convert the game camera orientation to a quaternion instead of spherical coordinates
    //to interpolate unless we can interpolate the spherical coordinates themselves for camera updating
    ak_v3f SphericalCoordinates;
    ak_v3f Target;    
    
    //CONFIRM(JJ): Can we delete these or are we going to change some of this, only the field of view is 
    //something I can consider but it may be something that all game cameras share and shouldn't be specific to the individual camera
    ak_f32 FieldOfView;
    ak_f32 ZNear;
    ak_f32 ZFar;
};

inline camera_transform
GetCameraTransform(game_camera* Camera)
{
    camera_transform Result = {};
    Result.Translation = Camera->Target+AK_SphericalToCartesian(Camera->SphericalCoordinates);    
    Result.Orientation = AK_OrientAt(Result.Translation, Camera->Target);       
    return Result;
}

inline view_settings 
GetViewSettings(game_camera* Camera)
{
    view_settings Result = {};
    camera_transform CameraTransform = GetCameraTransform(Camera);            
    Result.Position = CameraTransform.Translation;
    Result.Orientation = CameraTransform.Orientation;
    Result.FieldOfView = Camera->FieldOfView;
    Result.ZNear = Camera->ZNear;
    Result.ZFar = Camera->ZFar;
    return Result;
}

inline ak_v3f* 
GetFrustumCorners(view_settings* ViewSettings, ak_v2i RenderDim)
{
    ak_arena* GlobalArena = AK_GetGlobalArena();
    ak_v3f* Result = GlobalArena->PushArray<ak_v3f>(8);
    ak_m4f Perspective = AK_Perspective(ViewSettings->FieldOfView, AK_SafeRatio(RenderDim.w, RenderDim.h), ViewSettings->ZNear, ViewSettings->ZFar);    
    AK_GetFrustumCorners(Result, Perspective);
    AK_TransformPoints(Result, 8, AK_TransformM4(ViewSettings->Position, ViewSettings->Orientation));    
    return Result;
}

#endif