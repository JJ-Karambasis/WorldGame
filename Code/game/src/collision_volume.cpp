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