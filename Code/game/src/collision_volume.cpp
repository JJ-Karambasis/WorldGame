inline sphere 
CreateSphere(ak_v3f CenterP, ak_f32 Radius) 
{
    sphere Sphere = {CenterP, Radius};
    return Sphere;
}

inline capsule 
CreateCapsule(ak_v3f P0, ak_v3f P1, ak_f32 Radius)
{
    capsule Capsule = {P0, P1, Radius};
    return Capsule;
}

inline capsule 
CreateCapsule(ak_v3f Bottom, ak_f32 Height, ak_f32 Radius)
{
    ak_v3f P0 = Bottom + AK_ZAxis()*Radius;
    return CreateCapsule(P0, P0+AK_ZAxis()*Height, Radius);
}

void AddConvexHullVolume(physics_object* PhysicsObject, ak_pool<collision_volume>* Storage, convex_hull ConvexHull)
{
    ak_u64 VolumeID = Storage->Allocate();
    collision_volume* Volume = Storage->Get(VolumeID);
    
    Volume->ID = VolumeID;
    Volume->NextID = PhysicsObject->CollisionVolumeID;
    Volume->Type = COLLISION_VOLUME_TYPE_CONVEX_HULL;
    Volume->ConvexHull = ConvexHull;
    
    PhysicsObject->CollisionVolumeID = VolumeID;
}

void AddCapsuleVolume(physics_object* PhysicsObject, ak_pool<collision_volume>* Storage, capsule Capsule)
{
    ak_u64 VolumeID = Storage->Allocate();
    collision_volume* Volume = Storage->Get(VolumeID);
    
    Volume->ID = VolumeID;
    Volume->NextID = PhysicsObject->CollisionVolumeID;
    Volume->Type = COLLISION_VOLUME_TYPE_CAPSULE;
    Volume->Capsule = Capsule;
    
    PhysicsObject->CollisionVolumeID = VolumeID;
}

void AddSphereVolume(physics_object* PhysicsObject, ak_pool<collision_volume>* Storage, sphere Sphere)
{
    ak_u64 VolumeID = Storage->Allocate();
    collision_volume* Volume = Storage->Get(VolumeID);
    
    Volume->ID = VolumeID;
    Volume->NextID = PhysicsObject->CollisionVolumeID;
    Volume->Type = COLLISION_VOLUME_TYPE_SPHERE;
    Volume->Sphere = Sphere;
    
    PhysicsObject->CollisionVolumeID = VolumeID;
}


capsule TransformCapsule(capsule* Capsule, ak_sqtf Transform)
{
    capsule Result;
    
    ak_v3f ZScale = AK_V3(1.0f, 1.0f, Transform.Scale.z);
    Result.P0 = AK_Transform(Capsule->P0, Transform.Translation, Transform.Orientation, ZScale);
    Result.P1 = AK_Transform(Capsule->P1, Transform.Translation, Transform.Orientation, ZScale);
    
    ak_u32 Component = Transform.Scale.xy.LargestComp();
    Result.Radius = Capsule->Radius*Transform.Scale[Component];
    return Result;
}

sphere TransformSphere(sphere* Sphere, ak_sqtf Transform)
{
    sphere Result = {};
    
    ak_u32 Component = Transform.Scale.LargestComp();    
    Result.Radius = Sphere->Radius*Transform.Scale[Component];        
    Result.CenterP = AK_Transform(Sphere->CenterP, Transform);
    return Result;
}

inline void 
TranslateCapsule(capsule* Capsule, ak_v3f Delta)
{
    Capsule->P0 += Delta;
    Capsule->P1 += Delta;
}

inline void
TranslateSphere(sphere* Sphere, ak_v3f Delta)
{
    Sphere->CenterP += Delta;
}