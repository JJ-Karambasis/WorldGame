#define TOI_FUNCTION(name) ak_bool name(ak_f32* t, physics_object* ObjectA, physics_object* ObjectB, collision_volume* VolumeA, collision_volume* VolumeB) 
typedef TOI_FUNCTION(toi_function);

TOI_FUNCTION(CollisionTable_SphereSphereTOI)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return SphereSphereTOI(t, &SphereA, MoveDeltaA, &SphereB, MoveDeltaB);
}

TOI_FUNCTION(CollisionTable_SphereCapsuleTOI)
{
    sphere  SphereA  = TransformSphere(&VolumeA->Sphere,    ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return SphereCapsuleTOI(t, &SphereA, MoveDeltaA, &CapsuleB, MoveDeltaB);
}

TOI_FUNCTION(CollisionTable_SphereHullTOI)
{
    sphere  SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    ak_v3f  MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f  MoveDeltaB = ObjectB->MoveDelta;
    return SphereHullTOI(t, &SphereA, MoveDeltaA, &VolumeB->ConvexHull, HullTransformB, MoveDeltaB);    
}

TOI_FUNCTION(CollisionTable_CapsuleSphereTOI)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    sphere  SphereB  = TransformSphere(&VolumeB->Sphere,    ObjectB->Transform);    
    ak_v3f    MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f    MoveDeltaB = ObjectB->MoveDelta;
    return SphereCapsuleTOI(t, &SphereB, MoveDeltaB, &CapsuleA, MoveDeltaA);
}

TOI_FUNCTION(CollisionTable_CapsuleCapsuleTOI)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return CapsuleCapsuleTOI(t, &CapsuleA, MoveDeltaA, &CapsuleB, MoveDeltaB);
}

TOI_FUNCTION(CollisionTable_CapsuleHullTOI)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return CapsuleHullTOI(t, &CapsuleA, MoveDeltaA, &VolumeB->ConvexHull, HullTransformB, MoveDeltaB);
}

TOI_FUNCTION(CollisionTable_HullSphereTOI)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return SphereHullTOI(t, &SphereB, MoveDeltaB, &VolumeA->ConvexHull, HullTransformA, MoveDeltaA);
}

TOI_FUNCTION(CollisionTable_HullCapsuleTOI)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return CapsuleHullTOI(t, &CapsuleB, MoveDeltaB, &VolumeA->ConvexHull, HullTransformA, MoveDeltaA);
}

TOI_FUNCTION(CollisionTable_HullHullTOI)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    ak_v3f MoveDeltaA = ObjectA->MoveDelta;
    ak_v3f MoveDeltaB = ObjectB->MoveDelta;
    return HullHullTOI(t, &VolumeA->ConvexHull, HullTransformA, MoveDeltaA, &VolumeB->ConvexHull, HullTransformB, MoveDeltaB);
}

global toi_function* TOIFunctions[3][3] = 
{
    {CollisionTable_SphereSphereTOI, CollisionTable_SphereCapsuleTOI, CollisionTable_SphereHullTOI},
    {CollisionTable_CapsuleSphereTOI, CollisionTable_CapsuleCapsuleTOI, CollisionTable_CapsuleHullTOI},
    {CollisionTable_HullSphereTOI, CollisionTable_HullCapsuleTOI, CollisionTable_HullHullTOI}
};

#define CCD_CONTACT_FUNCTION(name) contact name(physics_object* ObjectA, physics_object* ObjectB, collision_volume* VolumeA, collision_volume* VolumeB, ak_f32 tHit)
typedef CCD_CONTACT_FUNCTION(ccd_contact_function);

CCD_CONTACT_FUNCTION(CollisionTable_SphereSphereContactCCD)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    SphereA.CenterP += ObjectA->MoveDelta*tHit;
    SphereB.CenterP += ObjectB->MoveDelta*tHit;
    return GetSphereSphereDeepestContact(&SphereA, &SphereB);
}

CCD_CONTACT_FUNCTION(CollisionTable_SphereCapsuleContactCCD)
{
    sphere SphereA   = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    SphereA.CenterP += ObjectA->MoveDelta*tHit;
    TranslateCapsule(&CapsuleB, ObjectB->MoveDelta*tHit);    
    return GetSphereCapsuleDeepestContact(&SphereA, &CapsuleB);
}

CCD_CONTACT_FUNCTION(CollisionTable_SphereHullContactCCD)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    ak_sqtf ConvexHullTransform = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    SphereA.CenterP += ObjectA->MoveDelta*tHit;
    ConvexHullTransform.Translation += ObjectB->MoveDelta*tHit;    
    return GetSphereHullDeepestContact(&SphereA, &VolumeB->ConvexHull, ConvexHullTransform);
}

CCD_CONTACT_FUNCTION(CollisionTable_CapsuleSphereContactCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    sphere SphereB   = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);    
    TranslateCapsule(&CapsuleA, ObjectA->MoveDelta*tHit);    
    SphereB.CenterP += ObjectB->MoveDelta*tHit;    
    contact Contact = GetSphereCapsuleDeepestContact(&SphereB, &CapsuleA);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CollisionTable_CapsuleCapsuleContactCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    TranslateCapsule(&CapsuleA, ObjectA->MoveDelta*tHit);
    TranslateCapsule(&CapsuleB, ObjectB->MoveDelta*tHit);
    return GetCapsuleCapsuleDeepestContact(&CapsuleA, &CapsuleB);
}

CCD_CONTACT_FUNCTION(CollisionTable_CapsuleHullContactCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    ak_sqtf ConvexHullTransform = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    TranslateCapsule(&CapsuleA, ObjectA->MoveDelta*tHit);
    ConvexHullTransform.Translation += ObjectB->MoveDelta*tHit;    
    return GetCapsuleHullDeepestContact(&CapsuleA, &VolumeB->ConvexHull, ConvexHullTransform);
}

CCD_CONTACT_FUNCTION(CollisionTable_HullSphereContactCCD)
{
    ak_sqtf ConvexHullTransform = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    ConvexHullTransform.Translation += ObjectA->MoveDelta*tHit;
    SphereB.CenterP += ObjectB->MoveDelta*tHit;
    contact Contact = GetSphereHullDeepestContact(&SphereB, &VolumeA->ConvexHull, ConvexHullTransform);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CollisionTable_HullCapsuleContactCCD)
{
    ak_sqtf ConvexHullTransform = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    ConvexHullTransform.Translation += ObjectA->MoveDelta*tHit;        
    TranslateCapsule(&CapsuleB, ObjectB->MoveDelta*tHit);    
    contact Contact = GetCapsuleHullDeepestContact(&CapsuleB, &VolumeA->ConvexHull, ConvexHullTransform);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CollisionTable_HullHullContactCCD)
{
    
    ak_sqtf ConvexHullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    ak_sqtf ConvexHullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    ConvexHullTransformA.Translation += ObjectA->MoveDelta*tHit;
    ConvexHullTransformB.Translation += ObjectB->MoveDelta*tHit;
    contact Contact = GetHullHullDeepestContact(&VolumeA->ConvexHull, ConvexHullTransformA, &VolumeB->ConvexHull, ConvexHullTransformB);        
    return Contact;
}

global ccd_contact_function* CCDContactFunctions[3][3] = 
{
    {CollisionTable_SphereSphereContactCCD, CollisionTable_SphereCapsuleContactCCD, CollisionTable_SphereHullContactCCD},
    {CollisionTable_CapsuleSphereContactCCD, CollisionTable_CapsuleCapsuleContactCCD, CollisionTable_CapsuleHullContactCCD},
    {CollisionTable_HullSphereContactCCD, CollisionTable_HullCapsuleContactCCD, CollisionTable_HullHullContactCCD}
};

#define INTERSECTION_FUNCTION(name) ak_bool name(physics_object* ObjectA, physics_object* ObjectB, collision_volume* VolumeA, collision_volume* VolumeB)
typedef INTERSECTION_FUNCTION(intersection_function);

INTERSECTION_FUNCTION(CollisionTable_SphereSphereIntersection)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    return SphereSphereOverlap(&SphereA, &SphereB);
}

INTERSECTION_FUNCTION(CollisionTable_SphereCapsuleIntersection)
{
    sphere  SphereA  = TransformSphere(&VolumeA->Sphere,    ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    return SphereCapsuleOverlap(&SphereA, &CapsuleB);
}

INTERSECTION_FUNCTION(CollisionTable_SphereHullIntersection)
{
    sphere  SphereA = TransformSphere(&VolumeA->Sphere, ObjectA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    return SphereHullOverlap(&SphereA, &VolumeB->ConvexHull, HullTransformB);
}

INTERSECTION_FUNCTION(CollisionTable_CapsuleSphereIntersection)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    sphere  SphereB  = TransformSphere(&VolumeB->Sphere,    ObjectB->Transform);    
    return CapsuleSphereOverlap(&CapsuleA, &SphereB);
}

INTERSECTION_FUNCTION(CollisionTable_CapsuleCapsuleIntersection)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    return CapsuleCapsuleOverlap(&CapsuleA, &CapsuleB);
}

INTERSECTION_FUNCTION(CollisionTable_CapsuleHullIntersection)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, ObjectA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;
    return CapsuleHullOverlap(&CapsuleA, &VolumeB->ConvexHull, HullTransformB);
}

INTERSECTION_FUNCTION(CollisionTable_HullSphereIntersection)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    sphere SphereB = TransformSphere(&VolumeB->Sphere, ObjectB->Transform);
    return HullSphereOverlap(&VolumeA->ConvexHull, HullTransformA, &SphereB);
}

INTERSECTION_FUNCTION(CollisionTable_HullCapsuleIntersection)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, ObjectB->Transform);
    return HullCapsuleOverlap(&VolumeA->ConvexHull, HullTransformA, &CapsuleB);
}

INTERSECTION_FUNCTION(CollisionTable_HullHullIntersection)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull.Header.Transform*ObjectA->Transform;
    ak_sqtf HullTransformB = VolumeB->ConvexHull.Header.Transform*ObjectB->Transform;    
    return HullHullOverlap(&VolumeA->ConvexHull, HullTransformA, &VolumeB->ConvexHull, HullTransformB);
}

global intersection_function* IntersectionFunctions[3][3] = 
{
    {CollisionTable_SphereSphereIntersection, CollisionTable_SphereCapsuleIntersection, CollisionTable_SphereHullIntersection}, 
    {CollisionTable_CapsuleSphereIntersection, CollisionTable_CapsuleCapsuleIntersection, CollisionTable_CapsuleHullIntersection}, 
    {CollisionTable_HullSphereIntersection, CollisionTable_HullCapsuleIntersection, CollisionTable_HullHullIntersection}
};