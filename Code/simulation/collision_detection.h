#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

struct ray
{
    ak_v3f Origin;
    ak_v3f Direction;
};

struct contact
{
    ak_v3f Position;
    ak_v3f Normal;
    ak_f32 Penetration;
};

struct contact_list
{
    contact* Ptr;
    ak_u32 Count;
    
    inline void FlipNormals()
    {
        for(ak_u32 ContactIndex = 0; ContactIndex < Count; ContactIndex++)
            Ptr[ContactIndex].Normal = -Ptr[ContactIndex].Normal;
    }
    
    inline contact* GetDeepestContact()
    {
        contact* Best = NULL;
        for(ak_u32 ContactIndex = 0; ContactIndex < Count; ContactIndex++)
        {
            if(!Best || Ptr[ContactIndex].Penetration > Best->Penetration)
                Best = &Ptr[ContactIndex];            
        }
        return Best;
    }
};

struct toi_result
{
    ak_f32 t;
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
    ak_f32 t;
    sim_entity* HitEntity;
    contact Contact;    
};

ak_f32 RaySphereIntersection(ak_v3f Origin, ak_v3f Direction, sphere* Sphere);
ak_f32 RayCapsuleIntersection(ak_v3f Origin, ak_v3f Direction, capsule* Capsule);
ak_bool RayTriangleIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* t, ak_f32* u, ak_f32* v);
ak_f32 LineSegmentSphereIntersection(ak_v3f* LineSegment, sphere* Sphere);
ak_f32 LineSegmentCapsuleIntersection(ak_v3f* LineSegment, capsule* Capsule);
ak_f32 LineSegmentsClosestPoints(ak_v3f* Result, ak_v3f* A, ak_v3f* B);
ak_v3f PointLineSegmentClosestPoint(ak_v3f P, ak_v3f* LineSegment);
contact_list GetSphereCapsuleContacts(sphere* Sphere, capsule* Capsule);
contact_list GetSphereHullContacts(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform);
contact GetQuadraticDeepestContact(ak_v3f P0, ak_v3f P1, ak_f32 RadiusA, ak_f32 RadiusB);
contact GetSphereSphereDeepestContact(sphere* SphereA, sphere* SphereB);
contact GetSphereCapsuleDeepestContact(sphere* SphereA, capsule* CapsuleB);
contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform);
contact GetCapsuleCapsuleDeepestContact(capsule* CapsuleA, capsule* CapsuleB);
contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform);
contact GetHullHullDeepestContact(convex_hull* ConvexHullA, ak_sqtf ConvexHullTransformA, convex_hull* ConvexHullB, ak_sqtf ConvexHullTransformB);
ak_f32 GetBisectionMid(ak_v3f DeltaA, ak_v3f DeltaB, ak_f32 tStart, ak_f32 tEnd);
ak_f32 HullHullTOI(convex_hull* HullA, ak_sqtf TransformA, ak_v3f DeltaA, convex_hull* HullB, ak_sqtf TransformB, ak_v3f DeltaB);
ak_f32 CapsuleHullTOI(capsule* Capsule, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB);
ak_f32 SphereHullTOI(sphere* Sphere, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB);
ak_f32 CapsuleCapsuleTOI(capsule* CapsuleA, ak_v3f DeltaA, capsule* CapsuleB, ak_v3f DeltaB);
ak_f32 SphereSphereTOI(sphere* SphereA, ak_v3f DeltaA, sphere* SphereB, ak_v3f DeltaB);
ak_f32 SphereCapsuleTOI(sphere* Sphere, ak_v3f DeltaA, capsule* Capsule, ak_v3f DeltaB);

#endif