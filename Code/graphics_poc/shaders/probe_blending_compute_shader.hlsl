#include "common.hlsl"

#if IRRADIANCE_BLENDING
#define PROBE_NUM_TEXELS IRRADIANCE_TEXEL_COUNT
#define PROBE_OUTPUT 0 
#else
#define PROBE_NUM_TEXELS DISTANCE_TEXEL_COUNT
#define PROBE_OUTPUT 1
#endif

cbuffer irradiance_field_buffer : register(b0, space0)
{
    irradiance_field IrradianceField;    
};

RWTexture2D<ak_color4f> ProbeOutputs[2] : register(u0, space0);
Texture2D<ak_color4f> ProbeInput : register(t0, space0);

[numthreads(PROBE_NUM_TEXELS, PROBE_NUM_TEXELS, 1)]
void ProbeBlendingComputeShader(ak_v3u DispatchThreadID : SV_DispatchThreadID)
{
    ak_color4f Result = ak_color4f(0, 0, 0, 0);
    
    ak_i32 ProbeIndex = GetProbeIndex(DispatchThreadID.xy, IrradianceField.Count, PROBE_NUM_TEXELS);
    if(ProbeIndex < 0)
        return;
    
    ak_v2u ProbeTexCoords =  DispatchThreadID.xy + ak_v2u(1, 1) + (DispatchThreadID.xy / PROBE_NUM_TEXELS)*2;   
    
    ak_v2f ProbeOctantTexCoords = GetNormalizedOctahedralCoordinates(DispatchThreadID.xy, PROBE_NUM_TEXELS);
    ak_v3f ProbeRayDirection = GetOctahedralDirection(ProbeOctantTexCoords);
    
    for(ak_u32 RayIndex = 0; RayIndex < RAYS_PER_PROBE; RayIndex++)
    {
        ak_v3f RayDirection = GenerateSphericalFibonacciDir(RayIndex, RAYS_PER_PROBE, IrradianceField.RotationTransform);
        ak_f32 Weight  = max(0.0f, dot(ProbeRayDirection, RayDirection));
        ak_v2i ProbeRayIndex = ak_v2i(ProbeIndex, RayIndex);
        
#if IRRADIANCE_BLENDING
        ak_color3f RayRadiance = ProbeInput[ProbeRayIndex].rgb;        
        Result += ak_color4f(RayRadiance*Weight, Weight);        
#else        
        ak_f32 MaxRayDistance = length(IrradianceField.Spacing)*0.75f;           
        Weight = pow(Weight, PROBE_DISTANCE_EXPONENT);
        
        ak_f32 ProbeRayDistance = min(abs(ProbeInput[ProbeRayIndex].w), MaxRayDistance);        
        Result += ak_color4f(ProbeRayDistance*Weight, (ProbeRayDistance*ProbeRayDistance)*Weight, 0.0f, Weight);
#endif                        
    }
    
    const ak_f32 epsilon = 1e-9f*ak_f32(RAYS_PER_PROBE);
    Result.rgb *= 1.0f/max(2.0f*Result.a, epsilon);
    
#if IRRADIANCE_BLENDING
    Result.rgb = pow(Result.rgb, PROBE_INVERSE_IRRADIANCE_ENCODING_GAMMA);
#endif
    
    ak_f32 Hysteresis = HYSTERESIS;
    ak_color3f Previous = ProbeOutputs[PROBE_OUTPUT][ProbeTexCoords].rgb;    
    
    Result = ak_v4f(lerp(Result.rgb, Previous.rgb, Hysteresis), 1.0f);    
    ProbeOutputs[PROBE_OUTPUT][ProbeTexCoords] = Result;
}