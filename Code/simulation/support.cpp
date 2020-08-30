u32 ConvexHullSupport(convex_hull* ConvexHull, v3f Direction)
{               
    u32 BestVertexIndex = 0;
    half_vertex* BestVertex = ConvexHull->Vertices + BestVertexIndex;
    f32 BestDot = Dot(BestVertex->V, Direction);
    
    for(;;)
    {
        i32 E = BestVertex->Edge;
        i32 StartEdge = E;
        f32 OldBestDot = BestDot;
        
        do
        {
            ASSERT(E != -1);
            half_edge* Edge = ConvexHull->Edges + E;
            
            i32 VertexIndex = Edge->Vertex;
            half_vertex* Vertex = ConvexHull->Vertices + VertexIndex;
            
            f32 TempDot = Dot(Direction, Vertex->V);
            if(TempDot > BestDot)
            {
                BestVertexIndex = VertexIndex;
                BestVertex = Vertex;
                BestDot = TempDot;
            }
            
            E = ConvexHull->Edges[Edge->EdgePair].NextEdge;     
            if(E == -1)
                E = Edge->NextEdge;
            
        } while(StartEdge != E);     
        
        if(BestDot == OldBestDot)
            break;
    }
    
    return BestVertexIndex;
}

v3f ConvexHullSupportPoint(convex_hull* ConvexHull, sqt Transform, v3f D)
{
    quaternion InverseOrientation = Conjugate(Transform.Orientation);
    D = Rotate(D, InverseOrientation);        
    u32 VertexID = ConvexHullSupport(ConvexHull, D);        
    v3f Result = TransformV3(ConvexHull->Vertices[VertexID].V, Transform);
    return Result;
}

u32 LineSegmentSupport(v3f* LineSegment, v3f Direction)
{
    if(Dot(Direction, LineSegment[0]) < Dot(Direction, LineSegment[1]))
        return 1;
    return 0;
}

struct convex_hull_support
{
    convex_hull* Hull;
    sqt Transform;
    
    v3f Support(v3f D)
    {
        v3f Result = ConvexHullSupportPoint(Hull, Transform, D);        
        return Result;        
    }
};

struct moving_convex_hull_support
{
    convex_hull* Hull;
    sqt Transform;
    v3f Delta;
    
    v3f Support(v3f D)
    {        
        v3f Result = ConvexHullSupportPoint(Hull, Transform, D);        
        if(Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};

struct margin_convex_hull_support
{
    convex_hull* Hull;
    sqt Transform;
    f32 Margin;
    
    v3f Support(v3f D)
    {
        if(SquareMagnitude(D) < Square(FLT_EPSILON))
            D = V3(-1, -1, -1);
        D = Normalize(D);
        v3f Result = ConvexHullSupportPoint(Hull, Transform, D);
        Result += (Margin*D);
        return Result;
    }
};

struct line_segment_support
{
    union
    {
        struct
        {
            v3f P0;
            v3f P1;
        };
        v3f P[2];
    };
    
    v3f Support(v3f D)
    {
        v3f Result = P[LineSegmentSupport(P, D)];
        return Result;
    }
};

struct moving_line_segment_support
{
    union
    {
        struct
        {
            v3f P0; 
            v3f P1;
        };
        v3f P[2];
    };
    v3f Delta;
    
    v3f Support(v3f D)
    {
        v3f Result = P[LineSegmentSupport(P, D)];
        if(Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};

struct point_support
{
    v3f P;
    
    v3f Support(v3f D)
    {
        return P;
    }
};

struct moving_point_support
{
    v3f P;
    v3f Delta;
    
    v3f Support(v3f D)
    {
        v3f Result = P;
        if(Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};