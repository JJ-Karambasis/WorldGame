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

void SetAnimationFrame(pose* Pose, animation_clip* Clip, u32 FrameIndex)
{
    skeleton* Skeleton = Pose->Skeleton;
    ASSERT(Clip->JointCount == Skeleton->JointCount);        
    
    animation_frame* Frame = Clip->Frames + FrameIndex;
    
    u32 JointCount = Clip->JointCount;
    
    m4* JointToModels = PushArray(JointCount, m4, Clear, 0);
    for(u32 JointIndex = 0; JointIndex < JointCount; JointIndex++)
    {
        joint_pose JointPose = Frame->JointPoses[JointIndex];
        joint* Joint = Skeleton->Joints + JointIndex;        
        
        m4 JointToParent = TransformM4(JointPose.Translation, JointPose.Orientation);        
        if(Joint->ParentIndex != NO_PARENT_JOINT)
        {   
            JointToModels[JointIndex] = JointToParent*JointToModels[Joint->ParentIndex];                
        }
        else
        {   
            JointToModels[JointIndex] = JointToParent;            
        }
        
        Pose->GlobalPoses[JointIndex] = Joint->ModelToJoint * JointToModels[JointIndex];
    }
}