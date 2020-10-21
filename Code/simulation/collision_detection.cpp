ak_f32 RaySphereIntersection(ak_v3f Origin, ak_v3f Direction, ak_v3f CenterP, ak_f32 Radius)
{
    ak_v3f CO = Origin-CenterP;
    ak_f32 A = AK_Dot(CO, Direction);
    ak_f32 B = AK_SqrMagnitude(CO) - AK_Square(Radius);
    
    if(A > 0 && B > 0) return INFINITY;
    ak_f32 Discr = AK_Square(A) - B;
    if(Discr < 0) return INFINITY;
    
    ak_f32 Result = -A - AK_Sqrt(Discr);
    if(Result < 0) Result = 0;
    return Result;
}

ak_f32 RaySphereIntersection(ak_v3f Origin, ak_v3f Direction, sphere* Sphere)
{
    return RaySphereIntersection(Origin, Direction, Sphere->CenterP, Sphere->Radius);
}

ak_f32 RayCapsuleIntersection(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, ak_f32 Radius)
{
    ak_v3f AB = P1-P0;
    ak_v3f AO = Origin - P0;
    
    ak_f32 RayCapsuleProjection = AK_Dot(AB, Direction);
    ak_f32 LineSegmentCapsuleProjection = AK_Dot(AB, AO);
    ak_f32 ABSqr = AK_SqrMagnitude(AB);
    
    ak_f32 m = RayCapsuleProjection / ABSqr;
    ak_f32 n = LineSegmentCapsuleProjection / ABSqr;
    
    ak_v3f Q = Direction - (AB*m);
    ak_v3f R = AO - (AB*n);
    
    ak_f32 a = AK_SqrMagnitude(Q);
    ak_f32 b = 2.0f * AK_Dot(Q, R);
    ak_f32 c = AK_SqrMagnitude(R) - AK_Square(Radius);
    
    if(AK_EqualZeroEps(a))
    {        
        ak_f32 tA = RaySphereIntersection(Origin, Direction, P0, Radius);
        ak_f32 tB = RaySphereIntersection(Origin, Direction, P1, Radius);
        
        if(tA == INFINITY || tB == INFINITY)
            return INFINITY;
        
        ak_f32 Result = AK_Min(tA, tB);
        return Result;
    }
    
    ak_quadratic_equation_result QEqu = AK_SolveQuadraticEquation(a, b, c);
    if(QEqu.RootCount < 2)
        return INFINITY;
    
    if(QEqu.Roots[0] < 0 && QEqu.Roots[1] < 0)
        return INFINITY;
    
    ak_f32 tMin = AK_Min(QEqu.Roots[0], QEqu.Roots[1]);
    tMin = AK_Max(tMin, 0.0f);
    
    ak_f32 t0 = tMin*m + n;
    if(t0 < 0)
    {        
        return RaySphereIntersection(Origin, Direction, P0, Radius);
    }
    else if(t0 > 1)
    {        
        return RaySphereIntersection(Origin, Direction, P1, Radius);
    }
    else
    {
        return tMin;
    }        
}

ak_f32 RayCapsuleIntersection(ak_v3f Origin, ak_v3f Direction, capsule* Capsule)
{
    return RayCapsuleIntersection(Origin, Direction, Capsule->P0, Capsule->P1, Capsule->Radius);
}

ak_bool RayTriangleIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* t, ak_f32* u, ak_f32* v)
{    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(RayDirection, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))
        return false;
    
    ak_v3f TVec = RayOrigin - P0;
    
    *u = AK_Dot(TVec, PVec);
    if(*u < 0.0f || *u > Det)
        return false;
    
    ak_v3f QVec = AK_Cross(TVec, Edge1);
    
    *v = AK_Dot(RayDirection, QVec);
    if(*v < 0.0f || *u + *v > Det)
        return false;
    
    *t = AK_Dot(Edge2, QVec);
    
    ak_f32 InvDet = 1.0f / Det;
    *t *= InvDet;
    *u *= InvDet;
    *v *= InvDet;
    
    return true;
}

ak_bool RayTriangleIntersectionNoCull(ak_v3f RayOrigin, ak_v3f RayDirection, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* t, ak_f32* u, ak_f32* v)
{    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(RayDirection, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))    
        return false;    
    
    ak_f32 InverseDeterminant = 1.0f / Det;
    
    ak_v3f TVec = RayOrigin - P0;

    *u = AK_Dot(TVec, PVec) * InverseDeterminant;
    if(*u < 0 || *u > 1)
    {
        return false;
    }

    ak_v3f QVec = AK_Cross(TVec, Edge1);

    *v = AK_Dot(RayDirection, QVec) * InverseDeterminant;
    if(*v < 0 || *u + *v > 1)
    {
        return false;
    }

    *t = AK_Dot(Edge2, QVec) * InverseDeterminant;
    
    return true;
}

ray_mesh_intersection_result RayMeshIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, mesh* Mesh, mesh_info* MeshInfo, ak_m4f MeshTransform, ak_bool IsDevMesh = false, ak_bool TestCull = true)
{
    ray_mesh_intersection_result Result = {};    
    
    ak_u32 VertexStride = GetVertexStride(MeshInfo);
    ak_u32 IndexStride = GetIndexStride(MeshInfo);        
    ak_u32 IndexSize = IndexStride*MeshInfo->Header.IndexCount;
    
    ak_u32 i = 0;
    while(i*IndexStride < IndexSize)
    {
        ak_u32 Index1 = 0;
        ak_u32 Index2 = 0;
        ak_u32 Index3 = 0;
        ak_v3f Vertex1 = AK_V3f(0,0,0);
        ak_v3f Vertex2 = AK_V3f(0,0,0);
        ak_v3f Vertex3 = AK_V3f(0,0,0);
        
        if(MeshInfo->Header.IsIndexFormat32)
        {
            Index1 = *(ak_u32*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index2 = *(ak_u32*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index3 = *(ak_u32*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;            
        }
        else
        {            
            Index1 = *(ak_u16*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index2 = *(ak_u16*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;
            Index3 = *(ak_u16*)((ak_u8*)Mesh->Indices + i*IndexStride);
            i++;
        }        
        
        if(IsDevMesh)
        {
            VertexStride = sizeof(ak_vertex_p3);
            ak_vertex_p3 TVertex1 = *(ak_vertex_p3*)((ak_u8*)Mesh->Vertices + (Index1*VertexStride));
            ak_vertex_p3 TVertex2 = *(ak_vertex_p3*)((ak_u8*)Mesh->Vertices + (Index2*VertexStride));
            ak_vertex_p3 TVertex3 = *(ak_vertex_p3*)((ak_u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        else if(MeshInfo->Header.IsSkeletalMesh)
        {
            ak_vertex_p3_n3_uv_w TVertex1 = *(ak_vertex_p3_n3_uv_w*)((ak_u8*)Mesh->Vertices + (Index1*VertexStride));
            ak_vertex_p3_n3_uv_w TVertex2 = *(ak_vertex_p3_n3_uv_w*)((ak_u8*)Mesh->Vertices + (Index2*VertexStride));
            ak_vertex_p3_n3_uv_w TVertex3 = *(ak_vertex_p3_n3_uv_w*)((ak_u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;            
        }
        else
        {            
            ak_vertex_p3_n3_uv TVertex1 = *(ak_vertex_p3_n3_uv*)((ak_u8*)Mesh->Vertices + (Index1*VertexStride));
            ak_vertex_p3_n3_uv TVertex2 = *(ak_vertex_p3_n3_uv*)((ak_u8*)Mesh->Vertices + (Index2*VertexStride));
            ak_vertex_p3_n3_uv TVertex3 = *(ak_vertex_p3_n3_uv*)((ak_u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;
        }
        
        Vertex1 = AK_TransformPoint(Vertex1, MeshTransform);
        Vertex2 = AK_TransformPoint(Vertex2, MeshTransform);
        Vertex3 = AK_TransformPoint(Vertex3, MeshTransform);
        
        ak_f32 t, u, v;
        ak_bool IntersectionFound = false;
        if(TestCull)
        {
            IntersectionFound = RayTriangleIntersection(RayOrigin, RayDirection, Vertex1, Vertex2, Vertex3, &t, &u, &v);
        }
        else
        {
            IntersectionFound = RayTriangleIntersectionNoCull(RayOrigin, RayDirection, Vertex1, Vertex2, Vertex3, &t, &u, &v);
        }
        
        if(IntersectionFound)
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

ray_mesh_intersection_result RayMeshIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, mesh* Mesh, mesh_info* MeshInfo, ak_sqtf MeshTransform, ak_bool IsDevMesh = false, ak_bool TestCull = true)
{
    return RayMeshIntersection(RayOrigin, RayDirection, Mesh, MeshInfo, AK_TransformM4(MeshTransform), IsDevMesh, TestCull);
}


ak_f32 RayPlaneIntersection(ak_v3f PlaneNormal, ak_v3f PlanePoint, ak_v3f Origin, ak_v3f Direction)
{    
    ak_f32 Denom = AK_Dot(PlaneNormal, Direction);
    if(AK_EqualZeroEps(Denom)) return INFINITY;    
    
    ak_f32 t = AK_Dot(PlanePoint - Origin, PlaneNormal) / Denom;
    if(t <= 0) return INFINITY;    
    
    return t;
}

ak_f32 LineSegmentSphereIntersection(ak_v3f* LineSegment, sphere* Sphere)
{
    ak_v3f D = LineSegment[1]-LineSegment[0];
    ak_f32 SegmentLength = AK_Magnitude(D);
    if(AK_EqualZeroEps(SegmentLength))
        return INFINITY;
    
    D /= SegmentLength;
    
    ak_f32 Result = RaySphereIntersection(LineSegment[0], D, Sphere);
    if(Result != INFINITY)
    {
        if(Result > SegmentLength)
            return INFINITY;
        
        Result /= SegmentLength;        
    }    
    return Result;
}

ak_f32 LineSegmentCapsuleIntersection(ak_v3f* LineSegment, capsule* Capsule)
{
    ak_v3f D = LineSegment[1]-LineSegment[0];
    ak_f32 SegmentLength = AK_Magnitude(D);
    if(AK_EqualZeroEps(SegmentLength))
        return INFINITY;
    
    D /= SegmentLength;
    
    ak_f32 Result = RayCapsuleIntersection(LineSegment[0], D, Capsule);
    if(Result != INFINITY)
    {
        if(Result > SegmentLength)
            return INFINITY;
        
        Result /= SegmentLength;
    }
    return Result;
}

ak_f32 LineSegmentsClosestPoints(ak_v3f* Result, ak_v3f* A, ak_v3f* B)
{
    ak_v3f D1 = A[1]-A[0];
    ak_v3f D2 = B[1]-B[0];
    ak_v3f R = A[0]-B[0];
    
    ak_f32 a = AK_SqrMagnitude(D1);
    ak_f32 e = AK_SqrMagnitude(D2);
    ak_f32 f = AK_Dot(D2, R);
    
    if(AK_EqualZeroEps(a) && AK_EqualZeroEps(e))
    {
        Result[0] = A[0];
        Result[1] = B[0];
        ak_f32 SqrDist = AK_SqrMagnitude(Result[0] - Result[1]);
        return SqrDist;
    }
    
    ak_f32 t;
    ak_f32 s;
    
    if(AK_EqualZeroEps(a))
    {
        s = 0.0f;
        t = AK_Saturate(f/e);        
    }
    else
    {
        ak_f32 c = AK_Dot(D1, R);
        if(AK_EqualZeroEps(e))
        {
            t = 0.0f;
            s = AK_Saturate(-c/a);
        }
        else
        {
            ak_f32 b = AK_Dot(D1, D2);
            ak_f32 Denom = (a*e)-AK_Square(b);
            
            if(Denom != 0.0f)            
                s = AK_Saturate((b*f - c*e) / Denom);            
            else            
                s = 0.0f;    
            
            ak_f32 tnom = b*s + f;
            
            if(tnom < 0.0f)
            {
                t = 0.0f;
                s = AK_Saturate(-c / a);
            }
            else if(tnom > e)
            {
                t = 1.0f;
                s = AK_Saturate((b - c) / a);
            }
            else
            {
                t = tnom / e;
            }             
        }
    }
    
    Result[0] = A[0] + D1*s;
    Result[1] = B[0] + D2*t;
    
    ak_f32 SqrDist = AK_SqrMagnitude(Result[0] - Result[1]);
    return SqrDist;
}

ak_v3f PointLineSegmentClosestPoint(ak_v3f P, ak_v3f* LineSegment)
{
    ak_v3f AB = LineSegment[1]-LineSegment[0];    
    ak_f32 t = AK_Saturate(AK_Dot(P - LineSegment[0], AB) / AK_SqrMagnitude(AB));
    ak_v3f Result = LineSegment[0] + t*AB;        
    return Result;
}

contact_list GetSphereSphereContacts(sphere* SphereA, sphere* SphereB)
{
    contact_list Result = {};
    
    ak_v3f P0 = SphereA->CenterP;
    ak_v3f P1 = SphereB->CenterP;
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_SqrMagnitude(Normal);  
    
    //TODO(JJ): This assertion is a degenerate case. We probably should support it at some point. Happens when the sphere center point 
    //is on the capsule line thus a contact normal cannot be computed. Can probably just use an arbitrary normal
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Not Implemented"); 
    
    ak_f32 Radius = SphereA->Radius+SphereB->Radius;        
    if(NormalLength <= AK_Square(Radius))
    {
        NormalLength = AK_Sqrt(NormalLength);
        Normal /= NormalLength;
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        Result.Count = 1;
        Result.Ptr = GlobalArena->PushArray<contact>(Result.Count);
        
        ak_v3f PointOnSphereA  = P0 + Normal*SphereA->Radius;
        ak_v3f PointOnSphereB = P1 - Normal*SphereB->Radius;
        
        Result.Ptr[0].Normal = Normal;        
        Result.Ptr[0].Position = PointOnSphereA + ((PointOnSphereB-PointOnSphereA)*0.5f);
        Result.Ptr[0].Penetration = Radius-NormalLength;
    }
    
    return Result;    
}

contact_list GetSphereCapsuleContacts(sphere* Sphere, capsule* Capsule)
{
    contact_list Result = {};
    
    ak_v3f P0 = Sphere->CenterP;
    ak_v3f P1 = PointLineSegmentClosestPoint(P0, Capsule->P);
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_SqrMagnitude(Normal);  
    
    //TODO(JJ): This assertion is a degenerate case. We probably should support it at some point. Happens when the sphere center point 
    //is on the capsule line thus a contact normal cannot be computed. Can probably just use an arbitrary normal
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Not Implemented"); 
    
    ak_f32 Radius = Sphere->Radius+Capsule->Radius;        
    if(NormalLength <= AK_Square(Radius))
    {
        NormalLength = AK_Sqrt(NormalLength);
        Normal /= NormalLength;
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        Result.Count = 1;
        Result.Ptr = GlobalArena->PushArray<contact>(Result.Count);
        
        ak_v3f PointOnSphere  = P0 + Normal*Sphere->Radius;
        ak_v3f PointOnCapsule = P1 - Normal*Capsule->Radius;
        
        Result.Ptr[0].Normal = Normal;        
        Result.Ptr[0].Position = PointOnSphere + ((PointOnCapsule-PointOnSphere)*0.5f);
        Result.Ptr[0].Penetration = Radius-NormalLength;
    }
    
    return Result;    
}

contact_list GetSphereHullContacts(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    contact_list Result = {};
    
    point_support PointGJK = {Sphere->CenterP};
    convex_hull_support ConvexHullGJK = {ConvexHull, ConvexHullTransform};        
    gjk_distance Distance = GJKDistance(&PointGJK, &ConvexHullGJK);
    
    ak_v3f P0, P1;
    Distance.GetClosestPoints(&P0, &P1);
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_SqrMagnitude(Normal);
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined from gjk");
    
    if(NormalLength <= AK_Square(Sphere->Radius))
    {        
        NormalLength = AK_Sqrt(NormalLength);
        Normal /= NormalLength;
        
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        Result.Count = 1;
        Result.Ptr = GlobalArena->PushArray<contact>(Result.Count);
        
        Result.Ptr[0].Normal = Normal;
        Result.Ptr[0].Position = P1;
        Result.Ptr[0].Penetration = Sphere->Radius-NormalLength;
    }
    
    return Result;
}

//TODO(JJ): This function is unfinished. There are a couple of degenerate cases we need to handle
contact_list GetCapsuleCapsuleContacts(capsule* CapsuleA, capsule* CapsuleB)
{
    ak_v3f P[2];
    LineSegmentsClosestPoints(P, CapsuleA->P, CapsuleB->P);
    
    ak_v3f P0 = P[0];
    ak_v3f P1 = P[1];
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_SqrMagnitude(Normal);  
        
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined from gjk");
        
    ak_f32 Radius = CapsuleA->Radius+CapsuleB->Radius;        
    
    contact_list Result = {};
    if(NormalLength <= AK_Square(Radius))
    {
        NormalLength = AK_Sqrt(NormalLength);
        Normal /= NormalLength;
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        Result.Count = 1;
        Result.Ptr = GlobalArena->PushArray<contact>(Result.Count);
        
        ak_v3f PointOnSphere  = P0 + Normal*CapsuleA->Radius;
        ak_v3f PointOnCapsule = P1 - Normal*CapsuleB->Radius;
        
        Result.Ptr[0].Normal = Normal;        
        Result.Ptr[0].Position = PointOnSphere + ((PointOnCapsule-PointOnSphere)*0.5f);
        Result.Ptr[0].Penetration = Radius-NormalLength;
    }    
    return Result;
}

contact_list GetCapsuleHullContacts(capsule* Capsule, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    line_segment_support AGJK = {Capsule->P0, Capsule->P1};
    convex_hull_support  BGJK = {ConvexHull, ConvexHullTransform};    
    gjk_distance Distance = GJKDistance(&AGJK, &BGJK);
    
    ak_v3f P0, P1;
    Distance.GetClosestPoints(&P0, &P1);                
    
    ak_v3f Normal = P1-P0;
    ak_f32 NormalLength = AK_SqrMagnitude(Normal);  
    
    AK_Assert(!AK_EqualZeroEps(NormalLength), "Normal is not defined from gjk");
    
    ak_f32 Radius = Capsule->Radius;
    
    contact_list Result = {};
    if(NormalLength <= AK_Square(Radius))
    {        
        NormalLength = AK_Sqrt(NormalLength);
        Normal /= NormalLength;
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        
        Result.Count = 1;
        Result.Ptr = GlobalArena->PushArray<contact>(Result.Count);
        
        Result.Ptr[0].Position = P1;
        Result.Ptr[0].Penetration = NormalLength - Radius;        
        Result.Ptr[0].Normal = Normal;        
    }
    
    return Result;
}


contact GetQuadraticDeepestContact(ak_v3f P0, ak_v3f P1, ak_f32 RadiusA, ak_f32 RadiusB)
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
    contact Result = GetQuadraticDeepestContact(SphereA->CenterP, SphereB->CenterP, SphereA->Radius, SphereB->Radius);
    return Result;
}

contact GetSphereCapsuleDeepestContact(sphere* SphereA, capsule* CapsuleB)
{
    ak_v3f P0 = SphereA->CenterP;
    ak_v3f P1 = PointLineSegmentClosestPoint(P0, CapsuleB->P);
    contact Result = GetQuadraticDeepestContact(P0, P1, SphereA->Radius, CapsuleB->Radius);
    return Result;
}

contact GetSphereHullDeepestContact(sphere* Sphere, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    point_support PointGJK = {Sphere->CenterP};
    convex_hull_support ConvexHullGJK = {ConvexHull, ConvexHullTransform};
    
    gjk_distance Distance = GJKDistance(&PointGJK, &ConvexHullGJK);
    
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
    ak_v3f P[2];
    LineSegmentsClosestPoints(P, CapsuleA->P, CapsuleB->P);
    contact Result = GetQuadraticDeepestContact(P[0], P[1], CapsuleA->Radius, CapsuleB->Radius);
    return Result;
}

contact GetCapsuleHullDeepestContact(capsule* Capsule, convex_hull* ConvexHull, ak_sqtf ConvexHullTransform)
{
    line_segment_support AGJK = {Capsule->P0, Capsule->P1};
    convex_hull_support  BGJK = {ConvexHull, ConvexHullTransform};
    
    gjk_distance Distance = GJKDistance(&AGJK, &BGJK);
        
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

ak_bool EPATest(contact* Contact, convex_hull* ConvexHullA, ak_sqtf ConvexHullTransformA, convex_hull* ConvexHullB, ak_sqtf ConvexHullTransformB)
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
    
    ak_bool Intersecting = GJKIntersected(&AGJK, &BGJK);
    gjk_distance DistanceResult = GJKDistance(&AGJK, &BGJK);                                                                                                               
    ak_f32 SqrLength = AK_SqrMagnitude(DistanceResult.V);                                                
    ak_bool ShouldDoEPA = SqrLength < GJK_RELATIVE_ERROR;
    
    contact Contact;
    if(Intersecting || ShouldDoEPA)
    {
        if(EPATest(&Contact, ConvexHullA, ConvexHullTransformA, ConvexHullB, ConvexHullTransformB))
            return Contact;                            
    }    
    
    ak_v3f Witness0, Witness1;
    DistanceResult.GetClosestPoints(&Witness0, &Witness1);                            
    Contact.Normal = AK_Normalize(Witness1-Witness0);
    Contact.Penetration = 0;
    Contact.Position = Witness0 + ((Witness1-Witness0)*0.5f);
    return Contact;    
}

#define BISECTION_EPSILON 1e-6f
inline ak_f32 
GetBisectionMid(ak_v3f DeltaA, ak_v3f DeltaB, ak_f32 tStart, ak_f32 tEnd)
{
    ak_v3f Delta = DeltaA-DeltaB;    
    if(AK_EqualApprox(Delta*tStart, Delta*tEnd, BISECTION_EPSILON))
        return tStart;
    
    return (tStart+tEnd)*0.5f;    
}

ak_f32 HullHullTOI(convex_hull* HullA, ak_sqtf TransformA, ak_v3f DeltaA, 
                   convex_hull* HullB, ak_sqtf TransformB, ak_v3f DeltaB)
{    
    moving_convex_hull_support A = {HullA, TransformA, DeltaA};
    moving_convex_hull_support B = {HullB, TransformB, DeltaB};
    
    if(GJKIntersected(&A, &B))    
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        for(;;)
        {        
            ak_f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_convex_hull_support StartA = {HullA, AK_SQT(TransformA.Translation+(DeltaA*tStart), TransformA.Orientation, TransformA.Scale), DeltaA*tMid};
            moving_convex_hull_support StartB = {HullB, AK_SQT(TransformB.Translation+(DeltaB*tStart), TransformB.Orientation, TransformB.Scale), DeltaB*tMid};
            if(GJKIntersected(&StartA, &StartB))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_convex_hull_support EndA = {HullA, AK_SQT(TransformA.Translation+(DeltaA*tMid), TransformA.Orientation, TransformA.Scale), DeltaA*tEnd};
            moving_convex_hull_support EndB = {HullB, AK_SQT(TransformB.Translation+(DeltaB*tMid), TransformB.Orientation, TransformB.Scale), DeltaB*tEnd};
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


ak_f32 CapsuleHullTOI(capsule* Capsule, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB)
{    
    moving_line_segment_support A = {Capsule->P0, Capsule->P1, DeltaA};
    moving_convex_hull_support  B = {Hull, Transform, DeltaB};
    
    if(GJKQuadraticIntersected(&A, &B, Capsule->Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        for(;;)
        {
            ak_f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_line_segment_support StartA = {Capsule->P0+DeltaA*tStart, Capsule->P1+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tStart), Transform.Orientation, Transform.Scale), DeltaB*tMid};
            if(GJKQuadraticIntersected(&StartA, &StartB, Capsule->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_line_segment_support EndA = {Capsule->P0+DeltaA*tMid, Capsule->P1+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tMid), Transform.Orientation, Transform.Scale), DeltaB*tEnd};
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

ak_f32 SphereHullTOI(sphere* Sphere, ak_v3f DeltaA, convex_hull* Hull, ak_sqtf Transform, ak_v3f DeltaB)
{    
    moving_point_support A = {Sphere->CenterP, DeltaA};
    moving_convex_hull_support B = {Hull, Transform, DeltaB};
    
    if(GJKQuadraticIntersected(&A, &B, Sphere->Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        
        for(;;)
        {
            ak_f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
            if(tMid == tStart)
                return tStart;
            
            moving_point_support StartA = {Sphere->CenterP+DeltaA*tStart, DeltaA*tMid};
            moving_convex_hull_support StartB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tStart), Transform.Orientation, Transform.Scale), DeltaB*tMid};
            if(GJKQuadraticIntersected(&StartA, &StartB, Sphere->Radius))
            {
                tEnd = tMid;
                continue;
            }
            
            moving_point_support EndA = {Sphere->CenterP+DeltaA*tMid, DeltaA*tEnd};
            moving_convex_hull_support EndB = {Hull, AK_SQT(Transform.Translation+(DeltaB*tMid), Transform.Orientation, Transform.Scale), DeltaB*tEnd};
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

ak_f32 CapsuleCapsuleTOI(capsule* CapsuleA, ak_v3f DeltaA, capsule* CapsuleB, ak_v3f DeltaB)
{
    moving_line_segment_support A = {CapsuleA->P0, CapsuleA->P1, DeltaA};
    moving_line_segment_support B = {CapsuleB->P0, CapsuleB->P1, DeltaB};
    
    ak_f32 Radius = CapsuleA->Radius+CapsuleB->Radius;
    if(GJKQuadraticIntersected(&A, &B, Radius))
    {
        ak_f32 tStart = 0.0f;
        ak_f32 tEnd = 1.0f;
        
        for(;;)
        {
            ak_f32 tMid = GetBisectionMid(DeltaA, DeltaB, tStart, tEnd);
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

ak_f32 SphereSphereTOI(sphere* SphereA, ak_v3f DeltaA, sphere* SphereB, ak_v3f DeltaB)
{
    sphere LargeSphere = CreateSphere(SphereB->CenterP, SphereA->Radius+SphereB->Radius);
    ak_v3f Delta = DeltaA-DeltaB;
    ak_v3f LineSegment[2] = {SphereA->CenterP, SphereA->CenterP+Delta};
    
    ak_f32 Result = LineSegmentSphereIntersection(LineSegment, &LargeSphere);
    return Result;
}

ak_f32 SphereCapsuleTOI(sphere* Sphere, ak_v3f DeltaA, capsule* Capsule, ak_v3f DeltaB)
{
    capsule LargeCapsule = CreateCapsule(Capsule->P0, Capsule->P1, Sphere->Radius+Capsule->Radius);
    ak_v3f Delta = DeltaA-DeltaB;
    ak_v3f LineSegment[2] = {Sphere->CenterP, Sphere->CenterP+Delta};
    
    ak_f32 Result = LineSegmentCapsuleIntersection(LineSegment, &LargeCapsule);
    return Result;
}
