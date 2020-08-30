#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

struct contact
{
    v3f Position;
    v3f Normal;
    f32 Penetration;
};

struct contact_list
{
    contact* Ptr;
    u32 Count;
    
    inline void FlipNormals()
    {
        for(u32 ContactIndex = 0; ContactIndex < Count; ContactIndex++)
            Ptr[ContactIndex].Normal = -Ptr[ContactIndex].Normal;
    }
    
    inline contact* GetDeepestContact()
    {
        contact* Best = NULL;
        for(u32 ContactIndex = 0; ContactIndex < Count; ContactIndex++)
        {
            if(!Best || Ptr[ContactIndex].Penetration > Best->Penetration)
                Best = &Ptr[ContactIndex];            
        }
        return Best;
    }
};

struct toi_result
{
    f32 t;
    sim_entity* HitEntity;
    collision_volume* VolumeA;
    collision_volume* VolumeB;    
};

inline toi_result
InvalidTOIResult()
{
    toi_result Result;
    Result.t = INFINITY;
    Result.HitEntity = NULL;
    return Result;
}

struct continuous_contact
{
    f32 t;
    sim_entity* HitEntity;
    contact Contact;    
};

f32 RaySphereIntersection(v3f Origin, v3f Direction, sphere* Sphere);
f32 RayCapsuleIntersection(v3f Origin, v3f Direction, capsule* Capsule);
b32 RayTriangleIntersection(v3f RayOrigin, v3f RayDirection, v3f P0, v3f P1, v3f P2, f32* t, f32* u, f32* v);
f32 LineSegmentSphereIntersection(v3f* LineSegment, sphere* Sphere);
f32 LineSegmentCapsuleIntersection(v3f* LineSegment, capsule* Capsule);
f32 LineSegmentsClosestPoints(v3f* Result, v3f* A, v3f* B);
v3f PointLineSegmentClosestPoint(v3f P, v3f* LineSegment);
contact_list GetSphereCapsuleContacts(sphere* Sphere, capsule* Capsule);
contact_list GetSphereHullContacts(sphere* Sphere, convex_hull* ConvexHull, sqt ConvexHullTransform);
contact GetQuadraticDeepestContact(v3f P0, v3f P1, f32 RadiusA, f32 RadiusB);
contact GetSphereSphereDeepestContact(sphere* SphereA, sphere* SphereB);
contact GetSphereCapsuleDeepestContact(sphere* SphereA, capsule* CapsuleB);
contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, sqt ConvexHullTransform);
contact GetCapsuleCapsuleDeepestContact(capsule* CapsuleA, capsule* CapsuleB);
contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, sqt ConvexHullTransform);
contact GetHullHullDeepestContact(convex_hull* ConvexHullA, sqt ConvexHullTransformA, convex_hull* ConvexHullB, sqt ConvexHullTransformB);
f32 GetBisectionMid(v3f DeltaA, v3f DeltaB, f32 tStart, f32 tEnd);
f32 HullHullTOI(convex_hull* HullA, sqt TransformA, v3f DeltaA, convex_hull* HullB, sqt TransformB, v3f DeltaB);
f32 CapsuleHullTOI(capsule* Capsule, v3f DeltaA, convex_hull* Hull, sqt Transform, v3f DeltaB);
f32 SphereHullTOI(sphere* Sphere, v3f DeltaA, convex_hull* Hull, sqt Transform, v3f DeltaB);
f32 CapsuleCapsuleTOI(capsule* CapsuleA, v3f DeltaA, capsule* CapsuleB, v3f DeltaB);
f32 SphereSphereTOI(sphere* SphereA, v3f DeltaA, sphere* SphereB, v3f DeltaB);
f32 SphereCapsuleTOI(sphere* Sphere, v3f DeltaA, capsule* Capsule, v3f DeltaB);

#endif