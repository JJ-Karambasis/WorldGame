#include "common.hlsl"

struct ps_shadow_map_input
{
    ak_v4f SSPosition : SV_Position;
    ak_v3f WorldPosition : Position;
};

cbuffer constant_buffer : register(b0, space1)
{
    ak_v4f ShadowMapVector;
}

ak_f32 ShadowMapPixelShader(ps_shadow_map_input Input) : SV_Depth
{
    ak_v3f LightPosition = ShadowMapVector.xyz;
    ak_f32 FarPlaneDistance = ShadowMapVector.w;
    
    ak_f32 LightDistance = length(Input.WorldPosition-LightPosition);
    LightDistance /= FarPlaneDistance;
    return LightDistance;
}