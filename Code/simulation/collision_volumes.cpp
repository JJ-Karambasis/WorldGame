void AttachToCollisionVolume(collision_volume* CollisionVolume, convex_hull* ConvexHull)
{
    CollisionVolume->Type = COLLISION_VOLUME_TYPE_CONVEX_HULL;
    CollisionVolume->ConvexHull = ConvexHull;
}

void AttachToCollisionVolume(collision_volume* CollisionVolume, sphere* Sphere)
{
    CollisionVolume->Type = COLLISION_VOLUME_TYPE_SPHERE;
    CollisionVolume->Sphere = *Sphere;
}

void AttachToCollisionVolume(collision_volume* CollisionVolume, capsule* Capsule)
{
    CollisionVolume->Type = COLLISION_VOLUME_TYPE_CAPSULE;
    CollisionVolume->Capsule = *Capsule;
}

capsule TransformCapsule(capsule* Capsule, sqt Transform)
{
    capsule Result;
    
    v3f ZScale = V3(1.0f, 1.0f, Transform.Scale.z);
    Result.P0 = TransformV3(Capsule->P0, Transform.Translation, Transform.Orientation, ZScale);
    Result.P1 = TransformV3(Capsule->P1, Transform.Translation, Transform.Orientation, ZScale);
    
    u32 Component = Transform.Scale.xy.LargestComponent();
    Result.Radius = Capsule->Radius*Transform.Scale[Component];
    return Result;
}

sphere TransformSphere(sphere* Sphere, sqt Transform)
{
    sphere Result = {};
    
    u32 Component = Transform.Scale.LargestComponent();    
    Result.Radius = Sphere->Radius*Transform.Scale[Component];        
    Result.CenterP = TransformV3(Sphere->CenterP, Transform);
    return Result;
}

inline void 
TranslateCapsule(capsule* Capsule, v3f Delta)
{
    Capsule->P0 += Delta;
    Capsule->P1 += Delta;
}

inline void
TranslateSphere(sphere* Sphere, v3f Delta)
{
    Sphere->CenterP += Delta;
}

m3 GetSphereInvInertiaTensor(f32 Radius, f32 Mass)
{
    f32 I = 0.4f * Mass * Square(Radius);
    m3 Result = 
    {
        1/I, 0,   0, 
        0,   1/I, 0, 
        0,   0,   1/I
    };    
    return Result;
}