#ifndef MATH_H
#define MATH_H

#define PI 3.14159265359f
#define INV_PI 0.31830988618f

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

struct v4f
{
    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        
        struct
        {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
    };
};

inline v4f V4(f32 x, f32 y, f32 z, f32 w)
{
    v4f Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
    return Result;
}

inline v4f RGBA(f32 r, f32 g, f32 b, f32 a)
{
    v4f Result = V4(r, g, b, a);
    return Result;
}

typedef v4f c4;

struct m4
{
    union
    {
        f32 M[16];
        struct
        {
            f32 m00, m01, m02, m03;
            f32 m10, m11, m12, m13;
            f32 m20, m21, m22, m23;
            f32 m30, m31, m32, m33;
        };
        
        struct
        {
            v4f Column0;
            v4f Column1;
            v4f Column2;
            v4f Column3;
        };
        
        struct
        {
            v4f XAxis;
            v4f YAxis;
            v4f ZAxis;
            v4f Translation;
        };
    };
    
    inline f32& operator[](u32 Index)
    {
        ASSERT(Index < 16);
        return M[Index];
    }
};

inline m4 M4(f32 Diagonal)
{
    m4 Result = {};
    Result.m00 = Diagonal;
    Result.m11 = Diagonal;
    Result.m22 = Diagonal;
    Result.m33 = Diagonal;
    return Result;
}

inline m4 IdentityM4()
{
    m4 Result = M4(1.0f);    
    return Result;
}

inline m4 TranslationM4(f32 x, f32 y, f32 z)
{
    m4 Result = IdentityM4();
    Result.Translation = V4(x, y, z, 1.0f);
    return Result;
}

inline m4 PerspectiveM4(f32 FieldOfView, f32 AspectRatio, f32 Near, f32 Far)
{
    f32 c = 1.0f/Tan(FieldOfView*0.5f);
    f32 a = AspectRatio;
    
    m4 Result = 
    {
        c/a,  0.0f,   0.0f,                       0.0f,
        0.0f, c,      0.0f,                       0.0f,
        0.0f, 0.0f, -(Far+Near)/(Far-Near),      -1.0f,
        0.0f, 0.0f, -(2.0f*Far*Near)/(Far-Near),  0.0f
    };
    
    return Result;    
}

#endif