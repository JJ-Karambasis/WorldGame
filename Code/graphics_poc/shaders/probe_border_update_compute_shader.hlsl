#include "common.hlsl"

cbuffer ProbeBuffer : register(b0, space0)
{
    ak_u32 ProbeTexelCount;
    ak_u32 UAVIndex;    
};

cbuffer irradiance_field_buffer : register(b1, space0)
{
    irradiance_field IrradianceField;
};

RWTexture2D<float4> ProbeTextures[2];

[numthreads(8, 8, 1)]
void ProbeBorderUpdateRowComputeShader(ak_v3u ThreadCoord : SV_DispatchThreadID)
{
    ak_u32 ActualProbeTexelCount = ProbeTexelCount+2;
    ak_u32 ActualProbeTexelCountMinusOne = ActualProbeTexelCount-1;
    
    ak_v2u ThreadCoord2D = ThreadCoord.xy;
    ThreadCoord2D.y *= ActualProbeTexelCount;
    
    ak_i32 mod = (ThreadCoord.x % ActualProbeTexelCount);
    if(mod == 0 || mod == ak_i32(ActualProbeTexelCountMinusOne))
        return;
    
    ak_u32 ProbeStart = ak_u32(ThreadCoord2D.x / ActualProbeTexelCount) * ActualProbeTexelCount;
    ak_u32 Offset = ActualProbeTexelCountMinusOne - (ThreadCoord2D.x % ActualProbeTexelCount);
    
    ak_v2u CopyCoords = ak_v2u(ProbeStart+Offset, (ThreadCoord2D.y+1));
    
    ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];
    
    ThreadCoord2D.y += ActualProbeTexelCountMinusOne;
    CopyCoords = ak_v2u(ProbeStart+Offset, ThreadCoord2D.y-1);
    
    ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];
}

[numthreads(8, 8, 1)]
void ProbeBorderUpdateColumnComputeShader(ak_v3i ThreadCoord : SV_DispatchThreadID)
{
    ak_u32 ActualProbeTexelCount = ProbeTexelCount+2;
    ak_u32 ActualProbeTexelCountMinusOne = ActualProbeTexelCount-1;
    
    ak_v2u ThreadCoord2D = ThreadCoord.xy;
    ThreadCoord2D.x *= ActualProbeTexelCount;
    
    ak_v2u CopyCoords = ak_v2u(0, 0);
    
    ak_i32 mod = (ThreadCoord2D.y % ActualProbeTexelCount);
    if(mod == 0 || mod == ak_i32(ActualProbeTexelCountMinusOne))
    {
        CopyCoords.x = ThreadCoord2D.x + ProbeTexelCount;
        CopyCoords.y = ThreadCoord2D.y - sign(mod-1)*ProbeTexelCount;        
        ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];
        
        ThreadCoord2D.x += ActualProbeTexelCountMinusOne;
        CopyCoords.x = ThreadCoord2D.x - ProbeTexelCount;        
        ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];
        return;
    }
    
    ak_u32 ProbeStart = ak_u32(ThreadCoord2D.y / ActualProbeTexelCount) * ActualProbeTexelCount;
    ak_u32 Offset = ActualProbeTexelCountMinusOne - (ThreadCoord2D.y % ActualProbeTexelCount);
    
    CopyCoords = ak_v2u(ThreadCoord2D.x+1, ProbeStart+Offset);
    
    ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];
    
    ThreadCoord2D.x += ActualProbeTexelCountMinusOne;
    CopyCoords = ak_v2u(ThreadCoord2D.x-1, ProbeStart+Offset);
    
    ProbeTextures[UAVIndex][ThreadCoord2D] = ProbeTextures[UAVIndex][CopyCoords];    
}
