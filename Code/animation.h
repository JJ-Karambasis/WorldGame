#ifndef ANIMATION_H
#define ANIMATION_H

struct pose
{
    skeleton* Skeleton;
    joint_pose* LocalPoses;
    m4* GlobalPoses;
};

#endif