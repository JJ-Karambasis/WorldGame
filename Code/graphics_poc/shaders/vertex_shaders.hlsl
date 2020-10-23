#include "common.hlsl"

cbuffer model_buffer : register(b0, space0)
{
    ak_m4f Model;
};

cbuffer view_proj_buffer : register(b1, space0)
{
    ak_m4f View;
    ak_m4f Proj;
}

ak_v4f TestVertexShader(in ak_v3f Position : Position0) : SV_Position
{   
    return mul(Proj, mul(View, mul(Model, ak_v4f(Position, 1.0f))));    
}