#include "common.hlsl"

cbuffer model_buffer : register(b0, space0)
{
    ak_m4f Model;
};

cbuffer view_proj_buffer : register(b1, space0)
{
    ak_m4f ViewProj;    
};

struct vs_shadow_map_output
{
    ak_v4f Position : SV_Position;
    ak_v3f WorldPosition : Position;
};

vs_shadow_map_output ShadowMapVertexShader(in ak_v3f Position : Position0)
{
    vs_shadow_map_output Result;    
    Result.WorldPosition = mul(Model, ak_v4f(Position, 1.0f)).xyz;
    Result.Position = mul(ViewProj, ak_v4f(Result.WorldPosition, 1.0f));    
    return Result;
}