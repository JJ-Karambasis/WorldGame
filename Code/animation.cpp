pose CreatePose(arena* Storage, skeleton* Skeleton)
{
    pose Result = {};
    Result.Skeleton = Skeleton;
    Result.LocalPoses = PushArray(Storage, Skeleton->JointCount, joint_pose, Clear, 0);
    Result.GlobalPoses = PushArray(Storage, Skeleton->JointCount, m4, Clear, 0);
    
    for(u32 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {
        Result.LocalPoses[JointIndex].Orientation = IdentityQuaternion();
        Result.GlobalPoses[JointIndex] = IdentityM4();
    }
    
    return Result;
}