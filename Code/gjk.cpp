/* Original Author: Armand (JJ) Karambasis */

inline 
v3f GetBarycentricPoint(v3f Point, f32 Barycentric, f32 BarycentricRatio)
{
    v3f Result = Point*(BarycentricRatio*Barycentric);
    return Result;
}

inline f32 
GetInvTotalBarycentric(f32* Barycentric, u32 VertexCount)
{
    f32 Result = 0.0f;
    for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
        Result += Barycentric[VertexIndex];
    Result = 1.0f/Result;
    return Result;
}

b32 FindDuplicateSupports(gjk_simplex_vertex* Vertices, u32 VertexCount,
                          v3f ASupportPos, v3f BSupportPos)
{
    for(u32 VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++)
    {
        if(Vertices[VertexIndex].ASupportPos != ASupportPos) continue;
        if(Vertices[VertexIndex].BSupportPos != BSupportPos) continue;        
        return true;
    }    
    return false;
}

b32 AreGettingCloser(gjk_simplex* Simplex)
{    
    f32 BarycentricRatio = GetInvTotalBarycentric(Simplex->Barycentric, Simplex->VertexCount);        
    v3f Point = {};
    switch(Simplex->VertexCount)
    {
        case 1:
        {         
            Point = Simplex->Vertices[0].CsoD;                
        } break;
        
        case 2:
        {
            v3f A = GetBarycentricPoint(Simplex->Vertices[0].CsoD, Simplex->Barycentric[0], BarycentricRatio);
            v3f B = GetBarycentricPoint(Simplex->Vertices[1].CsoD, Simplex->Barycentric[1], BarycentricRatio);
            Point = A+B;
        } break;
        
        case 3:
        {
            v3f A = GetBarycentricPoint(Simplex->Vertices[0].CsoD, Simplex->Barycentric[0], BarycentricRatio);
            v3f B = GetBarycentricPoint(Simplex->Vertices[1].CsoD, Simplex->Barycentric[1], BarycentricRatio);
            v3f C = GetBarycentricPoint(Simplex->Vertices[2].CsoD, Simplex->Barycentric[2], BarycentricRatio);
            Point = A+B+C;
        } break;
        
        case 4:
        {
            v3f A = GetBarycentricPoint(Simplex->Vertices[0].CsoD, Simplex->Barycentric[0], BarycentricRatio);
            v3f B = GetBarycentricPoint(Simplex->Vertices[1].CsoD, Simplex->Barycentric[1], BarycentricRatio);
            v3f C = GetBarycentricPoint(Simplex->Vertices[2].CsoD, Simplex->Barycentric[2], BarycentricRatio);
            v3f D = GetBarycentricPoint(Simplex->Vertices[3].CsoD, Simplex->Barycentric[3], BarycentricRatio);
            Point = A+B+C+D;
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    f32 Distance = SquareMagnitude(Point);
    if(Distance >= Simplex->ClosestDistance) return false;
    
    Simplex->ClosestDistance = Distance;
    return true;
}

v3f NewSearchDirection(gjk_simplex* Simplex)
{    
    v3f Result = {};
    switch(Simplex->VertexCount)
    {
        case 1:
        {
            Result = -Simplex->Vertices[0].CsoD;
        } break;
        
        case 2:
        {
            v3f A = Simplex->Vertices[1].CsoD - Simplex->Vertices[0].CsoD;
            v3f B = -Simplex->Vertices[1].CsoD;
            Result = Cross(Cross(A, B), A);
        } break;
        
        case 3:
        {
            v3f A = Simplex->Vertices[1].CsoD - Simplex->Vertices[0].CsoD;
            v3f B = Simplex->Vertices[2].CsoD - Simplex->Vertices[0].CsoD;
            v3f N = Cross(A, B);
            Result = (Dot(N, Simplex->Vertices[0].CsoD) <= 0.0f) ? N : -N;                    
        } break;
        
        INVALID_DEFAULT_CASE;
    }           
    
    return Result;
}

void LineTest(gjk_simplex* Simplex)
{
    v3f A = Simplex->Vertices[0].CsoD;
    v3f B = Simplex->Vertices[1].CsoD;
    
    v3f AB = A-B;
    v3f BA = B-A;
    
    f32 U = Dot(BA, B);
    f32 V = Dot(AB, A);
    
    if(V <= 0.0f)
    {
        Simplex->VertexCount = 1;
        Simplex->Barycentric[0] = 1.0f;
        return;
    }
    
    if(U <= 0.0f)
    {
        Simplex->VertexCount = 1;
        Simplex->Barycentric[0] = 1.0f;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        return;
    }
    
    Simplex->Barycentric[0] = U;
    Simplex->Barycentric[1] = V;
    Simplex->VertexCount = 2;
}

void TriangleTest(gjk_simplex* Simplex)
{
    v3f A = Simplex->Vertices[0].CsoD;
    v3f B = Simplex->Vertices[1].CsoD;
    v3f C = Simplex->Vertices[2].CsoD;
    
    v3f AB = A-B;
    v3f BA = B-A;
    v3f BC = B-C;
    v3f CB = C-B;
    v3f CA = C-A;
    v3f AC = A-C;
    
    f32 U_AB = Dot(BA, B);
    f32 V_AB = Dot(AB, A);
    
    f32 U_BC = Dot(CB, C);
    f32 V_BC = Dot(BC, B);
    
    f32 U_CA = Dot(AC, A);
    f32 V_CA = Dot(CA, C);
    
    if((V_AB <= 0.0f) && (U_CA <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        return;
    }
    
    if((U_AB <= 0.0f) && (V_BC <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        return;
    }
    
    if((U_BC <= 0.0f) && (V_CA <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        Simplex->Vertices[0] = Simplex->Vertices[2];
        return;
    }
    
    v3f N[4] = { Cross(BA, CA), Cross(B, C), Cross(C, A), Cross(A, B) };
    
    f32 U = Dot(N[1], N[0]);
    f32 V = Dot(N[2], N[0]);
    f32 W = Dot(N[3], N[0]);
    
    if((U_AB > 0.0f) && (V_AB > 0.0f) && (W <= 0.0f))
    {
        Simplex->Barycentric[0] = U_AB;
        Simplex->Barycentric[1] = V_AB;
        Simplex->VertexCount = 2;
        return;
    }
    
    if((U_BC > 0.0f) && (V_BC > 0.0f) && (U <= 0.0f))
    {
        Simplex->Barycentric[0] = U_BC;
        Simplex->Barycentric[1] = V_BC;
        Simplex->VertexCount = 2;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        Simplex->Vertices[1] = Simplex->Vertices[2];
        return;
    }
    
    if((U_CA > 0.0f) && (V_CA > 0.0f) && (V <= 0.0f))
    {
        Simplex->Barycentric[0] = U_CA;
        Simplex->Barycentric[1] = V_CA;
        Simplex->VertexCount = 2;
        Simplex->Vertices[1] = Simplex->Vertices[0];
        Simplex->Vertices[0] = Simplex->Vertices[2];
        return;
    }
    
    Simplex->Barycentric[0] = U;
    Simplex->Barycentric[1] = V;
    Simplex->Barycentric[2] = W;
    Simplex->VertexCount = 3;
}

void TetrahedronTest(gjk_simplex* Simplex)
{
    v3f A = Simplex->Vertices[0].CsoD;
    v3f B = Simplex->Vertices[1].CsoD;
    v3f C = Simplex->Vertices[2].CsoD;
    v3f D = Simplex->Vertices[3].CsoD;
    
    v3f AB = A-B;
    v3f BA = B-A;
    v3f BC = B-C;
    v3f CB = C-B;
    v3f CA = C-A;
    v3f AC = A-C;
    
    v3f DB = D-B;
    v3f BD = B-D;
    v3f DC = D-C;
    v3f CD = C-D;
    v3f DA = D-A;
    v3f AD = A-D;
    
    f32 U_AB = Dot(BA, B);
    f32 V_AB = Dot(AB, A);
    
    f32 U_BC = Dot(CB, C);
    f32 V_BC = Dot(BC, B);
    
    f32 U_CA = Dot(AC, A);
    f32 V_CA = Dot(CA, C);
    
    f32 U_BD = Dot(DB, D);
    f32 V_BD = Dot(BD, B);
    
    f32 U_DC = Dot(CD, C);
    f32 V_DC = Dot(DC, D);
    
    f32 U_AD = Dot(DA, D);
    f32 V_AD = Dot(AD, A);
    
    if((V_AB <= 0.0f) && (U_CA <= 0.0f) && (V_AD <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        return;
    }
    
    if((U_AB <= 0.0f) && (V_BC <= 0.0f) && (V_BD <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        return;
    }
    
    if((U_BC <= 0.0f) && (V_CA <= 0.0f) && (U_DC <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        Simplex->Vertices[0] = Simplex->Vertices[2];
        return;
    }
    
    if((U_BD <= 0.0f) && (V_DC <= 0.0f) && (U_AD <= 0.0f))
    {
        Simplex->Barycentric[0] = 1.0f;
        Simplex->VertexCount = 1;
        Simplex->Vertices[0] = Simplex->Vertices[3];
        return;
    }
    
    v3f N[4];
    N[0] = Cross(DA, BA);
    N[1] = Cross(D, B);
    N[2] = Cross(B, A);
    N[3] = Cross(A, D);
    
    f32 U_ADB = Dot(N[1], N[0]);
    f32 V_ADB = Dot(N[2], N[0]);
    f32 W_ADB = Dot(N[3], N[0]);
    
    N[0] = Cross(CA, DA);
    N[1] = Cross(C, D);
    N[2] = Cross(D, A);
    N[3] = Cross(A, C);
    
    f32 U_ACD = Dot(N[1], N[0]);
    f32 V_ACD = Dot(N[2], N[0]);
    f32 W_ACD = Dot(N[3], N[0]);
    
    N[0] = Cross(BC, DC);
    N[1] = Cross(B, D);
    N[2] = Cross(D, C);
    N[3] = Cross(C, B);
    
    f32 U_CBD = Dot(N[1], N[0]);
    f32 V_CBD = Dot(N[2], N[0]);
    f32 W_CBD = Dot(N[3], N[0]);
    
    N[0] = Cross(BA, CA);
    N[1] = Cross(B, C);
    N[2] = Cross(C, A);
    N[3] = Cross(A, B);
    
    f32 U_ABC = Dot(N[1], N[0]);
    f32 V_ABC = Dot(N[2], N[0]);
    f32 W_ABC = Dot(N[3], N[0]);
    
    if((W_ABC <= 0.0f) && (V_ADB <= 0.0f) && (U_AB > 0.0f) && (V_AB > 0.0f))
    {
        Simplex->Barycentric[0] = U_AB;
        Simplex->Barycentric[1] = V_AB;
        Simplex->VertexCount = 2;
        return;
    }
    
    if((U_ABC <= 0.0f) && (W_CBD <= 0.0f) && (U_BC > 0.0f) && (V_BC > 0.0f))
    {
        Simplex->Barycentric[0] = U_BC;
        Simplex->Barycentric[1] = V_BC;
        Simplex->VertexCount = 2;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        Simplex->Vertices[1] = Simplex->Vertices[2];
        return;
    }   
    
    if((V_ABC <= 0.0f) && (W_ACD <= 0.0f) && (U_CA > 0.0f) && (V_CA > 0.0f))
    {
        Simplex->Barycentric[0] = U_CA;
        Simplex->Barycentric[1] = V_CA;
        Simplex->VertexCount = 2;
        Simplex->Vertices[1] = Simplex->Vertices[0];
        Simplex->Vertices[0] = Simplex->Vertices[2];
        return;
    }
    
    if((V_CBD <= 0.0f) && (U_ACD <= 0.0f) && (U_DC > 0.0f) && (V_DC > 0.0f))
    {
        Simplex->Barycentric[0] = U_DC;
        Simplex->Barycentric[1] = V_DC;
        Simplex->VertexCount = 2;
        Simplex->Vertices[0] = Simplex->Vertices[3];
        Simplex->Vertices[1] = Simplex->Vertices[2];
        return;
    }
    
    if((V_ACD <= 0.0f) && (W_ADB <= 0.0f) && (U_AD > 0.0f) && (V_AD > 0.0f))
    {
        Simplex->Barycentric[0] = U_AD;
        Simplex->Barycentric[1] = V_AD;
        Simplex->VertexCount = 2;
        Simplex->Vertices[1] = Simplex->Vertices[3];
        return;
    }
    
    if((U_CBD <= 0.0f) && (U_ADB <= 0.0f) && (U_BD > 0.0f) && (V_BD > 0.0f))
    {
        Simplex->Barycentric[0] = U_BD;
        Simplex->Barycentric[1] = V_BD;
        Simplex->VertexCount = 2;
        Simplex->Vertices[0] = Simplex->Vertices[1];
        Simplex->Vertices[1] = Simplex->Vertices[3];
        return;
    }
    
#define BOX_VOLUME(A, B, C) Dot(Cross(A, B), C)
    
    f32 Volume = BOX_VOLUME(CB, AB, DB);
    f32 InvVolume = (Volume == 0) ? 1.0f : 1.0f/Volume;
    
    f32 U = BOX_VOLUME(C, D, B) * InvVolume;
    f32 V = BOX_VOLUME(C, A, D) * InvVolume;
    f32 W = BOX_VOLUME(D, A, B) * InvVolume;
    f32 X = BOX_VOLUME(B, A, C) * InvVolume;
    
    if((X <= 0.0f) && (U_ABC > 0.0f) && (V_ABC > 0.0f) && (W_ABC > 0.0f))
    {
        Simplex->Barycentric[0] = U_ABC;
        Simplex->Barycentric[1] = V_ABC;
        Simplex->Barycentric[2] = W_ABC;
        Simplex->VertexCount = 3;
        return;
    }
    
    if((U <= 0.0f) && (U_CBD > 0.0f) && (V_CBD > 0.0f) && (W_CBD > 0.0f))
    {
        Simplex->Barycentric[0] = U_CBD;
        Simplex->Barycentric[1] = V_CBD;
        Simplex->Barycentric[2] = W_CBD;
        Simplex->VertexCount = 3;
        Simplex->Vertices[0] = Simplex->Vertices[2];
        Simplex->Vertices[2] = Simplex->Vertices[3];
        return;
    }
    
    if((V <= 0.0f) && (U_ACD > 0.0f) && (V_ACD > 0.0f) && (W_ACD > 0.0f))
    {
        Simplex->Barycentric[0] = U_ACD;
        Simplex->Barycentric[1] = V_ACD;
        Simplex->Barycentric[2] = W_ACD;
        Simplex->VertexCount = 3;
        Simplex->Vertices[1] = Simplex->Vertices[2];
        Simplex->Vertices[2] = Simplex->Vertices[3];
        return;
    }
    
    if((W <= 0.0f) && (U_ADB > 0.0f) && (V_ADB > 0.0f) && (W_ADB > 0.0f))
    {
        Simplex->Barycentric[0] = U_ADB;
        Simplex->Barycentric[1] = V_ADB;
        Simplex->Barycentric[2] = W_ADB;
        Simplex->VertexCount = 3;
        Simplex->Vertices[2] = Simplex->Vertices[1];
        Simplex->Vertices[1] = Simplex->Vertices[3];
        return;
    }
    
    Simplex->Barycentric[0] = U;
    Simplex->Barycentric[1] = V;
    Simplex->Barycentric[2] = W;
    Simplex->Barycentric[3] = X;
    Simplex->VertexCount = 4;    
#undef BOX_VOLUME
}

gjk_result GJK(support_function* SupportA, void* SupportAData, 
               support_function* SupportB, void* SupportBData)
{
    gjk_result Result = {};
    Result.Intersected = false;
    
    gjk_simplex Simplex = {};    
    Simplex.ClosestDistance = FLT_MAX;

    v3f CsoD = V3(1.0f, 0.0f, 0.0f);    
    u32 MaxIterations = GJK_MAX_ITERATIONS;
    
    u32 Iteration = 0;
    for(Iteration = 0; Iteration < MaxIterations; Iteration++)
    {
        v3f ASupportPos = SupportA(SupportAData, -CsoD);
        v3f BSupportPos = SupportB(SupportBData,  CsoD);
        CsoD = BSupportPos - ASupportPos;
        
        if(FindDuplicateSupports(Simplex.Vertices, Simplex.VertexCount, ASupportPos, BSupportPos))
            break;
        
        gjk_simplex_vertex* Vertex = &Simplex.Vertices[Simplex.VertexCount];
        Vertex->ASupportPos = ASupportPos;
        Vertex->BSupportPos = BSupportPos;        
        Vertex->CsoD = CsoD;

        Simplex.Barycentric[Simplex.VertexCount++] = 1.0f;
        switch(Simplex.VertexCount)
        {
            case 2:
            {
                LineTest(&Simplex);
            } break;
            
            case 3:
            {
                TriangleTest(&Simplex);
            } break;
            
            case 4:
            {
                TetrahedronTest(&Simplex);
            } break;
        }
        
        if(Simplex.VertexCount == 4)
        {            
            Result.Intersected = true;
            return Result;            
        }
        
        if(!AreGettingCloser(&Simplex))        
            break;

        CsoD = NewSearchDirection(&Simplex);
        
        if(SquareMagnitude(CsoD) < (GJK_EPSILON*GJK_EPSILON))
            break;        
    }

    ASSERT(Iteration < MaxIterations);
    
    DEVELOPER_MAX_GJK_ITERATIONS(Iteration);   
    
    f32 BarycentricRatio = GetInvTotalBarycentric(Simplex.Barycentric, Simplex.VertexCount);
    switch(Simplex.VertexCount)
    {
        case 1:
        {
            Result.ClosestPoints[0] = Simplex.Vertices[0].ASupportPos;
            Result.ClosestPoints[1] = Simplex.Vertices[0].BSupportPos;
        } break;
        
        case 2:
        {                        
            v3f A = GetBarycentricPoint(Simplex.Vertices[0].ASupportPos, Simplex.Barycentric[0], BarycentricRatio);
            v3f B = GetBarycentricPoint(Simplex.Vertices[1].ASupportPos, Simplex.Barycentric[1], BarycentricRatio);
            v3f C = GetBarycentricPoint(Simplex.Vertices[0].BSupportPos, Simplex.Barycentric[0], BarycentricRatio);
            v3f D = GetBarycentricPoint(Simplex.Vertices[1].BSupportPos, Simplex.Barycentric[1], BarycentricRatio);
            
            Result.ClosestPoints[0] = A+B;
            Result.ClosestPoints[1] = C+D;
        } break;
        
        case 3:
        {            
            v3f A = GetBarycentricPoint(Simplex.Vertices[0].ASupportPos, Simplex.Barycentric[0], BarycentricRatio);
            v3f B = GetBarycentricPoint(Simplex.Vertices[1].ASupportPos, Simplex.Barycentric[1], BarycentricRatio);
            v3f C = GetBarycentricPoint(Simplex.Vertices[2].ASupportPos, Simplex.Barycentric[2], BarycentricRatio);
            
            v3f D = GetBarycentricPoint(Simplex.Vertices[0].BSupportPos, Simplex.Barycentric[0], BarycentricRatio);
            v3f E = GetBarycentricPoint(Simplex.Vertices[1].BSupportPos, Simplex.Barycentric[1], BarycentricRatio);
            v3f F = GetBarycentricPoint(Simplex.Vertices[2].BSupportPos, Simplex.Barycentric[2], BarycentricRatio);
            
            Result.ClosestPoints[0] = A+B+C;
            Result.ClosestPoints[1] = D+E+F;            
        } break;
        
        case 4:
        {
            //TODO(JJ): Do we need to handle this case?
            ASSERT(false);
        } break;
    }    
    return Result;
}