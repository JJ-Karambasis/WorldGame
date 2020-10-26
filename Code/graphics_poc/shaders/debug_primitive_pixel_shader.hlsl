#include "common.hlsl"

cbuffer color_buffer : register(b0, space1)
{
    ak_color4f Color;
};

ak_v4f DebugPrimitivePixelShader() : SV_Target
{
    return Color;
}