#ifndef COMMON_H
#define COMMON_H

#if defined(_WIN32)
#undef OS_WINDOWS
#define OS_WINDOWS 
#define OS_PATH_DELIMITER '\\'
#include <Windows.h>
#endif

#if defined(_MSC_VER)
#undef MSVC_COMPILER
#define MSVC_COMPILER 1
#include <xmmintrin.h>
#include <smmintrin.h>
#endif

#if MSVC_COMPILER
#define EXPORT __declspec(dllexport)
#endif


#define CURRENT_FILENAME (FindLastChar(__FILE__, OS_PATH_DELIMITER) ? FindLastChar(__FILE__, OS_PATH_DELIMITER)+1 : __FILE__)

#define ARRAYCOUNT(x) (sizeof(x) / sizeof(x[0]))

#define BYTE(x) (x)
#define KILOBYTE(x) (BYTE(x)*1024LL)
#define MEGABYTE(x) (KILOBYTE(x)*1024LL)
#define GIGABYTE(x) (MEGABYTE(x)*1024LL)
#define TERABYTE(x) (GIGABYTE(x)*1024LL)

#define SQR(x) ((x)*(x))

#define BIT_SET(x) (1 << x)

#define SIGN(x) (((x) < 0) ? -1.0f : 1.0f)

#define SWAP(x, y) do { auto temp = x; x = y; y = temp; } while(0)

#include <stdarg.h>

#define RIFF(x, y, z, w) (x | (y << 8) | (z << 16) | (w << 24))

//IMPORTANT(JJ): This macro crap may not compile for non MSVC compilers... sooo
//TODO(JJ): Make these work for non macro compilers (or just confirm these already work)
#define EXPAND( x ) x

#define CAT( A, B ) A ## B
#define SELECT( NAME, NUM ) CAT( NAME ## _, NUM )

#define GET_COUNT( _1, _2, _3, _4, _5, _6, COUNT, ... ) COUNT
#define VA_SIZE( ... ) EXPAND(GET_COUNT( __VA_ARGS__, 6, 5, 4, 3, 2, 1 ))

#define VA_SELECT( NAME, ... ) SELECT( NAME, VA_SIZE(__VA_ARGS__) )(__VA_ARGS__)
#define ALIGN(value, alignment) ((value+alignment-1) & (-alignment))

#define internal static
#define local static
#define global static

#define INVALID_CODE *(int*)0 = 0
#define INVALID_CASE(case_name) case case_name: { INVALID_CODE; } break
#define INVALID_DEFAULT_CASE default: { INVALID_CODE; } break

#ifdef OS_WINDOWS
#define DEBUG_IF_AVAILABLE_OR_TERM __debugbreak()
#else
#define DEBUG_IF_AVAILABLE_OR_TERM INVALID_CODE
#endif

#ifdef DEVELOPER_BUILD
#define ASSERT(x) if(!(x)) DEBUG_IF_AVAILABLE_OR_TERM
#else
#define ASSERT(x)
#endif

#define NOT_IMPLEMENTED ASSERT(!"NOT IMPLEMENTED")

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>

typedef float  f32;
typedef double f64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i32 b32;

typedef size_t ptr;

inline f32 Abs(f32 Value)
{
    f32 Result = (Value < 0) ? -Value : Value;
    return Result;
}

inline u16 SafeU16(u32 Value)
{
    ASSERT(Value <= 0xFFFF);
    return (u16)Value;
}

inline u32 SafeU32(u64 Value)
{
    ASSERT(Value <= 0xFFFFFFFF);
    return (u32)Value;
}

inline i32 MinimumI32(i32 A, i32 B)
{
    i32 Result = (A <= B) ? A : B;
    return Result;
}

inline i32 MaximumI32(i32 A, i32 B)
{
    i32 Result = (A >= B) ? A : B;
    return Result;
}

inline f32 MinimumF32(f32 A, f32 B)
{
    f32 Result = (A <= B) ? A : B;
    return Result;
}

inline f32 MaximumF32(f32 A, f32 B)
{
    f32 Result = (A >= B) ? A : B;
    return Result;
}

inline u64 MaximumU64(u64 A, u64 B)
{
    u64 Result = (A >= B) ? A : B;
    return Result;
}

inline f32 Clamp(f32 Value, f32 Min, f32 Max)
{
    f32 Result = MinimumF32(MaximumF32(Value, Min), Max);
    return Result;
}

inline f32 Saturate(f32 Value)
{
    f32 Result = Clamp(Value, 0.0f, 1.0f);
    return Result;
}

inline i32 CeilI32(f32 Value)
{
    i32 Result;
#if MSVC_COMPILER
    Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(Value)));
#else
    INVALID_CODE;
#endif
    return Result;
}

inline u32 CeilU32(f32 Value)
{
    ASSERT(Value >= 0.0f);
    u32 Result = (u32)CeilI32(Value);
    return Result;
}

inline i32 RoundI32(f32 Value)
{
    i32 Result;
#if MSVC_COMPILER
    Result = _mm_cvtss_si32(_mm_round_ss(_mm_setzero_ps(), _mm_set_ss(Value), _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
#else
    INVALID_CODE;
#endif    
    return Result;
}

inline u32 RoundU32(f32 Value)
{
    ASSERT(Value >= 0.0f);
    u32 Result = (u32)RoundI32(Value);
    return Result;
}

inline i32 FloorI32(f32 Value)
{
    i32 Result;
#if MSVC_COMPILER
    Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(Value)));
#else
    INVALID_CODE;
#endif
    return Result;
}

inline u32 FloorU32(f32 Value)
{
    ASSERT(Value >= 0.0f);
    u32 Result = (u32)FloorI32(Value);
    return Result;
}

f32 RoundPrecisionMag(f32 Value, u32 X)
{        
    f32 Result = (f32)RoundI32(X*Value)/(f32)X;
    return Result;
}

inline u32 GetPrecision(u32 Precision)
{
    u32 X = 1;
    Precision--;
    while(Precision > 0) { X *= 10; Precision--; }    
    return X;
}

inline f32 RoundPrecision(f32 Value, u32 Precision)
{    
    u32 X = GetPrecision(Precision);    
    f32 Result = RoundPrecisionMag(Value, X);
    return Result;
}

inline b32 IsDigit(char Character)
{
    b32 Result = isdigit(Character);
    return Result;
}

inline b32 IsNewline(char Character)
{
    b32 Result = (Character == '\n' || Character == '\r');
    return Result;
}

inline b32 IsNullChar(char Character)
{
    b32 Result = (Character == '\0');
    return Result;
}

inline b32 IsWhitespace(char Character)
{
    b32 Result = IsNewline(Character) || (Character == ' ') || (Character == '\t');
    return Result;
}

inline i32 ToNumeric(char Character)
{
    i32 Result = (i32)(Character - '0');
    return Result;
}

inline i32 ToInt(char* String)
{
    i32 Result = (i32)atoi(String);
    return Result;
}

inline f32 ToF32(char* String)
{
    f32 Result = (f32)atof(String);
    return Result;
}

inline char ToLower(char Value)
{
    char Result = (char)tolower(Value);
    return Result;
}

inline f32 SafeRatio(i32 Numerator, i32 Denominator)
{
    ASSERT(Denominator != 0);
    f32 Result = (f32)Numerator/(f32)Denominator;
    return Result;
}

#endif