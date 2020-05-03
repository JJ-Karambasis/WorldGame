#ifndef ANIMATION_H
#define ANIMATION_H

struct joint_pose
{
    quaternion Orientation;
    v3f Translation;
};

struct pose
{
    skeleton* Skeleton;
    joint_pose* LocalPoses;
    m4* GlobalPoses;
};

#endif