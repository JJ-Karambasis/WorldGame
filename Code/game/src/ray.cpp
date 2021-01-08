ak_v3f Ray_PixelToView(ak_v2i PixelCoords, ak_v2i PixelDim, ak_m4f Perspective)
{
    ak_v3f NDC = AK_ToNormalizedDeviceCoordinates(AK_V2f(PixelCoords), AK_V2f(PixelDim));
    ak_v4f Clip = AK_V4(NDC.xy, -1.0f, 1.0f);
    
    ak_m4f InvPerspective = AK_Inverse(Perspective);
    ak_v4f RayView = Clip*InvPerspective;
    
    return RayView.xyz;
}

ak_v3f Ray_PixelToWorld(ak_v2i PixelCoords, ak_v2i PixelDim, ak_m4f Perspective, ak_m4f View)
{
    ak_m4f InvView = AK_InvTransformM4(View);
    ak_v3f RayView = Ray_PixelToView(PixelCoords, PixelDim, Perspective);
    ak_v3f RayWorld = AK_Normalize(RayView*AK_M3(InvView));
    return RayWorld;
}

ray_cast Ray_SphereCast(ak_v3f Origin, ak_v3f Direction, ak_v3f CenterP, ak_f32 Radius)
{
    ray_cast Result = {};
    
    ak_v3f CO = Origin-CenterP;
    ak_f32 A = AK_Dot(CO, Direction);
    ak_f32 B = AK_SqrMagnitude(CO) - AK_Square(Radius);
    
    if(A > 0 && B > 0) return Result;
    ak_f32 Discr = AK_Square(A) - B;
    if(Discr < 0) return Result;
    
    Result.Intersected = true;
    
    Result.t = -A - AK_Sqrt(Discr);
    if(Result.t < 0) Result.t = 0;
    return Result;
}

ray_cast Ray_CapsuleCast(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, ak_f32 Radius)
{
    ray_cast Result = {};
    
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
        ray_cast tA = Ray_SphereCast(Origin, Direction, P0, Radius);
        ray_cast tB = Ray_SphereCast(Origin, Direction, P1, Radius);
        
        if(!tA.Intersected || !tB.Intersected)
            return Result;
        
        Result.Intersected = true;
        Result.t = AK_Min(tA.t, tB.t);
        return Result;
    }
    
    ak_quadratic_equation_result QEqu = AK_SolveQuadraticEquation(a, b, c);
    if(QEqu.RootCount < 2)
        return Result;
    
    if(QEqu.Roots[0] < 0 && QEqu.Roots[1] < 0)
        return Result;
    
    ak_f32 tMin = AK_Min(QEqu.Roots[0], QEqu.Roots[1]);
    tMin = AK_Max(tMin, 0.0f);
    
    ak_f32 t0 = tMin*m + n;
    if(t0 < 0)
    {        
        return Ray_SphereCast(Origin, Direction, P0, Radius);
    }
    else if(t0 > 1)
    {        
        return Ray_SphereCast(Origin, Direction, P1, Radius);
    }
    else
    {
        Result.Intersected = true;
        Result.t = tMin;
        return Result;
    }        
}

ray_cast Ray_TriangleCast(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(Direction, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))
        return Result;
    
    ak_v3f TVec = Origin - P0;
    
    ak_f32 u = AK_Dot(TVec, PVec);
    if(u < 0.0f || u > Det)
        return Result;
    
    ak_v3f QVec = AK_Cross(TVec, Edge1);
    
    ak_f32 v = AK_Dot(Direction, QVec);
    if(v < 0.0f || u + v > Det)
        return Result;
    
    Result.t = AK_Dot(Edge2, QVec);
    
    ak_f32 InvDet = 1.0f / Det;
    Result.t *= InvDet;
    if(uOut) *uOut = u*InvDet;
    if(vOut) *vOut = v*InvDet;
    
    Result.Intersected = true;
    
    return Result;
}

ray_cast Ray_TriangleCastNoCull(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, 
                                ak_v3f P2, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(Direction, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))    
        return Result;    
    
    ak_f32 InverseDeterminant = 1.0f / Det;
    ak_v3f TVec = Origin - P0;
    
    ak_f32 u = AK_Dot(TVec, PVec) * InverseDeterminant;
    if(u < 0 || u > 1)
    {
        return Result;
    }
    
    ak_v3f QVec = AK_Cross(TVec, Edge1);
    
    ak_f32 v = AK_Dot(Direction, QVec) * InverseDeterminant;
    if(v < 0 || u + v > 1)
    {
        return Result;
    }
    
    Result.Intersected = true;
    Result.t = AK_Dot(Edge2, QVec) * InverseDeterminant;
    
    if(uOut) *uOut = u;
    if(vOut) *vOut = v;
    
    return Result;
}

ray_cast Ray_TriangleMeshCast(ak_v3f RayOrigin, ak_v3f RayDirection, 
                              ak_v3f* Vertices, ak_u16* Indices, ak_u32 IndexCount, 
                              ak_m4f MeshTransform, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_f32 u = 0, v = 0;
    
    ak_u32 TriangleCount = IndexCount/3;
    for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
    {
        ak_u32 Index = TriangleIndex*3;
        
        ak_v3f P0 = AK_TransformPoint(Vertices[Indices[Index]], MeshTransform);
        ak_v3f P1 = AK_TransformPoint(Vertices[Indices[Index+1]], MeshTransform);
        ak_v3f P2 = AK_TransformPoint(Vertices[Indices[Index+2]], MeshTransform);
        
        ak_f32 uHit, vHit;
        ray_cast TriangleCast = Ray_TriangleCast(RayOrigin, RayDirection, P0, P1, P2, &uHit, &vHit);
        
        if(TriangleCast.Intersected)
        {
            if(!Result.Intersected || (TriangleCast.t < Result.t))
            {
                Result = TriangleCast;
                u = uHit;
                v = vHit;
            }
        }
    }
    
    if(uOut) *uOut = u;
    if(vOut) *vOut = v;
    
    return Result;
}

ray_cast Ray_TriangleMeshCastNoCull(ak_v3f RayOrigin, ak_v3f RayDirection, 
                                    ak_v3f* Vertices, ak_u16* Indices, ak_u32 IndexCount, 
                                    ak_m4f MeshTransform, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_f32 u = 0, v = 0;
    
    ak_u32 TriangleCount = IndexCount/3;
    for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
    {
        ak_u32 Index = TriangleIndex*3;
        
        ak_v3f P0 = AK_TransformPoint(Vertices[Indices[Index]], MeshTransform);
        ak_v3f P1 = AK_TransformPoint(Vertices[Indices[Index+1]], MeshTransform);
        ak_v3f P2 = AK_TransformPoint(Vertices[Indices[Index+2]], MeshTransform);
        
        ak_f32 uHit, vHit;
        ray_cast TriangleCast = Ray_TriangleCastNoCull(RayOrigin, RayDirection, P0, P1, P2, &uHit, &vHit);
        
        if(TriangleCast.Intersected)
        {
            if(!Result.Intersected || (TriangleCast.t < Result.t))
            {
                Result = TriangleCast;
                u = uHit;
                v = vHit;
            }
        }
    }
    
    if(uOut) *uOut = u;
    if(vOut) *vOut = v;
    
    return Result;
}


ray_cast Ray_TriangleMeshCast(ak_v3f RayOrigin, ak_v3f RayDirection, 
                              ak_v3f* Vertices, ak_u32* Indices, ak_u32 IndexCount, 
                              ak_m4f MeshTransform, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_f32 u = 0, v = 0;
    
    ak_u32 TriangleCount = IndexCount/3;
    for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
    {
        ak_u32 Index = TriangleIndex*3;
        
        ak_v3f P0 = AK_TransformPoint(Vertices[Indices[Index]], MeshTransform);
        ak_v3f P1 = AK_TransformPoint(Vertices[Indices[Index+1]], MeshTransform);
        ak_v3f P2 = AK_TransformPoint(Vertices[Indices[Index+2]], MeshTransform);
        
        ak_f32 uHit, vHit;
        ray_cast TriangleCast = Ray_TriangleCast(RayOrigin, RayDirection, P0, P1, P2, &uHit, &vHit);
        
        if(TriangleCast.Intersected)
        {
            if(!Result.Intersected || (TriangleCast.t < Result.t))
            {
                Result = TriangleCast;
                u = uHit;
                v = vHit;
            }
        }
    }
    
    if(uOut) *uOut = u;
    if(vOut) *vOut = v;
    
    return Result;
}

ray_cast Ray_TriangleMeshCastNoCull(ak_v3f RayOrigin, ak_v3f RayDirection, 
                                    ak_v3f* Vertices, ak_u32* Indices, ak_u32 IndexCount, 
                                    ak_m4f MeshTransform, ak_f32* uOut, ak_f32* vOut)
{
    ray_cast Result = {};
    
    ak_f32 u = 0, v = 0;
    
    ak_u32 TriangleCount = IndexCount/3;
    for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
    {
        ak_u32 Index = TriangleIndex*3;
        
        ak_v3f P0 = AK_TransformPoint(Vertices[Indices[Index]], MeshTransform);
        ak_v3f P1 = AK_TransformPoint(Vertices[Indices[Index+1]], MeshTransform);
        ak_v3f P2 = AK_TransformPoint(Vertices[Indices[Index+2]], MeshTransform);
        
        ak_f32 uHit, vHit;
        ray_cast TriangleCast = Ray_TriangleCastNoCull(RayOrigin, RayDirection, P0, P1, P2, &uHit, &vHit);
        
        if(TriangleCast.Intersected)
        {
            if(!Result.Intersected || (TriangleCast.t < Result.t))
            {
                Result = TriangleCast;
                u = uHit;
                v = vHit;
            }
        }
    }
    
    if(uOut) *uOut = u;
    if(vOut) *vOut = v;
    
    return Result;
}

ray_cast Ray_PlaneCast(ak_v3f Origin, ak_v3f Direction, ak_v3f PlaneNormal, ak_v3f PlanePoint)
{
    ray_cast Result = {};
    
    ak_f32 Denom = AK_Dot(PlaneNormal, Direction);
    if(AK_EqualZeroEps(Denom)) return Result;    
    
    ak_f32 t = AK_Dot(PlanePoint - Origin, PlaneNormal) / Denom;
    if(t <= 0) return Result;    
    
    Result.Intersected = true;
    Result.t = t;
    
    return Result;
}

ray_cast LineSegment_SphereCast(ak_v3f* LineSegment, ak_v3f CenterP, ak_f32 Radius)
{
    ray_cast Result = {};
    
    ak_v3f D = LineSegment[1]-LineSegment[0];
    ak_f32 SegmentLength = AK_Magnitude(D);
    if(AK_EqualZeroEps(SegmentLength))
        return Result;
    
    D /= SegmentLength;
    
    Result = Ray_SphereCast(LineSegment[0], D, CenterP, Radius);
    if(Result.Intersected)
    {
        if(Result.t > SegmentLength)
        {
            Result.Intersected = false;
            return Result;
        }
        
        Result.t /= SegmentLength;        
    }    
    
    return Result;
}

ray_cast LineSegment_CapsuleCast(ak_v3f* LineSegment, ak_v3f P0, ak_v3f P1, ak_f32 Radius)
{
    ray_cast Result = {};
    
    ak_v3f D = LineSegment[1]-LineSegment[0];
    ak_f32 SegmentLength = AK_Magnitude(D);
    if(AK_EqualZeroEps(SegmentLength))
        return Result;
    
    D /= SegmentLength;
    
    Result = Ray_CapsuleCast(LineSegment[0], D, P0, P1, Radius);
    if(Result.Intersected)
    {
        if(Result.t > SegmentLength)
        {
            Result.Intersected = false;
            return Result;
        }
        
        Result.t /= SegmentLength;
    }
    
    return Result;
}