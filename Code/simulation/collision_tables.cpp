#define CCD_FUNCTION(name) ak_f32 name(sim_entity* SimEntityA, sim_entity* SimEntityB, collision_volume* VolumeA, collision_volume* VolumeB) 
typedef CCD_FUNCTION(ccd_function);

CCD_FUNCTION(SphereSphereCCD)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return SphereSphereTOI(&SphereA, MoveDeltaA, &SphereB, MoveDeltaB);
}

CCD_FUNCTION(SphereCapsuleCCD)
{
    sphere  SphereA  = TransformSphere(&VolumeA->Sphere,    SimEntityA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return SphereCapsuleTOI(&SphereA, MoveDeltaA, &CapsuleB, MoveDeltaB);
}

CCD_FUNCTION(SphereHullCCD)
{
    sphere  SphereA = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    ak_v3f  MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f  MoveDeltaB = SimEntityB->GetMoveDelta();
    return SphereHullTOI(&SphereA, MoveDeltaA, VolumeB->ConvexHull, HullTransformB, MoveDeltaB);    
}

CCD_FUNCTION(CapsuleSphereCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    sphere  SphereB  = TransformSphere(&VolumeB->Sphere,    SimEntityB->Transform);    
    ak_v3f    MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f    MoveDeltaB = SimEntityB->GetMoveDelta();
    return SphereCapsuleTOI(&SphereB, MoveDeltaB, &CapsuleA, MoveDeltaA);
}

CCD_FUNCTION(CapsuleCapsuleCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return CapsuleCapsuleTOI(&CapsuleA, MoveDeltaA, &CapsuleB, MoveDeltaB);
}

CCD_FUNCTION(CapsuleHullCCD)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    ak_sqtf HullTransformB = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return CapsuleHullTOI(&CapsuleA, MoveDeltaA, VolumeB->ConvexHull, HullTransformB, MoveDeltaB);
}

CCD_FUNCTION(HullSphereCCD)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return SphereHullTOI(&SphereB, MoveDeltaB, VolumeA->ConvexHull, HullTransformA, MoveDeltaA);
}

CCD_FUNCTION(HullCapsuleCCD)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return CapsuleHullTOI(&CapsuleB, MoveDeltaB, VolumeA->ConvexHull, HullTransformA, MoveDeltaA);
}

CCD_FUNCTION(HullHullCCD)
{
    ak_sqtf HullTransformA = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    ak_sqtf HullTransformB = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    ak_v3f MoveDeltaA = SimEntityA->GetMoveDelta();
    ak_v3f MoveDeltaB = SimEntityB->GetMoveDelta();
    return HullHullTOI(VolumeA->ConvexHull, HullTransformA, MoveDeltaA, VolumeB->ConvexHull, HullTransformB, MoveDeltaB);
}

global ccd_function* CCDFunctions[3][3] = 
{
    {SphereSphereCCD, SphereCapsuleCCD, SphereHullCCD},
    {CapsuleSphereCCD, CapsuleCapsuleCCD, CapsuleHullCCD},
    {HullSphereCCD, HullCapsuleCCD, HullHullCCD}
};

#define CONTACT_FUNCTION(name) contact_list name(sim_entity* SimEntityA, sim_entity* SimEntityB, collision_volume* VolumeA, collision_volume* VolumeB)
typedef CONTACT_FUNCTION(contact_function);

CONTACT_FUNCTION(SphereSphereContact)
{
    AK_NotImplemented();
    return {};
}

CONTACT_FUNCTION(SphereCapsuleContact)
{
    sphere SphereA   = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    return GetSphereCapsuleContacts(&SphereA, &CapsuleB);
}

CONTACT_FUNCTION(SphereHullContact)
{
    sphere SphereA   = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    ak_sqtf ConvexHullTransformB = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    return GetSphereHullContacts(&SphereA, VolumeB->ConvexHull, ConvexHullTransformB);
}

CONTACT_FUNCTION(CapsuleSphereContact)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    sphere SphereB   = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);    
    contact_list Contacts = GetSphereCapsuleContacts(&SphereB, &CapsuleA);
    Contacts.FlipNormals();
    return Contacts;
}

CONTACT_FUNCTION(CapsuleCapsuleContact)
{
    AK_NotImplemented();
    return {};
}

CONTACT_FUNCTION(CapsuleHullContact)
{
    //NOT_IMPLEMENTED;
    return {};
}

CONTACT_FUNCTION(HullSphereContact)
{
    sphere SphereB   = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
    ak_sqtf ConvexHullTransformA = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    contact_list Contacts = GetSphereHullContacts(&SphereB, VolumeA->ConvexHull, ConvexHullTransformA);
    Contacts.FlipNormals();
    return Contacts;
}

CONTACT_FUNCTION(HullCapsuleContact)
{
    AK_NotImplemented();
    return {};
}

CONTACT_FUNCTION(HullHullContact)
{
    AK_NotImplemented();
    return {};
}

global contact_function* ContactFunctions[3][3] = 
{
    {SphereSphereContact, SphereCapsuleContact, SphereHullContact},
    {CapsuleSphereContact, CapsuleCapsuleContact, CapsuleHullContact},
    {HullSphereContact, HullCapsuleContact, HullHullContact}
};

#define CCD_CONTACT_FUNCTION(name) contact name(sim_entity* SimEntityA, sim_entity* SimEntityB, collision_volume* VolumeA, collision_volume* VolumeB, ak_f32 tHit)
typedef CCD_CONTACT_FUNCTION(ccd_contact_function);

CCD_CONTACT_FUNCTION(CCDSphereSphereContact)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
    SphereA.CenterP += SimEntityA->GetMoveDelta()*tHit;
    SphereB.CenterP += SimEntityB->GetMoveDelta()*tHit;
    return GetSphereSphereDeepestContact(&SphereA, &SphereB);
}

CCD_CONTACT_FUNCTION(CCDSphereCapsuleContact)
{
    sphere SphereA   = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    SphereA.CenterP += SimEntityA->GetMoveDelta()*tHit;
    TranslateCapsule(&CapsuleB, SimEntityB->GetMoveDelta()*tHit);    
    return GetSphereCapsuleDeepestContact(&SphereA, &CapsuleB);
}

CCD_CONTACT_FUNCTION(CCDSphereHullContact)
{
    sphere SphereA = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
    ak_sqtf ConvexHullTransform = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    SphereA.CenterP += SimEntityA->GetMoveDelta()*tHit;
    ConvexHullTransform.Translation += SimEntityB->GetMoveDelta()*tHit;    
    return GetSphereHullDeepestContact(&SphereA, VolumeB->ConvexHull, ConvexHullTransform);
}

CCD_CONTACT_FUNCTION(CCDCapsuleSphereContact)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    sphere SphereB   = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);    
    TranslateCapsule(&CapsuleA, SimEntityA->GetMoveDelta()*tHit);    
    SphereB.CenterP += SimEntityB->GetMoveDelta()*tHit;    
    contact Contact = GetSphereCapsuleDeepestContact(&SphereB, &CapsuleA);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CCDCapsuleCapsuleContact)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    TranslateCapsule(&CapsuleA, SimEntityA->GetMoveDelta()*tHit);
    TranslateCapsule(&CapsuleB, SimEntityB->GetMoveDelta()*tHit);
    return GetCapsuleCapsuleDeepestContact(&CapsuleA, &CapsuleB);
}

CCD_CONTACT_FUNCTION(CCDCapsuleHullContact)
{
    capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
    ak_sqtf ConvexHullTransform = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    TranslateCapsule(&CapsuleA, SimEntityA->GetMoveDelta()*tHit);
    ConvexHullTransform.Translation += SimEntityB->GetMoveDelta()*tHit;    
    return GetCapsuleHullDeepestContact(&CapsuleA, VolumeB->ConvexHull, ConvexHullTransform);
}

CCD_CONTACT_FUNCTION(CCDHullSphereContact)
{
    ak_sqtf ConvexHullTransform = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
    ConvexHullTransform.Translation += SimEntityA->GetMoveDelta()*tHit;
    SphereB.CenterP += SimEntityB->GetMoveDelta()*tHit;
    contact Contact = GetSphereHullDeepestContact(&SphereB, VolumeA->ConvexHull, ConvexHullTransform);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CCDHullCapsuleContact)
{
    ak_sqtf ConvexHullTransform = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
    ConvexHullTransform.Translation += SimEntityA->GetMoveDelta()*tHit;        
    TranslateCapsule(&CapsuleB, SimEntityB->GetMoveDelta()*tHit);    
    contact Contact = GetCapsuleHullDeepestContact(&CapsuleB, VolumeA->ConvexHull, ConvexHullTransform);
    Contact.Normal = -Contact.Normal;
    return Contact;
}

CCD_CONTACT_FUNCTION(CCDHullHullContact)
{
    
    ak_sqtf ConvexHullTransformA = VolumeA->ConvexHull->Header.Transform*SimEntityA->Transform;
    ak_sqtf ConvexHullTransformB = VolumeB->ConvexHull->Header.Transform*SimEntityB->Transform;
    ConvexHullTransformA.Translation += SimEntityA->GetMoveDelta()*tHit;
    ConvexHullTransformB.Translation += SimEntityB->GetMoveDelta()*tHit;
    contact Contact = GetHullHullDeepestContact(VolumeA->ConvexHull, ConvexHullTransformA, VolumeB->ConvexHull, ConvexHullTransformB);        
    return Contact;
}

global ccd_contact_function* CCDContactFunctions[3][3] = 
{
    {CCDSphereSphereContact, CCDSphereCapsuleContact, CCDSphereHullContact},
    {CCDCapsuleSphereContact, CCDCapsuleCapsuleContact, CCDCapsuleHullContact},
    {CCDHullSphereContact, CCDHullCapsuleContact, CCDHullHullContact}
};