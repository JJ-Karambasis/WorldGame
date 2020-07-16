#ifndef CAMERA_H
#define CAMERA_H

#define CAMERA_FIELD_OF_VIEW PI*0.3f
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 1000.0f

struct game_camera
{
    spherical_coordinates Coordinates;
    v3f Target;    
    
    //CONFIRM(JJ): Can we delete these or are we going to change some of this, only the field of view is 
    //something I can consider but it may be something that all game cameras share and shouldn't be specific to the individual camera
    f32 FieldOfView;
    f32 ZNear;
    f32 ZFar;
};

struct camera
{
    v3f Velocity;
    v3f Position;    
    v3f FocalPoint;
    m3 Orientation;
    v3f AngularVelocity;
    f32 Distance;
};

inline rigid_transform_matrix 
GetCameraTransform(game_camera* Camera)
{
    rigid_transform_matrix Result = {};
    Result.Translation = Camera->Target+ToCartesianCoordinates(Camera->Coordinates);
    Result.Orientation = CreateBasis(Normalize(Result.Translation-Camera->Target));       
    return Result;
}

inline view_settings 
GetViewSettings(game_camera* Camera)
{
    view_settings Result = {};
    rigid_transform_matrix CameraTransform = GetCameraTransform(Camera);            
    Result.Position = CameraTransform.Translation;
    Result.Orientation = CameraTransform.Orientation;
    Result.FieldOfView = Camera->FieldOfView;
    Result.ZNear = Camera->ZNear;
    Result.ZFar = Camera->ZFar;
    return Result;
}

#endif