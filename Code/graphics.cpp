void DEBUGPopulateCircleIndices(u16** Indices, u16 StartSampleIndex, u16 CircleSampleCount)
{
    u16* IndicesAt = *Indices;    
    u16 TotalSampleCount = StartSampleIndex+CircleSampleCount;
    for(u16 SampleIndex = StartSampleIndex; SampleIndex < TotalSampleCount; SampleIndex++)
    {
        if(SampleIndex == (TotalSampleCount-1))
        {
            *IndicesAt++ = SampleIndex;
            *IndicesAt++ = SampleIndex - (CircleSampleCount-1);
        }
        else
        {
            *IndicesAt++ = SampleIndex;
            *IndicesAt++ = SampleIndex+1;
        }
    }            
    *Indices = IndicesAt;
} 

debug_capsule_mesh DEBUGCreateCapsuleMesh(u16 CircleSampleCount)
{
    debug_capsule_mesh Result = {};    
    mesh* Cap = &Result.Cap;
    
    u16 HalfCircleSampleCountPlusOne = (CircleSampleCount/2)+1;
    f32 CircleSampleIncrement = (2.0f*PI)/(f32)CircleSampleCount;            
    
    Cap->VertexCount = CircleSampleCount+(HalfCircleSampleCountPlusOne*2);
    Cap->IndexCount = Cap->VertexCount*2;
    Cap->Vertices = PushArray(Cap->VertexCount, v3f, Clear, 0);
    Cap->Indices = PushArray(Cap->IndexCount, u16, Clear, 0);
    
    v3f* VertexAt = Cap->Vertices;
    
    f32 Radians;
    Radians = 0.0f;        
    for(u32 SampleIndex = 0; SampleIndex < CircleSampleCount; SampleIndex++, Radians += CircleSampleIncrement)
        *VertexAt++ = V3(Cos(Radians), Sin(Radians), 0.0f);
    
    Radians = 0.0;
    for(u32 SampleIndex = 0; SampleIndex < HalfCircleSampleCountPlusOne; SampleIndex++, Radians += CircleSampleIncrement)                
        *VertexAt++ = V3(0.0f, Cos(Radians), Sin(Radians));
    
    Radians = 0.0f;
    for(u32 SampleIndex = 0; SampleIndex < HalfCircleSampleCountPlusOne; SampleIndex++, Radians += CircleSampleIncrement)
        *VertexAt++ = V3(Cos(Radians), 0.0f, Sin(Radians));    
    
    u16* IndicesAt = Cap->Indices;
    DEBUGPopulateCircleIndices(&IndicesAt, 0, CircleSampleCount);
    DEBUGPopulateCircleIndices(&IndicesAt, CircleSampleCount, HalfCircleSampleCountPlusOne);
    DEBUGPopulateCircleIndices(&IndicesAt, CircleSampleCount+HalfCircleSampleCountPlusOne, HalfCircleSampleCountPlusOne);
    
    mesh* Body = &Result.Body;
    
    Body->VertexCount = 8;
    Body->IndexCount = 8;
    
    Body->Vertices = PushArray(Body->VertexCount, v3f, Clear, 0);
    Body->Indices = PushArray(Body->IndexCount, u16, Clear, 0);
    
    v3f DebugCapsuleBodyVertices[] = 
    {
        V3( 1.0f,  0.0f, -0.5f),
        V3( 1.0f,  0.0f,  0.5f),
        
        V3( 0.0f,  1.0f, -0.5f),
        V3( 0.0f,  1.0f,  0.5f), 
        
        V3(-1.0f,  0.0f, -0.5f),
        V3(-1.0f,  0.0f,  0.5f), 
        
        V3( 0.0f, -1.0f, -0.5f),
        V3( 0.0f, -1.0f,  0.5f)
    };
    
    u16 DebugCapsuleBodyIndices[] = 
    {
        0, 1, 2, 3, 4, 5, 6, 7
    };
    
    
    CopyArray(Body->Vertices, DebugCapsuleBodyVertices, 8, v3f);
    CopyArray(Body->Indices, DebugCapsuleBodyIndices, 8, u16);
    
    return Result;
}