#define AK_COMMON_IMPLEMENTATION
#include <ak_common.h>

struct bitmap
{
    ak_v2i  Resolution;
    ak_u32* Pixels;
};

struct camera
{
    ak_v3f Position;
    ak_v3f Direction;    
};

struct mesh
{
    ak_u32 VertexCount;
    ak_vertex_p3_n3* Vertices;
    
    ak_u32 IndexCount;
    ak_u16* Indices;
};

struct object
{
    ak_sqtf     Transform;
    ak_color4f  Color;
    mesh*       Mesh;
};

struct light
{
    ak_v3f Position;
    ak_f32 Radius;
    ak_color3f Color;
    ak_f32 Intensity;
};

struct ray_trace_common_data
{
    bitmap* Bitmap;
    camera* Camera;
    ak_array<object> Objects;    
    ak_array<light> Lights;
    ak_m4f InvPerspective;
    ak_m4f InvView;
};

struct ray_trace_row_task
{    
    ak_i32 YIndex;    
    ray_trace_common_data* CommonData;
};

struct ray_trace
{
    ak_bool Hit;
    object* HitObject;
    ak_v3f  Position;
    ak_v3f  Normal;
};

ak_bool RayTriangleIntersection(ak_v3f RayOrigin, ak_v3f RayDirection, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* t, ak_f32* u, ak_f32* v)
{    
    ak_v3f Edge1 = P1 - P0;
    ak_v3f Edge2 = P2 - P0;
    
    ak_v3f PVec = AK_Cross(RayDirection, Edge2);
    
    ak_f32 Det = AK_Dot(Edge1, PVec);
    
    if(AK_EqualZeroEps(Det))
        return false;
    
    ak_v3f TVec = RayOrigin - P0;
    
    *u = AK_Dot(TVec, PVec);
    if(*u < 0.0f || *u > Det)
        return false;
    
    ak_v3f QVec = AK_Cross(TVec, Edge1);
    
    *v = AK_Dot(RayDirection, QVec);
    if(*v < 0.0f || *u + *v > Det)
        return false;
    
    *t = AK_Dot(Edge2, QVec);
    
    ak_f32 InvDet = 1.0f / Det;
    *t *= InvDet;
    *u *= InvDet;
    *v *= InvDet;
    
    return true;
}

ak_bool RayIntersected(ak_v3f Origin, ak_v3f Direction, ak_array<object> Objects)
{
    AK_ForEach(Object, &Objects)
    {
        mesh* Mesh = Object->Mesh;
        ak_u32 TriangleCount = Mesh->IndexCount/3;
        
        for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
        {
            ak_v3f V0 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+0]].P, Object->Transform);
            ak_v3f V1 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+1]].P, Object->Transform);
            ak_v3f V2 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+2]].P, Object->Transform);
            
            ak_f32 t, u, v;
            if(RayTriangleIntersection(Origin, Direction, V0, V1, V2, &t, &u, &v))
            {
                if(t > 0.001f)
                {
                    return true;
                }
            }
        }
    }
    
    return false;
}

ray_trace CastRay(ak_v3f Origin, ak_v3f Direction, ak_array<object> Objects)
{
    ray_trace Result = {};
    
    ak_f32 tMin = INFINITY, uMin = INFINITY, vMin = INFINITY;
    ak_u32 BestTriangleIndex = 0;
    AK_ForEach(Object, &Objects)
    {
        mesh* Mesh = Object->Mesh;
        ak_u32 TriangleCount = Mesh->IndexCount/3;
        
        for(ak_u32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
        {
            ak_v3f V0 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+0]].P, Object->Transform);
            ak_v3f V1 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+1]].P, Object->Transform);
            ak_v3f V2 = AK_Transform(Mesh->Vertices[Mesh->Indices[(TriangleIndex*3)+2]].P, Object->Transform);
            
            ak_f32 t, u, v;
            if(RayTriangleIntersection(Origin, Direction, V0, V1, V2, &t, &u, &v))
            {
                if((t < tMin) && (t > 0.001f))
                {
                    Result.Hit = true;
                    Result.HitObject = Object;                    
                    
                    tMin = t;
                    uMin = u;
                    vMin = v;                    
                    BestTriangleIndex = TriangleIndex;
                }
            }
        }
    }
    
    if(Result.Hit)
    {
        mesh* Mesh = Result.HitObject->Mesh;
        Result.Position = Origin + Direction*tMin;
        
        ak_m3f Transform = AK_M3(AK_TransformM4(Result.HitObject->Transform));
        ak_m3f NormalTransform = AK_Transpose(AK_InvTransformM3(Transform));
        
        ak_v3f VertexN[3] = 
        {
            AK_Normalize(Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+0]].N*NormalTransform), 
            AK_Normalize(Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+1]].N*NormalTransform),
            AK_Normalize(Mesh->Vertices[Mesh->Indices[(BestTriangleIndex*3)+2]].N*NormalTransform)
        };
        
        Result.Normal = AK_Normalize(VertexN[0]*uMin + VertexN[1]*vMin + VertexN[2]*(1-uMin-vMin));        
    }
    
    return Result;
}

ak_v3f RandomDirectionInUnitSphere()
{
    ak_v3f P = AK_V3((ak_f32)AK_Random32(), (ak_f32)AK_Random32(), (ak_f32)AK_Random32());
    return P;
}

ak_color4f GetPixelColorFromLights(ak_v3f P, ak_v3f N, ak_color3f SurfaceColor, ak_array<light> Lights, ak_array<object> Objects)
{
    ak_color4f PixelColor = {};
    AK_ForEach(Light, &Lights)
    {                                
        ak_v3f L = Light->Position - P;
        ak_f32 DistanceFromLight = AK_Magnitude(L);
        L /= DistanceFromLight;
        
        ak_v3f ShadowRayOrigin = P;
        ak_v3f ShadowRayDirection = L;
        
        if(!RayIntersected(ShadowRayOrigin, ShadowRayDirection, Objects))
        {                                        
            ak_color3f LightColor = Light->Color*Light->Intensity;                
            ak_f32 Numerator = AK_Clamp(1.0f - AK_Pow((DistanceFromLight/Light->Radius), 4.0f), 0.0f, 1.0f);
            ak_f32 Denominator = (DistanceFromLight)+1.0f;
            ak_f32 Falloff = (Numerator*Numerator)/Denominator;
            
            LightColor *= Falloff;        
            ak_color3f Lambertian = AK_Max(AK_Dot(N, L), 0.0f)*LightColor*SurfaceColor;
            PixelColor.xyz += Lambertian;                
        }
    }    
    PixelColor.w = 1.0f;
    return PixelColor;
}

ak_bool UpdateIrradiance(ak_color3f* Irradiance, ak_v3f Origin, ak_v3f Direction, ak_array<object> Objects, ak_array<light> Lights)
{
    ray_trace RayTrace = CastRay(Origin, Direction, Objects);
    if(RayTrace.Hit)
    {        
        ak_color4f PixelColor = GetPixelColorFromLights(RayTrace.Position, RayTrace.Normal, RayTrace.HitObject->Color.xyz, Lights, Objects);                            
        *Irradiance += PixelColor.xyz*0.5f;  
        return true;
    }        
    *Irradiance += AK_RGB(0.03f, 0.03f, 0.03f);
    return false;
}

object CreateObject(ak_v3f Position, ak_quatf Orientation, ak_v3f Scale, mesh* Mesh, ak_color4f Color)
{
    object Result;
    Result.Transform = AK_SQT(Position, Orientation, Scale);
    Result.Color = Color;
    Result.Mesh = Mesh;
    return Result;
}

light CreateLight(ak_v3f Position, ak_f32 Radius, ak_color3f Color, ak_f32 Intensity)
{
    light Light;
    Light.Position = Position;
    Light.Radius = Radius;
    Light.Color = Color;
    Light.Intensity = Intensity;
    return Light;
}

void ClearBitmap(bitmap* Bitmap, ak_u32 Color)
{        
    ak_u32* Dst = Bitmap->Pixels;
    for(ak_i32 YIndex = 0; YIndex < Bitmap->Resolution.h; YIndex++)
    {        
        for(ak_i32 XIndex = 0; XIndex < Bitmap->Resolution.w; XIndex++)        
            *Dst++ = Color;                        
    }
}

mesh ToMesh(ak_mesh_result<ak_vertex_p3_n3> MeshResult)
{
    mesh Result = {};
    Result.VertexCount = MeshResult.VertexCount;
    Result.IndexCount = MeshResult.IndexCount;
    Result.Vertices = MeshResult.Vertices;
    Result.Indices = MeshResult.Indices;
    return Result;
}

global ak_bool Global_Running;

void Win32_ProcessMessages()
{
    MSG Message = {};
    for(;;)
    {
        ak_bool GotMessage = false;
        
        DWORD SkipMessages[] = 
        {
            0x738, 0xFFFFFFFF
        };
        
        DWORD LastMessage = 0;
        for(ak_u32 SkipIndex = 0;
            SkipIndex < AK_Count(SkipMessages);
            ++SkipIndex)
        {
            
            DWORD Skip = SkipMessages[SkipIndex];
            GotMessage = PeekMessage(&Message, 0, LastMessage, Skip - 1, PM_REMOVE);
            if(GotMessage)
            {
                break;
            }
            
            LastMessage = Skip + 1;
        }
        
        if(!GotMessage)
            return;
        
        switch(Message.message)
        {
            case WM_QUIT:
            {                    
                Global_Running = false;
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }        
}

AK_ASYNC_TASK_CALLBACK(RayTraceRow_Task)
{
    ray_trace_row_task* Task = (ray_trace_row_task*)UserData;
    bitmap* Bitmap = Task->CommonData->Bitmap;
    ak_i32 YIndex = Task->YIndex;
    camera* Camera = Task->CommonData->Camera;
    ak_array<object> Objects = Task->CommonData->Objects;
    ak_array<light> Lights = Task->CommonData->Lights;
    
    ak_m4f InvPerspective = Task->CommonData->InvPerspective;
    ak_m4f InvView = Task->CommonData->InvView;
    
    for(ak_i32 XIndex = 0; XIndex < Bitmap->Resolution.w; XIndex++)
    {
        ak_v3f NDC = AK_ToNormalizedDeviceCoordinates(AK_V2f(XIndex, YIndex), AK_V2f(Bitmap->Resolution));
        ak_v4f Clip = AK_V4(NDC.xy, -1.0f, 1.0f);
        
        ak_v4f RayView = Clip*InvPerspective;                
        ak_v3f RayWorld = AK_Normalize((AK_V4(RayView.xy, -1.0f, 0.0f)*InvView).xyz);
        
        ak_v3f RayOrigin = Camera->Position;
        ak_v3f RayDirection = RayWorld;
        
        ray_trace RayTrace = CastRay(RayOrigin, RayDirection, Objects);        
        if(RayTrace.Hit)       
        {   
            object* HitObject = RayTrace.HitObject; 
            ak_v3f PixelWorldPosition = RayTrace.Position;
            ak_v3f N = RayTrace.Normal;
            
            ak_color4f PixelColor = GetPixelColorFromLights(PixelWorldPosition, N, HitObject->Color.xyz, Lights, Objects);            
            
            
            ak_v3f DiffuseOrigin = PixelWorldPosition;
            
            ak_u32 SampleCount = 64;
            ak_color3f Irradiance = {};            
            for(ak_u32 SampleIndex = 0; SampleIndex < SampleCount; SampleIndex++)
            {                
                ak_v3f DiffuseDirection = AK_Normalize(RayTrace.Position + RayTrace.Normal + RandomDirectionInUnitSphere());                        
                UpdateIrradiance(&Irradiance, DiffuseOrigin, DiffuseDirection, Objects, Lights);                    
            }
            Irradiance *= (1.0f/SampleCount);                        
            
            ak_color3f Diffuse = Irradiance*HitObject->Color.xyz;
            
            
            PixelColor.xyz += Diffuse;
            
            PixelColor.w = 1.0f;
            Bitmap->Pixels[(YIndex*Bitmap->Resolution.w)+XIndex] = AK_BGRA_U32(PixelColor);                
        }
    }    
}

int CALLBACK 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLineArgs, int CmdLineOpts)
{            
    AK_SetGlobalArena(AK_CreateArena(AK_Megabyte(32)));
    
    ak_arena* POCArena = AK_CreateArena(AK_Megabyte(1));
    
    ak_window* PlatformWindow = AK_CreateWindow(1920, 1080, "Graphics_POC");
    if(!PlatformWindow)
    {
        AK_MessageBoxOk("Fatal Error", "Failed to create the platform window. Quitting");
        return -1;
    }
    
    ak_async_task_queue* TaskQueue = AK_CreateAsyncTaskQueue(11);
    
    mesh BoxMesh = ToMesh(AK_GenerateTriangleBoxN(POCArena, AK_V3(1.0f, 1.0f, 1.0f)));
    //mesh SphereMesh = ToMesh(AK_GenerateTriangleSphere(POCArena, 0.5f, 60));
    
    BITMAPINFO BitmapInfo = {};    
    BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    bitmap Bitmap = {};
    
    ak_v3f CameraTarget = AK_V3(0.0f, 0.0f, 0.0f);
    
    camera Camera = {};
    Camera.Position = AK_V3(5.0f, 3.0f, 11.0f);
    Camera.Direction = AK_Normalize(CameraTarget-Camera.Position);
    
    ak_array<object> Objects = {};    
    Objects.Add(CreateObject(AK_V3( 0.0f, 0.0f, 0.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 1.0f, 1.0f), &BoxMesh, AK_Blue4()));
    Objects.Add(CreateObject(AK_V3( 0.0f, -1.0f, 0.0f), AK_IdentityQuat<ak_f32>(), AK_V3(10.0f, 1.0f, 10.0f), &BoxMesh, AK_Blue4()));
    Objects.Add(CreateObject(AK_V3(-3.0f, 0.0f, 3.0f), AK_RotQuat(AK_ZAxis(), AK_ToRadians(35.0f)), AK_V3(1.0f, 1.0f, 1.0f), &BoxMesh, AK_Red4()));
    Objects.Add(CreateObject(AK_V3( 0.0f, 0.0f, 6.0f), AK_IdentityQuat<ak_f32>(), AK_V3(1.0f, 1.0f, 1.0f), &BoxMesh, AK_Green4()));
    Objects.Add(CreateObject(AK_V3( 3.0f, 0.0f, 3.0f), AK_RotQuat(AK_ZAxis(), AK_ToRadians(-35.0f)), AK_V3(1.0f, 1.0f, 1.0f), &BoxMesh, AK_Red4()));
    
    ak_array<light> Lights = {};
    Lights.Add(CreateLight(AK_V3(0.0f, 2.0f, 1.0f), 5.0f, AK_RGB(1.0f, 1.0f, 1.0f), 3.0f));
    Lights.Add(CreateLight(AK_V3(-3.0f, 2.0f, 3.0f), 5.0f, AK_White3(), 3.0f));
    Lights.Add(CreateLight(AK_V3( 3.0f, 2.0f, 3.0f), 5.0f, AK_White3(), 3.0f));
    
    Global_Running = true;
    while(Global_Running)
    {        
        Win32_ProcessMessages();
        
        ak_arena* GlobalArena = AK_GetGlobalArena();
        ak_temp_arena TempArena = GlobalArena->BeginTemp();
        
        
        HWND Window = AK_GetPlatformWindow(PlatformWindow);
        HDC DeviceContext = GetDC(Window);
        
        ak_v2i CurrentWindowResolution = AK_V2<ak_i32>();
        AK_GetWindowResolution(PlatformWindow, (ak_u16*)&CurrentWindowResolution.w, (ak_u16*)&CurrentWindowResolution.h);
        
        
        ak_m4f Perspective = AK_Perspective(AK_ToRadians(35.0f), AK_SafeRatio(CurrentWindowResolution.w, CurrentWindowResolution.h), 0.001f, 100.0f);
        ak_m4f View = AK_LookAt(Camera.Position, Camera.Position+Camera.Direction, AK_YAxis());
        
        ak_m4f InvPerspective = AK_Inverse(Perspective);
        ak_m4f InvView = AK_InvTransformM4(View);
        
        
        if(CurrentWindowResolution != Bitmap.Resolution)
        {            
            BitmapInfo.bmiHeader.biWidth = CurrentWindowResolution.w;
            BitmapInfo.bmiHeader.biHeight = -CurrentWindowResolution.h;
            
            ak_u32* NewPixels = (ak_u32*)AK_Allocate(CurrentWindowResolution.w*CurrentWindowResolution.h*sizeof(ak_u32));            
            if(Bitmap.Pixels)
                AK_Free(Bitmap.Pixels);
            
            Bitmap.Pixels = NewPixels;            
            Bitmap.Resolution = CurrentWindowResolution;
        }
        
        ClearBitmap(&Bitmap, 0xFF000000);
        
        ak_high_res_clock Start = AK_WallClock();
        ak_u64 StartHz = AK_Cycles();
        
        ray_trace_common_data CommonData = {};
        CommonData.Bitmap = &Bitmap;
        CommonData.Camera = &Camera;
        CommonData.Objects = Objects;
        CommonData.Lights = Lights;
        CommonData.InvPerspective = InvPerspective;
        CommonData.InvView = InvView;        
        
        for(ak_i32 YIndex = 0; YIndex < Bitmap.Resolution.h; YIndex++)
        {
            ray_trace_row_task* RayTraceRowTask = GlobalArena->Push<ray_trace_row_task>();
            RayTraceRowTask->YIndex = YIndex;
            RayTraceRowTask->CommonData = &CommonData;
            TaskQueue->AddTask(RayTraceRow_Task, RayTraceRowTask);            
        }        
        TaskQueue->CompleteAllTasks();
        
        ak_u64 EndHz = AK_Cycles();
        ak_high_res_clock End = AK_WallClock();
        ak_f64 Elapsed = AK_GetElapsedTime(End, Start);
        AK_ConsoleLog("Elapsed Time %f Elapsed Hz %ull\n", Elapsed, EndHz-StartHz);
        
        StretchDIBits(DeviceContext, 0, 0, CurrentWindowResolution.w, CurrentWindowResolution.h, 
                      0, 0, CurrentWindowResolution.w, CurrentWindowResolution.h, 
                      Bitmap.Pixels, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
        
        GlobalArena->EndTemp(&TempArena);
    }
    
    return 0;
}