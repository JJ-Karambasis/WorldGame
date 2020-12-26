closest_points ClosestPoints_LineSegments(ak_v3f* A, ak_v3f* B)
{
    closest_points Result = {};
    ak_v3f D1 = A[1]-A[0];
    ak_v3f D2 = B[1]-B[0];
    ak_v3f R = A[0]-B[0];
    
    ak_f32 a = AK_SqrMagnitude(D1);
    ak_f32 e = AK_SqrMagnitude(D2);
    ak_f32 f = AK_Dot(D2, R);
    
    if(AK_EqualZeroEps(a) && AK_EqualZeroEps(e))
    {
        Result.PointA = A[0];
        Result.PointB = B[0];
        return Result;
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
    
    Result.PointA = A[0] + D1*s;
    Result.PointB = B[0] + D2*t;
    
    return Result;
}

closest_points ClosestPoints_PointLineSegment(ak_v3f P, ak_v3f* LineSegment)
{
    ak_v3f AB = LineSegment[1]-LineSegment[0];    
    ak_f32 t = AK_Saturate(AK_Dot(P - LineSegment[0], AB) / AK_SqrMagnitude(AB));
    
    closest_points Result;
    Result.PointA = P;
    Result.PointB = LineSegment[0] + t*AB;
    return Result;
}