#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

struct contact
{
    ak_v3f Position;
    ak_v3f Normal;
    ak_f32 Penetration;
};

struct toi_result
{
    ak_bool Intersected;
    ak_f32 t;
    ak_u64 EntityA;
    ak_u64 EntityB;
    collision_volume* VolumeA;
    collision_volume* VolumeB;
};

struct ccd_contact
{
    ak_bool Intersected;
    ak_u64 EntityA;
    ak_u64 EntityB;
    ak_f32 t;
    contact Contact;
};

struct collision_detection
{
    broad_phase BroadPhase;
    ak_arena* Arena;
};

ccd_contact CCD_GetEarliestContact(collision_detection* CollisionDetection, ak_u64 ID, 
                                   broad_phase_pair_filter_func* FilterFunc=NULL, void* UserData=NULL);

ak_bool HullHullTOI(ak_f32* t, convex_hull* HullA, ak_m4f TransformA, ak_v3f DeltaA, convex_hull* HullB, ak_m4f TransformB, ak_v3f DeltaB);
ak_bool CapsuleHullTOI(ak_f32* t, capsule* Capsule, ak_v3f DeltaA, convex_hull* Hull, ak_m4f Transform, ak_v3f DeltaB);
ak_bool SphereHullTOI(ak_f32* t, sphere* Sphere, ak_v3f DeltaA, convex_hull* Hull, ak_m4f Transform, ak_v3f DeltaB);
ak_bool CapsuleCapsuleTOI(ak_f32* t, capsule* CapsuleA, ak_v3f DeltaA, capsule* CapsuleB, ak_v3f DeltaB);
ak_bool SphereSphereTOI(ak_f32* t, sphere* SphereA, ak_v3f DeltaA, sphere* SphereB, ak_v3f DeltaB);
ak_bool SphereCapsuleTOI(ak_f32* t, sphere* Sphere, ak_v3f DeltaA, capsule* Capsule, ak_v3f DeltaB);

contact GetSphereSphereDeepestContact(sphere* SphereA, sphere* SphereB);
contact GetSphereCapsuleDeepestContact(sphere* SphereA, capsule* CapsuleB);
contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, ak_m4f ConvexHullTransform);
contact GetCapsuleCapsuleDeepestContact(capsule* CapsuleA, capsule* CapsuleB);
contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, ak_m4f ConvexHullTransform);
contact GetHullHullDeepestContact(convex_hull* ConvexHullA, ak_m4f ConvexHullTransformA, convex_hull* ConvexHullB, ak_m4f ConvexHullTransformB);

ak_bool SphereSphereOverlap(sphere* SphereA, sphere* SphereB);
ak_bool SphereCapsuleOverlap(sphere* Sphere, capsule* Capsule);
ak_bool SphereHullOverlap(sphere* Sphere, convex_hull* ConvexHull, ak_m4f ConvexHullTransform);
ak_bool CapsuleSphereOverlap(capsule* Capsule, sphere* Sphere);
ak_bool CapsuleCapsuleOverlap(capsule* CapsuleA, capsule* CapsuleB);
ak_bool CapsuleHullOverlap(capsule* Capsule, convex_hull* ConvexHull, ak_m4f ConvexHullTransform);
ak_bool HullSphereOverlap(convex_hull* ConvexHull, ak_m4f ConvexHullTransform, sphere* Sphere);
ak_bool HullCapsuleOverlap(convex_hull* ConvexHull, ak_m4f ConvexHullTransform, capsule* Capsule);
ak_bool HullHullOverlap(convex_hull* ConvexHullA, ak_m4f ConvexHullTransformA, convex_hull* ConvexHullB, ak_m4f ConvexHullTransformB);

#endif
