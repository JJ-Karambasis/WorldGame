#include "common.hlsl"

cbuffer model_buffer : register(b0, space0)
{
    ak_m4f Model;
};

cbuffer view_proj_buffer : register(b1, space0)
{
    ak_m4f View;
    ak_m4f Proj;
};

struct vs_object_output
{
    ak_v4f Position : SV_Position;
    ak_v3f WorldPosition : Position;
    ak_v3f WorldNormal : Normal;
};

vs_object_output ObjectVertexShader(in ak_v3f Position : Position0, 
                                    in ak_v3f Normal : Normal0)
{   
    vs_object_output Result;    
    ak_m4f ViewProj = mul(Proj, View);
    Result.WorldNormal = mul(transpose(AK_InvTransformM3((ak_m3f)Model)), Normal);
    Result.WorldPosition = mul(Model, ak_v4f(Position, 1.0f)).xyz;
    Result.Position = mul(ViewProj, ak_v4f(Result.WorldPosition, 1.0f));        
    return Result;
}