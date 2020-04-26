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
    f32 Result;
#if MSVC_COMPILER        
    Result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(Value)));
#else    
    Result = 1.0f/Sqrt(Value);    
#endif
    
    ASSERT(Result != INFINITY);
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

inline v2i V2i(i32 x, i32 y)
{
    v2i Result = {x, y};
    return Result;
}

inline v2i V2i(f32 x, f32 y)
{
    v2i Result = {(i32)x, (i32)y};
    return Result;
}

inline v2i operator+(v2i Left, i32 Right)
{
    v2i Result = {Left.x+Right, Left.y+Right};
    return Result;
}

inline v2i operator+(v2i Left, v2i Right)
{
    v2i Result = {Left.x+Right.x, Left.y+Right.y};
    return Result;
}

inline v2i& 
operator+=(v2i& Left, i32 Right)
{
    Left = Left + Right;
    return Left;
}

inline v2i 
operator-(v2i Left, i32 Right)
{
    v2i Result = {Left.x-Right, Left.y-Right};
    return Result;
}

inline v2i& 
operator-=(v2i& Left, i32 Right)
{
    Left = Left - Right;
    return Left;
}

inline b32 operator!=(v2i Left, v2i Right)
{
    b32 Result = (Left.x != Right.x) || (Left.y != Right.y);
    return Result;
}

inline b32 operator!=(v2i Left, i32 Right)
{
    b32 Result = (Left.x != Right) || (Left.y != Right);
    return Result;
}

inline b32 operator==(v2i Left, v2i Right)
{
    b32 Result = (Left.x == Right.x) && (Left.y == Right.y);
    return Result;
}

inline b32 operator<(v2i Left, v2i Right)
{
    b32 Result = (Left.x < Right.x) && (Left.y < Right.y);
    return Result;
}

inline b32 operator>=(v2i Left, i32 Right)
{
    b32 Result = (Left.x >= Right) && (Left.y >= Right);
    return Result;
}

inline v2i MinimumV2(v2i Left, v2i Right)
{
    v2i Result;
    Result.x = MinimumI32(Left.x, Right.x);
    Result.y = MinimumI32(Left.y, Right.y);
    return Result;
}

inline v2i MaximumV2(v2i Left, v2i Right)
{
    v2i Result;
    Result.x = MaximumI32(Left.x, Right.x);
    Result.y = MaximumI32(Left.y, Right.y);
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

inline v2i Round2i(v2f V)
{
    v2i Result = {RoundI32(V.x), RoundI32(V.y)};
    return Result;
}

inline v2f RoundPrecisionMag2f(v2f V, u32 X)
{
    v2f Result = {RoundPrecisionMag(V.x, X), RoundPrecisionMag(V.y, X)};
    return Result;
}

inline v2f RoundPrecision(v2f V, u32 Precision)
{
    u32 X = GetPrecision(Precision);
    v2f Result = RoundPrecisionMag2f(V, X);
    return Result;
}

inline v2f
InvalidV2()
{
    v2f Result = {INFINITY, INFINITY};
    return Result;
}

inline v2f 
V2()
{
    v2f Result = {};
    return Result;
}

inline v2f 
V2(i32 x, i32 y)
{
    v2f Result = {(f32)x, (f32)y};
    return Result;
}

inline v2f 
V2(v2i V)
{
    v2f Result = V2(V.x, V.y);
    return Result;
}

inline v2f 
V2(f32 x, f32 y)
{
    v2f Result = {x, y};
    return Result;
}

inline v2f
operator+(v2f Left, f32 Right)
{
    v2f Result = {Left.x+Right, Left.y+Right};
    return Result;
}

inline v2f
operator+(v2f Left, v2f Right)
{
    v2f Result = {Left.x+Right.x, Left.y+Right.y};
    return Result;
}

inline v2f& 
operator+=(v2f& Left, f32 Right)
{
    Left = Left + Right;
    return Left;
}

inline v2f&
operator+=(v2f& Left, v2f Right)
{
    Left = Left + Right;
    return Left;
}

inline v2f
operator-(f32 Left, v2f Right)
{
    v2f Result = {Left-Right.x, Left-Right.y};
    return Result;
}

inline v2f
operator-(v2f Left, f32 Right)
{
    v2f Result = {Left.x-Right, Left.y-Right};
    return Result;
}

inline v2f
operator-(v2f Left, v2f Right)
{
    v2f Result = {Left.x-Right.x, Left.y-Right.y};
    return Result;
}

inline v2f& 
operator-=(v2f& Left, f32 Right)
{
    Left = Left - Right;
    return Left;
}

inline v2f&
operator-=(v2f& Left, v2f Right)
{
    Left = Left - Right;
    return Left;
}

inline v2f 
operator*(v2f Left, f32 Right)
{
    v2f Result = {Left.x*Right, Left.y*Right};
    return Result;
}

inline v2f 
operator*(f32 Left, v2f Right)
{
    v2f Result = Right*Left;
    return Result;
}

inline v2f
operator*(v2i Left, f32 Right)
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

inline v2f 
operator*(v2f Left, v2f Right)
{
    v2f Result = {Left.x*Right.x, Left.y*Right.y};
    return Result;
}

inline v2f&
operator*=(v2f& Left, v2f Right)
{
    Left = Left * Right;
    return Left;
}

inline v2f
operator/(v2f V, f32 Right)
{
    v2f Result = {V.x/Right, V.y/Right};
    return Result;
}

inline v2f
operator/(f32 Left, v2f Right)
{
    v2f Result = {Left/Right.x, Left/Right.y};
    return Result;
}

inline v2f
Negate(v2f V)
{
    v2f Result = {-V.x, -V.y};
    return Result;
}

inline v2f
operator-(v2f V)
{
    v2f Result = Negate(V);
    return Result;
}

inline b32 operator!=(v2f Left, f32 Right)
{
    b32 Result = (Left.x != Right) || (Left.y != Right);
    return Result;
}

inline b32 operator!=(v2f Left, v2f Right)
{
    b32 Result = (Left.x != Right.x) || (Left.y != Right.y);
    return Result;
}

inline b32 operator==(v2f Left, v2f Right)
{
    b32 Result = (Left.x == Right.x) && (Left.y == Right.y);
    return Result;
}

inline b32 operator<(v2f Left, v2f Right)
{
    b32 Result = (Left.x < Right.x) && (Left.y < Right.y);
    return Result;
}

inline b32 operator>=(v2f Left, f32 Right)
{
    b32 Result = (Left.x >= Right) && (Left.y >= Right);
    return Result;
}

inline v2f Abs2f(v2f V)
{
    v2f Result = {Abs(V.x), Abs(V.y)};
    return Result;
}

inline f32
Dot(v2f Left, v2f Right)
{
    f32 Result = Left.x*Right.x + Left.y*Right.y;
    return Result;
}

inline f32
DotEpsilon(v2f Left, v2f Right)
{
    f32 Result = 2.0f * FLT_EPSILON * Dot(Abs2f(Left), Abs2f(Right));
    return Result;
}

inline f32 
SquareMagnitude(v2f V)
{
    f32 Result = Dot(V, V);
    return Result;
}

inline f32
Magnitude(v2f V)
{
    f32 Result = Sqrt(SquareMagnitude(V));
    return Result;
}

inline f32
InverseMagnitude(v2f V)
{
    f32 Result = RSqrt(SquareMagnitude(V));
    return Result;
}

inline v2f
Normalize(v2f V)
{
    f32 InvMag = InverseMagnitude(V);
    v2f Result = InvMag*V;
    return Result;
}

inline v2f
Perp(v2f V)
{
    v2f Result = {V.y, -V.x};
    return Result;
}

inline v2f
PerpCCW(v2f V)
{
    v2f Result = {-V.y, V.x};
    return Result;
}

inline f32
Cross2D(v2f Left, v2f Right)
{
    f32 Result = Left.y*Right.x - Left.x*Right.y;
    return Result;
}

inline v2i 
FloorV2(v2f Value)
{
    v2i Result;
    Result.x = FloorI32(Value.x);
    Result.y = FloorI32(Value.y);
    return Result;
}

inline b32
AreEqual(v2f P0, v2f P1, f32 Epsilon)
{
    b32 Result = SquareMagnitude(P0-P1) < Epsilon;
    return Result;
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
V3()
{
    v3f Result = {};
    return Result;
}

inline v3f
V3(f32 Value)
{
    v3f Result = {Value, Value, Value};
    return Result;
}

inline v3f 
V3(v2f v)
{
    v3f Result = {v.x, v.y};
    return Result;
}

inline v3f 
V3(v2f v, f32 z)
{
    v3f Result = {v.x, v.y, z};
    return Result;
}

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
V3(f32* V)
{
    v3f Result = V3(V[0], V[1], V[2]);
    return Result;
}

inline v3f
InvalidV3()
{
    v3f Result = {INFINITY, INFINITY, INFINITY};
    return Result;
}

inline b32 
IsInvalidV3(v3f V)
{
    b32 Result = ((V.x == INFINITY) || (V.y == INFINITY) || (V.z == INFINITY));
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

inline v3f&
operator-=(v3f& Left, v3f Right)
{
    Left = Left - Right;
    return Left;
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

inline v3f 
operator*(v3f Left, v3f Right)
{
    v3f Result;
    Result.x = Left.x * Right.x;
    Result.y = Left.y * Right.y;
    Result.z = Left.z * Right.z;
    return Result;
}

inline v3f&
operator*=(v3f& Left, v3f Right)
{
    Left = Left * Right;
    return Left;
}

inline v3f 
operator/(v3f Left, v3f Right)
{
    v3f Result;
    Result.x = Left.x / Right.x;
    Result.y = Left.y / Right.y;
    Result.z = Left.z / Right.z;
    return Result;
}

inline v3f 
operator-(v3f V)
{
    v3f Result = {-V.x, -V.y, -V.z};
    return Result;
}

inline b32 operator!=(v3f Left, v3f Right)
{
    b32 Result = (Left.x != Right.x) || (Left.y != Right.y) || (Left.z != Right.z);
    return Result;
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
            v2f xy;
            v2f zw;
        };
        
        struct
        {
            v3f xyz;
            f32 __unused_0__;
        };
        
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
V4(v2f xy, f32 z, f32 w)
{
    v4f Result;
    Result.xy = xy;    
    Result.z = z;
    Result.w = w;
    return Result;
}

inline v4f 
V4(f32* xyz, f32 w)
{
    v4f Result = V4(xyz[0], xyz[1], xyz[2], w);    
    return Result;
}

inline v4f 
V4(v3f xyz, f32 w)
{
    v4f Result;
    Result.xyz = xyz;    
    Result.w = w;
    return Result;
}

inline f32
Dot(v4f Left, v4f Right)
{
    f32 Result = Left.x*Right.x + Left.y*Right.y + Left.z*Right.z + Left.w*Right.w;
    return Result;
}

inline v4f 
RGBA(f32 r, f32 g, f32 b, f32 a)
{
    v4f Result = V4(r, g, b, a);
    return Result;
}

inline v4f White()
{
    v4f Result = RGBA(1.0f, 1.0f, 1.0f, 1.0f);
    return Result;
}

global v4f Global_Red = RGBA(1.0f, 0.0f, 0.0f, 1.0f);
inline v4f Red()
{
    return Global_Red;
}

inline v4f Green()
{
    v4f Result = RGBA(0.0f, 1.0f, 0.0f, 1.0f);
    return Result;
}

global v4f Global_Blue = RGBA(0.0f, 0.0f, 1.0f, 0.0f);
inline v4f Blue()
{    
    return Global_Blue;
}

inline v4f Yellow()
{
    v4f Result = RGBA(1.0f, 1.0f, 0.0f, 1.0f);
    return Result;
}

inline v4f Black()
{
    v4f Result = RGBA(0.0f, 0.0f, 0.0f, 1.0f);
    return Result;
}

typedef v4f c4;

struct m3
{
    union
    {
        f32 M[9];
        v3f Rows[3];
        struct
        {
            f32 m00, m01, m02;
            f32 m10, m11, m12;
            f32 m20, m21, m22;
            f32 m30, m31, m32;
        };
        
        struct
        {
            v3f Row0;
            v3f Row1;
            v3f Row2;            
        };
        
        struct
        {
            v3f XAxis;
            v3f YAxis;
            v3f ZAxis;            
        };
    };
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
M3(v3f XAxis, v3f YAxis, v3f ZAxis)
{
    m3 Result;
    Result.XAxis = XAxis;
    Result.YAxis = YAxis;
    Result.ZAxis = ZAxis;    
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
    Right = Transpose(Right);
    m3 Result;
    
    Result.m00 = Dot(Left.Rows[0], Right.Rows[0]);
    Result.m01 = Dot(Left.Rows[0], Right.Rows[1]);
    Result.m02 = Dot(Left.Rows[0], Right.Rows[2]);
    Result.m10 = Dot(Left.Rows[1], Right.Rows[0]);
    Result.m11 = Dot(Left.Rows[1], Right.Rows[1]);
    Result.m12 = Dot(Left.Rows[1], Right.Rows[2]);
    Result.m20 = Dot(Left.Rows[2], Right.Rows[0]);
    Result.m21 = Dot(Left.Rows[2], Right.Rows[1]);
    Result.m22 = Dot(Left.Rows[2], Right.Rows[2]);
    return Result;
}

inline m3& 
operator*=(m3& Left, m3 Right)
{
    Left = Left*Right;
    return Left;
}

struct m4
{
    union
    {
        f32 M[16];
        v4f Rows[4];
        struct
        {
            f32 m00, m01, m02, m03;
            f32 m10, m11, m12, m13;
            f32 m20, m21, m22, m23;
            f32 m30, m31, m32, m33;
        };
        
        struct
        {
            v4f Row0;
            v4f Row1;
            v4f Row2;
            v4f Row3;
        };
        
        struct
        {
            v4f XAxis;
            v4f YAxis;
            v4f ZAxis;
            v4f Translation;
        };
    };
};

inline m4
M4(f32 Diagonal)
{
    m4 Result = {};
    Result.m00 = Diagonal;
    Result.m11 = Diagonal;
    Result.m22 = Diagonal;
    Result.m33 = Diagonal;
    return Result;
}

inline m4
IdentityM4()
{
    m4 Result = M4(1.0f);
    return Result;
}

inline m4
Transpose(m4 M)
{
    m4 Result;
    Result.m00 = M.m00;
    Result.m01 = M.m10;
    Result.m02 = M.m20;
    Result.m03 = M.m30;
    Result.m10 = M.m01;
    Result.m11 = M.m11;
    Result.m12 = M.m21;
    Result.m13 = M.m31;
    Result.m20 = M.m02;
    Result.m21 = M.m12;
    Result.m22 = M.m22;
    Result.m23 = M.m32;
    Result.m30 = M.m03;
    Result.m31 = M.m13;
    Result.m32 = M.m23;
    Result.m33 = M.m33;
    return Result;
}

inline m4 
TransformM4(v3f Position, v3f Scale)
{
    m4 Result;
    Result.XAxis = V4(Scale.x, 0.0f,    0.0f,    0.0f);
    Result.YAxis = V4(0.0f,    Scale.y, 0.0f,    0.0f);
    Result.ZAxis = V4(0.0f,    0.0f,    Scale.z, 0.0f);
    Result.Translation = V4(Position, 1.0f);
    return Result;
}

inline m4
TransformM4(v3f Position, m3 Orientation)
{
    m4 Result;
    Result.XAxis       = V4(Orientation.XAxis, 0.0f);
    Result.YAxis       = V4(Orientation.YAxis, 0.0f);
    Result.ZAxis       = V4(Orientation.ZAxis, 0.0f);
    Result.Translation = V4(Position, 1.0f);
    return Result;
}

inline m4 
InverseTransformM4(v3f Position, m3 Orientation)
{
    f32 tx = -Dot(Position, Orientation.XAxis);
    f32 ty = -Dot(Position, Orientation.YAxis);
    f32 tz = -Dot(Position, Orientation.ZAxis);
    
    m4 Result = 
    {
        Orientation.m00, Orientation.m10, Orientation.m20, 0.0f,
        Orientation.m01, Orientation.m11, Orientation.m21, 0.0f,
        Orientation.m02, Orientation.m12, Orientation.m22, 0.0f,
        tx,              ty,              tz,              1.0f 
    };
    
    return Result;
}

inline m4
InverseTransformM4(m4 M)
{
    f32 sx = 1.0f/SquareMagnitude(M.XAxis.xyz);
    f32 sy = 1.0f/SquareMagnitude(M.YAxis.xyz);
    f32 sz = 1.0f/SquareMagnitude(M.ZAxis.xyz);
    
    v3f x = sx*M.XAxis.xyz;
    v3f y = sy*M.YAxis.xyz;
    v3f z = sz*M.ZAxis.xyz;
    
    f32 tx = -Dot(M.Translation.xyz, x);
    f32 ty = -Dot(M.Translation.xyz, y);
    f32 tz = -Dot(M.Translation.xyz, z);
    
    m4 Result = 
    {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        tx,  ty,  tz,  1
    };
    
    return Result;
}

inline m4
operator*(m4 Left, m4 Right)
{
    Right = Transpose(Right);
    m4 Result;
    
    Result.m00 = Dot(Left.Rows[0], Right.Rows[0]);
    Result.m01 = Dot(Left.Rows[0], Right.Rows[1]);
    Result.m02 = Dot(Left.Rows[0], Right.Rows[2]);
    Result.m03 = Dot(Left.Rows[0], Right.Rows[3]);
    
    Result.m10 = Dot(Left.Rows[1], Right.Rows[0]);
    Result.m11 = Dot(Left.Rows[1], Right.Rows[1]);
    Result.m12 = Dot(Left.Rows[1], Right.Rows[2]);
    Result.m13 = Dot(Left.Rows[1], Right.Rows[3]);
    
    Result.m20 = Dot(Left.Rows[2], Right.Rows[0]);
    Result.m21 = Dot(Left.Rows[2], Right.Rows[1]);
    Result.m22 = Dot(Left.Rows[2], Right.Rows[2]);
    Result.m23 = Dot(Left.Rows[2], Right.Rows[3]);
    
    Result.m30 = Dot(Left.Rows[3], Right.Rows[0]);
    Result.m31 = Dot(Left.Rows[3], Right.Rows[1]);
    Result.m32 = Dot(Left.Rows[3], Right.Rows[2]);
    Result.m33 = Dot(Left.Rows[3], Right.Rows[3]);
    return Result;
}

inline m4& 
operator*=(m4& Left, m4 Right)
{
    Left = Left*Right;
    return Left;
}

inline v4f
operator*(v4f Left, m4 Right)
{
    Right = Transpose(Right);
    v4f Result;
    Result.x = Dot(Left, Right.Rows[0]);
    Result.y = Dot(Left, Right.Rows[1]);
    Result.z = Dot(Left, Right.Rows[2]);
    Result.w = Dot(Left, Right.Rows[3]);
    return Result;
}

inline v4f&
operator*=(v4f& Left, m4 Right)
{    
    Left = Left*Right;
    return Left;
}

inline v4f
operator*(m4 Left, v4f Right)
{              
    v4f Result;
    Result.x = Dot(Left.Rows[0], Right);
    Result.y = Dot(Left.Rows[1], Right);
    Result.z = Dot(Left.Rows[2], Right);
    Result.w = Dot(Left.Rows[3], Right);
    return Result;
}

inline m4
PerspectiveM4(f32 FieldOfView, f32 AspectRatio, f32 Near, f32 Far)
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

inline m4
OrthographicM4(f32 Left, f32 Right, f32 Top, f32 Bottom, f32 Near, f32 Far)
{
    m4 Result = 
    {
        2.0f/(Right-Left),           0.0f,                       0.0f,                  0.0f,
        0.0f,                        2.0f/(Top-Bottom),          0.0f,                  0.0f, 
        0.0f,                        0.0f,                      -2.0f/(Far-Near),       0.0f, 
        -(Right+Left)/(Right-Left), -(Top+Bottom)/(Top-Bottom), -(Far+Near)/(Far-Near), 1.0f
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

inline quaternion 
Conjugate(quaternion Q)
{
    quaternion Result = Quaternion(-Q.v, Q.s);
    return Result;
}

inline quaternion 
IdentityQuaternion()
{
    quaternion Result;
    Result.v = {};
    Result.s = 1.0f;
    return Result;
}

inline quaternion 
EulerQuaternion(f32 Pitch, f32 Yaw, f32 Roll)
{
    f32 cp = Cos(Yaw*0.5f);
    f32 sp = Sin(Yaw*0.5f);
    f32 cy = Cos(Roll*0.5f);
    f32 sy = Sin(Roll*0.5f);  
    f32 cr = Cos(Pitch*0.5f);
    f32 sr = Sin(Pitch*0.5f);
    
    quaternion Result;
    Result.x = (cy*cp*sr) - (sy*sp*cr);
    Result.y = (sy*cp*sr) + (cy*sp*cr);
    Result.z = (sy*cp*cr) - (cy*sp*sr);
    Result.w = (cy*cp*cr) + (sy*sp*sr);
    return Result;
}

inline quaternion
RollQuaternion(quaternion Q)
{
    f32 Mag = Sqrt(Q.w*Q.w + Q.z*Q.z);    
    ASSERT(Abs(Mag) > 1e-6f); 
    Mag = 1.0f/Mag;
    
    quaternion Result = {};    
    Result.z = Q.z*Mag;
    Result.w = Q.w*Mag;
    return Result;
}

inline quaternion 
EulerQuaternion(v3f Euler)
{
    quaternion Result = EulerQuaternion(Euler.pitch, Euler.yaw, Euler.roll);
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
operator*(quaternion Left, f32 Right)
{
    quaternion Result = Quaternion(Left.v*Right, Left.s*Right);    
    return Result;
}

inline quaternion 
operator*(f32 Left, quaternion Right)
{
    quaternion Result = Quaternion(Left*Right.v, Left*Right.s);
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

quaternion RotQuat(v3f Axis, f32 Angle)
{
    quaternion Result;
    Result.v = Axis*Sin(Angle*0.5f);
    Result.s = Cos(Angle*0.5f);
    Result = Normalize(Result);
    return Result;
}

inline v3f 
Rotate(v3f V, quaternion Q)
{
    v3f Result = ((2.0f * Dot(Q.v, V) * Q.v)   + 
                  ((Square(Q.s) - SquareMagnitude(Q.v))*V) + 
                  (2.0f * Q.s * Cross(Q.v, V)));
    return Result;
}

struct sqt
{
    quaternion Orientation;
    v3f Position;
    v3f Scale;
};

inline sqt 
CreateSQT(v3f Position, v3f Scale, v3f Euler)
{
    sqt Result;
    Result.Position = Position;
    Result.Scale = Scale;
    Result.Orientation = EulerQuaternion(Euler);
    return Result;
}

inline m3 
ToMatrix3(quaternion Q)
{
    f32 qxqy = Q.x*Q.y;
    f32 qwqz = Q.w*Q.z;
    f32 qxqz = Q.x*Q.z;
    f32 qwqy = Q.w*Q.y;
    f32 qyqz = Q.y*Q.z;
    f32 qwqx = Q.w*Q.x;
    
    f32 qxqx = Square(Q.x);
    f32 qyqy = Square(Q.y);
    f32 qzqz = Square(Q.z);
    
    m3 Result = 
    {
        1 - 2*(qyqy+qzqz), 2*(qxqy+qwqz),     2*(qxqz-qwqy),   
        2*(qxqy-qwqz),     1 - 2*(qxqx+qzqz), 2*(qyqz+qwqx),   
        2*(qxqz+qwqy),     2*(qyqz-qwqx),     1 - 2*(qxqx+qyqy)
    };
    return Result;
}

inline m4
TransformM4(sqt SQT)
{
    m3 Orientation = ToMatrix3(SQT.Orientation);
    Orientation.XAxis *= SQT.Scale.x;
    Orientation.YAxis *= SQT.Scale.y;
    Orientation.ZAxis *= SQT.Scale.z;
    m4 Result = TransformM4(SQT.Position, Orientation);
    return Result;
}

inline v3f TransformV3(v3f Point, sqt Transform)
{
    v3f Result = Rotate(Point*Transform.Scale, Transform.Orientation) + Transform.Position;
    return Result;
}

inline v3f InverseTransformV3(v3f Point, sqt Transform)
{
    v3f Result = Rotate(Point-Transform.Position, Conjugate(Transform.Orientation)) / Transform.Scale;
    return Result;
}

inline v3f TransformV3(v3f Point, m4 Transform)
{
    v4f Result = V4(Point, 1.0f)*Transform;
    return Result.xyz;
}

#endif