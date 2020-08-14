#define GJK_RELATIVE_ERROR 1e-6f
struct gjk_vertex
{
    v3f W;
    v3f V;
    v3f A;
    v3f B;
};

template <typename typeA, typename typeB>
gjk_vertex GetSupport(typeA* ObjectA, typeB* ObjectB, v3f V)
{
    gjk_vertex Vertex;
    Vertex.V = V;
    Vertex.A = ObjectA->Support(-V);
    Vertex.B = ObjectB->Support( V);
    Vertex.W = Vertex.A-Vertex.B;
    return Vertex;
}

template <typename typeA, typename typeB>
gjk_vertex GetSupport2(typeA* ObjectA, typeB* ObjectB, v3f V)
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
    b32 UsedVertex0;
    b32 UsedVertex1;
    b32 UsedVertex2;
    b32 UsedVertex3;
};

struct gjk_simplex_barycentric
{
    v3f ClosestPoint;
    gjk_simplex_usage Usage;
    f32 U, V, W, X;    
    b32 IsDegenerate;
    
    void Set(f32 A = 0.0f, f32 B = 0.0f, f32 C = 0.0f, f32 D = 0.0f)
    {
        U = A;
        V = B;
        W = C;
        X = D;        
    }
    
    b32 IsValid()
    {
        b32 Result = ((U >= 0) && (V >= 0) && (W >= 0) && (X >= 0));
        return Result;
    }
};

b32 ClosestPointFromPointToTriangle(v3f P, v3f A, v3f B, v3f C, gjk_simplex_barycentric* Barycentric)
{
    Barycentric->Usage = {};
    
    v3f AB = B-A;
    v3f AC = C-A;
    v3f AP = P-A;
    
    f32 D1 = Dot(AB, AP);
    f32 D2 = Dot(AC, AP);
    
    if((D1 <= 0.0f) && (D2 <= 0.0f))
    {
        Barycentric->ClosestPoint = A;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Set(1, 0, 0);
        return true;
    }
    
    v3f BP = P-B;
    f32 D3 = Dot(AB, BP);
    f32 D4 = Dot(AC, BP);
    if((D3 >= 0.0f) &&  (D4 <= D3))
    {
        Barycentric->ClosestPoint = B;
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Set(0, 1, 0);
        return true;
    }
    
    f32 VC = D1*D4 - D3*D2;
    if((VC <= 0.0f) && (D1 >= 0.0f) && (D3 <= 0.0f))
    {
        f32 V = D1 / (D1-D3);
        Barycentric->ClosestPoint = A + V*AB;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Set(1-V, V, 0);
        return true;
    }
    
    v3f CP = P-C;
    f32 D5 = Dot(AB, CP);
    f32 D6 = Dot(AC, CP);
    if((D6 >= 0.0f) && (D5 <= D6))
    {
        Barycentric->ClosestPoint = C;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(0, 0, 1);
        return true;
    }
    
    f32 VB = D5*D2 - D1*D6;
    if((VB <= 0.0f) && (D2 >= 0.0f) && (D6 <= 0.0f))
    {
        f32 W = D2/(D2-D6);
        Barycentric->ClosestPoint = A + W*AC;
        Barycentric->Usage.UsedVertex0 = true;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(1-W, 0, W);
        return true;
    }
    
    f32 VA = D3*D6 - D5*D4;
    if((VA <= 0.0f) && ((D4-D3) >= 0.0f) && ((D5-D6) >= 0.0f))
    {
        f32 W = (D4-D3) / ((D4-D3) + (D5-D6));
        Barycentric->ClosestPoint = B + W*(C-B);
        Barycentric->Usage.UsedVertex1 = true;
        Barycentric->Usage.UsedVertex2 = true;
        Barycentric->Set(0, 1-W, W);
        return true;
    }
    
    f32 Denominator = 1.0f / (VA+VB+VC);
    f32 V = VB*Denominator;
    f32 W = VC*Denominator;
    
    Barycentric->ClosestPoint = A + AB*V + AC*W;
    Barycentric->Usage.UsedVertex0 = true;
    Barycentric->Usage.UsedVertex1 = true;
    Barycentric->Usage.UsedVertex2 = true;
    Barycentric->Set(1-V-W, V, W);
    return true;
}

int IsPointOutsidePlane(v3f P, v3f A, v3f B, v3f C, v3f D)
{
    v3f Normal = Cross(B-A, C-A);
    
    f32 SignP = Dot(P-A, Normal);
    f32 SignD = Dot(D-A, Normal);
    
    if(Square(SignD) < (Square(1e-4f)))
        return -1;
    
    int Result = (SignP*SignD) < 0.0f;
    return Result;
}

b32 ClosestPointFromPointToTetrahedron(v3f P, v3f A, v3f B, v3f C, v3f D, gjk_simplex_barycentric* Barycentric)
{
    gjk_simplex_barycentric TempBarycentric;
    
    Barycentric->ClosestPoint = P;
    Barycentric->Usage = {true, true, true, true};
    
    int IsPointOutsideABC = IsPointOutsidePlane(P, A, B, C, D);
    int IsPointOutsideACD = IsPointOutsidePlane(P, A, C, D, B);
    int IsPointOutsideADB = IsPointOutsidePlane(P, A, D, B, C);
    int IsPointOutsideBDC = IsPointOutsidePlane(P, B, D, C, A);
    
    if((IsPointOutsideABC < 0) || (IsPointOutsideACD < 0) || (IsPointOutsideADB < 0) || (IsPointOutsideBDC < 0))
    {
        Barycentric->IsDegenerate = true;
        return false;
    }
    
    if(!IsPointOutsideABC && !IsPointOutsideACD && !IsPointOutsideADB && !IsPointOutsideBDC)
        return false;
    
    f32 BestSqrDistance = FLT_MAX;
    if(IsPointOutsideABC)
    {
        ClosestPointFromPointToTriangle(P, A, B, C, &TempBarycentric);
        v3f Q = TempBarycentric.ClosestPoint;
        f32 SqrDistance = SquareMagnitude(Q-P);
        
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
        v3f Q = TempBarycentric.ClosestPoint;
        f32 SqrDistance = SquareMagnitude(Q-P);
        
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
        v3f Q = TempBarycentric.ClosestPoint;
        f32 SqrDistance = SquareMagnitude(Q-P);
        
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
        v3f Q = TempBarycentric.ClosestPoint;
        f32 SqrDistance = SquareMagnitude(Q-P);
        
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
    u32 VertexCount;
    v3f Vertex[4];
    v3f AVertex[4];
    v3f BVertex[4];
    v3f LastVertex;
    gjk_simplex_barycentric Barycentric;
    
    b32 NeedsUpdate;
    v3f V;
    v3f CP[2];
    
    void Reset()
    {
        LastVertex = InvalidV3();
        NeedsUpdate = true;
        VertexCount = 0;
        Barycentric = {};
    }
    
    b32 HasVertex(v3f W)
    {
        for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
        {
            if(Vertex[VertexIndex] == W)
                return true;
        }
        
        if(W == LastVertex)
            return true;
        
        return false;
    }
    
    void RemoveVertex(u32 Index)
    {
        ASSERT(VertexCount > 0);
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
        
        ASSERT(VertexCount < 5);
    }
    
    b32 UpdateVectorAndPoints()
    {
        b32 Result = true;
        
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
                    v3f P0 = -Vertex[0];
                    v3f P1 = Vertex[1]-Vertex[0];
                    f32 t = Dot(P1, P0);
                    
                    if(t > 0)
                    {
                        f32 P1Sqr = SquareMagnitude(P1);
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
                    v3f A = Vertex[0];
                    v3f B = Vertex[1];
                    v3f C = Vertex[2];
                    
                    ClosestPointFromPointToTriangle(V3(), A, B, C, &Barycentric);
                    
                    CP[0] = ((AVertex[0] * Barycentric.U) + (AVertex[1] * Barycentric.V) + (AVertex[2] * Barycentric.W));                    
                    CP[1] = ((BVertex[0] * Barycentric.U) + (BVertex[1] * Barycentric.V) + (BVertex[2] * Barycentric.W));
                    
                    V = CP[0] - CP[1];
                    
                    ReduceVertices(Barycentric.Usage);
                } break;
                
                case 4:
                {
                    v3f A = Vertex[0];
                    v3f B = Vertex[1];
                    v3f C = Vertex[2];
                    v3f D = Vertex[3];
                    
                    b32 NotIntersected = ClosestPointFromPointToTetrahedron(V3(), A, B, C, D, &Barycentric);
                    
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
                            V = V3();
                    }
                    
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }
        
        return Barycentric.IsValid();        
    }
    
    void GetClosestPoints(v3f* A, v3f* B)
    {
        UpdateVectorAndPoints();
        *A = CP[0];
        *B = CP[1];
    }    
    
    b32 GetClosestPointToOrigin(v3f* P)
    {
        b32 Result = UpdateVectorAndPoints();
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
    v3f V;
    f32 SquareDistance;
    
    void GetClosestPoints(v3f* A, v3f* B)
    {
        Simplex.GetClosestPoints(A, B);
    }
};

template <typename typeA, typename typeB>
gjk_distance GJKDistance(typeA* ObjectA, typeB* ObjectB)
{    
    v3f V = Global_WorldXAxis;
    
    gjk_distance_simplex Simplex = {};    
    Simplex.Reset();
    gjk_distance_status Status = GJK_DISTANCE_STATUS_NONE;
    
    f32 SqrDistance = FLT_MAX;
    
    u32 Iterations = 0;
    for(;;)
    {
        Iterations++;
        
        gjk_vertex Support = GetSupport(ObjectA, ObjectB, V);
        
        if(Simplex.HasVertex(Support.W))
        {
            Status = GJK_DISTANCE_STATUS_ALREADY_IN_SIMPLEX;
            break;
        }
        
        f32 Delta = Dot(V, Support.W);
        
        if((SqrDistance - Delta) <= (SqrDistance * GJK_RELATIVE_ERROR))
        {
            Status = GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER;
            break;
        }
        
        Simplex.AddVertex(&Support);
        
        v3f NewV;
        if(!Simplex.GetClosestPointToOrigin(&NewV))
        {
            Status = GJK_DISTANCE_STATUS_INVALID_SIMPLEX;
            break;
        }
        
        f32 PrevSqrDistance = SqrDistance;
        SqrDistance = SquareMagnitude(NewV);
        if(SqrDistance < GJK_RELATIVE_ERROR)
        {
            V = NewV;
            Status = GJK_DISTANCE_STATUS_DIRECTION_CLOSE_TO_ORIGIN;
            break;
        }
        
        if((PrevSqrDistance - SqrDistance) <= (FLT_EPSILON*PrevSqrDistance))
        {
            Status = GJK_DISTANCE_STATUS_NOT_GETTING_CLOSER_2;
            break;
        }
        
        V = NewV;
        
        ASSERT(Simplex.VertexCount < 4);        
    }
    
    ASSERT(Status != GJK_DISTANCE_STATUS_NONE);
    
    gjk_distance Result;
    Result.Status         = Status;
    Result.Simplex        = Simplex;
    Result.V              = V;
    Result.SquareDistance = SquareMagnitude(V);
    
    DEVELOPER_MAX_GJK_ITERATIONS(Iterations);
    
    return Result;    
}

struct gjk_simplex
{
    gjk_vertex Vertex[4];     
    i32 LastIndex;
    
    inline void Add(gjk_vertex* Support)
    {
        LastIndex++;
        Vertex[LastIndex] = *Support;        
    }
    
    inline u32 GetVertexCount()
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

i32 PerformLineTest(gjk_simplex* Simplex, v3f* V)
{
    v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    v3f B = Simplex->Vertex[0].W;    
    v3f AB = B-A;
    v3f AO =  -A;
    
    f32 DotResult = Dot(AB, AO);
    v3f Temp = Cross(AB, AO);
    if(IsFuzzyZero(SquareMagnitude(Temp)) && (DotResult > 0))
        return 1;
    
    if(IsFuzzyZero(DotResult) || (DotResult < 0))
    {
        Simplex->Vertex[0] = Simplex->Vertex[Simplex->LastIndex];
        Simplex->LastIndex = 0;
        *V = AO;
    }
    else
    {
        *V = Cross(Cross(AB, AO), AB);
    }
    
    return 0;
}

i32 SignCCD(f32 Value)
{
    if(IsFuzzyZero(Value))
        return 0;
    else if(Value < 0)
        return -1;
    else
        return 1;
}

f32 PointSegmentDistance(v3f P, v3f A, v3f B)
{
    v3f AB = B-A;
    v3f PA = A-P;
    
    f32 t = -1 * Dot(PA, AB);
    t /= SquareMagnitude(AB);
    
    f32 Result;
    if(t < 0 || IsFuzzyZero(t))
    {
        Result = SquareMagnitude(A-P);
    }
    else if(t > 1 || AreEqual32(t, 1))
    {
        Result = SquareMagnitude(B-P);
    }
    else
    {
        AB *= t;
        AB += PA;
        Result = SquareMagnitude(AB);
    }
    
    return Result;
}

f32 PointTriangleDistance(v3f P, v3f A, v3f B, v3f C)
{
    v3f AB = B-A;
    v3f AC = C-A;
    v3f PA = A-P;
    
    //IMPORTANT(EVERYONE): There are a lot of rounding and precision errors with these values so we made them
    //f64 bit floats  otherwise they can report a zero distance which is completely wrong. Maybe there is a better way
    f64 u = SquareMagnitude(PA);
    f64 v = SquareMagnitude(AB);
    f64 w = SquareMagnitude(AC);
    f64 p = Dot(PA, AB);
    f64 q = Dot(PA, AC);
    f64 r = Dot(AB, AC);
    
    f64 s = (q*r - w*p) / (w*v - r*r);
    f64 t = (-s*r - q) / w;
    
    f64 Result;
    if((IsFuzzyZero(s) || (s > 0)) && 
       (AreEqual64(s, 1) || s < 1) &&
       (IsFuzzyZero(t) || (t > 0)) &&
       (AreEqual64(t, 1) || t < 1) &&
       (AreEqual64(t+s, 1) || (t+s < 1)))
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
        f32 Dist1 = PointSegmentDistance(P, A, C);
        
        if(Result > Dist1)        
            Result = Dist1;        
        
        f32 Dist2 = PointSegmentDistance(P, B, C);
        if(Result > Dist2)
            Result = Dist2;                
    }
    
    return (f32)Result;
}

i32 PerformTriangleTest(gjk_simplex* Simplex, v3f* V)
{
    v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    v3f B = Simplex->Vertex[1].W;
    v3f C = Simplex->Vertex[0].W;
    
    f32 Distance = PointTriangleDistance(V3(), A, B, C);
    if(IsFuzzyZero(Distance))
        return 1;
    
    if(AreEqualV3(A, B) || AreEqualV3(A, C))
        return -1;
    
    v3f AO = -A;
    v3f AB = B-A;
    v3f AC = C-A;
    v3f ABC = Cross(AB, AC);
    v3f Temp = Cross(ABC, AC);
    f32 DotResult = Dot(Temp, AO);
    
    if(IsFuzzyZero(DotResult) || (DotResult > 0))
    {
        DotResult = Dot(AC, AO);
        if(IsFuzzyZero(DotResult) || (DotResult > 0))
        {
            Simplex->Vertex[1] = Simplex->Vertex[Simplex->LastIndex];
            Simplex->LastIndex = 1;
            *V = Cross(Cross(AC, AO), AC);
        }
        else
        {
            DotResult = Dot(AB, AO);
            if(IsFuzzyZero(DotResult) || (DotResult > 0))
            {
                Simplex->Vertex[0] = Simplex->Vertex[1];
                Simplex->Vertex[1] = Simplex->Vertex[Simplex->LastIndex];
                Simplex->LastIndex = 1;
                *V = Cross(Cross(AB, AO), AB);
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
        DotResult = Dot(ABC, AO);
        if(IsFuzzyZero(DotResult) || (DotResult > 0))
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

i32 PerformTetrahedronTest(gjk_simplex* Simplex, v3f* V)
{
    v3f A = Simplex->Vertex[Simplex->LastIndex].W;
    v3f B = Simplex->Vertex[2].W;
    v3f C = Simplex->Vertex[1].W;
    v3f D = Simplex->Vertex[0].W;
    
    f32 Distance = PointTriangleDistance(A, B, C, D);
    if(IsFuzzyZero(Distance))
        return -1;
    
    Distance = PointTriangleDistance(V3(), A, B, C);
    if(IsFuzzyZero(Distance))
        return 1;
    
    Distance = PointTriangleDistance(V3(), A, C, D);
    if(IsFuzzyZero(Distance))
        return 1;
    
    Distance = PointTriangleDistance(V3(), A, B, D);
    if(IsFuzzyZero(Distance))
        return 1;
    
    Distance = PointTriangleDistance(V3(), B, C, D);
    if(IsFuzzyZero(Distance))
        return 1;
    
    v3f AO  =  -A;
    v3f AB  = B-A;
    v3f AC  = C-A;
    v3f AD  = D-A;
    v3f ABC = Cross(AB, AC);
    v3f ACD = Cross(AC, AD);
    v3f ADB = Cross(AD, AB);
    
    i32 IsBOnACD = SignCCD(Dot(ACD, AB));
    i32 IsCOnADB = SignCCD(Dot(ADB, AC));
    i32 IsDOnABC = SignCCD(Dot(ABC, AD));
    
    b32 AB_O = SignCCD(Dot(ACD, AO)) == IsBOnACD;
    b32 AC_O = SignCCD(Dot(ADB, AO)) == IsCOnADB;
    b32 AD_O = SignCCD(Dot(ABC, AO)) == IsDOnABC;
    
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

i32 PerformSimplexTest(gjk_simplex* Simplex, v3f* V)
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
        
        INVALID_DEFAULT_CASE;        
    }
    
    return -1;
}

template <typename typeA, typename typeB>
b32 GJKIntersected(typeA* ObjectA, typeB* ObjectB, gjk_simplex* Simplex)
{    
    v3f V = Global_WorldXAxis;     
    *Simplex = InitSimplex();
    
    gjk_vertex Support = GetSupport2(ObjectA, ObjectB, V);    
    Simplex->Add(&Support);
    
    V = -Support.W;
    
    u32 Iterations = 0;
    for(;;)
    {
        DEVELOPER_MAX_GJK_ITERATIONS(Iterations);
        Iterations++;                        
        Support = GetSupport2(ObjectA, ObjectB, V);
        
        f32 Delta = Dot(V, Support.W);
        
        if(Delta < 0)        
            return false;        
        
        Simplex->Add(&Support);
        
        i32 SimplexResult = PerformSimplexTest(Simplex, &V);
        if(SimplexResult == 1)
            return true;
        else if(SimplexResult == -1)
            return false;
        
        if(IsFuzzyZero(SquareMagnitude(V)))
            return false;
    }    
}

template <typename typeA, typename typeB>
b32 GJKIntersected(typeA* ObjectA, typeB* ObjectB)
{
    gjk_simplex Simplex;
    return GJKIntersected(ObjectA, ObjectB, &Simplex);
}

template <typename typeA, typename typeB>
b32 GJKQuadraticIntersected(typeA* ObjectA, typeB* ObjectB, f32 Radius)
{
    gjk_distance Distance = GJKDistance(ObjectA, ObjectB);
    b32 Result = Distance.SquareDistance <= Square(Radius);
    return Result;
}