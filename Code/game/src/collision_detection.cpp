#include "collision_tables.cpp"

collision_detection CollisionDetection_Begin(broad_phase BroadPhase, ak_arena* Arena)
{
    collision_detection Result = {};
    Result.BroadPhase = BroadPhase;
    Result.Arena = Arena;
    return Result;
}

#define UPDATE_HIT(t, AID, BID, VolumeA, VolumeB) \
do \
{ \
if(!TOIResult.Intersected || (t < TOIResult.t)) \
{ \
TOIResult.Intersected = true; \
TOIResult.t = t; \
TOIResult.EntityA = AID; \
TOIResult.EntityB = BID; \
TOIResult.VolumeA = VolumeA; \
TOIResult.VolumeB = VolumeB; \
} \
} while(0)

ccd_contact CCD_GetEarliestContact(collision_detection* CollisionDetection, ak_u64 ID, 
                                   broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    broad_phase* BroadPhase = &CollisionDetection->BroadPhase;
    world* World = BroadPhase->World;
    ak_u32 WorldIndex = BroadPhase->WorldIndex;
    broad_phase_pair_list Pairs = BroadPhase->GetPairs(CollisionDetection->Arena, ID, FilterFunc, UserData);
    
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    
    toi_result TOIResult = {};
    AK_ForEach(Pair, &Pairs)
    {
        collision_volume* VolumeA = World->CollisionVolumeStorage.Get(Pair->VolumeA);
        collision_volume* VolumeB = World->CollisionVolumeStorage.Get(Pair->VolumeB);
        
        physics_object* ObjectA = PhysicsObjects->Get(AK_PoolIndex(Pair->EntityA));
        physics_object* ObjectB = PhysicsObjects->Get(AK_PoolIndex(Pair->EntityB));
        
        toi_function* TOIFunction = TOIFunctions[VolumeA->Type][VolumeB->Type];
        
        ak_f32 t;
        if(TOIFunction(&t, ObjectA, ObjectB, VolumeA, VolumeB))
            UPDATE_HIT(t, Pair->EntityA, Pair->EntityB, VolumeA, VolumeB);
    }
    
    ccd_contact Result = {};
    if(TOIResult.Intersected)
    {
        Result.Intersected = true;
        Result.EntityA = TOIResult.EntityA;
        Result.EntityB = TOIResult.EntityB;
        Result.t = TOIResult.t;
        
        ccd_contact_function* CCDContactFunction = CCDContactFunctions[TOIResult.VolumeA->Type][TOIResult.VolumeB->Type];
        
        physics_object* ObjectA = PhysicsObjects->Get(AK_PoolIndex(TOIResult.EntityA));
        physics_object* ObjectB = PhysicsObjects->Get(AK_PoolIndex(TOIResult.EntityB));
        
        Result.Contact = CCDContactFunction(ObjectA, ObjectB, TOIResult.VolumeA, TOIResult.VolumeB, Result.t);
    }
    
    return Result;
}

#undef UPDATE_HIT

#define BISECTION_EPSILON 1e-6f
inline ak_f32 
Internal__GetBisectionMid(ak_v3f DeltaA, ak_v3f DeltaB, ak_f32 tStart, ak_f32 tEnd)
{
    ak_v3f Delta = DeltaA-DeltaB;    
    if(AK_EqualApprox(Delta*tStart, Delta*tEnd, BISECTION_EPSILON))
        return tStart;
    
    return (tStart+tEnd)*0.5f;    
}

ak_bool HullHullTOI(ak_f32* t, convex_hull* HullA, ak_sqtf TransformA, ak_v3f DeltaA, convex_hull* HullB, ak_sqtf TransformB, ak_v3f DeltaB)
{
    moving_convex_hull_support A = {HullA, TransformA, DeltaA};
    moving_convex_hull_support B = {HullB, TransformB, DeltaB};
    
    if(GJK_Intersected(&A, &B))    
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        for(;;)
        {        
            ak_f32 tMid = Internal__GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
            {
                *t = tStart;
                return true;
            }
            
            moving_convex_hull_support StartA = {HullA, AK_SQT(TransformA.Translation+(DeltaA*tStart), TransformA.Orientation, TransformA.Scale), DeltaA*tMid};
            moving_convex_hull_support StartB = {HullB, AK_SQT(TransformB.Translation+(DeltaB*tStart), TransformB.Orientation, TransformB.Scale), DeltaB*tMid};
            if(GJK_Intersected(&StartA, &StartB))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_convex_hull_support EndA = {HullA, AK_SQT(TransformA.Translation+(DeltaA*tMid), TransformA.Orientation, TransformA.Scale), DeltaA*tEnd};
            moving_convex_hull_support EndB = {HullB, AK_SQT(TransformB.Translation+(DeltaB*tMid), TransformB.Orientation, TransformB.Scale), DeltaB*tEnd};
            if(GJK_Intersected(&EndA, &EndB))
            {
                tStart = tMid;
                continue;
            }
            
            *t = tStart;
            return true;
        }        
    }
    
    return false;
}

ak_bool CapsuleHullTOI(ak_f32* t, capsule* Capsule, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB)
{
    moving_line_segment_support A = {Capsule->P0, Capsule->P1, DeltaA};
    moving_convex_hull_support  B = {Hull, Transform, DeltaB};
    
    if(GJK_QuadraticIntersected(&A, &B, Capsule->Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        for(;;)
        {
            ak_f32 tMid = Internal__GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
            {
                *t = tStart;
                return true;
            }
            
            moving_line_segment_support StartA = {Capsule->P0+DeltaA*tStart, Capsule->P1+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tStart), Transform.Orientation, Transform.Scale), DeltaB*tMid};
            if(GJK_QuadraticIntersected(&StartA, &StartB, Capsule->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_line_segment_support EndA = {Capsule->P0+DeltaA*tMid, Capsule->P1+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tMid), Transform.Orientation, Transform.Scale), DeltaB*tEnd};
            if(GJK_QuadraticIntersected(&EndA, &EndB, Capsule->Radius))
            {
                tStart = tMid;
                continue;
            }
            
            *t = tStart;
            return true;
        }        
    }
    
    return false;    
}

ak_bool SphereHullTOI(ak_f32* t, sphere* Sphere, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB)
{
    moving_point_support A = {Sphere->CenterP, DeltaA};
    moving_convex_hull_support B = {Hull, Transform, DeltaB};
    
    if(GJK_QuadraticIntersected(&A, &B, Sphere->Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        
        for(;;)
        {
            ak_f32 tMid = Internal__GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
            {
                *t = tStart;
                return true;
            }
            
            moving_point_support StartA = {Sphere->CenterP+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tStart), Transform.Orientation, Transform.Scale), DeltaB*tMid};
            if(GJK_QuadraticIntersected(&StartA, &StartB, Sphere->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_point_support EndA = {Sphere->CenterP+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tMid), Transform.Orientation, Transform.Scale), DeltaB*tEnd};
            if(GJK_QuadraticIntersected(&EndA, &EndB, Sphere->Radius))
            {
                tStart = tMid;
                continue;
            }
            
            *t = tStart;
            return true;
        }        
    }
    
    return false;
}

ak_bool CapsuleCapsuleTOI(ak_f32* t, capsule* CapsuleA, ak_v3f DeltaA, capsule* CapsuleB, ak_v3f DeltaB)
{
    moving_line_segment_support A = {CapsuleA->P0, CapsuleA->P1, DeltaA};
    moving_line_segment_support B = {CapsuleB->P0, CapsuleB->P1, DeltaB};
    
    ak_f32 Radius = CapsuleA->Radius+CapsuleB->Radius;
    if(GJK_QuadraticIntersected(&A, &B, Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        
        for(;;)
        {
            ak_f32 tMid = Internal__GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
            {
                *t = tStart;
                return true;
            }
            
            moving_line_segment_support StartA = {CapsuleA->P0+DeltaA*tStart, CapsuleA->P1+DeltaA*tStart, DeltaA*tMid};
            moving_line_segment_support StartB = {CapsuleB->P0+DeltaB*tStart, CapsuleB->P1+DeltaB*tStart, DeltaB*tMid};
            if(GJK_QuadraticIntersected(&StartA, &StartB, Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_line_segment_support EndA = {CapsuleA->P0+DeltaA*tMid, CapsuleA->P1+DeltaA*tMid, DeltaA*tEnd};
            moving_line_segment_support EndB = {CapsuleB->P0+DeltaB*tMid, CapsuleB->P1+DeltaB*tMid, DeltaB*tEnd};            
            if(GJK_QuadraticIntersected(&EndA, &EndB, Radius))
            {
                tStart = tMid;
                continue;
            }
            
            *t = tStart;
            return true;
        }        
    }
    
    return false;
}

ak_bool SphereSphereTOI(ak_f32* t, sphere* SphereA, ak_v3f DeltaA, sphere* SphereB, ak_v3f DeltaB)
{
    sphere LargeSphere = CreateSphere(SphereB->CenterP, SphereA->Radius+SphereB->Radius);
    ak_v3f Delta = DeltaA-DeltaB;
    ak_v3f LineSegment[2] = {SphereA->CenterP, SphereA->CenterP+Delta};
    ray_cast Cast = LineSegment_SphereCast(LineSegment, LargeSphere.CenterP, LargeSphere.Radius);
    if(Cast.Intersected) *t = Cast.t;
    return Cast.Intersected;
}

ak_bool SphereCapsuleTOI(ak_f32* t, sphere* Sphere, ak_v3f DeltaA, capsule* Capsule, ak_v3f DeltaB)
{
    capsule LargeCapsule = CreateCapsule(Capsule->P0, Capsule->P1, Sphere->Radius+Capsule->Radius);
    ak_v3f Delta = DeltaA-DeltaB;
    ak_v3f LineSegment[2] = {Sphere->CenterP, Sphere->CenterP+Delta};
    
    ray_cast Cast = LineSegment_CapsuleCast(LineSegment, LargeCapsule.P0, LargeCapsule.P1, LargeCapsule.Radius);
    if(Cast.Intersected) *t = Cast.t;
    return Cast.Intersected;
}

contact Internal__GetQuadraticDeepestContact(ak_v3f P0, ak_v3f P1, ak_f32 RadiusA, ak_f32 RadiusB)
{
    ak_f32 Radius = RadiusA+RadiusB;
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_Magnitude(Normal);
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined");
    ak_f32 Penetration = Radius-NormalLength;
    
    Normal /= NormalLength;
    
    ak_v3f PointOnA = P0 + Normal*RadiusA;
    ak_v3f PointOnB = P1 - Normal*RadiusB;
    
    contact Result;
    Result.Normal = Normal;
    Result.Position = PointOnA + ((PointOnB-PointOnA)*0.5f);
    Result.Penetration = AK_Max(Radius-NormalLength, 0.0f);
    return Result;
}

contact GetSphereSphereDeepestContact(sphere* SphereA, sphere* SphereB)
{
    contact Result = Internal__GetQuadraticDeepestContact(SphereA->CenterP, SphereB->CenterP, SphereA->Radius, SphereB->Radius);
    return Result;
}

contact GetSphereCapsuleDeepestContact(sphere* SphereA, capsule* CapsuleB)
{
    closest_points ClosestPoints = ClosestPoints_PointLineSegment(SphereA->CenterP, CapsuleB->P);
    contact Result = Internal__GetQuadraticDeepestContact(ClosestPoints.PointA, ClosestPoints.PointB, SphereA->Radius, CapsuleB->Radius);
    return Result;
}

contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    point_support PointGJK = {Sphere->CenterP};
    convex_hull_support ConvexHullGJK = {ConvexHull, ConvexHullTransform};
    
    gjk_distance Distance = GJK_Distance(&PointGJK, &ConvexHullGJK);
    
    ak_v3f P0, P1;
    Distance.GetClosestPoints(&P0, &P1);
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_Magnitude(Normal);
    
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined for GJK");
    
    Normal /= NormalLength;
    
    contact Result;
    Result.Normal = Normal;
    Result.Position = P1;
    Result.Penetration = AK_Max(Sphere->Radius-NormalLength, 0.0f);
    
    return Result;
}

contact GetCapsuleCapsuleDeepestContact(capsule* CapsuleA, capsule* CapsuleB)
{
    closest_points ClosestPoints = ClosestPoints_LineSegments(CapsuleA->P, CapsuleB->P);
    contact Result = Internal__GetQuadraticDeepestContact(ClosestPoints.PointA, ClosestPoints.PointB, CapsuleA->Radius, CapsuleB->Radius);
    return Result;
}

contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    line_segment_support AGJK = {Capsule->P0, Capsule->P1};
    convex_hull_support  BGJK = {ConvexHull, ConvexHullTransform};
    
    gjk_distance Distance = GJK_Distance(&AGJK, &BGJK);
    
    ak_v3f P0, P1;
    Distance.GetClosestPoints(&P0, &P1);        
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_Magnitude(Normal);
    
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined for GJK");
    
    Normal /= NormalLength;
    
    contact Result;
    Result.Normal = Normal;
    Result.Position = P1;
    Result.Penetration = AK_Max(Capsule->Radius-NormalLength, 0.0f);    
    return Result;
}


ak_bool Internal__EPATest(contact* Contact, convex_hull* ConvexHullA, ak_sqtf ConvexHullTransformA, convex_hull* ConvexHullB, ak_sqtf ConvexHullTransformB)
{
    margin_convex_hull_support AEPA = {ConvexHullA, ConvexHullTransformA, 0.005f};
    margin_convex_hull_support BEPA = {ConvexHullB, ConvexHullTransformB, 0.005f};
    epa_result EPAResult = EPA(&AEPA, &BEPA);        
    if(!EPAResult.IsValid)
        return false;    
    
    Contact->Normal = EPAResult.Normal;        
    Contact->Penetration = AK_Max((AEPA.Margin + BEPA.Margin) - EPAResult.Penetration, 0.0f);
    ak_v3f PointOnA = EPAResult.Witness[0] + Contact->Normal*AEPA.Margin;
    ak_v3f PointOnB = EPAResult.Witness[1] - Contact->Normal*BEPA.Margin;    
    Contact->Position = PointOnA + ((PointOnB-PointOnA)*0.5f);        
    return true;
}

contact GetHullHullDeepestContact(convex_hull* ConvexHullA, ak_sqtf ConvexHullTransformA, convex_hull* ConvexHullB, ak_sqtf ConvexHullTransformB)
{
    convex_hull_support AGJK = {ConvexHullA, ConvexHullTransformA};
    convex_hull_support BGJK = {ConvexHullB, ConvexHullTransformB};
    
    ak_bool Intersecting = GJK_Intersected(&AGJK, &BGJK);
    gjk_distance DistanceResult = GJK_Distance(&AGJK, &BGJK);                                                                                                               
    ak_f32 SqrLength = AK_SqrMagnitude(DistanceResult.V);                                                
    ak_bool ShouldDoEPA = SqrLength < GJK_RELATIVE_ERROR;
    
    contact Contact;
    if(Intersecting || ShouldDoEPA)
    {
        if(Internal__EPATest(&Contact, ConvexHullA, ConvexHullTransformA, ConvexHullB, ConvexHullTransformB))
            return Contact;                            
    }    
    
    ak_v3f Witness0, Witness1;
    DistanceResult.GetClosestPoints(&Witness0, &Witness1);                            
    Contact.Normal = AK_Normalize(Witness0-Witness1);
    Contact.Penetration = 0;
    Contact.Position = Witness0 + ((Witness1-Witness0)*0.5f);
    return Contact;    
}

ak_bool SphereSphereOverlap(sphere* SphereA, sphere* SphereB)
{
    ak_f32 Radius = SphereA->Radius+SphereB->Radius;
    return AK_SqrMagnitude(SphereB->CenterP-SphereA->CenterP) <= AK_Square(Radius);
}

ak_bool SphereCapsuleOverlap(sphere* Sphere, capsule* Capsule)
{
    closest_points ClosestPoints = ClosestPoints_PointLineSegment(Sphere->CenterP, Capsule->P);
    ak_f32 Radius = Sphere->Radius+Capsule->Radius;
    return AK_SqrMagnitude(ClosestPoints.PointA-ClosestPoints.PointB) <= AK_Square(Radius);
}

ak_bool SphereHullOverlap(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    point_support APoint = {Sphere->CenterP};
    convex_hull_support BHull = {ConvexHull, ConvexHullTransform};
    gjk_distance Distance = GJK_Distance(&APoint, &BHull);    
    return Distance.SquareDistance <= AK_Square(Sphere->Radius);        
}

ak_bool CapsuleSphereOverlap(capsule* Capsule, sphere* Sphere)
{
    return SphereCapsuleOverlap(Sphere, Capsule);
}

ak_bool CapsuleCapsuleOverlap(capsule* CapsuleA, capsule* CapsuleB)
{
    closest_points ClosestPoints = ClosestPoints_LineSegments(CapsuleA->P, CapsuleB->P);
    ak_f32 Radius = CapsuleA->Radius+CapsuleB->Radius;
    return AK_SqrMagnitude(ClosestPoints.PointA-ClosestPoints.PointB) <= AK_Square(Radius);
}

ak_bool CapsuleHullOverlap(capsule* Capsule, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    line_segment_support ASupport = {Capsule->P[0], Capsule->P[1]};    
    convex_hull_support BSupport = {ConvexHull, ConvexHullTransform};    
    gjk_distance Distance = GJK_Distance(&ASupport, &BSupport);
    return Distance.SquareDistance <= AK_Square(Capsule->Radius);
}

ak_bool HullSphereOverlap(convex_hull* ConvexHull, ak_sqtf ConvexHullTransform, sphere* Sphere)
{
    return SphereHullOverlap(Sphere, ConvexHull, ConvexHullTransform);
}

ak_bool HullCapsuleOverlap(convex_hull* ConvexHull, ak_sqtf ConvexHullTransform, capsule* Capsule)
{
    return CapsuleHullOverlap(Capsule, ConvexHull, ConvexHullTransform);
}

ak_bool HullHullOverlap(convex_hull* ConvexHullA, ak_sqtf ConvexHullTransformA, convex_hull* ConvexHullB, ak_sqtf ConvexHullTransformB)
{
    convex_hull_support ASupport = {ConvexHullA, ConvexHullTransformA};
    convex_hull_support BSupport = {ConvexHullB, ConvexHullTransformB};
    return GJK_Intersected(&ASupport, &BSupport);
}