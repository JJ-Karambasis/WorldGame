#ifndef ANIMATION_H
#define ANIMATION_H

struct pose
{
    skeleton* Skeleton;
    joint_pose* LocalPoses;
    m4* GlobalPoses;
};

struct playing_animation
{
    animation_clip* Clip;   
    f32 t;        
};

struct animation_controller
{
    skeleton* Skeleton;        
    joint_pose* JointPoses;
    m4* GlobalPoses;        
    playing_animation PlayingAnimation;
};

#endif