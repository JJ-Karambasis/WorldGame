#define ak_f32 float
#define ak_i32 int
#define ak_u32 uint
#define ak_v2f float2
#define ak_v3f float3
#define ak_v4f float4
#define ak_v2i int2
#define ak_v3i int3
#define ak_v4i int4
#define ak_v2u uint2
#define ak_v3u uint3
#define ak_v4u uint4
#define ak_color3f float3
#define ak_color4f float4
#define ak_m3f float3x3
#define ak_m4f float4x4

#include "../shader_config.h"
#define AK_SqrMagnitude(v) dot((v), (v))
#define AK_PI 3.14159265359f
#define AK_INV_PI 0.31830988618f

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

ak_f32 SignNotZero(ak_f32 V)
{
    return (V >= 0.0f) ? 1.0f : -1.0f;
}

ak_v2f SignNotZero(ak_v2f V)
{
    return ak_v2f(SignNotZero(V.x), SignNotZero(V.y));
}

ak_m3f AK_InvTransformM3(ak_m3f M)
{
    ak_f32 sx = 1.0f/AK_SqrMagnitude(M[0]);
    ak_f32 sy = 1.0f/AK_SqrMagnitude(M[1]);
    ak_f32 sz = 1.0f/AK_SqrMagnitude(M[2]);
    
    ak_v3f x = sx*M[0];
    ak_v3f y = sy*M[1];
    ak_v3f z = sz*M[2];
    
    ak_m3f Result = 
    {
        x.x, y.x, z.x,
        x.y, y.y, z.y,
        x.z, y.z, z.z,        
    };
    
    return Result;
}

#define POSITIVE_X 0
#define NEGATIVE_X 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_Z 4
#define NEGATIVE_Z 5
ak_i32 ChooseFaceIndex(ak_v3f TexCoords, out ak_v2f OutTexCoords)
{       
    ak_f32 AbsX = abs(TexCoords.x);
    ak_f32 AbsY = abs(TexCoords.y);
    ak_f32 AbsZ = abs(TexCoords.z);
    
    ak_f32 LargestAxis = 0;
    ak_f32 U = 0, V = 0;
    ak_i32 Result = -1;
    if((AbsX >= AbsY) && (AbsX >= AbsZ))
    {
        LargestAxis = AbsX;
        if(TexCoords.x >= 0)
        {            
            U =  TexCoords.z;
            V =  TexCoords.y;
            Result = POSITIVE_X; 
        }
        else
        {
            U = -TexCoords.z;
            V = TexCoords.y;
            Result = NEGATIVE_X;
        }
    }
    if((AbsY >= AbsX) && (AbsY >= AbsZ))
    {
        LargestAxis = AbsY;
        if(TexCoords.y >= 0)
        {            
            U = -TexCoords.x;
            V = -TexCoords.z;
            Result = POSITIVE_Y;
        }
        else 
        {
            U =  TexCoords.x;
            V =  -TexCoords.z;
            Result = NEGATIVE_Y;                                    
        }
    }
    if((AbsZ >= AbsX) && (AbsZ >= AbsY))
    {
        LargestAxis = AbsZ;
        if(TexCoords.z >= 0)
        {
            U = TexCoords.x;
            V = TexCoords.y;
            Result = POSITIVE_Z;
        }
        else
        { 
            U =  TexCoords.x;
            V =  -TexCoords.y;
            Result = NEGATIVE_Z;
        }
    }
    
    OutTexCoords.x = 0.5f * (U / LargestAxis + 1.0f);
    OutTexCoords.y = 0.5f * (V / LargestAxis + 1.0f);
    return Result;
}

struct light
{
    ak_v4f Position;
    ak_color4f Color;
};

struct diffuse_payload 
{    
    light Lights[MAX_LIGHT_COUNT];    
    ak_u32 LightCount;
    Texture2DArray ShadowMap;
    sampler ShadowMapSampler;
};

ak_color3f GetDiffuseLighting(diffuse_payload Payload, ak_color3f SurfaceColor, ak_v3f SurfacePosition, ak_v3f SurfaceNormal)
{
    ak_color3f Lambertian = SurfaceColor * AK_INV_PI;
    ak_color3f Lighting = 0.0f;
        
    for(ak_u32 LightIndex = 0; LightIndex < Payload.LightCount; LightIndex++)
    {
        light Light = Payload.Lights[LightIndex];
        
        ak_f32 LightRadius  = Light.Position.w;
        ak_v3f L = Light.Position.xyz - SurfacePosition;
        ak_f32 DistanceFromLight = length(L);
        
        ak_v2f OutShadowTexCoords;
        ak_i32 FaceIndex = 6*LightIndex + ChooseFaceIndex(-L, OutShadowTexCoords);
        ak_f32 ClosestDepth = Payload.ShadowMap.SampleLevel(Payload.ShadowMapSampler, ak_v3f(OutShadowTexCoords, FaceIndex), 0).r;
        ClosestDepth *= LightRadius;        
        
        L /= DistanceFromLight;
        ak_f32 Bias = max(0.05f*(1.0f-dot(SurfaceNormal, L)), 0.05f);
        if((DistanceFromLight-Bias) < ClosestDepth)
        {                                            
            ak_f32 Numerator = clamp(1.0f - pow((DistanceFromLight/LightRadius), 4.0f), 0.0f, 1.0f);
            ak_f32 Denominator = (DistanceFromLight)+1.0f;
            ak_f32 Falloff = (Numerator*Numerator)/Denominator;
                        
            ak_color3f LightColor = Light.Color.xyz*Light.Color.w*Falloff;        
            Lighting += max(dot(SurfaceNormal, L), 0.0f)*LightColor;
        }                
    }        
    
    return Lambertian*Lighting;
}                            

struct irradiance_payload
{
    Texture2D ProbeIrradiance;
    Texture2D ProbeDistance;
    sampler BilinearSampler;
    irradiance_field IrradianceField;
};

ak_v3f GetSurfaceBias(ak_v3f SurfaceNormal, ak_v3f ViewDirection, 
                      ak_f32 NormalBias, ak_f32 ViewBias)
{
    return (SurfaceNormal*NormalBias) + (-ViewDirection*ViewBias);
}

ak_v3f GetProbeCoords(irradiance_field IrradianceField, ak_v3f WorldPosition)
{    
    ak_v3f Position = WorldPosition - IrradianceField.BottomLeft;
    ak_v3f ProbeCoords = Position/IrradianceField.Spacing;
    return ProbeCoords;
}

ak_v3i GetProbeBaseCoords(irradiance_field IrradianceField, ak_v3f WorldPosition)
{
    ak_v3f ProbeCoords = GetProbeCoords(IrradianceField, WorldPosition);
    return clamp(ak_v3i(ProbeCoords), ak_v3i(0, 0, 0), IrradianceField.Count-ak_v3i(1, 1, 1));
}

ak_v3f GetProbeWorldPosition(irradiance_field IrradianceField, ak_v3i ProbeCoords)
{
    return IrradianceField.BottomLeft + ak_v3f(ProbeCoords)*IrradianceField.Spacing;
}

ak_i32 GetProbeIndex(ak_v3i Coords, ak_v3i Count)
{
    return Coords.x + (Count.x*Coords.y) + (Count.x*Count.y)*Coords.z;
}

ak_i32 GetProbesPerPlane(ak_v3i Count)
{
    return Count.x*Count.y;
}

ak_i32 GetPlaneIndex(ak_v2u ThreadCoords, ak_v3i GridCount, ak_i32 ProbeNumTexels)
{
    return ThreadCoords.x / (GridCount.x*ProbeNumTexels);
}

ak_i32 GetProbeIndexInPlane(ak_v2u ThreadCoords, ak_i32 PlaneIndex, ak_v3i GridCount, ak_i32 ProbeNumTexels)
{
    return (ThreadCoords.x/ProbeNumTexels) - (PlaneIndex*GridCount.x)+(GridCount.y*(ThreadCoords.y/ProbeNumTexels));
}

ak_i32 GetProbeIndex(ak_v2i ThreadCoords, ak_v3i GridCount, ak_i32 ProbeNumTexels)
{
    ak_i32 ProbesPerPlane = GetProbesPerPlane(GridCount);
    ak_i32 PlaneIndex = GetPlaneIndex(ThreadCoords, GridCount, ProbeNumTexels);
    ak_i32 ProbeIndexInPlane = GetProbeIndexInPlane(ThreadCoords, PlaneIndex, GridCount, ProbeNumTexels);
    return (PlaneIndex*ProbesPerPlane) + ProbeIndexInPlane;
}

ak_v2f GetOctahedralCoordinates(ak_v3f Direction)
{    
    ak_f32 l1norm = abs(Direction.x) + abs(Direction.y) + abs(Direction.z);
    ak_v2f uv = Direction.xy * (1.f / l1norm);
    if (Direction.z < 0.f)
    {
        uv = (1.0f - abs(uv.yx)) * SignNotZero(uv.xy);
    }
    return uv;    
}

ak_v3f GetOctahedralDirection(ak_v2f Coords)
{
    ak_v3f Direction = ak_v3f(Coords.x, Coords.y, 1.0f - abs(Coords.x) - abs(Coords.y));
    if(Direction.z < 0) 
        Direction.xy = (1.0f - abs(Direction.yx)) * SignNotZero(Direction.xy);    
    return normalize(Direction);
}

ak_v2f GetNormalizedOctahedralCoordinates(ak_v2u ThreadCoords, ak_u32 NumTexels)
{    
    ak_v2f Result = ak_v2f(ThreadCoords.x % NumTexels, ThreadCoords.y % NumTexels);
    
    Result += 0.5f;
    Result /= ak_f32(NumTexels);
    
    Result *= 2.0f;
    Result -= 1.0f;
    return Result;
}

ak_v2f GetProbeUV(irradiance_field IrradianceField, ak_u32 ProbeIndex, ak_v2f OctantCoords, ak_i32 NumTexels)
{
    ak_i32 ProbesPerPlane = GetProbesPerPlane(IrradianceField.Count);
    ak_i32 PlaneIndex = ak_i32(ProbeIndex/ProbesPerPlane);
    
    ak_f32 InteriorTexels = ak_f32(NumTexels);
    ak_f32 ProbeTexels = InteriorTexels+2.0f;
    
    ak_i32 GridSpaceX = (ProbeIndex % IrradianceField.Count.x);
    ak_i32 GridSpaceY = (ProbeIndex / IrradianceField.Count.x);
    
    ak_i32 X = GridSpaceX + (PlaneIndex*IrradianceField.Count.x);
    ak_i32 Y = GridSpaceY % IrradianceField.Count.y;
    
    ak_f32 TextureWidth =  ProbeTexels*(IrradianceField.Count.x*IrradianceField.Count.z);
    ak_f32 TextureHeight = ProbeTexels*IrradianceField.Count.y;
    
    ak_v2f UV = ak_v2f(X*ProbeTexels, Y*ProbeTexels) + (ProbeTexels*0.5f);
    UV += OctantCoords.xy * (InteriorTexels*0.5f);
    UV /= ak_v2f(TextureWidth, TextureHeight);
    return UV;
}

ak_v3f GenerateSphericalFibonacciDir(ak_f32 Index, ak_f32 NumSamples)
{
    ak_f32 b = (sqrt(5.0f)*0.5f + 0.5f) - 1.0f;
    ak_f32 phi = 2*AK_PI*frac(Index*b);
    ak_f32 c = 1.0f - (2.0f*Index + 1.0f) * (1.0f/NumSamples);
    ak_f32 s = sqrt(saturate(1.0f - (c*c)));    
    ak_v3f Result = ak_v3f((cos(phi) * s), (sin(phi)*s), c);
    return Result;
}

ak_v3f GenerateSphericalFibonacciDir(ak_f32 Index, ak_f32 NumSamples, ak_m4f RotationTransform)
{
    ak_v4f Dir = ak_v4f(GenerateSphericalFibonacciDir(Index, NumSamples), 0.0f);
    return normalize(mul(Dir, RotationTransform).xyz);
}

ak_f32 GetBlendWeight(irradiance_field IrradianceField, ak_v3f WorldPosition)
{
    ak_f32 Result = 1.0f;
    
    
    ak_v3f ProbeCoords = GetProbeCoords(IrradianceField, WorldPosition);    
    ak_v3f OverProbeMax = (IrradianceField.Count-1.0f) - ProbeCoords;
    
    Result *= clamp(ProbeCoords.x, 0.0f, 1.0f);
    Result *= clamp(ProbeCoords.y, 0.0f, 1.0f);
    Result *= clamp(ProbeCoords.z, 0.0f, 1.0f);
    Result *= clamp(OverProbeMax.x, 0.0f, 1.0f);
    Result *= clamp(OverProbeMax.y, 0.0f, 1.0f);
    Result *= clamp(OverProbeMax.z, 0.0f, 1.0f);
    
    return Result;
}

ak_color3f GetIrradiance(irradiance_payload Payload, ak_v3f SurfacePosition, ak_v3f SurfaceNormal, ak_v3f SurfaceBias)
{
    ak_color3f Irradiance = ak_color3f(0.0f, 0.0f, 0.0f);  
    ak_f32 TotalWeight = 0.0f; 
    ak_v3f BiasPosition = SurfacePosition+SurfaceBias;
    
    ak_v3i BaseProbeCoords = GetProbeBaseCoords(Payload.IrradianceField, BiasPosition);
    ak_v3f BaseProbeWorldPosition = GetProbeWorldPosition(Payload.IrradianceField, BaseProbeCoords);
    
    ak_v3f Alpha = clamp(((BiasPosition - BaseProbeWorldPosition) / Payload.IrradianceField.Spacing), ak_v3f(0, 0, 0), ak_v3f(1, 1, 1));
    
    for(ak_i32 ProbeIndex = 0; ProbeIndex < 8; ProbeIndex++)
    {
        ak_v3i ProbeOffset = ak_v3i(ProbeIndex, ProbeIndex >> 1, ProbeIndex >> 2) & ak_v3i(1, 1, 1);
        ak_v3i ProbeCoords = clamp(BaseProbeCoords + ProbeOffset, ak_v3i(0, 0, 0), Payload.IrradianceField.Count-ak_v3i(1, 1, 1));        
        ak_v3f ProbeWorldPosition = GetProbeWorldPosition(Payload.IrradianceField, ProbeCoords);
        
        ak_i32 ProbeIndex = GetProbeIndex(ProbeCoords, Payload.IrradianceField.Count);
        
        ak_v3f WorldPToProbe = normalize(ProbeWorldPosition-SurfacePosition);
        ak_v3f BiasWorldPToProbe = normalize(ProbeWorldPosition-BiasPosition);
        ak_f32 BiasToProbeDistance = length(WorldPToProbe-BiasWorldPToProbe);        
        
        ak_v3f Trilinear = max(0.001f, lerp(1.0f-Alpha, Alpha, ProbeOffset));
        ak_f32 TrilinearWeight = Trilinear.x*Trilinear.y*Trilinear.z;
        ak_f32 Weight = 1.0f;
        
        ak_f32 WrapShading = (dot(WorldPToProbe, SurfaceNormal)+1.0f)*0.5f;
        Weight *= (WrapShading*WrapShading)+0.2f;
        
        ak_v2f OctantCoords = GetOctahedralCoordinates(-BiasWorldPToProbe);
        ak_v2f ProbeUV = GetProbeUV(Payload.IrradianceField, ProbeIndex, OctantCoords, DISTANCE_TEXEL_COUNT);
        ak_v2f FilteredDistance = 2.0f*Payload.ProbeDistance.SampleLevel(Payload.BilinearSampler, ProbeUV, 0).rg; 
        
        ak_f32 MeanDistanceToSurface = FilteredDistance.x;
        ak_f32 Variance = abs((FilteredDistance.x*FilteredDistance.x) - FilteredDistance.y);
        
        ak_f32 ChebyshevWeight = 1.0f;
        if(BiasToProbeDistance > MeanDistanceToSurface)
        {
            ak_f32 V = BiasToProbeDistance - MeanDistanceToSurface;
            ChebyshevWeight = Variance / (Variance + (V*V));
            ChebyshevWeight = max((ChebyshevWeight*ChebyshevWeight*ChebyshevWeight), 0.0f);
        }
        
        Weight *= max(0.05f, ChebyshevWeight);
        Weight = max(0.000001f, Weight);
        
        const ak_f32 CrushThreshold = 0.2f;
        if(Weight < CrushThreshold)        
            Weight *= (Weight*Weight) * (1.0f / (CrushThreshold*CrushThreshold));        
        
        Weight *= TrilinearWeight;
        
        OctantCoords = GetOctahedralCoordinates(SurfaceNormal);
        ProbeUV = GetProbeUV(Payload.IrradianceField, ProbeIndex, OctantCoords, IRRADIANCE_TEXEL_COUNT);
        ak_color3f ProbeIrradiance = Payload.ProbeIrradiance.SampleLevel(Payload.BilinearSampler, ProbeUV, 0).rgb;
        
        ak_f32 Exponent = PROBE_IRRADIANCE_ENCODING_GAMMA*0.5f;
        ProbeIrradiance = pow(ProbeIrradiance, Exponent);
        
        Irradiance += (Weight*ProbeIrradiance);
        TotalWeight += Weight;        
    }
    
    Irradiance *= (1.0f/TotalWeight);
    Irradiance *= Irradiance;
    Irradiance *= 2.0f*AK_PI;    
    
    return Irradiance;    
}

ak_color3f ToneMap(ak_color3f x)
{
    ak_f32 a = 2.51f;
    ak_f32 b = 0.03f;
    ak_f32 c = 2.43f;
    ak_f32 d = 0.59f;
    ak_f32 e = 0.14f;
    return saturate((x*(a*x + b)) / (x*(c*x + d) + e));
}