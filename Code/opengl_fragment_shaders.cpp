global const char* FragmentShader_None = "void main(){}\n";

global const char* FragmentShader_ImGui = R"(
out c4 FinalColor;

in v2f PixelUV;
in c4 PixelColor;

uniform sampler2D Texture;

void main()
{
    FinalColor = PixelColor * texture(Texture, PixelUV.st);
}

)";

global const char* FragmentShader_Simple = R"(
out c4 FragColor;

#ifdef HAS_TEXTURES
uniform sampler2D Texture;
in v2f PixelUV;
#else
uniform c3 Color;
#endif
                                             
void main()
{
#ifdef HAS_TEXTURES    
    FragColor = c4(texture(Texture, PixelUV).rgb, 1.0f);
#else
    FragColor = c4(Color, 1.0f);    
#endif
}

)";

global const char* FragmentShader_Shadow = R"(
in v3f PixelWorldPosition;
uniform f32 FarPlaneDistance;
uniform v3f LightPosition;

void main()
{
    f32 LightDistance = length(PixelWorldPosition-LightPosition);
    LightDistance /= FarPlaneDistance;    
    gl_FragDepth = LightDistance;
}

)";

global const char* FragmentShader_Lighting = R"(
out c4 FragColor;

in v3f PixelWorldPosition;
in v3f PixelWorldNormal;
in v4f PixelLightPositions[MAX_DIRECTIONAL_LIGHT_COUNT];

#ifdef HAS_TEXTURES
in v2f PixelUV;
#endif

#ifdef DIFFUSE_COLOR
uniform c3 DiffuseColor;
#endif

#ifdef DIFFUSE_TEXTURE
uniform sampler2D DiffuseTexture;
#endif

#ifdef SPECULAR_COLOR
uniform f32 SpecularColor;
#endif

#ifdef SPECULAR_TEXTURE
uniform sampler2D SpecularTexture;
#endif

#ifdef SPECULAR_SHININESS
uniform i32 Shininess;
#endif

#ifdef HAS_NORMAL_MAPPING
in v3f PixelWorldTangent;
in v3f PixelWorldBitangent;
uniform sampler2D NormalMap;
#endif

#define LAMBERTIAN_MODEL (defined(DIFFUSE_COLOR) || defined(DIFFUSE_TEXTURE)) && (!defined(SPECULAR_COLOR) && !defined(SPECULAR_TEXTURE) && !defined(SPECULAR_SHININESS))

struct directional_light
{
    v4f Direction;
    c4 Color;
};

struct point_light
{
    v4f Position;
    c4 Color;    
};

layout (std140) uniform LightBuffer
{
    directional_light DirectionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
    point_light PointLights[MAX_POINT_LIGHT_COUNT];
    i32 DirectionalLightCount;
    i32 PointLightCount;
};


uniform v3f ViewPosition;

uniform sampler2DArray ShadowMap;
uniform sampler2DArray OmniShadowMap;

struct brdf
{
    c3 Diffuse;
    c3 Specular;
};

c3 Lambertian(v3f N, v3f L, v3f SurfaceColor, v3f LightColor)
{
    c3 Result = max(dot(N, L), 0.0f)*SurfaceColor*LightColor;
    return Result;
}

brdf BlinnPhong(v3f N, v3f L, v3f V, c3 LightColor, c3 SurfaceColor, f32 SpecularColor, f32 Shininess)
{
    brdf Result;

    Result.Diffuse = Lambertian(N, L, SurfaceColor, LightColor);

    v3f H = normalize(L+V);    
    f32 NDotH = max(dot(N, H), 0.0f);
    Result.Specular = pow(NDotH, Shininess)*SpecularColor*LightColor;

    return Result;
}

f32 BillinearInterpolate(f32 BottomLeft, f32 BottomRight, f32 TopLeft, f32 TopRight, f32 t, f32 u)
{
    f32 LeftMix = mix(BottomLeft, TopLeft, u);
    f32 RightMix = mix(BottomRight, TopRight, u);
    f32 Result = mix(LeftMix, RightMix, t);
    return Result;
}

f32 BillinearSampleShadowMap(i32 ShadowMapIndex, v2f UV, v2f TexelSize, f32 Compare)
{
    v2f PixelPosition = UV/TexelSize + v2f(0.5f);
    v2f FractionPart = fract(PixelPosition);
    v2f StartTexel = (PixelPosition - FractionPart)*TexelSize;
    
    f32 BottomLeft = step(Compare, texture(ShadowMap, v3f(StartTexel, ShadowMapIndex)).r);
    f32 BottomRight = step(Compare, texture(ShadowMap, v3f(StartTexel + v2f(TexelSize.x, 0.0f), ShadowMapIndex)).r);
    f32 TopLeft = step(Compare, texture(ShadowMap, v3f(StartTexel + v2f(0.0f, TexelSize.y), ShadowMapIndex)).r);
    f32 TopRight = step(Compare, texture(ShadowMap, v3f(StartTexel + TexelSize, ShadowMapIndex)).r);

    f32 Result = BillinearInterpolate(BottomLeft, BottomRight, TopLeft, TopRight, FractionPart.x, FractionPart.y);
    return Result;
}

#define NUM_SAMPLES 3.0f
#define NUM_SAMPLES_SQR NUM_SAMPLES*NUM_SAMPLES
#define NUM_SAMPLES_CUBE NUM_SAMPLES*NUM_SAMPLES*NUM_SAMPLES

f32 SampleShadowMap(i32 ShadowMapIndex, v4f FragPosition, v3f N, v3f L)
{
    v3f ProjPosition = FragPosition.xyz / FragPosition.w;
    ProjPosition = ProjPosition * 0.5f + 0.5f;    

    if(ProjPosition.z > 1.0f)
        return 1;

    f32 ClosestDepth = texture(ShadowMap, v3f(ProjPosition.xy, ShadowMapIndex)).r;
    f32 CurrentDepth = ProjPosition.z;

    f32 Bias = max(0.05f * (1.0f - dot(N, L)), 0.005f);        
    //f32 Bias = 0.005f;
    f32 Result = step(CurrentDepth-Bias, ClosestDepth);

    return Result;
}

#define POSITIVE_X 0
#define NEGATIVE_X 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_Z 4
#define NEGATIVE_Z 5
i32 ChooseFaceIndex(v3f TexCoords, out v2f OutTexCoords)
{       
    f32 AbsX = abs(TexCoords.x);
    f32 AbsY = abs(TexCoords.y);
    f32 AbsZ = abs(TexCoords.z);
    
    f32 LargestAxis;
    f32 U, V;
    i32 Result;
    if((AbsX >= AbsY) && (AbsX >= AbsZ))
    {
        LargestAxis = AbsX;
        if(TexCoords.x >= 0)
        {            
            U =  TexCoords.z;
            V =  TexCoords.y;
            Result = POSITIVE_X; 
        }
        else
        {
            U = -TexCoords.z;
            V = TexCoords.y;
            Result = NEGATIVE_X;
        }
    }
    if((AbsY >= AbsX) && (AbsY >= AbsZ))
    {
        LargestAxis = AbsY;
        if(TexCoords.y >= 0)
        {
            U = TexCoords.x;
            V = TexCoords.z;
            Result = POSITIVE_Y;
        }
        else 
        { 
            U =  -TexCoords.x;
            V =  TexCoords.z;
            Result = NEGATIVE_Y;
        }
    }
    if((AbsZ >= AbsX) && (AbsZ >= AbsY))
    {
        LargestAxis = AbsZ;
        if(TexCoords.z >= 0)
        {
            U = -TexCoords.x;
            V = TexCoords.y;
            Result = POSITIVE_Z;
        }
        else
        { 
            U =  TexCoords.x;
            V =  TexCoords.y;
            Result = NEGATIVE_Z;
        }
    }
    OutTexCoords.x = 0.5f * (U / LargestAxis + 1.0f);
    OutTexCoords.y = 0.5f * (V / LargestAxis + 1.0f);
    return Result;
}

f32 SampleOmniShadowMap(i32 ShadowMapIndex, v3f PixelWorldPosition, v3f LightWorldPosition, f32 FarPlaneDistance, v3f N, v3f L)
{
    i32 ActualShadowMapIndex = 6*ShadowMapIndex;
    v3f LightToPixel = PixelWorldPosition-LightWorldPosition;
    f32 CurrentDepth = length(LightToPixel);
    v2f OutTexCoords;
    i32 FaceIndex = ChooseFaceIndex(LightToPixel, OutTexCoords);
    i32 FinalFaceIndex = ActualShadowMapIndex+FaceIndex;
    f32 ClosestDepth = texture(OmniShadowMap, v3f(OutTexCoords, FinalFaceIndex)).r;
    ClosestDepth *= FarPlaneDistance;
    f32 Bias = max(0.05f*(1.0f-dot(N, L)), 0.05f);
    f32 Result = step(CurrentDepth-Bias, ClosestDepth);
    return Result;
}

//NOTE(EVERYONE): Got this from here http://www.thetenthplanet.de/archives/1180
//I wanted to try and not have tangent and bitangent vectors stored in the mesh.
//While not as efficient on the pixel shader, it definitely produces nicer results 
//and is more memory efficient 
m3 CotangentBasis(v3f N, v3f p, v2f uv)
{
    // get edge vectors of the pixel triangle 
    v3f dp1 = dFdx( p ); 
    v3f dp2 = dFdy( p ); 
    v2f duv1 = dFdx( uv ); 
    v2f duv2 = dFdy( uv );   

    // solve the linear system 
    v3f dp2perp = cross( dp2, N ); 
    v3f dp1perp = cross( N, dp1 ); 
    v3f T = dp2perp * duv1.x + dp1perp * duv2.x; 
    v3f B = dp2perp * duv1.y + dp1perp * duv2.y;   

    // construct a scale-invariant frame 
    f32 invmax = inversesqrt( max( dot(T,T), dot(B,B) ) ); 
    return m3( T * invmax, B * invmax, N );
    
}

v3f NormalMapInfluence(sampler2D NormalMap, v3f Normal, v3f View, v2f UV)
{
    v3f N = texture(NormalMap, UV).xyz;
    N = N*2.0f - 1.0f;
    m3 TBN = CotangentBasis(Normal, -View, UV);
    N = normalize(TBN*N);
    return N;
}

void main()
{    
    v3f V = normalize(ViewPosition-PixelWorldPosition);

#ifdef HAS_NORMAL_MAPPING
    v3f N = NormalMapInfluence(NormalMap, PixelWorldNormal, V, PixelUV);
#else
    v3f N = normalize(PixelWorldNormal);
#endif    
    
#ifdef DIFFUSE_COLOR
    c3 SurfaceColor = DiffuseColor;
#endif

#ifdef DIFFUSE_TEXTURE
    c3 SurfaceColor = texture(DiffuseTexture, PixelUV).rgb;
#endif

#ifdef SPECULAR_COLOR
    f32 Specular = SpecularColor; 
#endif

#ifdef SPECULAR_TEXTURE
    f32 Specular = texture(SpecularTexture, PixelUV).r;
#endif

    c3 FinalColor = c3(0, 0, 0);
    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DirectionalLightCount; DirectionalLightIndex++)
    {
        directional_light DirectionalLight = DirectionalLights[DirectionalLightIndex];
        v3f L = -DirectionalLight.Direction.xyz;

        f32 Shadow = SampleShadowMap(DirectionalLightIndex, PixelLightPositions[DirectionalLightIndex], N, L);

#if LAMBERTIAN_MODEL
        c3 LambertianColor = Lambertian(N, L, DirectionalLight.Color.rgb, SurfaceColor);
        FinalColor += Shadow*LambertianColor;
#else
        brdf BRDF = BlinnPhong(N, L, V, DirectionalLight.Color.rgb, SurfaceColor, Specular, Shininess);
        FinalColor += Shadow*(BRDF.Diffuse+BRDF.Specular);    
#endif
    }

    for(i32 PointLightIndex = 0; PointLightIndex < PointLightCount; PointLightIndex++)
    {
        point_light PointLight = PointLights[PointLightIndex];
        
        f32 LightRadius  = PointLight.Position.w;
        v3f L = PointLight.Position.xyz - PixelWorldPosition;

        f32 DistanceFromLight = length(L);
        L /= DistanceFromLight;        
        
        f32 Numerator = clamp(1.0f - pow((DistanceFromLight/LightRadius), 4.0f), 0.0f, 1.0f);
        f32 Denominator = (DistanceFromLight)+1.0f;
        f32 Falloff = (Numerator*Numerator)/Denominator;

        c3 LightColor = PointLight.Color.xyz*Falloff;        
        
        f32 Shadow = SampleOmniShadowMap(PointLightIndex, PixelWorldPosition, PointLight.Position.xyz, LightRadius, N, L);        
#if LAMBERTIAN_MODEL
        c3 LambertianColor = Lambertian(N, L, LightColor, SurfaceColor);
        FinalColor += Shadow*LambertianColor;
#else
        brdf BRDF = BlinnPhong(N, L, V, LightColor, SurfaceColor, Specular, Shininess);
        FinalColor += Shadow*(BRDF.Diffuse+BRDF.Specular);    
#endif        
    }

    c3 Ambient = 0.03*SurfaceColor.xyz;
    FinalColor += Ambient;
    FragColor = c4(FinalColor, 1.0f);    
}

)";