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