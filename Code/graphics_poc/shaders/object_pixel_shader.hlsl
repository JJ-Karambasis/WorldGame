#include "common.hlsl"

struct ps_object_input
{
    ak_v4f SSPosition : SV_Position;
    ak_v3f WorldPosition : Position;
    ak_v3f WorldNormal : Normal;
};

cbuffer color_buffer : register(b0, space1)
{
    ak_color4f SurfaceColor; 
    ak_v4f CameraPosition;
};

cbuffer light_buffer : register(b1, space1)
{
    light Lights[MAX_LIGHT_COUNT];    
    ak_u32 LightCount;
};

cbuffer irradiance_field_buffer : register(b2, space1)
{
    irradiance_field IrradianceField;
}

Texture2DArray ShadowMap : register(t0, space1);
Texture2D ProbeIrradiance : register(t1, space1);
Texture2D ProbeDistance : register(t2, space1);

sampler ShadowMapSampler : register(s0, space1);
sampler ProbeSampler : register(s1, space1);

ak_color4f ObjectPixelShader(ps_object_input Input) : SV_TARGET
{
    ak_v3f Normal = normalize(Input.WorldNormal);    
    ak_color3f Lo = ak_color3f(0, 0, 0);
    
    ak_v3f V = normalize(CameraPosition.xyz-Input.WorldPosition);
    
    diffuse_payload Payload;
    Payload.Lights = Lights;
    Payload.LightCount = LightCount;
    Payload.ShadowMap = ShadowMap;
    Payload.ShadowMapSampler = ShadowMapSampler;
    ak_color3f Diffuse = GetDiffuseLighting(Payload, SurfaceColor.rgb, Input.WorldPosition, Normal);    
    
    ak_color3f Irradiance = ak_color3f(0, 0, 0);    
    ak_f32 BlendWeight = GetBlendWeight(IrradianceField, Input.WorldPosition);            
    if(BlendWeight > 0)
    {                
        ak_v3f SurfaceBias = GetSurfaceBias(Normal, V, NORMAL_BIAS, VIEW_BIAS);
        
        irradiance_payload IrradiancePayload;
        IrradiancePayload.ProbeIrradiance = ProbeIrradiance;
        IrradiancePayload.ProbeDistance = ProbeDistance;
        IrradiancePayload.BilinearSampler = ProbeSampler;
        IrradiancePayload.IrradianceField = IrradianceField;
        
        Irradiance = GetIrradiance(IrradiancePayload, Input.WorldPosition, Normal, SurfaceBias);
        Irradiance *= BlendWeight;            
    }            
#if 1
    ak_color3f Result = Diffuse + ((SurfaceColor.rgb/AK_PI)*Irradiance);                                       
    return ak_v4f(Result, SurfaceColor.a);
#else
    //return ak_v4f(Diffuse, SurfaceColor.a);
    return ak_v4f(AK_INV_PI*Irradiance, SurfaceColor.a);
#endif
}