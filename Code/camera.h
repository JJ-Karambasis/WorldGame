#ifndef CAMERA_H
#define CAMERA_H

#define CAMERA_FIELD_OF_VIEW PI*0.3f
#define CAMERA_ZNEAR 0.01f
#define CAMERA_ZFAR 100.0f

struct camera
{
    v3f Velocity;
    v3f Position;    
    v3f FocalPoint;
    m3 Orientation;
    v3f AngularVelocity;
    f32 Distance;
};

#endif