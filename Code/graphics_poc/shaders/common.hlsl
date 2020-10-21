#define ak_f32 float
#define ak_i32 int
#define ak_u32 uint
#define ak_v2f float2
#define ak_v3f float3
#define ak_v4f float4
#define ak_v2i int2
#define ak_v3i int3
#define ak_v4i int4
#define ak_color3f float3
#define ak_color4f float4
#define ak_m3f float3x3
#define ak_m4f float4x4


ak_color3f AK_RGB(ak_f32 r, ak_f32 g, ak_f32 b)
{
    return ak_color3f(r, g, b);
}

ak_color3f AK_White3()
{
    return AK_RGB(1.0f, 1.0f, 1.0f);
}

ak_color3f AK_Red3()
{
    return AK_RGB(1.0f, 0.0f, 0.0f);
}

ak_color3f AK_Green3()
{
    return AK_RGB(0.0f, 1.0f, 0.0f);
}

ak_color3f AK_Blue3()
{
    return AK_RGB(0.0f, 0.0f, 1.0f);
}

ak_color3f AK_Yellow3()
{
    return AK_RGB(1.0f, 1.0f, 0.0f);
}

ak_color3f AK_Black3()
{
    return AK_RGB(0.0f, 0.0f, 0.0f);
}

ak_color3f AK_Cyan3()
{
    return AK_RGB(0.0f, 1.0f, 1.0f);
}

ak_color3f AK_Magenta3()
{
    return AK_RGB(1.0f, 0.0f, 1.0f);
}

ak_color4f AK_RGBA(ak_f32 r, ak_f32 g, ak_f32 b, ak_f32 a = 1.0f)
{
    return ak_color4f(r, g, b, a);
}

ak_color4f AK_White4(ak_f32 a=1.0f)
{
    return AK_RGBA(1.0f, 1.0f, 1.0f, a);
}

ak_color4f AK_Red4(ak_f32 a = 1.0f)
{
    return AK_RGBA(1.0f, 0.0f, 0.0f, a);
}

ak_color4f AK_Green4(ak_f32 a = 1.0f)
{
    return AK_RGBA(0.0f, 1.0f, 0.0f, a);
}

ak_color4f AK_Blue4(ak_f32 a = 1.0f)
{
    return AK_RGBA(0.0f, 0.0f, 1.0f, a);
}

ak_color4f AK_Yellow4(ak_f32 a = 1.0f)
{
    return AK_RGBA(1.0f, 1.0f, 0.0f, a);
}

ak_color4f AK_Black4(ak_f32 a = 1.0f)
{
    return AK_RGBA(0.0f, 0.0f, 0.0f, a);
}

ak_color4f AK_Cyan4(ak_f32 a = 1.0f)
{
    return AK_RGBA(0.0f, 1.0f, 1.0f, a);
}

ak_color4f AK_Magenta4(ak_f32 a = 1.0f)
{
    return AK_RGBA(1.0f, 0.0f, 1.0f, a);
}

ak_color4f AK_Orange4(ak_f32 a = 1.0f)
{
    return AK_RGBA(1.0f, 0.0f, 1.0f, a);
}