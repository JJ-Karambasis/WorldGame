#include "support.h"
#include "gjk.cpp"
#include "epa.cpp"

capsule CreateCapsule(v3f P0, v3f P1, f32 Radius)
{
    capsule Result;
    Result.P0     = P0;
    Result.P1     = P1;
    Result.Radius = Radius;
    return Result;
}

sphere CreateSphere(v3f CenterP, f32 Radius)
{
    sphere Result;
    Result.CenterP = CenterP;    
    Result.Radius  = Radius;
    return Result;
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


void AttachCollisionVolume(world_entity* Entity, collision_volume* Volume)
{    
    if(!Entity->CollisionVolumes)
        Entity->CollisionVolumes = Volume;   
    else
    {
        Volume->Next = Entity->CollisionVolumes;
        Entity->CollisionVolumes = Volume;
    }    
}

template <typename type>
void AddCollisionVolume(game* Game, world_entity* Entity, type* Collider)
{
    collision_volume* Volume = AllocateListEntry<collision_volume>(&Game->CollisionVolumeStorage, &Game->GameStorage);
    AttachToCollisionVolume(Volume, Collider);
    AttachCollisionVolume(Entity, Volume);
}

capsule TransformCapsule(capsule* Capsule, sqt Transform)
{
    capsule Result;
    Result.P0 = NoScaleTransformV3(Capsule->P0, Transform);
    Result.P1 = NoScaleTransformV3(Capsule->P1, Transform);
    
    u32 Component = LargestComponent(Transform.Scale);
    Result.Radius = Capsule->Radius*Transform.Scale[Component];
    return Result;
}

sphere TransformSphere(sphere* Sphere, sqt Transform)
{
    sphere Result = {};
    
    u32 Component = LargestComponent(Transform.Scale);    
    Result.Radius = Sphere->Radius*Transform.Scale[Component];    
    
    Result.CenterP = Sphere->CenterP + Transform.Translation;    
    return Result;
}

f32 RaySphereIntersection(v3f Origin, v3f Direction, sphere* Sphere)
{
    v3f CO = Origin-Sphere->CenterP;
    f32 A = Dot(CO, Direction);
    f32 B = SquareMagnitude(CO) - Square(Sphere->Radius);
    
    if(A > 0 && B > 0) return INFINITY;
    f32 Discr = Square(A) - B;
    if(Discr < 0) return INFINITY;
    
    f32 Result = -A - Sqrt(Discr);
    if(Result < 0) Result = 0;
    return Result;
}

f32 RayCapsuleIntersection(v3f Origin, v3f Direction, capsule* Capsule)
{
    v3f AB = Capsule->P1-Capsule->P0;
    v3f AO = Origin - Capsule->P0;
    
    f32 RayCapsuleProjection = Dot(AB, Direction);
    f32 LineSegmentCapsuleProjection = Dot(AB, AO);
    f32 ABSqr = SquareMagnitude(AB);
    
    f32 m = RayCapsuleProjection / ABSqr;
    f32 n = LineSegmentCapsuleProjection / ABSqr;
    
    v3f Q = Direction - (AB*m);
    v3f R = AO - (AB*n);
    
    f32 a = SquareMagnitude(Q);
    f32 b = 2.0f * Dot(Q, R);
    f32 c = SquareMagnitude(R) - Square(Capsule->Radius);
    
    if(IsFuzzyZero(a))
    {
        sphere SphereA = CreateSphere(Capsule->P0, Capsule->Radius);
        sphere SphereB = CreateSphere(Capsule->P1, Capsule->Radius);
        
        f32 tA = RaySphereIntersection(Origin, Direction, &SphereA);
        f32 tB = RaySphereIntersection(Origin, Direction, &SphereB);
        
        if(tA == INFINITY || tB == INFINITY)
            return INFINITY;
        
        f32 Result = MinimumF32(tA, tB);
        return Result;
    }
    
    quadratic_equation_result QEqu = SolveQuadraticEquation(a, b, c);
    if(QEqu.RootCount < 2)
        return INFINITY;
    
    if(QEqu.Roots[0] < 0 && QEqu.Roots[1] < 0)
        return INFINITY;
    
    f32 tMin = MinimumF32(MaximumF32(QEqu.Roots[0], 0.0f), 
                          MaximumF32(QEqu.Roots[1], 0.0f));
    
    f32 t0 = tMin*m + n;
    if(t0 < 0)
    {
        sphere Sphere = CreateSphere(Capsule->P0, Capsule->Radius);
        return RaySphereIntersection(Origin, Direction, &Sphere);
    }
    else if(t0 > 1)
    {
        sphere Sphere = CreateSphere(Capsule->P1, Capsule->Radius);
        return RaySphereIntersection(Origin, Direction, &Sphere);
    }
    else
    {
        return tMin;
    }        
}

f32 LineSegmentSphereIntersection(v3f* LineSegment, sphere* Sphere)
{
    v3f D = LineSegment[1]-LineSegment[0];
    f32 SegmentLength = Magnitude(D);
    D /= SegmentLength;
    
    f32 Result = RaySphereIntersection(LineSegment[0], D, Sphere);
    if(Result != INFINITY)
    {
        if(Result > SegmentLength)
            return INFINITY;
        
        Result /= SegmentLength;        
    }    
    return Result;
}

f32 LineSegmentCapsuleIntersection(v3f* LineSegment, capsule* Capsule)
{
    v3f D = LineSegment[1]-LineSegment[0];
    f32 SegmentLength = Magnitude(D);
    D /= SegmentLength;
    
    f32 Result = RayCapsuleIntersection(LineSegment[0], D, Capsule);
    if(Result != INFINITY)
    {
        if(Result > SegmentLength)
            return INFINITY;
        
        Result /= SegmentLength;
    }
    return Result;
}

f32 LineSegmentsClosestPoints(v3f* Result, v3f* A, v3f* B)
{
    v3f D1 = A[1]-A[0];
    v3f D2 = B[1]-B[0];
    v3f R = A[0]-B[0];
    
    float a = SquareMagnitude(D1);
    float e = SquareMagnitude(D2);
    float f = Dot(D2, R);
    
    if(IsFuzzyZero(a) && IsFuzzyZero(e))
    {
        Result[0] = A[0];
        Result[1] = B[0];
        float SqrDist = SquareMagnitude(Result[0] - Result[1]);
        return SqrDist;
    }
    
    float t;
    float s;
    
    if(IsFuzzyZero(a))
    {
        s = 0.0f;
        t = SaturateF32(f/e);        
    }
    else
    {
        float c = Dot(D1, R);
        if(IsFuzzyZero(e))
        {
            t = 0.0f;
            s = SaturateF32(-c/a);
        }
        else
        {
            float b = Dot(D1, D2);
            float Denom = (a*e)-Square(b);
            
            if(Denom != 0.0f)            
                s = SaturateF32((b*f - c*e) / Denom);            
            else            
                s = 0.0f;    
            
            float tnom = b*s + f;
            
            if(tnom < 0.0f)
            {
                t = 0.0f;
                s = SaturateF32(-c / a);
            }
            else if(tnom > e)
            {
                t = 1.0f;
                s = SaturateF32((b - c) / a);
            }
            else
            {
                t = tnom / e;
            }             
        }
    }
    
    Result[0] = A[0] + D1*s;
    Result[1] = B[0] + D2*t;
    
    float SqrDist = SquareMagnitude(Result[0] - Result[1]);
    return SqrDist;
}

v3f PointLineSegmentClosestPoint(v3f P, v3f* LineSegment)
{
    v3f AB = LineSegment[1]-LineSegment[0];    
    f32 t = SaturateF32(Dot(P - LineSegment[0], AB) / SquareMagnitude(AB));
    v3f Result = LineSegment[0] + t*AB;        
    return Result;
}

b32 RayTriangleIntersection(v3f RayOrigin, v3f RayDirection, v3f P0, v3f P1, v3f P2, f32* t, f32* u, f32* v)
{    
    v3f Edge1 = P1 - P0;
    v3f Edge2 = P2 - P0;
    
    v3f PVec = Cross(RayDirection, Edge2);
    
    f32 Det = Dot(Edge1, PVec);
    
    if(Det == 0)
        return false;
    
    v3f TVec = RayOrigin - P0;
    
    *u = Dot(TVec, PVec);
    if(*u < 0.0f || *u > Det)
        return false;
    
    v3f QVec = Cross(TVec, Edge1);
    
    *v = Dot(RayDirection, QVec);
    if(*v < 0.0f || *u + *v > Det)
        return false;
    
    *t = Dot(Edge2, QVec);
    
    f32 InvDet = 1.0f / Det;
    *t *= InvDet;
    *u *= InvDet;
    *v *= InvDet;
    
    return true;
}

ray_mesh_intersection_result RayMeshIntersection(v3f RayOrigin, v3f RayDirection, mesh* Mesh, mesh_info* MeshInfo, sqt MeshTransform)
{
    ray_mesh_intersection_result Result = {};    
    
    u32 VertexStride = GetVertexStride(MeshInfo);
    u32 IndexStride = GetIndexStride(MeshInfo);        
    u32 IndexSize = IndexStride*MeshInfo->Header.IndexCount;
    
    u32 i = 0;
    while(i*IndexStride < IndexSize)
    {
        u32 Index1 = 0;
        u32 Index2 = 0;
        u32 Index3 = 0;
        v3f Vertex1 = V3(0,0,0);
        v3f Vertex2 = V3(0,0,0);
        v3f Vertex3 = V3(0,0,0);
        
        if(MeshInfo->Header.IsIndexFormat32)
        {
            Index1 = *(u32*)((u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index2 = *(u32*)((u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index3 = *(u32*)((u8*)Mesh->Indices + i*IndexStride);
            i++;            
        }
        else
        {            
            Index1 = *(u16*)((u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index2 = *(u16*)((u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index3 = *(u16*)((u8*)Mesh->Indices + i*IndexStride);
            i++;
        }        
        
        if(MeshInfo->Header.IsSkeletalMesh)
        {
            vertex_p3_n3_t4_uv_weights TVertex1 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Mesh->Vertices + (Index1*VertexStride));
            vertex_p3_n3_t4_uv_weights TVertex2 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Mesh->Vertices + (Index2*VertexStride));
            vertex_p3_n3_t4_uv_weights TVertex3 = *(vertex_p3_n3_t4_uv_weights*)((u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;            
        }
        else
        {            
            vertex_p3_n3_t4_uv TVertex1 = *(vertex_p3_n3_t4_uv*)((u8*)Mesh->Vertices + (Index1*VertexStride));
            vertex_p3_n3_t4_uv TVertex2 = *(vertex_p3_n3_t4_uv*)((u8*)Mesh->Vertices + (Index2*VertexStride));
            vertex_p3_n3_t4_uv TVertex3 = *(vertex_p3_n3_t4_uv*)((u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        
        Vertex1 = TransformV3(Vertex1, MeshTransform);
        Vertex2 = TransformV3(Vertex2, MeshTransform);
        Vertex3 = TransformV3(Vertex3, MeshTransform);
        
        f32 t, u, v;        
        if(RayTriangleIntersection(RayOrigin, RayDirection, Vertex1, Vertex2, Vertex3, &t, &u, &v))
        {
            if(!Result.FoundCollision || (t < Result.t))
            {
                Result.FoundCollision = true;
                Result.t = t;
                Result.u = u;
                Result.v = v;
            }
        }        
    }
    
    return Result;
}

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

b32 FindCollisions(time_of_impact_result* Result, world* World, world_entity* Entity)
{
    *Result = {};
    Result->t = INFINITY;
    
#define UPDATE_HIT() \
    if((t != INFINITY) && (t < Result->t)) \
    { \
        Result->HitEntity = TestEntity; \
        Result->t = t; \
        Result->VolumeA = VolumeA; \
        Result->VolumeB = VolumeB; \
    }
    
    for(collision_volume* VolumeA = Entity->CollisionVolumes; VolumeA; VolumeA = VolumeA->Next)
    {        
        switch(VolumeA->Type)
        {
            case COLLISION_VOLUME_TYPE_SPHERE:
            {
                sphere SphereA = TransformSphere(&VolumeA->Sphere, Entity->Transform);
                FOR_EACH(TestEntity, &World->EntityPool)
                {
                    if(TestEntity != Entity)
                    {
                        for(collision_volume* VolumeB = TestEntity->CollisionVolumes; VolumeB; VolumeB = VolumeB->Next)
                        {                            
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestEntity->Transform);
                                    
                                    f32 t = SphereSphereTOI(&SphereA, Entity->MoveDelta, &SphereB, V3());
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestEntity->Transform);
                                    
                                    f32 t = SphereCapsuleTOI(&SphereA, Entity->MoveDelta, &CapsuleB, V3());
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = HullB->Header.Transform*TestEntity->Transform;
                                    
                                    f32 t = SphereHullTOI(&SphereA, Entity->MoveDelta, HullB, TransformB, V3());
                                    UPDATE_HIT();                        
                                } break;
                            }
                        }
                    }
                }
                
            } break;
            
            case COLLISION_VOLUME_TYPE_CAPSULE:
            {
                capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, Entity->Transform);
                FOR_EACH(TestEntity, &World->EntityPool)
                {
                    if(TestEntity != Entity)
                    {
                        for(collision_volume* VolumeB = TestEntity->CollisionVolumes; VolumeB; VolumeB = VolumeB->Next)
                        {
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestEntity->Transform);
                                    
                                    f32 t = SphereCapsuleTOI(&SphereB, V3(), &CapsuleA, Entity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestEntity->Transform);
                                    
                                    f32 t = CapsuleCapsuleTOI(&CapsuleA, Entity->MoveDelta, &CapsuleB, V3());
                                    UPDATE_HIT();
                                } break;                                                
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = HullB->Header.Transform*TestEntity->Transform;
                                    
                                    f32 t = CapsuleHullTOI(&CapsuleA, Entity->MoveDelta, HullB, TransformB, V3());
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
                sqt TransformA = HullA->Header.Transform*Entity->Transform;
                
                FOR_EACH(TestEntity, &World->EntityPool)
                {
                    if(TestEntity != Entity)
                    {                                        
                        for(collision_volume* VolumeB = TestEntity->CollisionVolumes; VolumeB; VolumeB = VolumeB->Next)
                        {
                            switch(VolumeB->Type)
                            {
                                case COLLISION_VOLUME_TYPE_SPHERE:
                                {
                                    sphere SphereB = TransformSphere(&VolumeB->Sphere, TestEntity->Transform);                                    
                                    f32 t = SphereHullTOI(&SphereB, V3(), HullA, TransformA, Entity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CAPSULE:
                                {
                                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, TestEntity->Transform);
                                    
                                    f32 t = CapsuleHullTOI(&CapsuleB, V3(), HullA, TransformA, Entity->MoveDelta);
                                    UPDATE_HIT();
                                } break;
                                
                                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                                {
                                    convex_hull* HullB = VolumeB->ConvexHull;
                                    sqt TransformB = HullB->Header.Transform*TestEntity->Transform;                            
                                    
                                    f32 t = HullHullTOI(HullA, TransformA, Entity->MoveDelta, HullB, TransformB, V3());
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
    
    return (Result->HitEntity != NULL);
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

penetration GetPenetration(world_entity* AEntity, world_entity* BEntity, collision_volume* VolumeA, collision_volume* VolumeB, f32 tHit)
{
    ASSERT((tHit >= 0) && (tHit <= 1.0f));    
    
    penetration Result = {};    
    switch(VolumeA->Type)
    {
        case COLLISION_VOLUME_TYPE_SPHERE:
        {
            sphere SphereA = TransformSphere(&VolumeA->Sphere, AEntity->Transform);
            SphereA.CenterP += AEntity->MoveDelta*tHit;
            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, BEntity->Transform);
                    SphereB.CenterP += BEntity->MoveDelta*tHit;
                    
                    Result = GetSphereSpherePenetration(&SphereA, &SphereB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, BEntity->Transform);
                    CapsuleB.P0 += BEntity->MoveDelta*tHit;
                    CapsuleB.P1 += BEntity->MoveDelta*tHit;
                    
                    Result = GetSphereCapsulePenetration(&SphereA, &CapsuleB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = HullB->Header.Transform*BEntity->Transform;                        
                    TransformB.Translation += BEntity->MoveDelta*tHit;                                                            
                    
                    Result = GetSphereHullPenetration(&SphereA, HullB, TransformB);
                } break;
            }
            
        } break;
        
        case COLLISION_VOLUME_TYPE_CAPSULE:
        {
            capsule CapsuleA = TransformCapsule(&VolumeA->Capsule, AEntity->Transform);
            CapsuleA.P0 += AEntity->MoveDelta*tHit;
            CapsuleA.P1 += AEntity->MoveDelta*tHit;            
            
            switch(VolumeB->Type)
            {   
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, BEntity->Transform);
                    SphereB.CenterP += BEntity->MoveDelta*tHit;
                    
                    Result = GetSphereCapsulePenetration(&SphereB, &CapsuleA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, BEntity->Transform);
                    CapsuleB.P0 += BEntity->MoveDelta*tHit;
                    CapsuleB.P1 += BEntity->MoveDelta*tHit;
                    
                    Result = GetCapsuleCapsulePenetration(&CapsuleA, &CapsuleB);
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = HullB->Header.Transform*BEntity->Transform;                        
                    TransformB.Translation += BEntity->MoveDelta*tHit;
                    
                    Result = GetCapsuleHullPenetration(&CapsuleA, HullB, TransformB);
                }
            }
            
        } break;
        
        case COLLISION_VOLUME_TYPE_CONVEX_HULL:
        {
            convex_hull* HullA = VolumeA->ConvexHull;
            sqt TransformA = HullA->Header.Transform*AEntity->Transform;
            TransformA.Translation += AEntity->MoveDelta*tHit;
            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    sphere SphereB = TransformSphere(&VolumeB->Sphere, BEntity->Transform);
                    SphereB.CenterP += BEntity->MoveDelta*tHit;
                    
                    Result = GetSphereHullPenetration(&SphereB, HullA, TransformA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {
                    capsule CapsuleB = TransformCapsule(&VolumeB->Capsule, BEntity->Transform);
                    CapsuleB.P0 += BEntity->MoveDelta*tHit;
                    CapsuleB.P1 += BEntity->MoveDelta*tHit;
                    
                    Result = GetCapsuleHullPenetration(&CapsuleB, HullA, TransformA);
                    Result.Normal = -Result.Normal;
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {
                    convex_hull* HullB = VolumeB->ConvexHull;
                    sqt TransformB = HullB->Header.Transform*BEntity->Transform;                        
                    TransformB.Translation += BEntity->MoveDelta*tHit;
                    
                    Result = GetHullHullPenetration(HullA, TransformA, HullB, TransformB);
                }
            }
            
        } break;
    }
    
    return Result;
}

collision_result DetectCollisions(world* World, world_entity* Entity)
{
    platform_time Start = Global_Platform->Clock();
    
    collision_result Result = {};
    Result.t = INFINITY;    
    
    time_of_impact_result TOIResult = {};
    TOIResult.t = INFINITY;
    
    penetration Penetration = {};
    if(FindCollisions(&TOIResult, World, Entity))
    {
        Result.Penetration = GetPenetration(Entity, TOIResult.HitEntity, TOIResult.VolumeA, TOIResult.VolumeB, TOIResult.t);    
        Result.HitEntity = TOIResult.HitEntity;
        Result.t = TOIResult.t;
    }        
    
    platform_time End = Global_Platform->Clock();
    CONSOLE_LOG("Elapsed Collision Time %f\n", Global_Platform->ElapsedTime(End, Start)*1000.0);
    
    return Result;
}