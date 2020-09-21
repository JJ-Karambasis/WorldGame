#define GJK_RELATIVE_ERROR 1e-6f
struct gjk_vertex
{
    ak_v3f W;
    ak_v3f V;
    ak_v3f A;
    ak_v3f B;
};

template <typename typeA, typename typeB>
gjk_vertex GetSupport(typeA* ObjectA, typeB* ObjectB, ak_v3f V)
{
    gjk_vertex Vertex;
    Vertex.V = V;
    Vertex.A = ObjectA->Support(-V);
    Vertex.B = ObjectB->Support( V);
    Vertex.W = Vertex.A-Vertex.B;
    return Vertex;
}

template <typename typeA, typename typeB>
gjk_vertex GetSupport2(typeA* ObjectA, typeB* ObjectB, ak_v3f V)
{
    gjk_vertex Vertex;
    Vertex.V = V;
    Vertex.A = ObjectA->Support( V);
    Vertex.B = ObjectB->Support(-V);
    Vertex.W = Vertex.A-Vertex.B;
    return Vertex;
}

struct gjk_simplex_usage
{
    ak_bool UsedVertex0;
    ak_bool UsedVertex1;
    ak_bool UsedVertex2;
    ak_bool UsedVertex3;
};

struct gjk_simplex_barycentric
{
    ak_v3f ClosestPoint;
    gjk_simplex_usage Usage;
    ak_f32 U, V, W, X;    
    ak_bool IsDegenerate;
    
    void Set(ak_f32 A = 0.0f, ak_f32 B = 0.0f, ak_f32 C = 0.0f, ak_f32 D = 0.0f)
    {
        U = A;
        V = B;
        W = C;
        X = D;        
    }
    
    ak_bool IsValid()
    {
        ak_bool Result = ((U >= 0) && (V >= 0) && (W >= 0) && (X >= 0));
        return Result;
    }
};

ak_bool ClosestPointFromPointToTriangle(ak_v3f P, ak_v3f A, ak_v3f B, ak_v3f C, gjk_simplex_barycentric* Barycentric)
{
    Barycentric->Usage = {};
    
    ak_v3f AB = B-A;
    ak_v3f AC = C-A;
    ak_v3f AP = P-A;
    
    ak_f32 D1 = AK_Dot(AB, AP);
    ak_f32 D2 = AK_Dot(AC, AP);
    
    if((D1 <= 0.0f) && (D2 <= 0.0f))
    {
        Barycentric->ClosestPoint = A;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Set(1, 0, 0);
        return true;
    }
    
    ak_v3f BP = P-B;
    ak_f32 D3 = AK_Dot(AB, BP);
    ak_f32 D4 = AK_Dot(AC, BP);
    if((D3 >= 0.0f) &&  (D4 <= D3))
    {
        Barycentric->ClosestPoint = B;
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Set(0, 1, 0);
        return true;
    }
    
    ak_f32 VC = D1*D4 - D3*D2;
    if((VC <= 0.0f) && (D1 >= 0.0f) && (D3 <= 0.0f))
    {
        ak_f32 V = D1 / (D1-D3);
        Barycentric->ClosestPoint = A + V*AB;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Set(1-V, V, 0);
        return true;
    }
    
    ak_v3f CP = P-C;
    ak_f32 D5 = AK_Dot(AB, CP);
    ak_f32 D6 = AK_Dot(AC, CP);
    if((D6 >= 0.0f) && (D5 <= D6))
    {
        Barycentric->ClosestPoint = C;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(0, 0, 1);
        return true;
    }
    
    ak_f32 VB = D5*D2 - D1*D6;
    if((VB <= 0.0f) && (D2 >= 0.0f) && (D6 <= 0.0f))
    {
        ak_f32 W = D2/(D2-D6);
        Barycentric->ClosestPoint = A + W*AC;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(1-W, 0, W);
        return true;
    }
    
    ak_f32 VA = D3*D6 - D5*D4;
    if((VA <= 0.0f) && ((D4-D3) >= 0.0f) && ((D5-D6) >= 0.0f))
    {
        ak_f32 W = (D4-D3) / ((D4-D3) + (D5-D6));
        Barycentric->ClosestPoint = B + W*(C-B);
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(0, 1-W, W);
        return true;
    }
    
    ak_f32 Denominator = 1.0f / (VA+VB+VC);
    ak_f32 V = VB*Denominator;
    ak_f32 W = VC*Denominator;
    
    Barycentric->ClosestPoint = A + AB*V + AC*W;
    Barycentric->Usage.UsedVertex0 = true;
    Barycentric->Usage.UsedVertex1 = true;
    Barycentric->Usage.UsedVertex2 = true;
    Barycentric->Set(1-V-W, V, W);
    return true;
}

ak_i32  IsPointOutsidePlane(ak_v3f P, ak_v3f A, ak_v3f B, ak_v3f C, ak_v3f D)
{
    ak_v3f Normal = AK_Cross(B-A, C-A);
    
    ak_f32 SignP = AK_Dot(P-A, Normal);
    ak_f32 SignD = AK_Dot(D-A, Normal);
    
    if(AK_Square(SignD) < (AK_Square(1e-4f)))
        return -1;
    
    ak_i32 Result = (SignP*SignD) < 0.0f;
    return Result;
}

ak_bool ClosestPointFromPointToTetrahedron(ak_v3f P, ak_v3f A, ak_v3f B, ak_v3f C, ak_v3f D, gjk_simplex_barycentric* Barycentric)
{
    gjk_simplex_barycentric TempBarycentric;
    
    Barycentric->ClosestPoint = P;
    Barycentric->Usage = {true, true, true, true};
    
    ak_i32 IsPointOutsideABC = IsPointOutsidePlane(P, A, B, C, D);
    ak_i32 IsPointOutsideACD = IsPointOutsidePlane(P, A, C, D, B);
    ak_i32 IsPointOutsideADB = IsPointOutsidePlane(P, A, D, B, C);
    ak_i32 IsPointOutsideBDC = IsPointOutsidePlane(P, B, D, C, A);
    
    if((IsPointOutsideABC < 0) || (IsPointOutsideACD < 0) || (IsPointOutsideADB < 0) || (IsPointOutsideBDC < 0))
    {
        Barycentric->IsDegenerate = true;
        return false;
    }
    
    if(!IsPointOutsideABC && !IsPointOutsideACD && !IsPointOutsideADB && !IsPointOutsideBDC)
        return false;
    
    ak_f32 BestSqrDistance = AK_MAX32;
    if(IsPointOutsideABC)
    {
        ClosestPointFromPointToTriangle(P, A, B, C, &TempBarycentric);
        ak_v3f Q = TempBarycentric.ClosestPoint;
        ak_f32 SqrDistance = AK_SqrMagnitude(Q-P);
        
        if(SqrDistance < BestSqrDistance)
        {
            BestSqrDistance = SqrDistance;
            Barycentric->ClosestPoint = Q;
            Barycentric->Usage = {};
            
            Barycentric->Usage.UsedVertex0 = TempBarycentric.Usage.UsedVertex0;
            Barycentric->Usage.UsedVertex1 = TempBarycentric.Usage.UsedVertex1;
            Barycentric->Usage.UsedVertex2 = TempBarycentric.Usage.UsedVertex2;
            
            Barycentric->Set(TempBarycentric.U, TempBarycentric.V, TempBarycentric.W, 0);
        }
    }
    
    if(IsPointOutsideACD)
    {
        ClosestPointFromPointToTriangle(P, A, C, D, &TempBarycentric);
        ak_v3f Q = TempBarycentric.ClosestPoint;
        ak_f32 SqrDistance = AK_SqrMagnitude(Q-P);
        
        if(SqrDistance < BestSqrDistance)
        {
            BestSqrDistance = SqrDistance;
            Barycentric->ClosestPoint = Q;
            Barycentric->Usage = {};
            
            Barycentric->Usage.UsedVertex0 = TempBarycentric.Usage.UsedVertex0;
            Barycentric->Usage.UsedVertex2 = TempBarycentric.Usage.UsedVertex1;
            Barycentric->Usage.UsedVertex3 = TempBarycentric.Usage.UsedVertex2;
            
            Barycentric->Set(TempBarycentric.U, 0, TempBarycentric.V, TempBarycentric.W);
        }
    }
    
    if(IsPointOutsideADB)
    {
        ClosestPointFromPointToTriangle(P, A, D, B, &TempBarycentric);
        ak_v3f Q = TempBarycentric.ClosestPoint;
        ak_f32 SqrDistance = AK_SqrMagnitude(Q-P);
        
        if(SqrDistance < BestSqrDistance)
        {
            BestSqrDistance = SqrDistance;
            Barycentric->ClosestPoint = Q;
            Barycentric->Usage = {};
            
            Barycentric->Usage.UsedVertex0 = TempBarycentric.Usage.UsedVertex0;
            Barycentric->Usage.UsedVertex1 = TempBarycentric.Usage.UsedVertex2;
            Barycentric->Usage.UsedVertex3 = TempBarycentric.Usage.UsedVertex1;
            
            Barycentric->Set(TempBarycentric.U, TempBarycentric.W, 0, TempBarycentric.V);            
        }
    }
    
    if(IsPointOutsideBDC)
    {
        ClosestPointFromPointToTriangle(P, B, D, C, &TempBarycentric);
        ak_v3f Q = TempBarycentric.ClosestPoint;
        ak_f32 SqrDistance = AK_SqrMagnitude(Q-P);
        
        if(SqrDistance < BestSqrDistance)
        {
            BestSqrDistance = SqrDistance;
            Barycentric->ClosestPoint = Q;
            Barycentric->Usage = {};
            
            Barycentric->Usage.UsedVertex1 = TempBarycentric.Usage.UsedVertex0;
            Barycentric->Usage.UsedVertex2 = TempBarycentric.Usage.UsedVertex2;
            Barycentric->Usage.UsedVertex3 = TempBarycentric.Usage.UsedVertex1;
            
            Barycentric->Set(0, TempBarycentric.U, TempBarycentric.W, TempBarycentric.V);            
        }            
    }
    
    return true;
}

struct gjk_distance_simplex
{    
    ak_u32 VertexCount;
    ak_v3f Vertex[4];
    ak_v3f AVertex[4];
    ak_v3f BVertex[4];
    ak_v3f LastVertex;
    gjk_simplex_barycentric Barycentric;
    
    ak_bool NeedsUpdate;
    ak_v3f V;
    ak_v3f CP[2];
    
    void Reset()
    {
        LastVertex = AK_InvalidV3f();
        NeedsUpdate = true;
        VertexCount = 0;
        Barycentric = {};
    }
    
    ak_bool HasVertex(ak_v3f W)
    {
        for(ak_u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
        {
            if(Vertex[VertexIndex] == W)
                return true;
        }
        
        if(W == LastVertex)
            return true;
        
        return false;
    }
    
    void RemoveVertex(ak_u32 Index)
    {
        AK_Assert(VertexCount > 0, "Cannot remove vertex from simplex that has no vertices");
        VertexCount--;
        
        Vertex[Index]  = Vertex[VertexCount];
        AVertex[Index] = AVertex[VertexCount];
        BVertex[Index] = BVertex[VertexCount];
    }
    
    void ReduceVertices(gjk_simplex_usage Usage)
    {        
        if((VertexCount >= 4) && (!Usage.UsedVertex3))
            RemoveVertex(3);
        
        if((VertexCount >= 3) && (!Usage.UsedVertex2))
            RemoveVertex(2);
        
        if((VertexCount >= 2) && (!Usage.UsedVertex1))
            RemoveVertex(1);
        
        if((VertexCount >= 1) && (!Usage.UsedVertex0))
            RemoveVertex(0);
    }
    
    void AddVertex(gjk_vertex* SupportVertex)
    {
        NeedsUpdate = true;
        LastVertex = SupportVertex->W;
        
        Vertex[VertexCount]  = SupportVertex->W;
        AVertex[VertexCount] = SupportVertex->A;
        BVertex[VertexCount] = SupportVertex->B;
        
        VertexCount++;
        
        AK_Assert(VertexCount < 5, "Index out of bounds");
    }
    
    ak_bool UpdateVectorAndPoints()
    {
        ak_bool Result = true;
        
        if(NeedsUpdate)
        {                        
            NeedsUpdate = false;
            
            Barycentric = {};            
            
            switch(VertexCount)
            {
                case 1:
                {
                    CP[0] = AVertex[0];
                    CP[1] = BVertex[0];
                    V = Vertex[0];
                    Barycentric.Set(1.0f);                                        
                } break;                
                
                case 2:
                {                                           
                    ak_v3f P0 = -Vertex[0];
                    ak_v3f P1 = Vertex[1]-Vertex[0];
                    ak_f32 t = AK_Dot(P1, P0);
                    
                    if(t > 0)
                    {
                        ak_f32 P1Sqr = AK_SqrMagnitude(P1);
                        if(t < P1Sqr)
                        {
                            t /= P1Sqr;
                            Barycentric.Usage.UsedVertex0 = true;
                            Barycentric.Usage.UsedVertex1 = true;
                        }
                        else
                        {
                            t = 1;               
                            Barycentric.Usage.UsedVertex1 = true;
                        }
                    }
                    else
                    {
                        t = 0;
                        Barycentric.Usage.UsedVertex0 = true;
                    }
                    
                    Barycentric.Set(1-t, t);
                    
                    CP[0] = AVertex[0] + t*(AVertex[1] - AVertex[0]);
                    CP[1] = BVertex[0] + t*(BVertex[1] - BVertex[0]);
                    V = CP[0] - CP[1];
                    
                    ReduceVertices(Barycentric.Usage);                    
                } break;
                
                case 3:
                {
                    ak_v3f A = Vertex[0];
                    ak_v3f B = Vertex[1];
                    ak_v3f C = Vertex[2];
                    
                    ClosestPointFromPointToTriangle(AK_V3<ak_f32>(), A, B, C, &Barycentric);
                    
                    CP[0] = ((AVertex[0] * Barycentric.U) + (AVertex[1] * Barycentric.V) + (AVertex[2] * Barycentric.W));                    
                    CP[1] = ((BVertex[0] * Barycentric.U) + (BVertex[1] * Barycentric.V) + (BVertex[2] * Barycentric.W));
                    
                    V = CP[0] - CP[1];
                    
                    ReduceVertices(Barycentric.Usage);
                } break;
                
                case 4:
                {
                    ak_v3f A = Vertex[0];
                    ak_v3f B = Vertex[1];
                    ak_v3f C = Vertex[2];
                    ak_v3f D = Vertex[3];
                    
                    ak_bool NotIntersected = ClosestPointFromPointToTetrahedron(AK_V3<ak_f32>(), A, B, C, D, &Barycentric);
                    
                    if(NotIntersected)
                    {
                        CP[0] = ((AVertex[0]*Barycentric.U) + (AVertex[1]*Barycentric.V) + (AVertex[2]*Barycentric.W) + (AVertex[3]*Barycentric.X));
                        CP[1] = ((BVertex[0]*Barycentric.U) + (BVertex[1]*Barycentric.V) + (BVertex[2]*Barycentric.W) + (BVertex[3]*Barycentric.X));
                        
                        V = CP[0] - CP[1];
                        ReduceVertices(Barycentric.Usage);
                    }
                    else
                    {
                        if(Barycentric.IsDegenerate)
                            return false;
                        else
                            V = AK_V3<ak_f32>();
                    }
                    
                } break;
                
                AK_INVALID_DEFAULT_CASE;
            }
        }
        
        return Barycentric.IsValid();        
    }
    
    void GetClosestPoints(ak_v3f* A, ak_v3f* B)
    {
        UpdateVectorAndPoints();
        *A = CP[0];
        *B = CP[1];
    }    
    
    ak_bool GetClosestPointToOrigin(ak_v3f* P)
    {
        ak_bool Result = UpdateVectorAndPoints();
        *P = V;      
        return Result;
    }
};

enum gjk_distance_status
{
    GJK_DISTANCE_STATUS_NONE,
    GJK_DISTANCE_STATUS_ALREADY_IN_SIMPLEX, 
    GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER,
    GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER_2,
    GJK_DISTANCE_STATUS_DIRECTION_CLOSE_TO_ORIGIN,
    GJK_DISTANCE_STATUS_INVALID_SIMPLEX    
};

struct gjk_distance
{    
    gjk_distance_simplex Simplex;
    gjk_distance_status Status;
    ak_v3f V;
    ak_f32 SquareDistance;
    
    void GetClosestPoints(ak_v3f* A, ak_v3f* B)
    {
        Simplex.GetClosestPoints(A, B);
    }
};

template <typename typeA, typename typeB>
gjk_distance GJKDistance(typeA* ObjectA, typeB* ObjectB)
{    
    ak_v3f V = AK_XAxis();
    
    gjk_distance_simplex Simplex = {};    
    Simplex.Reset();
    gjk_distance_status Status = GJK_DISTANCE_STATUS_NONE;
    
    ak_f32 SqrDistance = AK_MAX32;
    
    ak_u64 Iterations = 0;
    for(;;)
    {
        Iterations++;
        
        gjk_vertex Support = GetSupport(ObjectA, ObjectB, V);
        
        if(Simplex.HasVertex(Support.W))
        {
            Status = GJK_DISTANCE_STATUS_ALREADY_IN_SIMPLEX;
            break;
        }
        
        ak_f32 Delta = AK_Dot(V, Support.W);
        
        if((SqrDistance - Delta) <= (SqrDistance * GJK_RELATIVE_ERROR))
        {
            Status = GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER;
            break;
        }
        
        Simplex.AddVertex(&Support);
        
        ak_v3f NewV;
        if(!Simplex.GetClosestPointToOrigin(&NewV))
        {
            Status = GJK_DISTANCE_STATUS_INVALID_SIMPLEX;
            break;
        }
        
        ak_f32 PrevSqrDistance = SqrDistance;
        SqrDistance = AK_SqrMagnitude(NewV);
        if(SqrDistance < GJK_RELATIVE_ERROR)
        {
            V = NewV;
            Status = GJK_DISTANCE_STATUS_DIRECTION_CLOSE_TO_ORIGIN;
            break;
        }
        
        if((PrevSqrDistance - SqrDistance) <= (AK_EPSILON32*PrevSqrDistance))
        {
            Status = GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER_2;
            break;
        }
        
        V = NewV;
        
        AK_Assert(Simplex.VertexCount < 4, "GJK Simplex vertex overflow");        
    }
    
    AK_Assert(Status != GJK_DISTANCE_STATUS_NONE, "GJK failed to get proper results");
    
    gjk_distance Result;
    Result.Status         = Status;
    Result.Simplex        = Simplex;
    Result.V              = V;
    Result.SquareDistance = AK_SqrMagnitude(V);
    
    return Result;    
}

struct gjk_simplex
{
    gjk_vertex Vertex[4];     
    ak_i32 LastIndex;
    
    inline void Add(gjk_vertex* Support)
    {
        LastIndex++;
        Vertex[LastIndex] = *Support;        
    }
    
    inline ak_u32 GetVertexCount()
    {
        return LastIndex+1;
    }
};

gjk_simplex InitSimplex()
{
    gjk_simplex Result = {};
    Result.LastIndex = -1;
    return Result;
}

ak_i32 PerformLineTest(gjk_simplex* Simplex, ak_v3f* V)
{
    ak_v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    ak_v3f B = Simplex->Vertex[0].W;    
    ak_v3f AB = B-A;
    ak_v3f AO =  -A;
    
    ak_f32 DotResult = AK_Dot(AB, AO);
    ak_v3f Temp = AK_Cross(AB, AO);
    if(AK_EqualZeroEps(AK_SqrMagnitude(Temp)) && (DotResult > 0))
        return 1;
    
    if(AK_EqualZeroEps(DotResult) || (DotResult < 0))
    {
        Simplex->Vertex[0] = Simplex->Vertex[Simplex->LastIndex];
        Simplex->LastIndex = 0;
        *V = AO;
    }
    else
    {
        *V = AK_Cross(AK_Cross(AB, AO), AB);
    }
    
    return 0;
}

ak_i32 SignCCD(ak_f32 Value)
{
    if(AK_EqualZeroEps(Value))
        return 0;
    else if(Value < 0)
        return -1;
    else
        return 1;
}

ak_f32 PointSegmentDistance(ak_v3f P, ak_v3f A, ak_v3f B)
{
    ak_v3f AB = B-A;
    ak_v3f PA = A-P;
    
    ak_f32 t = -1 * AK_Dot(PA, AB);
    t /= AK_SqrMagnitude(AB);
    
    ak_f32 Result;
    if(t < 0 || AK_EqualZeroEps(t))
    {
        Result = AK_SqrMagnitude(A-P);
    }
    else if(t > 1 || AK_EqualEps(t, 1))
    {
        Result = AK_SqrMagnitude(B-P);
    }
    else
    {
        AB *= t;
        AB += PA;
        Result = AK_SqrMagnitude(AB);
    }
    
    return Result;
}

ak_f32 PointTriangleDistance(ak_v3f P, ak_v3f A, ak_v3f B, ak_v3f C)
{
    ak_v3f AB = B-A;
    ak_v3f AC = C-A;
    ak_v3f PA = A-P;
    
    //IMPORTANT(EVERYONE): There are a lot of rounding and precision errors with these values so we made them
    //f64 bit floats  otherwise they can report a zero distance which is completely wrong. Maybe there is a better way
    ak_f64 u = AK_SqrMagnitude(PA);
    ak_f64 v = AK_SqrMagnitude(AB);
    ak_f64 w = AK_SqrMagnitude(AC);
    ak_f64 p = AK_Dot(PA, AB);
    ak_f64 q = AK_Dot(PA, AC);
    ak_f64 r = AK_Dot(AB, AC);
    
    ak_f64 s = (q*r - w*p) / (w*v - r*r);
    ak_f64 t = (-s*r - q) / w;
    
    ak_f64 Result;
    if((AK_EqualZeroEps(s) || (s > 0)) && 
       (AK_EqualEps(s, 1) || s < 1) &&
       (AK_EqualZeroEps(t) || (t > 0)) &&
       (AK_EqualEps(t, 1) || t < 1) &&
       (AK_EqualEps(t+s, 1) || (t+s < 1)))
    {
        Result = s*s*v;
        Result += t*t*w;
        Result += 2*s*t*r;
        Result += 2*s*p;
        Result += 2*t*q;
        Result += u;
    }
    else
    {
        Result = PointSegmentDistance(P, A, B);
        ak_f32 Dist1 = PointSegmentDistance(P, A, C);
        
        if(Result > Dist1)        
            Result = Dist1;        
        
        ak_f32 Dist2 = PointSegmentDistance(P, B, C);
        if(Result > Dist2)
            Result = Dist2;                
    }
    
    return (ak_f32)Result;
}

ak_i32 PerformTriangleTest(gjk_simplex* Simplex, ak_v3f* V)
{
    ak_v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    ak_v3f B = Simplex->Vertex[1].W;
    ak_v3f C = Simplex->Vertex[0].W;
    
    ak_f32 Distance = PointTriangleDistance(AK_V3<ak_f32>(), A, B, C);
    if(AK_EqualZeroEps(Distance))
        return 1;
    
    if(AK_EqualEps(A, B) || AK_EqualEps(A, C))
        return -1;
    
    ak_v3f AO = -A;
    ak_v3f AB = B-A;
    ak_v3f AC = C-A;
    ak_v3f ABC = AK_Cross(AB, AC);
    ak_v3f Temp = AK_Cross(ABC, AC);
    ak_f32 DotResult = AK_Dot(Temp, AO);
    
    if(AK_EqualZeroEps(DotResult) || (DotResult > 0))
    {
        DotResult = AK_Dot(AC, AO);
        if(AK_EqualZeroEps(DotResult) || (DotResult > 0))
        {
            Simplex->Vertex[1] = Simplex->Vertex[Simplex->LastIndex];
            Simplex->LastIndex = 1;
            *V = AK_Cross(AK_Cross(AC, AO), AC);
        }
        else
        {
            DotResult = AK_Dot(AB, AO);
            if(AK_EqualZeroEps(DotResult) || (DotResult > 0))
            {
                Simplex->Vertex[0] = Simplex->Vertex[1];
                Simplex->Vertex[1] = Simplex->Vertex[Simplex->LastIndex];
                Simplex->LastIndex = 1;
                *V = AK_Cross(AK_Cross(AB, AO), AB);
            }
            else
            {
                Simplex->Vertex[0] = Simplex->Vertex[Simplex->LastIndex];
                Simplex->LastIndex = 0;
                *V = AO;
            }
        }
    }
    else
    {
        DotResult = AK_Dot(ABC, AO);
        if(AK_EqualZeroEps(DotResult) || (DotResult > 0))
            *V = ABC;
        else
        {
            gjk_vertex TempVertex = Simplex->Vertex[0];
            Simplex->Vertex[0] = Simplex->Vertex[1];
            Simplex->Vertex[1] = TempVertex;            
            *V = -ABC;
        }
    }
    
    return 0;    
}

ak_i32 PerformTetrahedronTest(gjk_simplex* Simplex, ak_v3f* V)
{
    ak_v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    ak_v3f B = Simplex->Vertex[2].W;
    ak_v3f C = Simplex->Vertex[1].W;
    ak_v3f D = Simplex->Vertex[0].W;
    
    ak_f32 Distance = PointTriangleDistance(A, B, C, D);
    if(AK_EqualZeroEps(Distance))
        return -1;
    
    Distance = PointTriangleDistance(AK_V3<ak_f32>(), A, B, C);
    if(AK_EqualZeroEps(Distance))
        return 1;
    
    Distance = PointTriangleDistance(AK_V3<ak_f32>(), A, C, D);
    if(AK_EqualZeroEps(Distance))
        return 1;
    
    Distance = PointTriangleDistance(AK_V3<ak_f32>(), A, B, D);
    if(AK_EqualZeroEps(Distance))
        return 1;
    
    Distance = PointTriangleDistance(AK_V3<ak_f32>(), B, C, D);
    if(AK_EqualZeroEps(Distance))
        return 1;
    
    ak_v3f AO  =  -A;
    ak_v3f AB  = B-A;
    ak_v3f AC  = C-A;
    ak_v3f AD  = D-A;
    ak_v3f ABC = AK_Cross(AB, AC);
    ak_v3f ACD = AK_Cross(AC, AD);
    ak_v3f ADB = AK_Cross(AD, AB);
    
    ak_i32 IsBOnACD = SignCCD(AK_Dot(ACD, AB));
    ak_i32 IsCOnADB = SignCCD(AK_Dot(ADB, AC));
    ak_i32 IsDOnABC = SignCCD(AK_Dot(ABC, AD));
    
    ak_bool AB_O = SignCCD(AK_Dot(ACD, AO)) == IsBOnACD;
    ak_bool AC_O = SignCCD(AK_Dot(ADB, AO)) == IsCOnADB;
    ak_bool AD_O = SignCCD(AK_Dot(ABC, AO)) == IsDOnABC;
    
    if(AB_O && AC_O && AD_O)    
        return 1;
    else if(!AB_O)
    {
        Simplex->Vertex[2] = Simplex->Vertex[Simplex->LastIndex];
        Simplex->LastIndex = 2;
    }
    else if(!AC_O)
    {
        Simplex->Vertex[1] = Simplex->Vertex[0];
        Simplex->Vertex[0] = Simplex->Vertex[2];
        Simplex->Vertex[2] = Simplex->Vertex[Simplex->LastIndex];
        Simplex->LastIndex = 2;
    }
    else
    {        
        Simplex->Vertex[0] = Simplex->Vertex[1];
        Simplex->Vertex[1] = Simplex->Vertex[2];
        Simplex->Vertex[2] = Simplex->Vertex[Simplex->LastIndex];
        Simplex->LastIndex = 2;
    }
    
    return PerformTriangleTest(Simplex, V);
}

ak_i32 PerformSimplexTest(gjk_simplex* Simplex, ak_v3f* V)
{
    switch(Simplex->GetVertexCount())
    {
        case 2:
        {
            return PerformLineTest(Simplex, V);
        } break;
        
        case 3:
        {
            return PerformTriangleTest(Simplex, V);
        } break;
        
        case 4:
        {
            return PerformTetrahedronTest(Simplex, V);
        } break;
        
        AK_INVALID_DEFAULT_CASE;        
    }
    
    return -1;
}

template <typename typeA, typename typeB>
ak_bool GJKIntersected(typeA* ObjectA, typeB* ObjectB, gjk_simplex* Simplex)
{    
    ak_v3f V = AK_XAxis();
    *Simplex = InitSimplex();
    
    gjk_vertex Support = GetSupport2(ObjectA, ObjectB, V);    
    Simplex->Add(&Support);
    
    V = -Support.W;
    
    ak_u64 Iterations = 0;
    for(;;)
    {        
        Iterations++;                        
        Support = GetSupport2(ObjectA, ObjectB, V);
        
        ak_f32 Delta = AK_Dot(V, Support.W);
        
        if(Delta < 0)        
            return false;        
        
        Simplex->Add(&Support);
        
        ak_i32 SimplexResult = PerformSimplexTest(Simplex, &V);
        if(SimplexResult == 1)
            return true;
        else if(SimplexResult == -1)
            return false;
        
        if(AK_EqualZeroEps(AK_SqrMagnitude(V)))
            return false;
    }    
}

template <typename typeA, typename typeB>
ak_bool GJKIntersected(typeA* ObjectA, typeB* ObjectB)
{
    gjk_simplex Simplex;
    return GJKIntersected(ObjectA, ObjectB, &Simplex);
}

template <typename typeA, typename typeB>
ak_bool GJKQuadraticIntersected(typeA* ObjectA, typeB* ObjectB, ak_f32 Radius)
{
    gjk_distance Distance = GJKDistance(ObjectA, ObjectB);
    ak_bool Result = Distance.SquareDistance <= AK_Square(Radius);
    return Result;
}