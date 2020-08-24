#define BISECTION_EPSILON 1e-6f
inline f32 
GetBisectionMid(v3f DeltaA, v3f DeltaB, f32 tStart, f32 tEnd)
{
    v3f Delta = DeltaA-DeltaB;    
    if(AreNearlyEqualV3(Delta*tStart, Delta*tEnd, BISECTION_EPSILON))
        return tStart;
    
    return (tStart+tEnd)*0.5f;    
}

f32 HullHullTOI(convex_hull* HullA, sqt TransformA, v3f DeltaA, 
                convex_hull* HullB, sqt TransformB, v3f DeltaB)
{    
    moving_convex_hull_support A = {HullA, TransformA, DeltaA};
    moving_convex_hull_support B = {HullB, TransformB, DeltaB};
    
    if(GJKIntersected(&A, &B))    
    {
        f32 tStart = 0.0f;
        f32 tEnd = 1.0f;
        for(;;)
        {        
            f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_convex_hull_support StartA = {HullA, CreateSQT(TransformA.Translation+(DeltaA*tStart), TransformA.Scale, TransformA.Orientation), DeltaA*tMid};
            moving_convex_hull_support StartB = {HullB, CreateSQT(TransformB.Translation+(DeltaB*tStart), TransformB.Scale, TransformB.Orientation), DeltaB*tMid};
            if(GJKIntersected(&StartA, &StartB))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_convex_hull_support EndA = {HullA, CreateSQT(TransformA.Translation+(DeltaA*tMid), TransformA.Scale, TransformA.Orientation), DeltaA*tEnd};
            moving_convex_hull_support EndB = {HullB, CreateSQT(TransformB.Translation+(DeltaB*tMid), TransformB.Scale, TransformB.Orientation), DeltaB*tEnd};
            if(GJKIntersected(&EndA, &EndB))
            {
                tStart = tMid;
                continue;
            }
            
            return tStart;
        }        
    }
    
    return INFINITY;
}


f32 CapsuleHullTOI(capsule* Capsule, v3f DeltaA, convex_hull* Hull, sqt Transform, v3f DeltaB)
{    
    moving_line_segment_support A = {Capsule->P0, Capsule->P1, DeltaA};
    moving_convex_hull_support  B = {Hull, Transform, DeltaB};
    
    if(GJKQuadraticIntersected(&A, &B, Capsule->Radius))
    {
        f32 tStart = 0.0f;
        f32 tEnd = 1.0f;
        for(;;)
        {
            f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_line_segment_support StartA = {Capsule->P0+DeltaA*tStart, Capsule->P1+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, CreateSQT(Transform.Translation+(DeltaB*tStart), Transform.Scale, Transform.Orientation), DeltaB*tMid};
            if(GJKQuadraticIntersected(&StartA, &StartB, Capsule->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_line_segment_support EndA = {Capsule->P0+DeltaA*tMid, Capsule->P1+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, CreateSQT(Transform.Translation+(DeltaB*tMid), Transform.Scale, Transform.Orientation), DeltaB*tEnd};
            if(GJKQuadraticIntersected(&EndA, &EndB, Capsule->Radius))
            {
                tStart = tMid;
                continue;
            }
            
            return tStart;
        }        
    }
    
    return INFINITY;    
}

f32 SphereHullTOI(sphere* Sphere, v3f DeltaA, convex_hull* Hull, sqt Transform, v3f DeltaB)
{    
    moving_point_support A = {Sphere->CenterP, DeltaA};
    moving_convex_hull_support B = {Hull, Transform, DeltaB};
    
    if(GJKQuadraticIntersected(&A, &B, Sphere->Radius))
    {
        f32 tStart = 0.0f;
        f32 tEnd = 1.0f;
        
        for(;;)
        {
            f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_point_support StartA = {Sphere->CenterP+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, CreateSQT(Transform.Translation+(DeltaB*tStart), Transform.Scale, Transform.Orientation), DeltaB*tMid};
            if(GJKQuadraticIntersected(&StartA, &StartB, Sphere->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_point_support EndA = {Sphere->CenterP+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, CreateSQT(Transform.Translation+(DeltaB*tMid), Transform.Scale, Transform.Orientation), DeltaB*tEnd};
            if(GJKQuadraticIntersected(&EndA, &EndB, Sphere->Radius))
            {
                tStart = tMid;
                continue;
            }
            
            return tStart;
        }        
    }
    
    return INFINITY;
}

f32 CapsuleCapsuleTOI(capsule* CapsuleA, v3f DeltaA, capsule* CapsuleB, v3f DeltaB)
{
    moving_line_segment_support A = {CapsuleA->P0, CapsuleA->P1, DeltaA};
    moving_line_segment_support B = {CapsuleB->P0, CapsuleB->P1, DeltaB};
    
    f32 Radius = CapsuleA->Radius+CapsuleB->Radius;
    if(GJKQuadraticIntersected(&A, &B, Radius))
    {
        f32 tStart = 0.0f;
        f32 tEnd = 1.0f;
        
        for(;;)
        {
            f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_line_segment_support StartA = {CapsuleA->P0+DeltaA*tStart, CapsuleA->P1+DeltaA*tStart, DeltaA*tMid};
            moving_line_segment_support StartB = {CapsuleB->P0+DeltaB*tStart, CapsuleB->P1+DeltaB*tStart, DeltaB*tMid};
            if(GJKQuadraticIntersected(&StartA, &StartB, Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_line_segment_support EndA = {CapsuleA->P0+DeltaA*tMid, CapsuleA->P1+DeltaA*tMid, DeltaA*tEnd};
            moving_line_segment_support EndB = {CapsuleB->P0+DeltaB*tMid, CapsuleB->P1+DeltaB*tMid, DeltaB*tEnd};            
            if(GJKQuadraticIntersected(&EndA, &EndB, Radius))
            {
                tStart = tMid;
                continue;
            }
            
            return tStart;
        }        
    }
    
    return INFINITY;
}

f32 SphereSphereTOI(sphere* SphereA, v3f DeltaA, sphere* SphereB, v3f DeltaB)
{
    sphere LargeSphere = CreateSphere(SphereB->CenterP, SphereA->Radius+SphereB->Radius);
    v3f Delta = DeltaA-DeltaB;
    v3f LineSegment[2] = {SphereA->CenterP, SphereA->CenterP+Delta};
    
    f32 Result = LineSegmentSphereIntersection(LineSegment, &LargeSphere);
    return Result;
}

f32 SphereCapsuleTOI(sphere* Sphere, v3f DeltaA, capsule* Capsule, v3f DeltaB)
{
    capsule LargeCapsule = CreateCapsule(Capsule->P0, Capsule->P1, Sphere->Radius+Capsule->Radius);
    v3f Delta = DeltaA-DeltaB;
    v3f LineSegment[2] = {Sphere->CenterP, Sphere->CenterP+Delta};
    
    f32 Result = LineSegmentCapsuleIntersection(LineSegment, &LargeCapsule);
    return Result;
}

toi_result FindStaticTOI(simulation* Simulation, sim_entity* SimEntity)
{
    toi_result Result = InvalidTOIResult();    
    
#define UPDATE_HIT() \
    if((t != INFINITY) && (t < Result.t)) \
    { \
        Result.HitEntity = TestSimEntity; \
        Result.t = t; \
        Result.VolumeA = VolumeA; \
        Result.VolumeB = VolumeB; \
    }
    
    FOR_EACH(VolumeA, SimEntity->CollisionVolumes)        
    {        
        switch(VolumeA->Type)
        {
            case COLLISION_VOLUME_TYPE_SPHERE:
            {                
                sphere SphereA = TransformSphere(&VolumeA->Sphere, SimEntity->Transform);
                FOR_EACH(TestSimEntity, &Simulation->SimEntityStorage)
                {
                    if((TestSimEntity != SimEntity) && IsEntityType((entity*)TestSimEntity->UserData, ENTITY_TYPE_STATIC))
                    {                        
                        FOR_EACH(VolumeB, TestSimEntity->CollisionVolumes)                            
                        {                            
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {                                    
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestSimEntity->Transform);                                    
                                    f32 t = SphereSphereTOI(&SphereA, SimEntity->MoveDelta, &SphereB, V3());
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestSimEntity->Transform);                                    
                                    f32 t = SphereCapsuleTOI(&SphereA, SimEntity->MoveDelta, &CapsuleB, V3());
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, TestSimEntity->Transform);
                                    
                                    f32 t = SphereHullTOI(&SphereA, SimEntity->MoveDelta, HullB, TransformB, V3());
                                    UPDATE_HIT();                        
                                } break;
                            }
                        }
                    }
                }
                
            } break;
            
            case COLLISION_VOLUME_TYPE_CAPSULE:
            {                
                capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, SimEntity->Transform);
                FOR_EACH(TestSimEntity, &Simulation->SimEntityStorage)
                {
                    if((TestSimEntity != SimEntity) && IsEntityType((entity*)TestSimEntity->UserData, ENTITY_TYPE_STATIC))
                    {                        
                        FOR_EACH(VolumeB, TestSimEntity->CollisionVolumes)
                        {
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestSimEntity->Transform);                                    
                                    f32 t = SphereCapsuleTOI(&SphereB, V3(), &CapsuleA, SimEntity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestSimEntity->Transform);                                    
                                    f32 t = CapsuleCapsuleTOI(&CapsuleA, SimEntity->MoveDelta, &CapsuleB, V3());
                                    UPDATE_HIT();
                                } break;                                                
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, TestSimEntity->Transform);                                    
                                    f32 t = CapsuleHullTOI(&CapsuleA, SimEntity->MoveDelta, HullB, TransformB, V3());
                                    UPDATE_HIT();
                                } break;
                            }
                        }
                    }
                }            
                
            } break;
            
            case COLLISION_VOLUME_TYPE_CONVEX_HULL:
            {                
                convex_hull* HullA = VolumeA->ConvexHull;
                sqt TransformA = ToParentCoordinates(HullA->Header.Transform, SimEntity->Transform);
                
                FOR_EACH(TestSimEntity, &Simulation->SimEntityStorage)
                {
                    if((TestSimEntity != SimEntity) && IsEntityType((entity*)TestSimEntity->UserData, ENTITY_TYPE_STATIC))
                    {                              
                        FOR_EACH(VolumeB, TestSimEntity->CollisionVolumes)
                        {
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestSimEntity->Transform);                                    
                                    f32 t = SphereHullTOI(&SphereB, V3(), HullA, TransformA, SimEntity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestSimEntity->Transform);
                                    
                                    f32 t = CapsuleHullTOI(&CapsuleB, V3(), HullA, TransformA, SimEntity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = ToParentCoordinates(HullB->Header.Transform, TestSimEntity->Transform);                            
                                    
                                    f32 t = HullHullTOI(HullA, TransformA, SimEntity->MoveDelta, HullB, TransformB, V3());
                                    UPDATE_HIT();                                                        
                                } break;
                            }
                        }
                    }
                }            
            }
        }    
    }
#undef UPDATE_HIT
    
    return Result;
}