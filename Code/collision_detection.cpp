collision_volume CreateSphereCollisionVolume(v3f CenterP, f32 Radius)
{
    collision_volume Result;
    Result.Type = COLLISION_VOLUME_TYPE_SPHERE;
    Result.Sphere.CenterP = CenterP;
    Result.Sphere.Radius = Radius;        
    return Result;
}

collision_volume CreateCapsuleCollisionVolume(v3f BottomPosition, f32 Radius, f32 Height)
{
    collision_volume Result;
    Result.Type = COLLISION_VOLUME_TYPE_CAPSULE;
    Result.Capsule.P0 = BottomPosition + Global_WorldZAxis*Radius;
    Result.Capsule.P1 = Result.Capsule.P0 + Global_WorldZAxis*Height;
    Result.Capsule.Radius = Radius;
    return Result;
}

collision_volume CreateConvexHullCollisionVolume(convex_hull* ConvexHull, v3f Position, quaternion Orientation)
{
    collision_volume Result;
    Result.Type = COLLISION_VOLUME_TYPE_CONVEX_HULL;
    Result.ConvexHull = ConvexHull;
    Result.Transform = {Position, Orientation};
    return Result;
}

capsule TransformCapsule(capsule* Capsule, rigid_transform Transform)
{
    capsule Result;
    Result.P0 = TransformV3(Capsule->P0, Transform);
    Result.P1 = TransformV3(Capsule->P1, Transform);
    Result.Radius = Capsule->Radius;
    return Result;
}

sphere TransformSphere(sphere* Sphere, rigid_transform Transform)
{
    sphere Result = {};
    Result.Radius = Sphere->Radius;
    Result.CenterP = Sphere->CenterP + Transform.Translation;    
    return Result;
}

plane3D CreateSlidingPlane(collision_result* CollisionResult)
{
    plane3D Result = {CollisionResult->Normal, FindPlaneDistance(CollisionResult->ContactPoint, CollisionResult->Normal)}; 
    return Result;
}

collision_result ColliderTest(collision_volume* VolumeA, collision_volume* VolumeB)
{    
    switch(VolumeA->Type)
    {
        case COLLISION_VOLUME_TYPE_SPHERE:
        {
        } break;
        
        case COLLISION_VOLUME_TYPE_CAPSULE:
        {
        } break;
        
        case COLLISION_VOLUME_TYPE_CONVEX_HULL:
        {
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    return InvalidCollisionResult();
}

collision_result HandleWorldCollisions(world* World, world_entity* AEntity)
{
    collision_result Result = InvalidCollisionResult();
    FOR_EACH(BEntity, &World->EntityPool)
    {
        if((BEntity != AEntity) && (BEntity->CollisionVolume.Type != COLLISION_VOLUME_TYPE_NONE))
        {
            {
                collision_result CollisionResult = ColliderTest(&AEntity->CollisionVolume, &BEntity->CollisionVolume);
                if(CollisionResult.t < Result.t)
                    Result = CollisionResult;
            }
        }
    }
    
    return Result;
}