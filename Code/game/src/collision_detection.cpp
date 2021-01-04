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

ak_bool CollisionDetection_Intersect(collision_detection* CollisionDetection, broad_phase_pair* Pair)
{
    world* World = CollisionDetection->BroadPhase.World;
    ak_u32 WorldIndex = CollisionDetection->BroadPhase.WorldIndex;
    
    ak_pool<collision_volume>* CollisionVolumes = &World->CollisionVolumeStorage;
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    
    collision_volume* VolumeA = World->CollisionVolumeStorage.Get(Pair->VolumeA);
    collision_volume* VolumeB = World->CollisionVolumeStorage.Get(Pair->VolumeB);
    
    physics_object* ObjectA = PhysicsObjects->Get(AK_PoolIndex(Pair->EntityA));
    physics_object* ObjectB = PhysicsObjects->Get(AK_PoolIndex(Pair->EntityB));
    
    intersection_function* IntersectionFunction = IntersectionFunctions[VolumeA->Type][VolumeB->Type];
    return IntersectionFunction(ObjectA, ObjectB, VolumeA, VolumeB);
    
}

ccd_contact CCD_ComputeContact(collision_detection* CollisionDetection, ak_u64 AID, ak_u64 BID)
{
    broad_phase* BroadPhase = &CollisionDetection->BroadPhase;
    world* World = BroadPhase->World;
    ak_u32 WorldIndex = BroadPhase->WorldIndex;
    
    physics_object* PhysicsObjectA = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(AID));
    physics_object* PhysicsObjectB = World->PhysicsObjects[WorldIndex].Get(AK_PoolIndex(BID));
    
    ccd_contact Result = {};
    
    collision_volume* AHitVolume = NULL;
    collision_volume* BHitVolume = NULL;
    
    collision_volume* AVolume = World->CollisionVolumeStorage.Get(PhysicsObjectA->CollisionVolumeID);
    while(AVolume)
    {
        collision_volume* BVolume = 
            World->CollisionVolumeStorage.Get(PhysicsObjectB->CollisionVolumeID);
        while(BVolume)
        {
            toi_function* TOIFunction = TOIFunctions[AVolume->Type][BVolume->Type];
            ak_f32 t;
            if(TOIFunction(&t, PhysicsObjectA, PhysicsObjectB, AVolume, BVolume))
            {
                if(!Result.Intersected || (Result.t > t))
                {
                    Result.Intersected = true;
                    Result.t = t;
                    
                    AHitVolume = AVolume;
                    BHitVolume = BVolume;
                }
            }
            BVolume = World->CollisionVolumeStorage.Get(BVolume->NextID);
        }
        AVolume = World->CollisionVolumeStorage.Get(AVolume->NextID);
    }
    
    if(Result.Intersected)
    {
        Result.EntityA = AID;
        Result.EntityB = BID;
        
        ccd_contact_function* CCDContactFunction = CCDContactFunctions[AHitVolume->Type][BHitVolume->Type];
        Result.Contact = CCDContactFunction(PhysicsObjectA, PhysicsObjectB, AHitVolume, BHitVolume, Result.t);
    }
    
    return Result;
}

ccd_contact CCD_GetEarliestContact(collision_detection* CollisionDetection, ak_u64 ID, 
                                   broad_phase_pair_list* Pairs)
{
    broad_phase* BroadPhase = &CollisionDetection->BroadPhase;
    world* World = BroadPhase->World;
    ak_u32 WorldIndex = BroadPhase->WorldIndex;
    
    ak_array<physics_object>* PhysicsObjects = &World->PhysicsObjects[WorldIndex];
    
    toi_result TOIResult = {};
    AK_ForEach(Pair, Pairs)
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

ccd_contact CCD_GetEarliestContact(collision_detection* CollisionDetection, ak_u64 ID, 
                                   broad_phase_pair_filter_func* FilterFunc, void* UserData)
{
    broad_phase* BroadPhase = &CollisionDetection->BroadPhase;
    broad_phase_pair_list Pairs = BroadPhase->GetPairs(CollisionDetection->Arena, ID, FilterFunc, UserData);
    return CCD_GetEarliestContact(CollisionDetection, ID, &Pairs);
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

ak_bool HullHullTOI(ak_f32* t, convex_hull* HullA, ak_m4f TransformA, ak_v3f DeltaA, convex_hull* HullB, ak_m4f TransformB, ak_v3f DeltaB)
{
    moving_convex_hull_support A = {HullA, TransformA, AK_Transpose(AK_M3(TransformA)), DeltaA};
    
    moving_convex_hull_support B = {HullB, TransformB, AK_Transpose(AK_M3(TransformB)), DeltaB};
    
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
            
            ak_f32 tDiff = tMid-tStart;
            
            A.T.Translation.xyz = TransformA.Translation.xyz + (DeltaA*tStart);
            A.Delta = DeltaA*tDiff;
            B.T.Translation.xyz = TransformB.Translation.xyz + (DeltaB*tStart);
            B.Delta = DeltaB*tDiff;
            
            gjk_distance Distance0 = GJK_Distance(&A, &B);
            if(Distance0.SquareDistance < 1e-5f)
            {
                tEnd = tMid;
                continue;
            }
            
            tDiff = tEnd-tMid;
            
            A.T.Translation.xyz = TransformA.Translation.xyz + (DeltaA*tMid);
            A.Delta = DeltaA*tDiff;
            B.T.Translation.xyz = TransformB.Translation.xyz + (DeltaB*tMid);
            B.Delta = DeltaB*tDiff;
            
            gjk_distance Distance1 = GJK_Distance(&A, &B);
            if(Distance1.SquareDistance < 1e-5f)
            {
                tEnd = tMid;
                continue;
            }
            
            if(Distance0.SquareDistance < Distance1.SquareDistance)
                tEnd = tMid;
            else
                tStart = tMid;
        }        
    }
    
    return false;
}

ak_bool CapsuleHullTOI(ak_f32* t, capsule* Capsule, ak_v3f DeltaA, convex_hull* Hull, ak_m4f Transform, ak_v3f DeltaB)
{
    moving_line_segment_support A = {Capsule->P0, Capsule->P1, DeltaA};
    moving_convex_hull_support  B = {Hull, Transform, AK_Transpose(AK_M3(Transform)), DeltaB};
    
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
            
            ak_f32 tDiff = tMid-tStart;
            
            A.P0 = Capsule->P0 + DeltaA*tStart;
            A.P1 = Capsule->P1 + DeltaA*tStart;
            A.Delta = DeltaA*tDiff;
            
            B.T.Translation.xyz = Transform.Translation.xyz + DeltaB*tStart;
            B.Delta = DeltaB*tDiff;
            
            gjk_distance Distance0 = GJK_Distance(&A, &B);
            if(GJK_QuadraticIntersected(Distance0, Capsule->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            tDiff = tEnd-tMid;
            
            A.P0 = Capsule->P0 + DeltaA*tMid;
            A.P1 = Capsule->P1 + DeltaA*tMid;
            A.Delta = DeltaA*tDiff;
            
            B.T.Translation.xyz = Transform.Translation.xyz + DeltaB*tMid;
            B.Delta = DeltaB*tDiff;
            
            gjk_distance Distance1 = GJK_Distance(&A, &B);
            if(GJK_QuadraticIntersected(Distance1, Capsule->Radius))
            {
                tStart = tMid;
                continue;
            }
            
            if(Distance0.SquareDistance < Distance1.SquareDistance)
                tEnd = tMid;
            else
                tStart = tMid;
        }        
    }
    
    return false;    
}

ak_bool SphereHullTOI(ak_f32* t, sphere* Sphere, ak_v3f DeltaA, convex_hull* Hull, ak_m4f Transform, ak_v3f DeltaB)
{
    moving_point_support A = {Sphere->CenterP, DeltaA};
    moving_convex_hull_support B = {Hull, Transform, AK_Transpose(AK_M3(Transform)), DeltaB};
    
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
            
            A.P = Sphere->CenterP+DeltaA*tStart;
            A.Delta = DeltaA*tMid;
            
            B.T.Translation.xyz = Transform.Translation.xyz + DeltaB*tStart;
            B.Delta = DeltaB*tMid;
            if(GJK_QuadraticIntersected(&A, &B, Sphere->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            A.P = Sphere->CenterP+DeltaA*tMid;
            A.Delta = DeltaA*tEnd;
            
            B.T.Translation.xyz = Transform.Translation.xyz + DeltaB*tMid;
            B.Delta = DeltaB*tEnd;
            
            if(GJK_QuadraticIntersected(&A, &B, Sphere->Radius))
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

contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, ak_m4f ConvexHullTransform)
{
    point_support PointGJK = {Sphere->CenterP};
    convex_hull_support ConvexHullGJK = {ConvexHull, ConvexHullTransform, AK_Transpose(AK_M3(ConvexHullTransform))};
    
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

contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, ak_m4f ConvexHullTransform)
{
    line_segment_support AGJK = {Capsule->P0, Capsule->P1};
    convex_hull_support  BGJK = {ConvexHull, ConvexHullTransform, AK_Transpose(AK_M3(ConvexHullTransform))};
    
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


ak_bool Internal__EPATest(contact* Contact, convex_hull* ConvexHullA, ak_m4f ConvexHullTransformA, convex_hull* ConvexHullB, ak_m4f ConvexHullTransformB)
{
    margin_convex_hull_support AEPA = {ConvexHullA, ConvexHullTransformA, AK_Transpose(AK_M3(ConvexHullTransformA)), 0.005f};
    
    margin_convex_hull_support BEPA = {ConvexHullB, ConvexHullTransformB, 
        AK_Transpose(AK_M3(ConvexHullTransformB)), 0.005f};
    
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

contact GetHullHullDeepestContact(convex_hull* ConvexHullA, ak_m4f ConvexHullTransformA, convex_hull* ConvexHullB, ak_m4f ConvexHullTransformB)
{
    convex_hull_support AGJK = {ConvexHullA, ConvexHullTransformA, AK_Transpose(AK_M3(ConvexHullTransformA))};
    convex_hull_support BGJK = {ConvexHullB, ConvexHullTransformB, 
        AK_Transpose(AK_M3(ConvexHullTransformB))};
    
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
    Contact.Normal = AK_Normalize(Witness1-Witness0);
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

ak_bool SphereHullOverlap(sphere* Sphere, convex_hull* ConvexHull, ak_m4f ConvexHullTransform)
{
    point_support APoint = {Sphere->CenterP};
    convex_hull_support BHull = {ConvexHull, ConvexHullTransform, AK_Transpose(AK_M3(ConvexHullTransform))};
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

ak_bool CapsuleHullOverlap(capsule* Capsule, convex_hull* ConvexHull, ak_m4f ConvexHullTransform)
{
    line_segment_support ASupport = {Capsule->P[0], Capsule->P[1]};    
    convex_hull_support BSupport = {ConvexHull, ConvexHullTransform, AK_Transpose(AK_M3(ConvexHullTransform))};    
    gjk_distance Distance = GJK_Distance(&ASupport, &BSupport);
    return Distance.SquareDistance <= AK_Square(Capsule->Radius);
}

ak_bool HullSphereOverlap(convex_hull* ConvexHull, ak_m4f ConvexHullTransform, sphere* Sphere)
{
    return SphereHullOverlap(Sphere, ConvexHull, ConvexHullTransform);
}

ak_bool HullCapsuleOverlap(convex_hull* ConvexHull, ak_m4f ConvexHullTransform, capsule* Capsule)
{
    return CapsuleHullOverlap(Capsule, ConvexHull, ConvexHullTransform);
}

ak_bool HullHullOverlap(convex_hull* ConvexHullA, ak_m4f ConvexHullTransformA, convex_hull* ConvexHullB, ak_m4f ConvexHullTransformB)
{
    convex_hull_support ASupport = {ConvexHullA, ConvexHullTransformA, AK_Transpose(AK_M3(ConvexHullTransformA))};
    convex_hull_support BSupport = {ConvexHullB, ConvexHullTransformB, AK_Transpose(AK_M3(ConvexHullTransformB))};
    return GJK_Intersected(&ASupport, &BSupport);
}