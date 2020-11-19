#include "common.hlsl"

Texture2DArray ShadowMap : register(t0, space0);
Texture2D HitAlbedo : register(t1, space0);
Texture2D HitPosition : register(t2, space0);
Texture2D HitNormal : register(t3, space0);
Texture2D HitDepth : register(t4, space0);
Texture2D ProbeIrradiance : register(t5, space0);
Texture2D ProbeDistance : register(t6, space0);

sampler ShadowMapSampler : register(s0, space0);
sampler ProbeSampler : register(s1, space0);
cbuffer light_buffer : register(b0, space0)
{
    light Lights[MAX_LIGHT_COUNT];    
    ak_u32 LightCount;
};

cbuffer irradiance_field_buffer : register(b1, space0)
{
    irradiance_field IrradianceField;
};

RWTexture2D<ak_color4f> ProbeOutput : register(u0, space0);

[numthreads(8, 4, 1)]
void ShadeSurfelsComputeShader(uint3 ThreadIndex : SV_DispatchThreadID)
{    
    ak_f32 Depth = HitDepth[ThreadIndex.xy].r;
    if(Depth > 0)
    {
        ak_color3f Albedo = HitAlbedo[ThreadIndex.xy].rgb;
        ak_v3f Position = HitPosition[ThreadIndex.xy].xyz;
        ak_v3f Normal = normalize(HitNormal[ThreadIndex.xy].xyz);    
        
        diffuse_payload DiffusePayload;
        DiffusePayload.Lights = Lights;
        DiffusePayload.LightCount = LightCount;
        DiffusePayload.ShadowMap = ShadowMap;
        DiffusePayload.ShadowMapSampler = ShadowMapSampler;
        ak_color3f Diffuse = GetDiffuseLighting(DiffusePayload, Albedo, Position, Normal);
        
        ak_color3f Irradiance = ak_color3f(0, 0, 0);
        
        ak_f32 BlendWeight = GetBlendWeight(IrradianceField, Position);            
        if(BlendWeight > 0)
        {        
            ak_v3f RayDirection = GenerateSphericalFibonacciDir(ThreadIndex.y, RAYS_PER_PROBE);        
            ak_v3f SurfaceBias = GetSurfaceBias(Normal, RayDirection, NORMAL_BIAS, VIEW_BIAS);
            
            irradiance_payload IrradiancePayload;
            IrradiancePayload.ProbeIrradiance = ProbeIrradiance;
            IrradiancePayload.ProbeDistance = ProbeDistance;
            IrradiancePayload.BilinearSampler = ProbeSampler;
            IrradiancePayload.IrradianceField = IrradianceField;
            
            Irradiance = GetIrradiance(IrradiancePayload, Position, Normal, SurfaceBias);
            Irradiance *= BlendWeight;            
        }            
        ProbeOutput[ThreadIndex.xy] = ak_color4f(Diffuse + ((Albedo/AK_PI)*Irradiance), Depth);
        //ProbeOutput[ThreadIndex.xy] = ak_color4f(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        ProbeOutput[ThreadIndex.xy] = ak_color4f(0, 0, 0, 0);
    }
}