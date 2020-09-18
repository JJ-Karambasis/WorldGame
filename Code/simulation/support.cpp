ak_u32 ConvexHullSupport(convex_hull* ConvexHull, ak_v3f Direction)
{               
    ak_u32 BestVertexIndex = 0;
    half_vertex* BestVertex = ConvexHull->Vertices + BestVertexIndex;
    ak_f32 BestDot = AK_Dot(BestVertex->V, Direction);
    
    for(;;)
    {
        ak_i32 E = BestVertex->Edge;
        ak_i32 StartEdge = E;
        ak_f32 OldBestDot = BestDot;
        
        do
        {
            AK_Assert(E != -1, "Invalid half edge");
            half_edge* Edge = ConvexHull->Edges + E;
            
            ak_i32 VertexIndex = Edge->Vertex;
            half_vertex* Vertex = ConvexHull->Vertices + VertexIndex;
            
            ak_f32 TempDot = AK_Dot(Direction, Vertex->V);
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

ak_v3f ConvexHullSupportPoint(convex_hull* ConvexHull, ak_sqtf Transform, ak_v3f D)
{
    ak_quatf InverseOrientation = AK_Conjugate(Transform.Orientation);
    D = AK_Rotate(D, InverseOrientation);        
    ak_u32 VertexID = ConvexHullSupport(ConvexHull, D);        
    ak_v3f Result = AK_Transform(ConvexHull->Vertices[VertexID].V, Transform);
    return Result;
}

ak_u32 LineSegmentSupport(ak_v3f* LineSegment, ak_v3f Direction)
{
    if(AK_Dot(Direction, LineSegment[0]) < AK_Dot(Direction, LineSegment[1]))
        return 1;
    return 0;
}

struct convex_hull_support
{
    convex_hull* Hull;
    ak_sqtf Transform;
    
    ak_v3f Support(ak_v3f D)
    {
        ak_v3f Result = ConvexHullSupportPoint(Hull, Transform, D);        
        return Result;        
    }
};

struct moving_convex_hull_support
{
    convex_hull* Hull;
    ak_sqtf Transform;
    ak_v3f Delta;
    
    ak_v3f Support(ak_v3f D)
    {        
        ak_v3f Result = ConvexHullSupportPoint(Hull, Transform, D);        
        if(AK_Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};

struct margin_convex_hull_support
{
    convex_hull* Hull;
    ak_sqtf Transform;
    ak_f32 Margin;
    
    ak_v3f Support(ak_v3f D)
    {
        if(AK_SqrMagnitude(D) < AK_Square(FLT_EPSILON))
            D = AK_V3f(-1, -1, -1);
        D = AK_Normalize(D);
        ak_v3f Result = ConvexHullSupportPoint(Hull, Transform, D);
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
            ak_v3f P0;
            ak_v3f P1;
        };
        ak_v3f P[2];
    };
    
    ak_v3f Support(ak_v3f D)
    {
        ak_v3f Result = P[LineSegmentSupport(P, D)];
        return Result;
    }
};

struct moving_line_segment_support
{
    union
    {
        struct
        {
            ak_v3f P0; 
            ak_v3f P1;
        };
        ak_v3f P[2];
    };
    ak_v3f Delta;
    
    ak_v3f Support(ak_v3f D)
    {
        ak_v3f Result = P[LineSegmentSupport(P, D)];
        if(AK_Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};

struct point_support
{
    ak_v3f P;
    
    ak_v3f Support(ak_v3f D)
    {
        return P;
    }
};

struct moving_point_support
{
    ak_v3f P;
    ak_v3f Delta;
    
    ak_v3f Support(ak_v3f D)
    {
        ak_v3f Result = P;
        if(AK_Dot(D, Delta) > 0) Result += Delta;
        return Result;
    }
};