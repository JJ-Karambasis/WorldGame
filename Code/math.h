#ifndef MATH_H
#define MATH_H

inline f32 Sin(f32 Rads)
{
    f32 Result = sinf(Rads);
    return Result;
}

inline f32 Cos(f32 Rads)
{
    f32 Result = cosf(Rads);
    return Result;
} 

inline f32 Tan(f32 Rads)
{
    f32 Result = tanf(Rads);
    return Result;
}

inline f32 ACos(f32 Rads)
{
    f32 Result = acosf(Rads);
    return Result;
}

inline f32 Exp(f32 Value)
{
    f32 Result = expf(Value);
    return Result;
}

inline f32 Pow(f32 Value, f32 Exp)
{
    f32 Result = powf(Value, Exp);
    return Result;
}

inline f32 Sqrt(f32 Value)
{
    f32 Result;
#if MSVC_COMPILER
    Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Value)));    
#else
    Result = sqrtf(Value);    
#endif
    
    return Result;
}

inline f32 RSqrt(f32 Value)
{
    //IMPORTANT(JJ): Probably don't want the intrinsic to handle the divide by 0 but I want to determine where these happen first and what the intrinsic will return (NAN or infinity?)
    if(Abs(Value) < 0.00001f)
        ASSERT(false);
    
    f32 Result;
#if MSVC_COMPILER        
    Result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(Value)));
#else    
    Result = 1.0f/Sqrt(Value);    
#endif
    return Result;
}

struct v2i
{
    union
    {        
        struct
        {
            i32 x;
            i32 y;
        };
        
        struct
        {
            i32 width;
            i32 height;
        };
    };
};

v2i V2i(i32 x, i32 y)
{
    v2i Result = {x, y};
    return Result;
}

inline b32 operator!=(v2i Left, v2i Right)
{
    b32 Result = (Left.x != Right.x) || (Left.y != Right.y);
    return Result;
}

inline b32 operator==(v2i Left, v2i Right)
{
    b32 Result = (Left.x == Right.x) || (Left.y == Right.y);
    return Result;
}

#endif