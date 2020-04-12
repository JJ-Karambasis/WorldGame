#ifdef OS_WINDOWS
#define DEBUG_ARIAL_FONT_PATH "C:\Windows\Fonts\arial.ttf"
#endif

#if DEVELOPER_BUILD
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#endif

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

debug_graphics_mesh DEBUGCreateBoxMesh()
{
    debug_graphics_mesh Result = {};
    
    debug_graphics_vertex_array* VertexArray = &Result.Vertices;
    graphics_index_array* IndexArray = &Result.Indices;
    
    VertexArray->Count = 8;
    IndexArray->Count = 24;
    
    v3f Vertices[8] = 
    {
        V3(-0.5f, -0.5f, 0.0f),
        V3( 0.5f, -0.5f, 0.0f),
        V3( 0.5f,  0.5f, 0.0f),
        V3(-0.5f,  0.5f, 0.0f),
        
        V3(-0.5f, -0.5f, 1.0f),
        V3( 0.5f, -0.5f, 1.0f),
        V3( 0.5f,  0.5f, 1.0f),
        V3(-0.5f,  0.5f, 1.0f)        
    };   
    
    u16 Indices[24] = 
    {
        0, 1,
        1, 2,
        2, 3, 
        3, 0,
        4, 5, 
        5, 6, 
        6, 7, 
        7, 4, 
        0, 4, 
        1, 5, 
        2, 6, 
        3, 7
    };
    
    VertexArray->Ptr = PushWriteArray(Vertices, VertexArray->Count, v3f, 0);
    IndexArray->Ptr  = PushWriteArray(Indices,  IndexArray->Count,  u16, 0);    
    
    return Result;
}

debug_capsule_mesh DEBUGCreateCapsuleMesh(u16 CircleSampleCount)
{
    debug_capsule_mesh Result = {};    
    debug_graphics_mesh* Cap = &Result.Cap;
    
    u16 HalfCircleSampleCountPlusOne = (CircleSampleCount/2)+1;
    f32 CircleSampleIncrement = (2.0f*PI)/(f32)CircleSampleCount;            
    
    Cap->Vertices.Count = CircleSampleCount+(HalfCircleSampleCountPlusOne*2);
    Cap->Indices.Count = Cap->Vertices.Count*2;
    Cap->Vertices.Ptr = PushArray(Cap->Vertices.Count, v3f, Clear, 0);
    Cap->Indices.Ptr = PushArray(Cap->Indices.Count, u16, Clear, 0);
    
    v3f* VertexAt = Cap->Vertices.Ptr;
    
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
    
    u16* IndicesAt = Cap->Indices.Ptr;
    DEBUGPopulateCircleIndices(&IndicesAt, 0, CircleSampleCount);
    DEBUGPopulateCircleIndices(&IndicesAt, CircleSampleCount, HalfCircleSampleCountPlusOne);
    DEBUGPopulateCircleIndices(&IndicesAt, CircleSampleCount+HalfCircleSampleCountPlusOne, HalfCircleSampleCountPlusOne);
    
    debug_graphics_mesh* Body = &Result.Body;
    
    Body->Vertices.Count = 8;
    Body->Indices.Count = 8;
    
    Body->Vertices.Ptr = PushArray(Body->Vertices.Count, v3f, Clear, 0);
    Body->Indices.Ptr = PushArray(Body->Indices.Count, u16, Clear, 0);
    
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
    
    
    CopyArray(Body->Vertices.Ptr, DebugCapsuleBodyVertices, 8, v3f);
    CopyArray(Body->Indices.Ptr, DebugCapsuleBodyIndices, 8, u16);
    
    return Result;
}
