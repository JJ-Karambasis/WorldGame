#define EPA_MAX_VERTICES 128
#define EPA_MAX_FACES 256

#define EPA_EPSILON 1e-4f
#define EPA_PLANE_EPSILON 1e-5f

struct epa_face
{
    v3f Normal;
    f32 Distance;
    gjk_vertex Vertices[3];
    epa_face* AdjFaces[3];
    epa_face* ChildFaces[2];        
    u8 Edges[3];
    u8 Pass;
};

struct epa_list
{
    u32 Count;
    epa_face* Root;
    
    inline void Add(epa_face* Face)
    {
        Face->ChildFaces[0] = NULL;
        Face->ChildFaces[1] = Root;
        if(Root) Root->ChildFaces[0] = Face;
        Root = Face;
        Count++;
    }
    
    inline void Remove(epa_face* Face)
    {
        if(Face->ChildFaces[1]) Face->ChildFaces[1]->ChildFaces[0] = Face->ChildFaces[0];
        if(Face->ChildFaces[0]) Face->ChildFaces[0]->ChildFaces[1] = Face->ChildFaces[1];
        if(Face == Root) Root = Face->ChildFaces[1];
        Count--;
    }    
};

inline void 
BindFaces(epa_face* Face0, u32 Edge0, epa_face* Face1, u32 Edge1)
{
    Face0->Edges[Edge0] = (u8)Edge1;
    Face1->Edges[Edge1] = (u8)Edge0;    
    Face0->AdjFaces[Edge0] = Face1;
    Face1->AdjFaces[Edge1] = Face0;
}

b32 GetEdgeDistance(f32* Distance, v3f P0, v3f P1, v3f N)
{
    v3f Edge = P1-P0;
    v3f EdgeNormal = Cross(Edge, N);
    f32 DotResult = Dot(P0, EdgeNormal);
    
    if(DotResult < 0)
    {
        f32 EdgeSqr = SquareMagnitude(Edge);
        f32 a = Dot(P0, Edge);
        f32 b = Dot(P1, Edge);
        
        if(a > 0)
        {
            *Distance = Magnitude(P0);
        }
        else if(b < 0)
        {
            *Distance = Magnitude(P1);
        }
        else
        {
            f32 ab = Dot(P0, P1);
            f32 TempDistance = (SquareMagnitude(P0)*SquareMagnitude(P1) - Square(ab))/EdgeSqr;
            *Distance = (TempDistance > 0) ? Sqrt(TempDistance) : 0;
        }
        
        return true;
    }
    
    return false;
}

b32 ContainsOrigin(gjk_vertex** Tetrahedron)
{
    v3f n0 = Cross(Tetrahedron[1]->W-Tetrahedron[0]->W, Tetrahedron[2]->W-Tetrahedron[0]->W);
    f32 a = Dot(n0, Tetrahedron[0]->W);
    f32 b = Dot(n0, Tetrahedron[3]->W);    
    
    if(!IsFuzzyZero(a) && !IsFuzzyZero(b))       
    {
        if((a > 0) == (b > 0))
            return false;
    }
    
    v3f n1 = Cross(Tetrahedron[2]->W-Tetrahedron[1]->W, Tetrahedron[3]->W-Tetrahedron[1]->W);
    a = Dot(n1, Tetrahedron[1]->W);
    b = Dot(n1, Tetrahedron[0]->W);    
    
    if(!IsFuzzyZero(a) && !IsFuzzyZero(b))       
    {
        if((a > 0) == (b > 0))
            return false;
    }
    
    v3f n2 = Cross(Tetrahedron[3]->W-Tetrahedron[2]->W, Tetrahedron[0]->W-Tetrahedron[2]->W);
    a = Dot(n2, Tetrahedron[2]->W);
    b = Dot(n2, Tetrahedron[1]->W);    
    
    if(!IsFuzzyZero(a) && !IsFuzzyZero(b))       
    {
        if((a > 0) == (b > 0))
            return false;
    }
    
    v3f n3 = Cross(Tetrahedron[0]->W-Tetrahedron[3]->W, Tetrahedron[1]->W-Tetrahedron[3]->W);
    a = Dot(n3, Tetrahedron[3]->W);
    b = Dot(n3, Tetrahedron[2]->W);    
    
    if(!IsFuzzyZero(a) && !IsFuzzyZero(b))       
    {
        if((a > 0) == (b > 0))
            return false;
    }
    
    return true;
}

struct epa_silhouette
{
    epa_face* Face0;
    epa_face* Face1;
    u32       NextFace;
};

enum epa_status
{
    EPA_STATUS_NONE,
    EPA_STATUS_INVALID_TETRAHEDRON,    
    EPA_STATUS_OUT_OF_FACES,
    EPA_STATUS_DEGENERATE, 
    EPA_STATUS_NOT_CONVEX,
    EPA_STATUS_INVALID_HULL,
    EPA_STATUS_ACCURACY_REACHED, 
    EPA_STATUS_VALID
};

struct epa_result
{
    b32 IsValid;
    epa_status Status;
    v3f Witness[2];    
    v3f Normal;
    f32 Penetration;
};

struct epa_context
{
    epa_status Status;    
    epa_face FaceStore[EPA_MAX_FACES];    
    epa_list Hull;
    epa_list Free;
    
    epa_face* CreateFace(gjk_vertex P0, gjk_vertex P1, gjk_vertex P2, b32 Forced)
    {
        if(Free.Root)
        {
            epa_face* Face = Free.Root;
            Free.Remove(Face);
            Hull.Add(Face);
            
            Face->Pass = 0;
            Face->Vertices[0] = P0;
            Face->Vertices[1] = P1;
            Face->Vertices[2] = P2;
            Face->Normal = Cross(P1.W-P0.W, P2.W-P0.W);
            f32 Length = Magnitude(Face->Normal);
            if(Length > EPA_EPSILON)
            {
                b32 FoundEdgeDistance = (GetEdgeDistance(&Face->Distance, P0.W, P1.W, Face->Normal) ||
                                         GetEdgeDistance(&Face->Distance, P1.W, P2.W, Face->Normal) ||
                                         GetEdgeDistance(&Face->Distance, P2.W, P0.W, Face->Normal));
                if(!FoundEdgeDistance)
                    Face->Distance = Dot(P0.W, Face->Normal) / Length;
                
                Face->Normal /= Length;
                if(Forced || (Face->Distance >= -EPA_PLANE_EPSILON))
                    return Face;
                else
                    Status = EPA_STATUS_NOT_CONVEX;
                
            }
            else            
                Status = EPA_STATUS_DEGENERATE;            
            
            Hull.Remove(Face);
            Free.Add(Face);
            return NULL;
            
        }
        else
        {
            Status = EPA_STATUS_OUT_OF_FACES;
            return NULL;
        }
    }
    
    epa_face* GetBestFace()
    {
        epa_face* Result = Hull.Root;
        f32 BestSqrDistance = Square(Result->Distance);
        for(epa_face* Face = Result->ChildFaces[1]; Face; Face = Face->ChildFaces[1])
        {
            f32 SqrDistance = Square(Face->Distance);
            if(SqrDistance < BestSqrDistance)
            {
                BestSqrDistance = SqrDistance;
                Result = Face;
            }
        }
        
        return Result;
    }
    
    b32 Expand(u32 Pass, gjk_vertex Vertex, epa_face* Face, u32 Edge, epa_silhouette* Silhouette)
    {
        local const u32 Indices0[] = {1, 2, 0};
        local const u32 Indices1[] = {2, 0, 1};
        if(Face->Pass != Pass)
        {
            u32 E1 = Indices0[Edge];
            if((Dot(Face->Normal, Vertex.W) - Face->Distance) < -EPA_PLANE_EPSILON)
            {
                epa_face* NewFace = CreateFace(Face->Vertices[E1], Face->Vertices[Edge], Vertex, false);
                if(NewFace)
                {
                    BindFaces(NewFace, 0, Face, Edge);
                    if(Silhouette->Face0)
                        BindFaces(Silhouette->Face0, 1, NewFace, 2);
                    else
                        Silhouette->Face1 = NewFace;
                    Silhouette->Face0 = NewFace;
                    Silhouette->NextFace++;
                    
                    return true;
                }
            }
            else
            {                
                u32 E2 = Indices1[Edge];
                Face->Pass = (u8)Pass;
                if(Expand(Pass, Vertex, Face->AdjFaces[E1], Face->Edges[E1], Silhouette) &&
                   Expand(Pass, Vertex, Face->AdjFaces[E2], Face->Edges[E2], Silhouette))
                {
                    Hull.Remove(Face);
                    Free.Add(Face);
                    return true;
                }
            }
        }
        
        return false;
    }
    
    template <typename typeA, typename typeB>
        epa_result Evaluate(typeA* ObjectA, typeB* ObjectB, gjk_vertex* Vertices)
    {        
        epa_result Result = {};
        
        if(Determinant(Vertices[0].W - Vertices[3].W, Vertices[1].W - Vertices[3].W, Vertices[2].W - Vertices[3].W) < 0)        
            SWAP(Vertices[0], Vertices[1]);       
        
        for(u32 FaceIndex = 1; FaceIndex <= EPA_MAX_FACES; FaceIndex++)
            Free.Add(&FaceStore[EPA_MAX_FACES-FaceIndex]);
        
        epa_face* Tetrahedron[4] = 
        {
            CreateFace(Vertices[0], Vertices[1], Vertices[2], true),
            CreateFace(Vertices[1], Vertices[0], Vertices[3], true),
            CreateFace(Vertices[2], Vertices[1], Vertices[3], true),
            CreateFace(Vertices[0], Vertices[2], Vertices[3], true)
        };
        
        if(Hull.Count == 4)
        {
            epa_face* Best = GetBestFace();            
            u32 Pass = 0;
            u32 Iterations = 0;
            
            BindFaces(Tetrahedron[0], 0, Tetrahedron[1], 0);
            BindFaces(Tetrahedron[0], 1, Tetrahedron[2], 0);
            BindFaces(Tetrahedron[0], 2, Tetrahedron[3], 0);
            BindFaces(Tetrahedron[1], 1, Tetrahedron[3], 2);
            BindFaces(Tetrahedron[1], 2, Tetrahedron[2], 1);
            BindFaces(Tetrahedron[2], 2, Tetrahedron[3], 1);
            
            Status = EPA_STATUS_VALID;
            
            for(;;)
            {
                Iterations++;
                
                epa_silhouette Silhouette = {};
                b32 Valid = true;
                Best->Pass = (u8)(++Pass);
                
                gjk_vertex Vertex = GetSupport2(ObjectA, ObjectB, Best->Normal);
                f32 Distance = Dot(Best->Normal, Vertex.W) - Best->Distance;
                if(Distance > EPA_EPSILON)
                {
                    for(u32 Index = 0; (Index < 3) && Valid; Index++)
                        Valid &= Expand(Pass, Vertex, Best->AdjFaces[Index], Best->Edges[Index], &Silhouette);
                    
                    if(Valid && (Silhouette.NextFace >= 3))
                    {
                        BindFaces(Silhouette.Face0, 1, Silhouette.Face1, 2);
                        Hull.Remove(Best);
                        Free.Add(Best);
                        Best = GetBestFace();                        
                    }
                    else
                    {
                        Status = EPA_STATUS_INVALID_HULL;
                        break;
                    }                    
                }            
                else
                {
                    Status = EPA_STATUS_ACCURACY_REACHED;
                    break;
                }                
            }
            
            v3f Projection = Best->Normal*Best->Distance;
            
            f32 P[3] = 
            {
                Magnitude(Cross(Best->Vertices[1].W - Projection, Best->Vertices[2].W - Projection)),
                Magnitude(Cross(Best->Vertices[2].W - Projection, Best->Vertices[0].W - Projection)), 
                Magnitude(Cross(Best->Vertices[0].W - Projection, Best->Vertices[1].W - Projection))
            };
            
            f32 Sum = P[0]+P[1]+P[2];
            P[0] /= Sum;
            P[1] /= Sum;
            P[2] /= Sum;
            
            
            for(u32 Index = 0; Index < 3; Index++)
            {                
                gjk_vertex Support = GetSupport2(ObjectA, ObjectB, Best->Vertices[Index].V);
                Result.Witness[0] += Support.A*P[Index];
                Result.Witness[1] += Support.B*P[Index];                
            }
            
            Result.Status  = Status;
            Result.IsValid = true;                
            Result.Normal = Best->Normal;
            Result.Penetration = Best->Distance;                                                            
            
            return Result;
        }
        else
        {
            Result.IsValid = false;            
            Result.Status = EPA_STATUS_INVALID_TETRAHEDRON;
            return Result;
        }       
    }
};

template <typename typeA, typename typeB>
epa_result EPA(typeA* ObjectA, typeB* ObjectB)
{
    epa_result Result = {};
    
    gjk_simplex Simplex;
    b32 Intersected = GJKIntersected(ObjectA, ObjectB, &Simplex);
    if(!Intersected)
        return Result;
    
    gjk_vertex Tetrahedron[4] = {};
    switch(Simplex.GetVertexCount())
    {
        case 1:
        {
            Result.Status = EPA_STATUS_INVALID_TETRAHEDRON;
            return Result;
        } break;
        
        case 2:
        case 3:
        {
            gjk_vertex Hexahedron[5] = {};
            
            if(Simplex.GetVertexCount() == 2)
            {
                Hexahedron[0] = Simplex.Vertex[Simplex.LastIndex];
                Hexahedron[1] = Simplex.Vertex[0];                                           
                
                v3f D = Hexahedron[0].W - Hexahedron[1].W;
                v3f A = {};            
                if(Abs(D.x) < Abs(D.y))
                {
                    if(Abs(D.x) < Abs(D.z))
                        A = Global_WorldXAxis;
                    else
                        A = Global_WorldZAxis;
                }
                else
                {
                    if(Abs(D.y) < Abs(D.z))
                        A = Global_WorldYAxis;
                    else
                        A = Global_WorldZAxis;
                }
                
                m3 R = ToMatrix3(RotQuat(Normalize(D), PI/3)); 
                v3f V0 = Cross(D, A);                        
                v3f V1 = V0*R;
                v3f V2 = V1*R;
                
                Hexahedron[2] = GetSupport2(ObjectA, ObjectB, V0);
                Hexahedron[3] = GetSupport2(ObjectA, ObjectB, V1);
                Hexahedron[4] = GetSupport2(ObjectA, ObjectB, V2);                
            }
            else
            {
                Hexahedron[0] = Simplex.Vertex[Simplex.LastIndex];
                Hexahedron[1] = Simplex.Vertex[1];
                Hexahedron[2] = Simplex.Vertex[0];
                
                v3f N = Cross(Hexahedron[1].W-Hexahedron[0].W, Hexahedron[2].W-Hexahedron[0].W);
                Hexahedron[3] = GetSupport2(ObjectA, ObjectB,  N);
                Hexahedron[4] = GetSupport2(ObjectA, ObjectB, -N);
            }
            
            gjk_vertex* Tetrahedron0[4] = { &Hexahedron[0], &Hexahedron[1], &Hexahedron[2], &Hexahedron[4] };
            gjk_vertex* Tetrahedron1[4] = { &Hexahedron[0], &Hexahedron[1], &Hexahedron[3], &Hexahedron[4] };
            if(Determinant(Tetrahedron0[0]->W - Tetrahedron0[3]->W, Tetrahedron0[1]->W - Tetrahedron0[3]->W, Tetrahedron0[2]->W - Tetrahedron0[3]->W) < 0)        
                SWAP(Tetrahedron0[0], Tetrahedron0[1]);
            
            if(Determinant(Tetrahedron1[0]->W - Tetrahedron1[3]->W, Tetrahedron1[1]->W - Tetrahedron1[3]->W, Tetrahedron1[2]->W - Tetrahedron1[3]->W) < 0)        
                SWAP(Tetrahedron1[0], Tetrahedron1[1]);            
            
            b32 Contains = ContainsOrigin(Tetrahedron0);
            if(!Contains)
            {
                Contains = ContainsOrigin(Tetrahedron1);                
                if(!Contains)
                {
                    Result.Status = EPA_STATUS_INVALID_TETRAHEDRON;
                    return Result;                
                }
                
                Tetrahedron[0] = *Tetrahedron1[0];
                Tetrahedron[1] = *Tetrahedron1[1];
                Tetrahedron[2] = *Tetrahedron1[2];
                Tetrahedron[3] = *Tetrahedron1[3];
            }
            else
            {
                Tetrahedron[0] = *Tetrahedron0[0];
                Tetrahedron[1] = *Tetrahedron0[1];
                Tetrahedron[2] = *Tetrahedron0[2];
                Tetrahedron[3] = *Tetrahedron0[3];
            }                        
        } break;
        
        case 4:
        {
            Tetrahedron[0] = Simplex.Vertex[Simplex.LastIndex];            
            Tetrahedron[1] = Simplex.Vertex[2];            
            Tetrahedron[2] = Simplex.Vertex[1];
            Tetrahedron[3] = Simplex.Vertex[0];
        } break;
        
        INVALID_DEFAULT_CASE;
    }
    
    epa_context Context = {};
    Result = Context.Evaluate(ObjectA, ObjectB, Tetrahedron);
    
    return Result;
}