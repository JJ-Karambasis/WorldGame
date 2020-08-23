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
            vertex_p3_n3_uv_weights TVertex1 = *(vertex_p3_n3_uv_weights*)((u8*)Mesh->Vertices + (Index1*VertexStride));
            vertex_p3_n3_uv_weights TVertex2 = *(vertex_p3_n3_uv_weights*)((u8*)Mesh->Vertices + (Index2*VertexStride));
            vertex_p3_n3_uv_weights TVertex3 = *(vertex_p3_n3_uv_weights*)((u8*)Mesh->Vertices + (Index3*VertexStride));
            
            Vertex1 = TVertex1.P;
            Vertex2 = TVertex2.P;
            Vertex3 = TVertex3.P;            
        }
        else
        {            
            vertex_p3_n3_uv TVertex1 = *(vertex_p3_n3_uv*)((u8*)Mesh->Vertices + (Index1*VertexStride));
            vertex_p3_n3_uv TVertex2 = *(vertex_p3_n3_uv*)((u8*)Mesh->Vertices + (Index2*VertexStride));
            vertex_p3_n3_uv TVertex3 = *(vertex_p3_n3_uv*)((u8*)Mesh->Vertices + (Index3*VertexStride));
            
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