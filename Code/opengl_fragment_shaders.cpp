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
uniform c4 Color;
#endif
                                             
void main()
{
#ifdef HAS_TEXTURES    
    FragColor = texture(Texture, PixelUV);
#else
    FragColor = Color;    
#endif
}

)";

global const char* FragmentShader_Shadow = R"(
in v3f FragPosition;
uniform f32 FarPlaneDistance;
uniform v3f LightPosition;

void main()
{
    f32 LightDistance = length(FragPosition.xyz-LightPosition);
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
uniform c4 DiffuseColor;
#endif

#ifdef DIFFUSE_TEXTURE
uniform sampler2D DiffuseTexture;
#endif

#ifdef SPECULAR_COLOR
uniform c4 SpecularColor;
#endif

#ifdef SPECULAR_TEXTURE
uniform sampler2D SpecularTexture;
#endif

#ifdef SPECULAR_SHININESS
uniform i32 Shininess;
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

brdf BlinnPhong(v3f N, v3f L, v3f V, c3 LightColor, c3 SurfaceColor, c3 SpecularColor, f32 Shininess)
{
    brdf Result;

    Result.Diffuse = Lambertian(N, L, SurfaceColor, LightColor);

    v3f H = normalize(L+V);    
    f32 NDotH = max(dot(N, H), 0.0f);
    Result.Specular = pow(NDotH, Shininess)*SpecularColor*LightColor;

    return Result;
}

f32 SampleShadowMap(i32 ShadowMapIndex, v2f UV, v2f TexelSize, f32 Compare)
{
    v2f PixelPosition = UV/TexelSize + v2f(0.5f);
    v2f FractionPart = fract(PixelPosition);
    v2f StartTexel = (PixelPosition - FractionPart)*TexelSize;
    
    f32 BottomLeft = step(Compare, texture(ShadowMap, v3f(StartTexel, ShadowMapIndex)).r);
    f32 BottomRight = step(Compare, texture(ShadowMap, v3f(StartTexel + v2f(TexelSize.x, 0.0f), ShadowMapIndex)).r);
    f32 TopLeft = step(Compare, texture(ShadowMap, v3f(StartTexel + v2f(0.0f, TexelSize.y), ShadowMapIndex)).r);
    f32 TopRight = step(Compare, texture(ShadowMap, v3f(StartTexel + TexelSize, ShadowMapIndex)).r);

    f32 LeftMix = mix(BottomLeft, TopLeft, FractionPart.y);
    f32 RightMix = mix(BottomRight, TopRight, FractionPart.y);
    f32 Result = mix(LeftMix, RightMix, FractionPart.x);
    return Result;
}

f32 SampleShadow(v4f FragPosition, i32 ShadowMapIndex, v3f N, v3f L)
{
    v3f ProjPosition = FragPosition.xyz / FragPosition.w;
    ProjPosition = ProjPosition * 0.5f + 0.5f;    

    if(ProjPosition.z > 1.0f)
        return 0;

    f32 CurrentDepth = ProjPosition.z;

    f32 Bias = max(0.05f * (1.0f - dot(N, L)), 0.05f);
    v2f TexelSize = 1.0f/textureSize(ShadowMap, 0).xy;

    f32 CompareValue = CurrentDepth-Bias;

#define NUM_SAMPLES 3.0f
#define NUM_SAMPLES_SQR NUM_SAMPLES*NUM_SAMPLES

    f32 Samples = (NUM_SAMPLES-1.0f)/2.0f;

    f32 Result = 0.0f;
    for(f32 y = -Samples; y <= Samples; y += 1.0f)
    {
        for(f32 x = -Samples; x <= Samples; x += 1.0f)
        {
            v2f CoordOffset = v2f(x, y)*TexelSize;
            Result += SampleShadowMap(ShadowMapIndex, ProjPosition.xy+CoordOffset, TexelSize, CompareValue);
        }
    }

    Result /= NUM_SAMPLES_SQR;
    return Result;
}

void main()
{    
    v3f N = normalize(PixelWorldNormal);

#if LAMBERTIAN_MODEL == 0
    v3f V = normalize(ViewPosition-PixelWorldPosition);
#endif

#ifdef DIFFUSE_COLOR
    c3 SurfaceColor = DiffuseColor.rgb;
#endif

#ifdef DIFFUSE_TEXTURE
    c3 SurfaceColor = texture(DiffuseTexture, PixelUV).rgb;
#endif

#ifdef SPECULAR_COLOR
    c3 Specular = SpecularColor.rgb; 
#endif

#ifdef SPECULAR_TEXTURE
    c3 Specular = texture(SpecularTexture, PixelUV).rgb;
#endif

    c3 FinalColor = c3(0, 0, 0);
    for(i32 DirectionalLightIndex = 0; DirectionalLightIndex < DirectionalLightCount; DirectionalLightIndex++)
    {
        directional_light DirectionalLight = DirectionalLights[DirectionalLightIndex];
        v3f L = -DirectionalLight.Direction.xyz;

        f32 Shadow = SampleShadow(PixelLightPositions[DirectionalLightIndex], DirectionalLightIndex, N, L);

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

#if LAMBERTIAN_MODEL
        c3 LambertianColor = Lambertian(N, L, LightColor, SurfaceColor);
        FinalColor += LambertianColor;
#else
        brdf BRDF = BlinnPhong(N, L, V, LightColor, SurfaceColor, Specular, Shininess);
        FinalColor += (BRDF.Diffuse+BRDF.Specular);    
#endif        
    }

    c3 Ambient = 0.03*SurfaceColor.xyz;
    FinalColor += Ambient;
    FragColor = c4(FinalColor, 1.0f);
}

)";