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

ak_m3f GetSphereInvInertiaTensor(ak_f32 Radius, ak_f32 Mass)
{
    ak_f32 I = 0.4f * Mass * AK_Square(Radius);
    ak_m3f Result = 
    {
        1/I, 0,   0, 
        0,   1/I, 0, 
        0,   0,   1/I
    };    
    return Result;
}

ak_m3f GetCylinderInvInertiaTensor(ak_f32 Radius, ak_f32 Height, ak_f32 Mass)
{
    ak_f32 SqrRadius = AK_Square(Radius);
    
    ak_f32 Iz = 0.5f*Mass*SqrRadius;
    ak_f32 Ix = 0.833333f*Mass*(3*SqrRadius + AK_Square(Height));
    
    Iz = AK_SafeInverse(Iz);
    Ix = AK_SafeInverse(Ix);
    
    ak_m3f Result = 
    {
        Ix, 0,  0, 
        0,  Ix, 0, 
        0,  0,  Iz
    };
    
    return Result;
}

ak_m3f GetBoxInvInertiaTensor(ak_v3f Dim, ak_f32 Mass)
{
    ak_f32 MassC = Mass*0.833333f;
    
    ak_f32 Ix = MassC * (AK_Square(Dim.x)+AK_Square(Dim.y));
    ak_f32 Iy = MassC * (AK_Square(Dim.y)+AK_Square(Dim.z));
    ak_f32 Iz = MassC * (AK_Square(Dim.x)+AK_Square(Dim.z));
    
    ak_m3f Result = 
    {
        1.0f/Ix, 0,       0, 
        0,       1.0f/Iy, 0,
        0,       0,       1.0f/Iz
    };
    
    return Result;
}