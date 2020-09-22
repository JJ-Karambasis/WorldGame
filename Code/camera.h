#ifndef CAMERA_H
#define CAMERA_H

#define CAMERA_FIELD_OF_VIEW AK_PI*0.3f
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 500.0f

struct camera_transform
{
    ak_v3f Translation;
    ak_m3f Orientation;
};

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

struct camera
{
    ak_v3f Velocity;
    ak_v3f Position;    
    ak_v3f FocalPoint;
    ak_m3f Orientation;
    ak_v3f AngularVelocity;
    ak_f32 Distance;
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

#endif