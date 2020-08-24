b32 PerformConvexHullEPA(penetration* Result, convex_hull* HullA, sqt TransformA, convex_hull* HullB, sqt TransformB)
{
    margin_convex_hull_support AEPA = {HullA, TransformA, 0.005f};
    margin_convex_hull_support BEPA = {HullB, TransformB, 0.005f};
    epa_result EPAResult = EPA(&AEPA, &BEPA);        
    if(!EPAResult.IsValid)
        return false;    
    
    Result->Normal = EPAResult.Normal;        
    Result->Distance = (AEPA.Margin + BEPA.Margin) - EPAResult.Penetration;
    return true;
}

penetration GetQuadraticPenetration(v3f ClosestP0, v3f ClosestP1, f32 Radius)
{
    penetration Result;
    
    v3f PenetrationVector = ClosestP0-ClosestP1;
    f32 PenetrationLength = Magnitude(PenetrationVector);
    f32 Penetration = Radius-PenetrationLength;
    if(Penetration < 0)
        Penetration = 0;
    
    Result.Normal = PenetrationVector / PenetrationLength;
    Result.Distance = Penetration;
    
    return Result;
}

penetration GetQuadraticPenetration(gjk_distance* Distance, f32 Radius)
{    
    v3f Witness0, Witness1;
    Distance->GetClosestPoints(&Witness0, &Witness1);
    
    penetration Result = GetQuadraticPenetration(Witness0, Witness1, Radius);       
    return Result;
}

penetration GetSphereHullPenetration(sphere* Sphere, convex_hull* Hull, sqt HullTransform)
{
    
    point_support AGJK = {Sphere->CenterP};
    convex_hull_support BGJK = {Hull, HullTransform};
    gjk_distance DistanceResult = GJKDistance(&AGJK, &BGJK);
    
    penetration Result = GetQuadraticPenetration(&DistanceResult, Sphere->Radius);
    return Result;
}

penetration GetCapsuleHullPenetration(capsule* Capsule, convex_hull* Hull, sqt HullTransform)
{    
    
    line_segment_support AGJK = {Capsule->P0, Capsule->P1};
    convex_hull_support  BGJK = {Hull, HullTransform};
    gjk_distance DistanceResult = GJKDistance(&AGJK, &BGJK);
    
    penetration Result = GetQuadraticPenetration(&DistanceResult, Capsule->Radius);   
    return Result;
}

penetration GetCapsuleCapsulePenetration(capsule* CapsuleA, capsule* CapsuleB)
{
    v3f ClosestPoints[2];
    LineSegmentsClosestPoints(ClosestPoints, CapsuleA->P, CapsuleB->P);    
    penetration Result = GetQuadraticPenetration(ClosestPoints[0], ClosestPoints[1], CapsuleA->Radius+CapsuleB->Radius);
    return Result;
}

penetration GetSphereSpherePenetration(sphere* SphereA, sphere* SphereB)
{
    penetration Result = GetQuadraticPenetration(SphereA->CenterP, SphereB->CenterP, SphereA->Radius+SphereB->Radius);
    return Result;
}

penetration GetSphereCapsulePenetration(sphere* Sphere, capsule* CapsuleB)
{
    v3f P1 = PointLineSegmentClosestPoint(Sphere->CenterP, CapsuleB->P);
    penetration Result = GetQuadraticPenetration(Sphere->CenterP, P1, Sphere->Radius+CapsuleB->Radius);
    return Result;
}

penetration GetHullHullPenetration(convex_hull* HullA, sqt TransformA, 
                                   convex_hull* HullB, sqt TransformB)
{    
    convex_hull_support AGJK = {HullA, TransformA};
    convex_hull_support BGJK = {HullB, TransformB};
    
    b32 Intersecting = GJKIntersected(&AGJK, &BGJK);
    gjk_distance DistanceResult = GJKDistance(&AGJK, &BGJK);                                                                                                               
    f32 SqrLength = SquareMagnitude(DistanceResult.V);                                                
    b32 ShouldDoEPA = SqrLength < GJK_RELATIVE_ERROR;
    
    penetration Result;
    if(Intersecting || ShouldDoEPA)
    {
        if(PerformConvexHullEPA(&Result, HullA, TransformA, HullB, TransformB))
            return Result;                            
    }
    
    ASSERT(SqrLength > Square(FLT_EPSILON));                                                                            
    
    v3f Witness0, Witness1;
    DistanceResult.GetClosestPoints(&Witness0, &Witness1);                            
    Result.Normal = Normalize(Witness0-Witness1);
    Result.Distance = 0;                                                                            
    return Result;
}

penetration GetPenetration(sim_entity* SimEntityA, sim_entity* SimEntityB, collision_volume* VolumeA, collision_volume* VolumeB, f32 tHit)
{
    ASSERT((tHit >= 0) && (tHit <= 1.0f));    
    
    penetration Result = {};    
    switch(VolumeA->Type)
    {
        case COLLISION_VOLUME_TYPE_SPHERE:
        {            
            sphere SphereA = TransformSphere(&VolumeA->Sphere, SimEntityA->Transform);
            SphereA.CenterP += SimEntityA->MoveDelta*tHit;
            
            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {                    
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
                    SphereB.CenterP += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetSphereSpherePenetration(&SphereA, &SphereB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
                    CapsuleB.P0 += SimEntityB->MoveDelta*tHit;
                    CapsuleB.P1 += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetSphereCapsulePenetration(&SphereA, &CapsuleB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, SimEntityB->Transform);                        
                    TransformB.Translation += SimEntityB->MoveDelta*tHit;                                                            
                    
                    Result = GetSphereHullPenetration(&SphereA, HullB, TransformB);
                } break;
            }
            
        } break;
        
        case COLLISION_VOLUME_TYPE_CAPSULE:
        {
            capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntityA->Transform);
            CapsuleA.P0 += SimEntityA->MoveDelta*tHit;
            CapsuleA.P1 += SimEntityA->MoveDelta*tHit;            
            
            switch(VolumeB->Type)
            {   
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
                    SphereB.CenterP += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetSphereCapsulePenetration(&SphereB, &CapsuleA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
                    CapsuleB.P0 += SimEntityB->MoveDelta*tHit;
                    CapsuleB.P1 += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetCapsuleCapsulePenetration(&CapsuleA, &CapsuleB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, SimEntityB->Transform);                        
                    TransformB.Translation += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetCapsuleHullPenetration(&CapsuleA, HullB, TransformB);
                }
            }
            
        } break;
        
        case COLLISION_VOLUME_TYPE_CONVEX_HULL:
        {
            convex_hull* HullA = VolumeA->ConvexHull;
            sqt TransformA = ToParentCoordinates(HullA->Header.Transform, SimEntityA->Transform);
            TransformA.Translation += SimEntityA->MoveDelta*tHit;
            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, SimEntityB->Transform);
                    SphereB.CenterP += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetSphereHullPenetration(&SphereB, HullA, TransformA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, SimEntityB->Transform);
                    CapsuleB.P0 += SimEntityB->MoveDelta*tHit;
                    CapsuleB.P1 += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetCapsuleHullPenetration(&CapsuleB, HullA, TransformA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, SimEntityB->Transform);                        
                    TransformB.Translation += SimEntityB->MoveDelta*tHit;
                    
                    Result = GetHullHullPenetration(HullA, TransformA, HullB, TransformB);
                }
            }
            
        } break;
    }
    
    return Result;
}