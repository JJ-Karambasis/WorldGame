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