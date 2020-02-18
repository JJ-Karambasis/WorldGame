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

struct v2f
{
    union
    {
        struct
        {
            f32 x;
            f32 y;           
        };
        
        struct
        {
            f32 width;
            f32 height;
        };
    };
};

v2f V2(f32 x, f32 y)
{
    v2f Result = {x, y};
    return Result;
}

inline v2f 
operator*(v2f Left, f32 Right)
{
    v2f Result = {Left.x*Right, Left.y*Right};
    return Result;
}

inline v2f&
operator*=(v2f& Left, f32 Right)
{
    Left = Left * Right;
    return Left;
}

struct v3f
{
    union
    {
        struct
        {
            v2f xy;
            f32 __unused_0__;
        };
        
        struct 
        {
            f32 x;
            f32 y;
            f32 z;            
        };
        
        struct 
        {
            f32 r;
            f32 g;
            f32 b;            
        };
        
        struct
        {
            f32 pitch;
            f32 yaw;
            f32 roll;
        };
    };
};

inline v3f 
V3(f32 x, f32 y, f32 z)
{
    v3f Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    return Result;
}

inline v3f
operator+(v3f Left, v3f Right)
{
    v3f Result;
    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;
    Result.z = Left.z + Right.z;
    return Result;
}

inline v3f&
operator+=(v3f& Left, v3f Right)
{
    Left = Left + Right;    
    return Left;
}

inline v3f
operator-(v3f Left, v3f Right)
{
    v3f Result;
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;
    Result.z = Left.z - Right.z;
    return Result;
}

inline v3f 
operator*(v3f Left, f32 Right)
{
    v3f Result;
    Result.x = Left.x * Right;
    Result.y = Left.y * Right;
    Result.z = Left.z * Right;
    return Result;
}

inline v3f 
operator*(f32 Left, v3f Right)
{
    v3f Result = Right*Left;    
    return Result;
}

inline v3f&
operator*=(v3f& Left, f32 Right)
{
    Left = Left * Right;
    return Left;
}

inline f32
Dot(v3f Left, v3f Right)
{
    f32 Result = Left.x*Right.x + Left.y*Right.y + Left.z*Right.z;
    return Result;
}

inline v3f 
Cross(v3f Left, v3f Right)
{
    v3f Result = {Left.y*Right.z - Left.z*Right.y, Left.z*Right.x - Left.x*Right.z, Left.x*Right.y - Left.y*Right.x};
    return Result;
}

inline f32 
SquareMagnitude(v3f V)
{
    f32 Result = Dot(V, V);
    return Result;
}

inline f32
Magnitude(v3f V)
{
    f32 Result = Sqrt(SquareMagnitude(V));
    return Result;
}

inline f32
InverseMagnitude(v3f V)
{
    f32 Result = RSqrt(SquareMagnitude(V));
    return Result;
}

inline v3f
Normalize(v3f V)
{
    f32 InvMag = InverseMagnitude(V);
    v3f Result = InvMag*V;
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

inline v4f 
V4(f32 x, f32 y, f32 z, f32 w)
{
    v4f Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
    return Result;
}

inline v4f 
RGBA(f32 r, f32 g, f32 b, f32 a)
{
    v4f Result = V4(r, g, b, a);
    return Result;
}

typedef v4f c4;

struct m3
{
    union
    {
        f32 M[9];
        struct
        {
            f32 m00, m01, m02;
            f32 m10, m11, m12;
            f32 m20, m21, m22;
        };
        
        struct
        {
            v3f Column0;
            v3f Column1;
            v3f Column2;
        };
        
        struct 
        {
            v3f XAxis;
            v3f YAxis;
            v3f ZAxis;
        };
    };
    
    inline f32& operator[](u32 Index)
    {
        ASSERT(Index < 9);
        return M[Index];
    }
};

inline m3
M3(f32 Diagonal)
{
    m3 Result = {};
    Result.m00 = Diagonal;
    Result.m11 = Diagonal;
    Result.m22 = Diagonal;
    return Result;
}

inline m3
IdentityM3()
{
    m3 Result = M3(1.0f);
    return Result;
}

inline m3 
Transpose(m3 M)
{
    m3 Result;
    Result.m00 = M.m00;
    Result.m01 = M.m10;
    Result.m02 = M.m20;
    Result.m10 = M.m01;
    Result.m11 = M.m11;
    Result.m12 = M.m21;
    Result.m20 = M.m02;
    Result.m21 = M.m12;
    Result.m22 = M.m22;
    return Result;
}

inline m3 
operator*(m3 Left, m3 Right)
{
    m3 Result;
    Right = Transpose(Right);   
    
    Result.m00 = Dot(Left.Column0, Right.Column0);
    Result.m01 = Dot(Left.Column0, Right.Column1);
    Result.m02 = Dot(Left.Column0, Right.Column2);
    Result.m10 = Dot(Left.Column1, Right.Column0);
    Result.m11 = Dot(Left.Column1, Right.Column1);
    Result.m12 = Dot(Left.Column1, Right.Column2);
    Result.m20 = Dot(Left.Column2, Right.Column0);
    Result.m21 = Dot(Left.Column2, Right.Column1);
    Result.m22 = Dot(Left.Column2, Right.Column2);
    
    return Result;
}

inline m3& 
operator*=(m3& Left, m3 Right)
{
    Left = Left * Right;
    return Left;
}

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

inline m4 InverseTransformM4(v3f Pos, m3 Orient)
{    
    m4 Result = 
    {
        Orient.m00               , Orient.m10               , Orient.m20                 , 0.0f,
        Orient.m01               , Orient.m11               , Orient.m21                 , 0.0f,
        Orient.m02               , Orient.m12               , Orient.m22                 , 0.0f,
        -Dot(Pos, Orient.Column0), -Dot(Pos, Orient.Column1), -Dot(Pos, Orient.Column2)  , 1.0f
    };    
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

struct quaternion
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
            v3f v;
            f32 s;
        };
    };
};

quaternion Quaternion(f32 x, f32 y, f32 z, f32 w)
{
    quaternion Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
    return Result;
}

quaternion Quaternion(v3f v, f32 s)
{
    quaternion Result = Quaternion(v.x, v.y, v.z, s);
    return Result;
}

quaternion IdentityQuaternion()
{
    quaternion Result;
    Result.v = {};
    Result.s = 1.0f;
    return Result;
}

quaternion RotQuat(v3f Axis, f32 Angle)
{
    quaternion Result;
    Result.v = Axis*Sin(Angle*0.5f);
    Result.s = Cos(Angle*0.5f);
    return Result;
}

inline quaternion 
operator*(f32 Left, quaternion Right)
{
    quaternion Result = Quaternion(Left*Right.v, Left*Right.s);
    return Result;
}

inline quaternion 
operator*(quaternion Left, f32 Right)
{
    quaternion Result = Quaternion(Left.v*Right, Left.s*Right);    
    return Result;
}

inline f32 
SquareMagnitude(quaternion Q)
{
    f32 Result = SquareMagnitude(Q.v) + Square(Q.s);
    return Result;
}

inline f32
Magnitude(quaternion Q)
{
    f32 Result = Sqrt(SquareMagnitude(Q));
    return Result;
}

inline f32
InverseMagnitude(quaternion Q)
{
    f32 Result = RSqrt(SquareMagnitude(Q));
    return Result;
}

inline quaternion
Normalize(quaternion Q)
{
    f32 InvMag = InverseMagnitude(Q);
    quaternion Result = InvMag*Q;
    return Result;
}

inline quaternion 
operator*(quaternion Left, quaternion Right)
{
    quaternion Result = Quaternion(Cross(Left.v, Right.v) + Right.s*Left.v + Right.v*Left.s, 
                                   Left.s*Right.s - Dot(Left.v, Right.v));
    return Result;
}

inline m3 ToMatrix3(quaternion Q)
{
    f32 qxqy = Q.x*Q.y;
    f32 qwqz = Q.w*Q.z;
    f32 qxqz = Q.x*Q.z;
    f32 qwqy = Q.w*Q.y;
    f32 qyqz = Q.y*Q.z;
    f32 qwqx = Q.w*Q.x;
    
    f32 xSqr = Q.x*Q.x;
    f32 ySqr = Q.y*Q.y;
    f32 zSqr = Q.z*Q.z;
    
    m3 Result;
    Result[0] = 1.0f - 2.0f*(ySqr + zSqr);
    Result[1] = 2.0f*(qxqy + qwqz);
    Result[2] = 2.0f*(qxqz - qwqy);
    Result[3] = 2.0f*(qxqy - qwqz);
    Result[4] = 1.0f - 2.0f*(xSqr + zSqr);
    Result[5] = 2.0f*(qyqz + qwqx);
    Result[6] = 2.0f*(qxqz + qwqy);
    Result[7] = 2.0f*(qyqz - qwqx);
    Result[8] = 1.0f - 2.0f*(xSqr + ySqr);    
    return Result;
}

struct sqt
{
    quaternion Orientation;
    v3f Position;
    v3f Scale;
};

#endif