#include "common.hlsl"

ak_v4f TestVertexShader(in ak_v3f Position : Position0) : SV_Position
{    
    return ak_v4f(Position.xy, 0.0f, 1.0f);
}