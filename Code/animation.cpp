animation_controller 
CreateAnimationController(arena* Storage, skeleton* Skeleton)
{
    animation_controller Result = {};
    Result.Skeleton = Skeleton;
    Result.GlobalPoses = PushArray(Storage, Skeleton->JointCount, m4, Clear, 0);    
    Result.JointPoses = PushArray(Storage, Skeleton->JointCount, joint_pose, Clear, 0);
    for(u32 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++) { Result.GlobalPoses[JointIndex] = IdentityM4(); Result.JointPoses[JointIndex].Orientation = IdentityQuaternion(); }
    return Result;
}

inline joint_pose
InterpolatePose(joint_pose A, f32 t, joint_pose B)
{    
    ASSERT((t >= 0) && (t <= 1));    
    
    joint_pose Result;
    Result.Translation = Lerp(A.Translation, t, B.Translation);
    Result.Orientation = Lerp(A.Orientation, t, B.Orientation);
    return Result;
}

inline f32 
GetClipDuration(animation_clip* Clip)
{
    f32 Result = (f32)Clip->FrameCount * ANIMATION_HZ;
    return Result;
}

void GenerateGlobalPoses(animation_controller* Controller)
{
    skeleton* Skeleton = Controller->Skeleton;
    
    m4* JointToModels = PushArray(Skeleton->JointCount, m4, Clear, 0);
    for(u32 JointIndex = 0; JointIndex < Skeleton->JointCount; JointIndex++)
    {        
        joint* Joint = Skeleton->Joints + JointIndex;
        
        JointToModels[JointIndex] = TransformM4(Controller->JointPoses[JointIndex].Translation, 
                                                Controller->JointPoses[JointIndex].Orientation); 
        if(Joint->ParentIndex != NO_PARENT_JOINT)        
            JointToModels[JointIndex] *= JointToModels[Joint->ParentIndex];        
        
        Controller->GlobalPoses[JointIndex] = Joint->ModelToJoint*JointToModels[JointIndex];
    }
}